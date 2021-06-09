#include "Metrics.h"

void Metrics::updateMetrics(const char *op, int *metric) {
  if (debug_verbose) {
    printf("Update metrics for %s\n", op);
  }
  for (auto &opMetricsIt : opMetrics) {
    if (!strcmp(opMetricsIt.first, op)) {
      fatal_error("We already have metric info for %s", op);
    }
  }
  opMetrics.insert(std::make_pair(op, metric));
}

void Metrics::normalizeName(char *src, char toDel, char newChar) {
  char *pos = strchr(src, toDel);
  while (pos) {
    *pos = newChar;
    pos = strchr(pos + 1, toDel);
  }
}

void Metrics::getNormalizedOpName(const char *op, char *normalizedOp) {
  strcat(normalizedOp, op);
  normalizeName(normalizedOp, '<', '_');
  normalizeName(normalizedOp, '>', '_');
  normalizeName(normalizedOp, ',', '_');
}

int *Metrics::getOpMetric(const char *opName) {
  if (debug_verbose) {
    printf("get op metric for %s\n", opName);
  }
  if (opName == nullptr) {
    fatal_error("normalizedOp is NULL\n");
  }
  char *normalizedOp = new char[10240];
  normalizedOp[0] = '\0';
  getNormalizedOpName(opName, normalizedOp);
  for (auto &opMetricsIt : opMetrics) {
    if (!strcmp(opMetricsIt.first, normalizedOp)) {
      return opMetricsIt.second;
    }
  }
  warning ("Missing metric info for `%s'", normalizedOp);
  if (debug_verbose) {
    printOpMetrics();
    printf("\n\n\n\n\n");
  }
  return nullptr;
}

void Metrics::printOpMetrics() {
  printf("Info in opMetrics:\n");
  for (auto &opMetricsIt : opMetrics) {
    printf("%s ", opMetricsIt.first);
  }
  printf("\n");
}

void Metrics::writeMetricsFile(char *opName, int metric[4]) {
  printf("Write %s perf to metric file: %s\n", opName, metricFilePath);
  std::ofstream metricFp;
  metricFp.open(metricFilePath, std::ios_base::app);
  metricFp << opName << "  " << metric[0] << "  " << metric[1] << "  " << metric[2]
           << "  " << metric[3] << "\n";
}

void Metrics::readMetricsFile() {
  printf("Read metric file: %s\n", metricFilePath);
  std::ifstream metricFp(metricFilePath);
  std::string line;
  while (std::getline(metricFp, line)) {
    std::istringstream iss(line);
    char *instance = new char[10240];
    int metricCount = -1;
    int *metric = new int[4];
    bool emptyLine = true;
    do {
      std::string numStr;
      iss >> numStr;
      if (!numStr.empty()) {
        emptyLine = false;
        if (metricCount >= 0) {
          metric[metricCount] = std::atoi(numStr.c_str());
        } else {
          sprintf(instance, "%s", numStr.c_str());
        }
        metricCount++;
      }
    } while (iss);
    if (!emptyLine && (metricCount != 4)) {
      printf("%s has %d metrics!\n", metricFilePath, metricCount);
      exit(-1);
    }
    if (debug_verbose) {
      printf("Add metrics for %s\n", instance);
    }
    updateMetrics(instance, metric);
  }
}

Metrics::Metrics(const char *metricFP, const char *statisticsFP) {
  metricFilePath = metricFP;
  statisticsFilePath = statisticsFP;
}

void Metrics::updateCopyStatistics(unsigned bitwidth, unsigned numOutputs) {
  auto copyStatisticsIt = copyStatistics.find(bitwidth);
  if (copyStatisticsIt != copyStatistics.end()) {
    Map<int, int> &record = copyStatisticsIt->second;
    auto recordIt = record.find(numOutputs);
    if (recordIt != record.end()) {
      recordIt->second++;
    } else {
      record.insert(GenPair(numOutputs, 1));
    }
  } else {
    Map<int, int> record = {{numOutputs, 1}};
    copyStatistics.insert(GenPair(bitwidth, record));
  }
}

void Metrics::printStatistics() {
  printf("Print statistics to file: %s\n", statisticsFilePath);
  FILE *statisticsFP = fopen(statisticsFilePath, "w");
  if (!statisticsFP) {
    fatal_error("Could not create statistics file %s\n", statisticsFilePath);
  }
  printCopyStatistics(statisticsFP);
  printAreaStatistics(statisticsFP);
  fclose(statisticsFP);
}

void Metrics::printCopyStatistics(FILE *statisticsFP) {
  fprintf(statisticsFP, "%s\n", "COPY STATISTICS:");
  for (auto &copyStatisticsIt : copyStatistics) {
    fprintf(statisticsFP, "%d-bit COPY:\n", copyStatisticsIt.first);
    Map<int, int> &record = copyStatisticsIt.second;
    for (auto &recordIt : record) {
      fprintf(statisticsFP, "  %d outputs: %d\n", recordIt.first, recordIt.second);
    }
  }
  fprintf(statisticsFP, "\n");
}

void Metrics::updateAreaStatistics(const char *instance, int area) {
  totalArea += area;
  bool exist = false;
  for (auto &areaStatisticsIt : areaStatistics) {
    if (!strcmp(areaStatisticsIt.first, instance)) {
      areaStatisticsIt.second += area;
      exist = true;
    }
  }
  if (exist) {
    for (auto &instanceCntIt : instanceCnt) {
      if (!strcmp(instanceCntIt.first, instance)) {
        instanceCntIt.second += 1;
        return;
      }
    }
    fatal_error("We could find %s in areaStatistics, but not in instanceCnt!\n",
                instance);
  } else {
    areaStatistics.insert(GenPair(instance, area));
    instanceCnt.insert(GenPair(instance, 1));
  }
}

int Metrics::getInstanceCnt(const char *instance) {
  for (auto &instanceCntIt : instanceCnt) {
    if (!strcmp(instanceCntIt.first, instance)) {
      return instanceCntIt.second;
    }
  }
  fatal_error("We could not find %s in instanceCnt!\n", instance);
}

void Metrics::printAreaStatistics(FILE *statisticsFP) {
  fprintf(statisticsFP, "Area Statistics:\n");
  fprintf(statisticsFP, "totalArea: %d\n", totalArea);
  if (!areaStatistics.empty() && (totalArea == 0)) {
    printf("areaStatistics is not empty, but totalArea is 0!\n");
    exit(-1);
  }
  std::multimap<int, const char *> sortedAreas = flip_map(areaStatistics);
  for (auto iter = sortedAreas.rbegin(); iter != sortedAreas.rend(); iter++) {
    int area = iter->first;
    double ratio = (double) area / totalArea * 100;
    const char *instance = iter->second;
    if (ratio > 0.1) {
      int cnt = getInstanceCnt(instance);
      fprintf(statisticsFP, "%80.50s %5d %5.1f %5d\n", instance, area, ratio, cnt);
    }
  }
  fprintf(statisticsFP, "\n");
}

void Metrics::dump() {
  if (debug_verbose) {
    printOpMetrics();
  }
  printStatistics();
}

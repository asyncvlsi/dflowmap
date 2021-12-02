#include "Metrics.h"

void Metrics::updateMetrics(const char *op, long *metric) {
  for (auto &opMetricsIt : opMetrics) {
    if (!strcmp(opMetricsIt.first, op)) {
      if (debug_verbose) {
        printf("We already have metric info for %s", op);
      }
      long *oldMetric = opMetricsIt.second;
      if ((oldMetric[0] != metric[0])
          || (oldMetric[1] != metric[1])
          || (oldMetric[2] != metric[2])
          || (oldMetric[3] != metric[3])) {
        exit(-1);
      }
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

long *Metrics::getOpMetric(const char *opName) {
  if (opName == nullptr) {
    printf("normalizedOp is NULL\n");
    exit(-1);
  }
  unsigned opLen = strlen(opName);
  char *normalizedOp = new char[opLen + 1];
  normalizedOp[0] = '\0';
  getNormalizedOpName(opName, normalizedOp);
  if (debug_verbose) {
    printf("get op metric for %s, norm name: %s\n", opName, normalizedOp);
  }
  for (auto &opMetricsIt : opMetrics) {
    if (!strcmp(opMetricsIt.first, normalizedOp)) {
      return opMetricsIt.second;
    }
  }
  if (debug_verbose) {
    printf("We don't have metric info for (`%s`,`%s')\n", opName, normalizedOp);
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

void Metrics::writeMetricsFile(char *opName, long metric[4]) {
  if (debug_verbose) {
    printf("Write %s perf to metric file: %s\n", opName, metricFilePath);
  }
  std::ofstream metricFp;
  metricFp.open(metricFilePath, std::ios_base::app);
  metricFp << opName << "  " << metric[0] << "  " << metric[1] << "  "
           << metric[2]
           << "  " << metric[3] << "\n";
}

void Metrics::readMetricsFile() {
  if (debug_verbose) {
    printf("Read metric file: %s\n", metricFilePath);
  }
  std::ifstream metricFp(metricFilePath);
  std::string line;
  while (std::getline(metricFp, line)) {
    std::istringstream iss(line);
    char *instance = new char[MAX_INSTANCE_LEN];
    int metricCount = -1;
    long *metric = new long[4];
    bool emptyLine = true;
    do {
      std::string numStr;
      iss >> numStr;
      if (!numStr.empty()) {
        emptyLine = false;
        if (metricCount >= 0) {
          char *residual;
          metric[metricCount] = std::strtol(numStr.c_str(), &residual, 10);
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
  if (debug_verbose) {
    printf("Print statistics to file: %s\n", statisticsFilePath);
  }
  FILE *statisticsFP = fopen(statisticsFilePath, "w");
  if (!statisticsFP) {
    printf("Could not create statistics file %s\n", statisticsFilePath);
    exit(-1);
  }
  fprintf(statisticsFP, "totalArea: %ld, totalLeakPowewr: %ld\n", totalArea,
          totalLeakPowewr);
  fprintf(statisticsFP, "Merge area: %ld, ratio: %5.1f\n",
          mergeArea, ((double) 100 * mergeArea / totalArea));
  fprintf(statisticsFP, "Split area: %ld, ratio: %5.1f\n",
          splitArea, ((double) 100 * splitArea / totalArea));
  fprintf(statisticsFP, "ACTN CP area: %ld, ratio: %5.1f\n",
          actnCpArea, ((double) 100 * actnCpArea / totalArea));
  fprintf(statisticsFP, "ACTN DP area: %ld, ratio: %5.1f\n",
          actnDpArea, ((double) 100 * actnDpArea / totalArea));
  fprintf(statisticsFP, "Merge LeakPower: %ld, ratio: %5.1f\n",
          mergeLeakPower, ((double) 100 * mergeLeakPower / totalLeakPowewr));
  fprintf(statisticsFP, "Split LeakPower: %ld, ratio: %5.1f\n",
          splitLeakPower, ((double) 100 * splitLeakPower / totalLeakPowewr));
  fprintf(statisticsFP, "ACTN CP LeakPower: %ld, ratio: %5.1f\n",
          actnCpLeakPower, ((double) 100 * actnCpLeakPower / totalLeakPowewr));
  fprintf(statisticsFP, "ACTN DP LeakPower: %ld, ratio: %5.1f\n",
          actnDpLeakPower, ((double) 100 * actnDpLeakPower / totalLeakPowewr));
  printAreaStatistics(statisticsFP);
  printLeakpowerStatistics(statisticsFP);
  fclose(statisticsFP);
}

void Metrics::printCopyStatistics(FILE *statisticsFP) {
  fprintf(statisticsFP, "%s\n", "COPY STATISTICS:");
  for (auto &copyStatisticsIt : copyStatistics) {
    fprintf(statisticsFP, "%d-bit COPY:\n", copyStatisticsIt.first);
    Map<int, int> &record = copyStatisticsIt.second;
    for (auto &recordIt : record) {
      fprintf(statisticsFP, "  %d outputs: %d\n", recordIt.first,
              recordIt.second);
    }
  }
  fprintf(statisticsFP, "\n");
}

void Metrics::updateACTNCpMetrics(long area, long leakPower) {
  actnCpArea += area;
  actnCpLeakPower += leakPower;
}

void Metrics::updateACTNDpMetrics(long area, long leakPower) {
  actnDpArea += area;
  actnDpLeakPower += leakPower;
}

void Metrics::updateMergeMetrics(long area, long leakPower) {
  mergeArea += area;
  mergeLeakPower += leakPower;
}

void Metrics::updateSplitMetrics(long area, long leakPower) {
  splitArea += area;
  splitLeakPower += leakPower;
}

void
Metrics::updateStatistics(const char *instance, long area, long leakPower) {
  totalArea += area;
  totalLeakPowewr += leakPower;
  bool exist = false;
  for (auto &areaStatisticsIt : areaStatistics) {
    if (!strcmp(areaStatisticsIt.first, instance)) {
      areaStatisticsIt.second += area;
      exist = true;
    }
  }
  if (exist) {
    bool foundLP = false;
    for (auto &leakpowerStatisticsIt : leakpowerStatistics) {
      if (!strcmp(leakpowerStatisticsIt.first, instance)) {
        leakpowerStatisticsIt.second += leakPower;
        foundLP = true;
        break;
      }
    }
    if (!foundLP) {
      printf(
          "We could find %s in areaStatistics, but not in leakpowerStatistics!\n",
          instance);
      exit(-1);
    }
    for (auto &instanceCntIt : instanceCnt) {
      if (!strcmp(instanceCntIt.first, instance)) {
        instanceCntIt.second += 1;
        return;
      }
    }
    printf("We could find %s in areaStatistics, but not in instanceCnt!\n",
           instance);
    exit(-1);
  } else {
    areaStatistics.insert(GenPair(instance, area));
    leakpowerStatistics.insert(GenPair(instance, leakPower));
    instanceCnt.insert(GenPair(instance, 1));
  }
}

int Metrics::getInstanceCnt(const char *instance) {
  for (auto &instanceCntIt : instanceCnt) {
    if (!strcmp(instanceCntIt.first, instance)) {
      return instanceCntIt.second;
    }
  }
  printf("We could not find %s in instanceCnt!\n", instance);
  exit(-1);
}

long Metrics::getInstanceArea(const char *instance) {
  for (auto &areaStatisticsIt : areaStatistics) {
    if (!strcmp(areaStatisticsIt.first, instance)) {
      return areaStatisticsIt.second;
    }
  }
  printf("We could not find %s in instanceCnt!\n", instance);
  exit(-1);
}

void Metrics::printLeakpowerStatistics(FILE *statisticsFP) {
  fprintf(statisticsFP, "Leak Power Statistics:\n");
  fprintf(statisticsFP, "totalLeakPower: %ld\n", totalLeakPowewr);
  if (!leakpowerStatistics.empty() && (totalLeakPowewr == 0)) {
    printf("leakpowerStatistics is not empty, but totalLeakPowewr is 0!\n");
    exit(-1);
  }
  std::multimap<long, const char *>
      sortedLeakpowers = flip_map(leakpowerStatistics);
  for (auto iter = sortedLeakpowers.rbegin();
       iter != sortedLeakpowers.rend(); iter++) {
    double leakPower = iter->first;
    double ratio = (double) leakPower / totalLeakPowewr * 100;
    const char *instance = iter->second;
    if (ratio > 0.1) {
      int cnt = getInstanceCnt(instance);
      fprintf(statisticsFP, "%80.50s %5.1f %5.1f %5d\n", instance, leakPower,
              ratio, cnt);
    }
  }
  fprintf(statisticsFP, "\n");
}

void Metrics::printAreaStatistics(FILE *statisticsFP) {
  fprintf(statisticsFP, "Area Statistics:\n");
  fprintf(statisticsFP, "totalArea: %ld\n", totalArea);
  if (!areaStatistics.empty() && (totalArea == 0)) {
    printf("areaStatistics is not empty, but totalArea is 0!\n");
    exit(-1);
  }
  std::multimap<long, const char *> sortedAreas = flip_map(areaStatistics);
  fprintf(statisticsFP,
          "instance name      area     percentage     # of instances\n");
  long redundantArea = 0;
  for (auto iter = sortedAreas.rbegin(); iter != sortedAreas.rend(); iter++) {
    int area = iter->first;
    double ratio = (double) area / totalArea * 100;
    const char *instance = iter->second;
    if (ratio > 0.1) {
      int cnt = getInstanceCnt(instance);
      fprintf(statisticsFP, "%80.80s %5d %5.1f %5d\n", instance, area, ratio,
              cnt);
      if (cnt > 1) {
//        redundantArea += (cnt - 1) * getOpMetric(instance)[3];
      }
    }
  }
  fprintf(statisticsFP, "\n");
  if (debug_verbose) {
    printf("totalArea: %ld, redundant area: %ld, ratio: %5.1f\n",
           totalArea, redundantArea,
           ((double) redundantArea / totalArea * 100.0));
  }
}

void Metrics::dump() {
  printStatistics();
}

#include "Metrics.h"

void Metrics::updateMetrics(const char *op, double *metric) {
  for (auto &opMetricsIt : opMetrics) {
    if (!strcmp(opMetricsIt.first, op)) {
      if (debug_verbose) {
        printf("We already have metric info for %s\n", op);
      }
      double *oldMetric = opMetricsIt.second;
      if ((oldMetric[0] != metric[0])
          || (oldMetric[1] != metric[1])
          || (oldMetric[2] != metric[2])
          || (oldMetric[3] != metric[3])) {
        printf("We find different metric record for %s\n", op);
        exit(-1);
      }
    }
  }
  opMetrics.insert(std::make_pair(op, metric));
}

double *Metrics::getOpMetric(const char *instance) {
  if (instance == nullptr) {
    printf("Try to get metric for null instance!\n");
    exit(-1);
  }
  const char *normInstance = getNormInstanceName(instance);
  if (debug_verbose) {
    printf("get op metric for %s, norm name: %s\n", instance, normInstance);
  }
  for (auto &opMetricsIt : opMetrics) {
    if (!strcmp(opMetricsIt.first, normInstance)) {
      return opMetricsIt.second;
    }
  }
  if (debug_verbose) {
    printf("We don't have metric info for (`%s`,`%s')\n", instance, normInstance);
  }
  return nullptr;
}

double Metrics::getArea(double metric[4]) {
  if (!metric) {
    printf("Try to extract area for null metric!\n");
    exit(-1);
  }
  return metric[3];
}

double Metrics::getLP(double metric[4]) {
  if (!metric) {
    printf("Try to extract LP for null metric!\n");
    exit(-1);
  }
  return metric[0];
}

void Metrics::printOpMetrics() {
  printf("Info in opMetrics:\n");
  for (auto &opMetricsIt : opMetrics) {
    printf("%s ", opMetricsIt.first);
  }
  printf("\n");
}

void Metrics::writeMetricsFile(const char *opName, double *metric) {
  if (debug_verbose) {
    printf("Write %s perf to metric file: %s\n", opName, metricFilePath);
  }
  std::ofstream metricFp;
  metricFp.open(metricFilePath, std::ios_base::app);
  metricFp << opName << "  " << metric[0] << "  " << metric[1] << "  "
           << metric[2] << "  " << metric[3] << "\n";
  metricFp.close();
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
    auto metric = new double[4];
    bool emptyLine = true;
    do {
      std::string numStr;
      iss >> numStr;
      if (!numStr.empty()) {
        emptyLine = false;
        if (metricCount >= 0) {
          metric[metricCount] = std::strtod(numStr.c_str(), nullptr);
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

unsigned Metrics::getEquivalentBW(unsigned oriBW) {
  if (oriBW < 4) {
    return 1;
  } else if (oriBW < 12) {
    return 8;
  } else if (oriBW < 24) {
    return 16;
  } else if (oriBW < 48) {
    return 32;
  } else if (oriBW <= 64) {
    return 64;
  } else {
    printf("Invalid M/S bitwidth: %u!\n", oriBW);
    exit(-1);
  }
}

double *Metrics::getOrGenCopyMetric(unsigned bitwidth, unsigned numOut) {
  updateCopyStatistics(bitwidth, numOut);
  char *instName = new char[1500];
  sprintf(instName, "copy<%u,%u>", bitwidth, numOut);
  double *metric = getOpMetric(instName);
  if (!metric && LOGIC_OPTIMIZER) {
    char *equivInstance = new char[1500];
    int equivN = int(ceil(log2(numOut))) - 1;
    if (equivN < 1) {
      equivN = 1;
    }
    unsigned equivBW = getEquivalentBW(bitwidth);
    if (debug_verbose) {
      printf(
          "We are handling copy_%u_%u, and we are using mapping it to %d "
          "copy_%u_2_\n", bitwidth, numOut, equivN, equivBW);
    }
    sprintf(equivInstance, "copy<%u,2>", equivBW);
    double *equivMetric = getOpMetric(equivInstance);
    if (!equivMetric) {
      printf("Missing metrics for copy %s\n", equivInstance);
      exit(-1);
    }
    if (equivN == 1) {
      metric = equivMetric;
    } else {
      metric = new double[4];
      metric[0] = equivN * equivMetric[0];
      metric[1] = equivN * equivMetric[1];
      metric[2] = equivN * equivMetric[2];
      metric[3] = equivN * equivMetric[3];
    }
    const char *normInstance = getNormInstanceName(instName);
    updateMetrics(normInstance, metric);
    writeMetricsFile(normInstance, metric);
  }
  if (!metric) {
    bool actnCp = false;
    bool actnDp = false;
    updateStatistics(instName, actnCp, actnDp, metric);
  }

  return metric;
}

double *Metrics::getSinkMetric(unsigned bitwidth) {
  char *instance = new char[1500];
  sprintf(instance, "sink<%u>", bitwidth);
  char *unitInstance = new char[1500];
  sprintf(unitInstance, "sink_1_");
  double *metric = getOpMetric(unitInstance);
  if (!metric && LOGIC_OPTIMIZER) {
    printf("We fail to find metric for %s!\n", instance);
    exit(-1);
  }
  if (metric != nullptr) {
    bool actnCp = false;
    bool actnDp = false;
    updateStatistics(instance, actnCp, actnDp, metric);
  }
  return metric;
}

double *Metrics::getSourceMetric(const char *instance, unsigned int bitwidth) {

  char *unitInstance = new char[8];
  sprintf(unitInstance, "source1");
  double *metric = getOpMetric(unitInstance);
  if (!metric && LOGIC_OPTIMIZER) {
    printf("We fail to find metric for %s!\n", instance);
    exit(-1);
  }
  if (metric != nullptr) {
    bool actnCp = false;
    bool actnDp = false;
    updateStatistics(instance, actnCp, actnDp, metric);
  }
  return metric;
}

double *Metrics::getOrGenInitMetric(unsigned int bitwidth) {
  char *instName = new char[100];
  sprintf(instName, "init%u", bitwidth);
  double *metric = getOpMetric(instName);
  if (metric != nullptr) {
    bool actnCp = false;
    bool actnDp = false;
    updateStatistics(instName, actnCp, actnDp, metric);
  }
  return metric;
}

double *Metrics::getOrGenFUMetric(const char *instName,
                                  StringMap<unsigned> &inBW,
                                  StringMap<unsigned> &hiddenBW,
                                  Map<const char *, Expr *> &exprMap,
                                  Map<Expr *, Expr *> &hiddenExprs,
                                  Map<int, int> &outRecord,
                                  UIntVec &outWidthList) {
  double *metric = getOpMetric(instName);
  if (!metric) {
#if LOGIC_OPTIMIZER
    if (debug_verbose) {
      printf("Will run logic optimizer for %s\n", instName);
    }
    /* Prepare in_expr_list */
    list_t *in_expr_list = list_new();
    iHashtable *in_expr_map = ihash_new(0);
    iHashtable *in_width_map = ihash_new(0);
    unsigned totalInBW = 0;
    unsigned lowBWInPorts = 0;
    unsigned highBWInPorts = 0;
    for (auto &inBWIt : inBW) {
      String inName = inBWIt.first;
      unsigned bw = inBWIt.second;
      totalInBW += bw;
      if (bw >= 32) {
        highBWInPorts++;
      } else {
        lowBWInPorts++;
      }
      char *inChar = new char[10240];
      sprintf(inChar, "%s", inName.c_str());
      if (debug_verbose) {
        printf("inChar: %s\n", inChar);
      }
      Expr *inExpr = getExprFromName(inChar, exprMap, true, -1);
      list_append(in_expr_list, inExpr);
      ihash_bucket_t *b_expr, *b_width;
      b_expr = ihash_add(in_expr_map, (long) inExpr);
      b_expr->v = inChar;
      b_width = ihash_add(in_width_map, (long) inExpr);
      b_width->i = (int) bw;
    }
    /* Prepare hidden_expr_list */
    list_t *hidden_expr_list = list_new();
    list_t *hidden_expr_name_list = list_new();
    iHashtable *out_width_map = ihash_new(0);
    for (auto &hiddenBWIt : hiddenBW) {
      String hiddenName = hiddenBWIt.first;
      unsigned bw = hiddenBWIt.second;
      char *hiddenChar = new char[1024];
      sprintf(hiddenChar, "%s", hiddenName.c_str());
      if (debug_verbose) {
        printf("hiddenChar: %s\n", hiddenChar);
      }
      Expr *hiddenRHS = getExprFromName(hiddenChar, exprMap, true, -1);
      ihash_bucket_t *b_expr2, *b_width2;
      b_expr2 = ihash_add(in_expr_map, (long) hiddenRHS);
      b_expr2->v = hiddenChar;
      b_width2 = ihash_add(in_width_map, (long) hiddenRHS);
      b_width2->i = (int) bw;
      Expr *hiddenExpr = hiddenExprs.find(hiddenRHS)->second;
      list_append(hidden_expr_list, hiddenExpr);
      list_append(hidden_expr_name_list, hiddenChar);
      ihash_bucket_t *b_width = ihash_lookup(out_width_map, (long) hiddenExpr);
      if (!b_width) {
        b_width = ihash_add(out_width_map, (long) hiddenExpr);
        b_width->i = (int) bw;
      }
    }
    /* Prepare out_expr_list */
    list_t *out_expr_list = list_new();
    list_t *out_expr_name_list = list_new();
    IntVec processedResIDs;
    unsigned numOuts = outRecord.size();
    for (int ii = 0; ii < numOuts; ii++) {
      int resID = outRecord.find(ii)->second;
      char *resChar = new char[1024];
      sprintf(resChar, "res%d", resID);
      if (debug_verbose) {
        printf("resChar: %s\n", resChar);
      }
      Expr *resExpr = getExprFromName(resChar, exprMap, true, -1);
      list_append(out_expr_list, resExpr);
      char *outChar = new char[1024];
      sprintf(outChar, "out%d", ii);
      list_append(out_expr_name_list, outChar);
      if (std::find(processedResIDs.begin(), processedResIDs.end(), resID)
          != processedResIDs.end()) {
        continue;
      }
      ihash_bucket_t *b_width;
      b_width = ihash_add(out_width_map, (long) resExpr);
      unsigned bw = outWidthList[ii];
      b_width->i = (int) bw;
      processedResIDs.push_back(resID);
    }
    auto optimizer = new ExternalExprOpt(genus, bd, false);
    if (debug_verbose) {
      listitem_t *li;
      printf("in_expr_bundle:\n");
      for (li = list_first (in_expr_list); li; li = list_next (li)) {
        long key = (long) list_value(li);
        char *val = (char *) ihash_lookup(in_expr_map, key)->v;
        int bw = ihash_lookup(in_width_map, key)->i;
        printf("key: %ld, val: %s, bw: %d\n", key, val, bw);
        Expr *e = (Expr *) list_value (li);
        print_expr(stdout, e);
        printf("\n");
      }
      printf("\nout_expr_bundle:\n");
      for (li = list_first (out_expr_list); li; li = list_next (li)) {
        long key = (long) list_value(li);
        int bw = ihash_lookup(out_width_map, key)->i;
        printf("key: %ld, bw: %d\n", key, bw);
        Expr *e = (Expr *) list_value (li);
        print_expr(stdout, e);
        printf("\n");
      }
      for (li = list_first (out_expr_name_list); li; li = list_next (li)) {
        char *outName = (char *) list_value(li);
        printf("outName: %s\n", outName);
      }
      printf("\nhidden expr:\n");
      for (li = list_first (hidden_expr_list); li; li = list_next (li)) {
        Expr *e = (Expr *) list_value (li);
        print_expr(stdout, e);
        printf("\n");
        long key = (long) list_value(li);
        int bw = ihash_lookup(out_width_map, key)->i;
        printf("key: %ld, bw: %d\n", key, bw);
      }
      printf("\n");
    }
    const char *normalizedOp = getNormInstanceName(instName);
    char *optimizerProcName = new char[1000];
    sprintf(optimizerProcName, "op");
    if (debug_verbose) {
      printf("Run logic optimizer for %s\n", optimizerProcName);
    }
    ExprBlockInfo
        *info = optimizer->run_external_opt(optimizerProcName, in_expr_list,
                                            in_expr_map,
                                            in_width_map,
                                            out_expr_list, out_expr_name_list,
                                            out_width_map,
                                            hidden_expr_list,
                                            hidden_expr_name_list);
    if (debug_verbose) {
      printf(
          "Generated block %s: Area: %e m2, Dyn Power: %e W, Leak Power: %e W, delay: %e "
          "s\n",
          optimizerProcName,
          info->area,
          info->power_typ_dynamic,
          info->power_typ_static,
          info->delay_typ);
    }
    double leakpower = info->power_typ_static * 1e9;  // Leakage power (nW)
    double energy = info->power_typ_dynamic * info->delay_typ * 1e15;  // 1e-15J
    double delay = info->delay_typ * 1e12; // Delay (ps)
    double area = info->area * 1e12;  // AREA (um^2)
    /* adjust perf number by adding latch, etc. */
    double *latchMetric = getOpMetric("latch1");
    if (latchMetric == nullptr) {
      printf("We could not find metric for latch1!\n");
      exit(-1);
    }
    //TODO
    area = area + totalInBW * latchMetric[3] + lowBWInPorts * 1.43
        + highBWInPorts * 2.86 + delay / 500 * 1.43;
    leakpower = leakpower + totalInBW * latchMetric[0] + lowBWInPorts * 0.15
        + highBWInPorts * 5.36 + delay / 500 * 1.38;
    energy = energy + totalInBW * latchMetric[1] + lowBWInPorts * 4.516
        + highBWInPorts * 20.19 + delay / 500 * 28.544;
    double *twoToOneMetric = getOpMetric("twoToOne");
    if (twoToOneMetric == nullptr) {
      printf("We could not find metric for 2-in-1-out!\n");
      exit(-1);
    }
    delay = delay + twoToOneMetric[2] + latchMetric[2];
    /* get the final metric */
    metric = new double[4];
    metric[0] = leakpower;
    metric[1] = energy;
    metric[2] = delay;
    metric[3] = area;
    updateMetrics(normalizedOp, metric);
    writeMetricsFile(normalizedOp, metric);
#endif
  }
  if (metric != nullptr) {
    bool actnCp = false;
    bool actnDp = false;
    updateStatistics(instName, actnCp, actnDp, metric);
  }
  return metric;
}

double *Metrics::getMSMetric(const char *instance,
                             const char *procName,
                             unsigned guardBW,
                             unsigned inBW,
                             bool actnCp,
                             bool actnDp) {
  char *equivInstance = new char[MAX_INSTANCE_LEN];
  unsigned equivBW = getEquivalentBW(inBW);
  sprintf(equivInstance, "%s<%d,%d>", procName, guardBW, equivBW);
  double *metric = getOpMetric(equivInstance);
  if (metric) {
    updateStatistics(instance, actnCp, actnDp, metric);
    updateSplitMetrics(metric);
  } else if (LOGIC_OPTIMIZER) {
    printf("We could not find metrics for the SPLIT (%s, %s)!\n",
           equivInstance, instance);
    exit(-1);
  }
  return metric;
}

double *Metrics::getArbiterMetric(const char *instance,
                                  unsigned numInputs,
                                  unsigned inBW,
                                  unsigned coutBW,
                                  bool actnCp,
                                  bool actnDp) {
  char *equivInstance = new char[MAX_INSTANCE_LEN];
  unsigned equivBW = getEquivalentBW(inBW);
  sprintf(equivInstance,
          "%s_%d<%d,%d>",
          Constant::MERGE_PREFIX,
          numInputs,
          coutBW,
          equivBW);
  double *metric = getOpMetric(equivInstance);
  if (metric) {
    updateStatistics(instance, actnCp, actnDp, metric);
    updateSplitMetrics(metric);
  } else if (LOGIC_OPTIMIZER) {
    printf("We could not find metrics for the ARBITER %s!\n", equivInstance);
    exit(-1);
  }
  return metric;
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

void Metrics::updateACTNCpMetrics(double area, double leakPower) {
  actnCpArea += area;
  actnCpLeakPower += leakPower;
}

void Metrics::updateACTNDpMetrics(double area, double leakPower) {
  actnDpArea += area;
  actnDpLeakPower += leakPower;
}

void Metrics::updateMergeMetrics(double area, double leakPower) {
  mergeArea += area;
  mergeLeakPower += leakPower;
}

void Metrics::updateSplitMetrics(double metric[4]) {
  double area = getArea(metric);
  double leakPower = getLP(metric);
  splitArea += area;
  splitLeakPower += leakPower;
}

void Metrics::updateStatistics(const char *instName,
                               bool actnCp,
                               bool actnDp,
                               double metric[4]) {
  double area = getArea(metric);
  double leakPower = getLP(metric);
  totalArea += area;
  totalLeakPowewr += leakPower;
  bool exist = false;
  for (auto &areaStatisticsIt : areaStatistics) {
    if (!strcmp(areaStatisticsIt.first, instName)) {
      areaStatisticsIt.second += area;
      exist = true;
    }
  }
  if (exist) {
    bool foundLP = false;
    for (auto &leakpowerStatisticsIt : leakpowerStatistics) {
      if (!strcmp(leakpowerStatisticsIt.first, instName)) {
        leakpowerStatisticsIt.second += leakPower;
        foundLP = true;
        break;
      }
    }
    if (!foundLP) {
      printf(
          "We could find %s in areaStatistics, but not in leakpowerStatistics!\n",
          instName);
      exit(-1);
    }
    for (auto &instanceCntIt : instanceCnt) {
      if (!strcmp(instanceCntIt.first, instName)) {
        instanceCntIt.second += 1;
        return;
      }
    }
    printf("We could find %s in areaStatistics, but not in instanceCnt!\n",
           instName);
    exit(-1);
  } else {
    areaStatistics.insert(GenPair(instName, area));
    leakpowerStatistics.insert(GenPair(instName, leakPower));
    instanceCnt.insert(GenPair(instName, 1));
  }
  if (actnCp) {
    updateACTNCpMetrics(area, leakPower);
  } else if (actnDp) {
    updateACTNDpMetrics(area, leakPower);
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

double Metrics::getInstanceArea(const char *instance) {
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
  std::multimap<double, const char *>
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
  std::multimap<double, const char *> sortedAreas = flip_map(areaStatistics);
  fprintf(statisticsFP,
          "instance name      area     percentage     # of instances\n");
  double redundantArea = 0;
  for (auto iter = sortedAreas.rbegin(); iter != sortedAreas.rend(); iter++) {
    double area = iter->first;
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
    printf("totalArea: %f, redundant area: %f, ratio: %5.1f\n",
           totalArea, redundantArea, (redundantArea / totalArea * 100));
  }
}

void Metrics::dump() {
  printStatistics();
}

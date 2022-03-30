/*
 * This file is part of the ACT library
 *
 * Copyright (c) 2021 Rui Li
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "Metrics.h"

void Metrics::updateMetrics(const char *instance, double *metric) {
  const char *normInstance = getNormInstanceName(instance);
  for (auto &opMetricsIt: opMetrics) {
    if (!strcmp(opMetricsIt.first, normInstance)) {
      if (debug_verbose) {
        printf("We already have metric info for (%s, %s)\n",
               instance, normInstance);
      }
      double *oldMetric = opMetricsIt.second;
      if ((oldMetric[0] != metric[0])
          || (oldMetric[1] != metric[1])
          || (oldMetric[2] != metric[2])
          || (oldMetric[3] != metric[3])) {
        printf("We find different metric record for %s\n", normInstance);
        exit(-1);
      }
      return;
    }
  }
  opMetrics.insert(std::make_pair(normInstance, metric));
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
  for (auto &opMetricsIt: opMetrics) {
    if (!strcmp(opMetricsIt.first, normInstance)) {
      double *metric = opMetricsIt.second;
      return metric;
    }
  }
  if (debug_verbose) {
    printf("We don't have metric info for (`%s`,`%s')\n",
           instance,
           normInstance);
  }
  return nullptr;
}

double Metrics::getLP(double metric[4]) {
  if (!metric) {
    printf("Try to extract LP for null metric!\n");
    exit(-1);
  }
  return metric[0];
}

double Metrics::getEnergy(double metric[4]) {
  if (!metric) {
    printf("Try to extract energy for null metric!\n");
    exit(-1);
  }
  return metric[1];
}

double Metrics::getDelay(double metric[4]) {
  if (!metric) {
    printf("Try to extract delay for null metric!\n");
    exit(-1);
  }
  return metric[2];
}

double Metrics::getArea(double metric[4]) {
  if (!metric) {
    printf("Try to extract area for null metric!\n");
    exit(-1);
  }
  return metric[3];
}

void Metrics::printOpMetrics() {
  printf("Info in opMetrics:\n");
  for (auto &opMetricsIt: opMetrics) {
    printf("%s ", opMetricsIt.first);
  }
  printf("\n");
}

void Metrics::writeMetricsFile(const char *instance, double *metric) {
  if (debug_verbose) {
    printf("Write %s perf to metric file: %s\n", instance, metricFilePath);
  }
  const char *normInstance = getNormInstanceName(instance);
  std::ofstream metricFp;
  metricFp.open(metricFilePath, std::ios_base::app);
  metricFp << normInstance << "  " << metric[0] << "  " << metric[1] << "  "
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
  char *instance = new char[1500];
  sprintf(instance, "copy<%u,%u>", bitwidth, numOut);
  double *metric = getOpMetric(instance);
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
    const char *normInstance = getNormInstanceName(instance);
    updateMetrics(normInstance, metric);
    writeMetricsFile(normInstance, metric);
  }
  if (metric) {
    bool actnCp = false;
    bool actnDp = false;
    updateStatistics(instance, actnCp, actnDp, metric);
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
  if (metric) {
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
  if (metric) {
    bool actnCp = false;
    bool actnDp = false;
    updateStatistics(instance, actnCp, actnDp, metric);
  }
  return metric;
}

double *Metrics::getOrGenInitMetric(unsigned int bitwidth) {
  char *instance = new char[100];
  sprintf(instance, "init%u", bitwidth);
  double *metric = getOpMetric(instance);
  if (metric) {
    bool actnCp = false;
    bool actnDp = false;
    updateStatistics(instance, actnCp, actnDp, metric);
  }
  return metric;
}

double *Metrics::getBuffMetric(unsigned nBuff, unsigned bw) {
  char *instance = new char[100];
  sprintf(instance, "latch%u", bw);
  double *uniMetric = getOpMetric(instance);
  double *metric = nullptr;
  if (uniMetric) {
    metric = new double[4];
    metric[0] = nBuff * metric[0];
    metric[1] = nBuff * metric[1];
    metric[2] = nBuff * metric[2];
    metric[3] = nBuff * metric[3];
    bool actnCp = false;
    bool actnDp = false;
    updateStatistics(instance, actnCp, actnDp, metric);
  }
  return metric;
}

double *Metrics::getOrGenFUMetric(const char *instance,
                                  StringMap<unsigned> &inBW,
                                  StringMap<unsigned> &hiddenBW,
                                  Map<const char *, Expr *> &exprMap,
                                  Map<Expr *, Expr *> &hiddenExprs,
                                  Map<unsigned int, unsigned int> &outRecord,
                                  UIntVec &outBWList) {
  double *metric = getOpMetric(instance);
  if (!metric) {
#if LOGIC_OPTIMIZER
    if (debug_verbose) {
      printf("Will run logic optimizer for %s\n", instance);
    }
    /* Prepare in_expr_list */
    list_t *in_expr_list = list_new();
    iHashtable *in_expr_map = ihash_new(0);
    iHashtable *in_width_map = ihash_new(0);
    unsigned totalInBW = 0;
    unsigned lowBWInPorts = 0;
    unsigned highBWInPorts = 0;
    for (auto &inBWIt: inBW) {
      String inName = inBWIt.first;
      unsigned bw = inBWIt.second;
      totalInBW += bw;
      if (bw >= 32) {
        highBWInPorts++;
      } else {
        lowBWInPorts++;
      }
      char *inChar = new char[strlen(inName.c_str()) + 1];
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
    for (auto &hiddenBWIt: hiddenBW) {
      String hiddenName = hiddenBWIt.first;
      unsigned bw = hiddenBWIt.second;
      char *hiddenChar = new char[1 + strlen(hiddenName.c_str())];
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
    UIntVec processedResIDs;
    unsigned numOuts = outRecord.size();
    for (unsigned ii = 0; ii < numOuts; ii++) {
      unsigned resID = outRecord.find(ii)->second;
      char *resChar = new char[SHORT_STRING_LEN];
      sprintf(resChar, "res%u", resID);
      if (debug_verbose) {
        printf("resChar: %s\n", resChar);
      }
      Expr *resExpr = getExprFromName(resChar, exprMap, true, -1);
      list_append(out_expr_list, resExpr);
      char *outChar = new char[SHORT_STRING_LEN];
      sprintf(outChar, "out%d", ii);
      list_append(out_expr_name_list, outChar);
      if (std::find(processedResIDs.begin(), processedResIDs.end(), resID)
          != processedResIDs.end()) {
        continue;
      }
      ihash_bucket_t *b_width;
      b_width = ihash_add(out_width_map, (long) resExpr);
      unsigned bw = outBWList[ii];
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
    const char *normalizedOp = getNormInstanceName(instance);
    if (debug_verbose) {
      printf("Run logic optimizer for %s\n", normalizedOp);
    }
    char *rtlModuleName = new char[1000];
    sprintf(rtlModuleName, "op");
    ExprBlockInfo *info = optimizer->run_external_opt(rtlModuleName,
                                                      in_expr_list,
                                                      in_expr_map,
                                                      in_width_map,
                                                      out_expr_list,
                                                      out_expr_name_list,
                                                      out_width_map,
                                                      hidden_expr_list,
                                                      hidden_expr_name_list);
    if (debug_verbose) {
      printf("Generated block %s: Area: %e m2, Dyn Power: %e W, "
             "Leak Power: %e W, delay: %e s\n",
             normalizedOp,
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
    double *ebufMetric = getOpMetric("10ebuf");
    double *pulseGenMetric = getOpMetric("pulseGen");
    double *twoToOneMetric = getOpMetric("twoToOne");
    double *hornMetric = getOpMetric("horn2");
    if (!latchMetric || !ebufMetric || !pulseGenMetric || !twoToOneMetric
        || !hornMetric) {
      printf("No metric for the bundled-data control circuit!\n");
      exit(-1);
    }
    double latchLP = getLP(latchMetric);
    double latchEnergy = getEnergy(latchMetric);
    double latchDelay = getDelay(latchMetric);
    double latchArea = getArea(latchMetric);
    double ebufLP = getLP(ebufMetric);
    double ebufEnergy = getEnergy(ebufMetric);
    double ebufDelay = getDelay(ebufMetric);
    double ebufArea = getArea(ebufMetric);
    double pulseGenLP = getLP(pulseGenMetric);
    double pulseGenEnergy = getEnergy(pulseGenMetric);
    double pulseGenArea = getArea(pulseGenMetric);
    double twoToOneDelay = getDelay(twoToOneMetric);
    double hornLP = getLP(hornMetric);
    double hornEnergy = getEnergy(hornMetric);
    double hornArea = getArea(hornMetric);
    area = area + totalInBW * latchArea + lowBWInPorts * pulseGenArea
        + highBWInPorts * (pulseGenArea + hornArea)
        + delay / ebufDelay * ebufArea;
    leakpower = leakpower + totalInBW * latchLP + lowBWInPorts * pulseGenLP
        + highBWInPorts * (pulseGenLP + hornLP) + delay / ebufDelay * ebufLP;
    energy = energy + totalInBW * latchEnergy + lowBWInPorts * pulseGenEnergy
        + highBWInPorts * (pulseGenEnergy + hornEnergy)
        + delay / ebufDelay * ebufEnergy;
    delay = delay + twoToOneDelay + latchDelay;
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
  if (metric) {
    bool actnCp = false;
    bool actnDp = false;
    updateStatistics(instance, actnCp, actnDp, metric);
  }
  return metric;
}

double *Metrics::getOrGenMergeMetric(unsigned guardBW,
                                     unsigned inBW,
                                     unsigned numIn,
                                     bool actnCp,
                                     bool actnDp) {
  if (!LOGIC_OPTIMIZER) {
    return nullptr;
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  if (PIPELINE) {
    sprintf(instance,
            "pipe%s_%d<%d,%d>",
            Constant::MERGE_PREFIX,
            numIn,
            guardBW,
            inBW);
  } else {
    sprintf(instance,
            "unpipe%s_%d<%d,%d>",
            Constant::MERGE_PREFIX,
            numIn,
            guardBW,
            inBW);
  }
  double *metric = getOpMetric(instance);
  if (!metric) {
    char *equivInstance = new char[MAX_INSTANCE_LEN];
    unsigned equivBW = getEquivalentBW(inBW);
    if (equivBW != inBW) {
      if (PIPELINE) {
        sprintf(equivInstance,
                "pipe%s_%d<%d,%d>",
                Constant::MERGE_PREFIX,
                numIn,
                guardBW,
                equivBW);
      } else {
        sprintf(equivInstance,
                "unpipe%s_%d<%d,%d>",
                Constant::MERGE_PREFIX,
                numIn,
                guardBW,
                equivBW);
      }
      metric = getOpMetric(equivInstance);
    }
    if (!metric) {
      double *mergeCtrlMetric = getOpMetric("mergeControl");
      double *muxMetric = getOpMetric("mux1");
      double *decodeMetric = getOpMetric("decodeTwoToFour");
      double mergeCtrlLP = getLP(mergeCtrlMetric);
      double mergeCtrlEnergy = getEnergy(mergeCtrlMetric);
      double mergeCtrlDelay = getDelay(mergeCtrlMetric);
      double mergeCtrlArea = getArea(mergeCtrlMetric);
      double muxLP = getLP(muxMetric);
      double muxEnergy = getEnergy(muxMetric);
      double muxArea = getArea(muxMetric);
      double decodeLP = getLP(decodeMetric);
      double decodeEnergy = getEnergy(decodeMetric);
      double decodeDelay = getDelay(decodeMetric);
      double decodeArea = getArea(decodeMetric);
      double lp;
      double energy;
      double delay;
      double area;
      if (PIPELINE) {
        double *latchMetric = getOpMetric("latch1");
        double *hornMetric = getOpMetric("horn2");
        double *pulseGenMetric = getOpMetric("pulseGen");
        if (!latchMetric || !hornMetric || !pulseGenMetric || !decodeMetric
            || !mergeCtrlMetric || !muxMetric) {
          printf("No enough info to calculate metric for (%s, %s)!\n",
                 equivInstance, instance);
          exit(-1);
        }
        double latchLP = getLP(latchMetric);
        double latchEnergy = getEnergy(latchMetric);
        double latchArea = getArea(latchMetric);
        double hornLP = getLP(hornMetric);
        double hornEnergy = getEnergy(hornMetric);
        double hornDelay = getDelay(hornMetric);
        double hornArea = getArea(hornMetric);
        double pulseGenLP = getLP(pulseGenMetric);
        double pulseGenEnergy = getEnergy(pulseGenMetric);
        double pulseGenDelay = getDelay(pulseGenMetric);
        double pulseGenArea = getArea(pulseGenMetric);
        double pulseLP;
        double pulseEnergy;
        double pulseDelay;
        double pulseArea;
        if (inBW < 32) {
          pulseLP = pulseGenLP;
          pulseEnergy = pulseGenEnergy;
          pulseDelay = pulseGenDelay;
          pulseArea = pulseGenArea;
        } else {
          pulseLP = pulseGenLP + hornLP;
          pulseEnergy = pulseGenEnergy + hornEnergy;
          pulseDelay = pulseGenDelay + hornDelay;
          pulseArea = pulseGenArea + hornArea;
        }
        if (debug_verbose) {
          printf("For %s, mergeCtrlArea: %f, latchArea: %f, decodeArea: %f, "
                 "pulseArea: %f, muxArea: %f\n",
                 instance,
                 mergeCtrlArea,
                 latchArea,
                 decodeArea,
                 pulseArea,
                 muxArea);
        }
        lp = mergeCtrlLP + latchLP * guardBW + decodeLP
            + numIn * (pulseLP + inBW * muxLP / 2) + inBW * latchLP;
        energy = mergeCtrlEnergy + latchEnergy * guardBW + decodeEnergy
            + pulseEnergy + inBW * muxEnergy / 2 + inBW * latchEnergy;
        delay = mergeCtrlDelay + decodeDelay + pulseDelay;
        area = mergeCtrlArea + latchArea * guardBW + decodeArea
            + numIn * (pulseArea + inBW * muxArea / 2) + inBW * latchArea;
      } else {
        if (!decodeMetric || !mergeCtrlMetric || !muxMetric) {
          printf("No enough info to calculate metric for (%s, %s)!\n",
                 equivInstance, instance);
          exit(-1);
        }
        if (debug_verbose) {
          printf("For %s, mergeCtrlArea: %f, decodeArea: %f, muxArea: %f\n",
                 instance,
                 mergeCtrlArea,
                 decodeArea,
                 muxArea);
        }
        lp = mergeCtrlLP + decodeLP + numIn * (inBW * muxLP / 2);
        energy = mergeCtrlEnergy + decodeEnergy + inBW * muxEnergy / 2;
        delay = mergeCtrlDelay + decodeDelay;
        area = mergeCtrlArea + decodeArea + numIn * (inBW * muxArea / 2);
      }
      metric = new double[4];
      metric[0] = lp;
      metric[1] = energy;
      metric[2] = delay;
      metric[3] = area;
      updateMetrics(instance, metric);
      writeMetricsFile(instance, metric);
    }
  }
  updateStatistics(instance, actnCp, actnDp, metric);
  updateMergeMetrics(metric);
  return metric;
}

double *Metrics::getOrGenSplitMetric(unsigned guardBW,
                                     unsigned inBW,
                                     unsigned numOut,
                                     bool actnCp,
                                     bool actnDp) {
  if (!LOGIC_OPTIMIZER) {
    return nullptr;
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  if (PIPELINE) {
    sprintf(instance,
            "pipe%s_%d<%d,%d>",
            Constant::SPLIT_PREFIX,
            numOut,
            guardBW,
            inBW);
  } else {
    sprintf(instance,
            "unpipe%s_%d<%d,%d>",
            Constant::SPLIT_PREFIX,
            numOut,
            guardBW,
            inBW);
  }
  double *metric = getOpMetric(instance);
  if (!metric) {
    char *equivInstance = new char[MAX_INSTANCE_LEN];
    unsigned equivBW = getEquivalentBW(inBW);
    if (PIPELINE) {
      sprintf(equivInstance,
              "pipe%s_%d<%d,%d>",
              Constant::SPLIT_PREFIX,
              numOut,
              guardBW,
              equivBW);
    } else {
      sprintf(equivInstance,
              "unpipe%s_%d<%d,%d>",
              Constant::SPLIT_PREFIX,
              numOut,
              guardBW,
              equivBW);
    }
    metric = getOpMetric(equivInstance);
    if (!metric) {
      char *basicInstance = new char[MAX_INSTANCE_LEN];
      if (PIPELINE) {
        sprintf(basicInstance,
                "pipe%s_2<1,%d>",
                Constant::SPLIT_PREFIX,
                equivBW);
      } else {
        sprintf(basicInstance,
                "unpipe%s_2<1,%d>",
                Constant::SPLIT_PREFIX,
                equivBW);
      }
      double *basicMetric = getOpMetric(basicInstance);
      double *decodeMetric = getOpMetric("decodeTwoToFour");
      double *invMetric = getOpMetric("inv1");
      if (!basicMetric || !decodeMetric || !invMetric) {
        printf("No enough info to calculate metric for (%s, %s)!\n",
               equivInstance, instance);
        exit(-1);
      }
      double basicLP = getLP(basicMetric);
      double basicEnergy = getEnergy(basicMetric);
      double basicDelay = getDelay(basicMetric);
      double basicArea = getArea(basicMetric);
      double decodeLP = getLP(decodeMetric);
      double decodeEnergy = getEnergy(decodeMetric);
      double decodeDelay = getDelay(decodeMetric);
      double decodeArea = getArea(decodeMetric);
      double invLP = getLP(invMetric);
      double invEnergy = getEnergy(invMetric);
      double invDelay = getDelay(invMetric);
      double invArea = getArea(invMetric);
      double lp = basicLP + decodeLP + ceil((double) numOut / 2) * invLP;
      double energy =
          basicEnergy + decodeEnergy + +ceil((double) numOut / 2) * invEnergy;
      double
          delay =
          basicDelay + decodeDelay + ceil((double) numOut / 2) * invDelay;
      double
          area = basicArea + decodeArea + ceil((double) numOut / 2) * invArea;
      metric = new double[4];
      metric[0] = lp;
      metric[1] = energy;
      metric[2] = delay;
      metric[3] = area;
      updateMetrics(instance, metric);
      writeMetricsFile(instance, metric);
    }
  }
  updateStatistics(instance, actnCp, actnDp, metric);
  updateSplitMetrics(metric);
  return metric;
}

double *Metrics::getArbiterMetric(unsigned numInputs,
                                  unsigned inBW,
                                  unsigned coutBW,
                                  bool actnCp,
                                  bool actnDp) {
  double *metric = getOrGenMergeMetric(coutBW, inBW, numInputs, actnCp, actnDp);
  return metric;
}

double *Metrics::getMixerMetric(unsigned numInputs,
                                unsigned inBW) {
  unsigned ctrlBW = ceil(log2(numInputs));
  double *metric = getOrGenMergeMetric(ctrlBW, inBW, numInputs, false, false);
  return metric;
}

void Metrics::updateCopyStatistics(unsigned bitwidth, unsigned numOutputs) {
  auto copyStatisticsIt = copyStatistics.find(bitwidth);
  if (copyStatisticsIt != copyStatistics.end()) {
    Map<unsigned, unsigned> &record = copyStatisticsIt->second;
    auto recordIt = record.find(numOutputs);
    if (recordIt != record.end()) {
      recordIt->second++;
    } else {
      record.insert({numOutputs, 1});
    }
  } else {
    Map<unsigned, unsigned> record = {{numOutputs, 1}};
    copyStatistics.insert({bitwidth, record});
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
  for (auto &copyStatisticsIt: copyStatistics) {
    fprintf(statisticsFP, "%d-bit COPY:\n", copyStatisticsIt.first);
    Map<unsigned, unsigned> &record = copyStatisticsIt.second;
    for (auto &recordIt: record) {
      fprintf(statisticsFP, "  %u outputs: %u\n",
              recordIt.first, recordIt.second);
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

void Metrics::updateMergeMetrics(double metric[4]) {
  double area = getArea(metric);
  double leakPower = getLP(metric);
  mergeArea += area;
  mergeLeakPower += leakPower;
}

void Metrics::updateSplitMetrics(double metric[4]) {
  double area = getArea(metric);
  double leakPower = getLP(metric);
  splitArea += area;
  splitLeakPower += leakPower;
}

void Metrics::updateStatistics(const char *instance,
                               bool actnCp,
                               bool actnDp,
                               double metric[4]) {
  double area = getArea(metric);
  double leakPower = getLP(metric);
  totalArea += area;
  totalLeakPowewr += leakPower;
  bool exist = false;
  for (auto &areaStatisticsIt: areaStatistics) {
    if (!strcmp(areaStatisticsIt.first, instance)) {
      areaStatisticsIt.second += area;
      exist = true;
    }
  }
  if (exist) {
    bool foundLP = false;
    for (auto &leakpowerStatisticsIt: leakpowerStatistics) {
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
    for (auto &instanceCntIt: instanceCnt) {
      if (!strcmp(instanceCntIt.first, instance)) {
        instanceCntIt.second += 1;
        return;
      }
    }
    printf("We could find %s in areaStatistics, but not in instanceCnt!\n",
           instance);
    exit(-1);
  } else {
    areaStatistics.insert({instance, area});
    leakpowerStatistics.insert({instance, leakPower});
    instanceCnt.insert({instance, 1});
  }
  if (actnCp) {
    updateACTNCpMetrics(area, leakPower);
  } else if (actnDp) {
    updateACTNDpMetrics(area, leakPower);
  }
}

int Metrics::getInstanceCnt(const char *instance) {
  for (auto &instanceCntIt: instanceCnt) {
    if (!strcmp(instanceCntIt.first, instance)) {
      return instanceCntIt.second;
    }
  }
  printf("We could not find %s in instanceCnt!\n", instance);
  exit(-1);
}

double Metrics::getInstanceArea(const char *instance) {
  for (auto &areaStatisticsIt: areaStatistics) {
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
  for (auto iter = sortedAreas.rbegin(); iter != sortedAreas.rend(); iter++) {
    double area = iter->first;
    double ratio = (double) area / totalArea * 100;
    const char *instance = iter->second;
    if (ratio > 0.1) {
      int cnt = getInstanceCnt(instance);
      fprintf(statisticsFP, "%80.80s %5d %5.1f %5d\n", instance, area, ratio,
              cnt);
    }
  }
  fprintf(statisticsFP, "\n");
}

void Metrics::dump() {
  printStatistics();
}

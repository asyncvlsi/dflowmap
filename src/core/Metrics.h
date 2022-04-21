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

#ifndef DFLOWMAP_METRICS_H
#define DFLOWMAP_METRICS_H

#include <cstdio>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <act/act.h>
#include "src/common/common.h"
#include "src/common/Helper.h"
#include "src/common/config.h"
#if LOGIC_OPTIMIZER
#include <act/expropt.h>
#endif

class Metrics {
 public:
  Metrics(const char *customFUMetricsFP,
          const char *stdFUMetricsFP,
          const char *statisticsFP);

  void updateMetrics(const char *instance, double *metric);

  void updateCopyStatistics(unsigned bitwidth, unsigned numOutputs);

  void updateStatistics(const char *instName, double metric[4]);

  void printOpMetrics();

  double *getOpMetric(const char *instance);

  int getInstanceCnt(const char *instance);

  double getInstanceArea(const char *instance);

  void readMetricsFile();

  void writeMetricsFile(const char *instance, double *metric);

  void updateMergeMetrics(double metric[4]);

  void updateSplitMetrics(double metric[4]);

  void dump();

  static unsigned getEquivalentBW(unsigned oriBW);

  double *getOrGenCopyMetric(unsigned bitwidth, unsigned numOut);

  double *getSinkMetric(unsigned bitwidth);

  double *getOrGenInitMetric(unsigned bitwidth);

  double *getBuffMetric(unsigned nBuff, unsigned bw);

  void callLogicOptimizer(const char *instance,
                          StringMap<unsigned> &inBW,
                          StringMap<unsigned> &hiddenBW,
                          Map<const char *, Expr *> &exprMap,
                          Map<Expr *, Expr *> &hiddenExprs,
                          Map<unsigned int, unsigned int> &outRecord,
                          UIntVec &outBWList,
                          double *&metric);

  double *getOrGenFUMetric(
#if LOGIC_OPTIMIZER
      StringMap<unsigned> &inBW,
      StringMap<unsigned> &hiddenBW,
      Map<const char *, Expr *> &exprMap,
      Map<Expr *, Expr *> &hiddenExprs,
      Map<unsigned int, unsigned int> &outRecord,
      UIntVec &outBWList,
#endif
      const char *instance);

  double *getSourceMetric(const char *instance);

  double *getOrGenMergeMetric(unsigned guardBW, unsigned inBW, unsigned numIn);

  double *getOrGenSplitMetric(unsigned guardBW, unsigned inBW, unsigned numOut);

  double *getArbiterMetric(unsigned numInputs, unsigned inBW, unsigned coutBW);

  double *getMixerMetric(unsigned numInputs,
                         unsigned inBW,
                         unsigned coutBW);

 private:
  /* operator, (leak power (nW), dyn energy (e-15J), delay (ps), area (um^2)) */
  Map<const char *, double *> opMetrics;

  /* copy bitwidth,< # of output, # of instances of this COPY> */
  Map<unsigned, Map<unsigned, unsigned >> copyStatistics;

  double totalArea = 0;

  /* instanceName, area (um^2) of all of the instances of the process */
  Map<const char *, double> areaStatistics;

  double totalLeakPowewr = 0;

  /* instanceName, LeakPower (nW) of all of the instances of the process */
  Map<const char *, double> leakpowerStatistics;

  double mergeArea = 0;

  double splitArea = 0;

  double mergeLeakPower = 0;

  double splitLeakPower = 0;

  /* instanceName, # of instances */
  Map<const char *, int> instanceCnt;

  const char *customFUMetricsFP;

  const char *stdFUMetricsFP;

  const char *statisticsFilePath;

  void printLeakpowerStatistics(FILE *statisticsFP);

  void printAreaStatistics(FILE *statisticsFP);

  void printCopyStatistics(FILE *statisticsFP);

  void printStatistics();

  static double getArea(double metric[4]);

  static double getLP(double metric[4]);

  static double getEnergy(double metric[4]);

  static double getDelay(double metric[4]);

  void readMetricsFile(const char *metricsFP);
};

#endif //DFLOWMAP_METRICS_H

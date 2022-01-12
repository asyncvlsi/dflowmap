#ifndef DFLOWMAP_METRICS_H
#define DFLOWMAP_METRICS_H

#include <cstdio>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <act/act.h>
#include "common.h"
#include "Helper.h"
#if LOGIC_OPTIMIZER
#include <act/expropt.h>
#endif

class Metrics {
 public:
  Metrics(const char *metricFP, const char *statisticsFP);

  void updateMetrics(const char *instance, double *metric);

  void updateCopyStatistics(unsigned bitwidth, unsigned numOutputs);

  void updateStatistics(const char *instName,
                        bool actnCp,
                        bool actnDp,
                        double metric[4]);

  void printOpMetrics();

  double *getOpMetric(const char *instance);

  int getInstanceCnt(const char *instance);

  double getInstanceArea(const char *instance);

  void readMetricsFile();

  void writeMetricsFile(const char *instance, double *metric);

  void updateMergeMetrics(double metric[4]);

  void updateSplitMetrics(double metric[4]);

  void updateACTNCpMetrics(double area, double leakPower);

  void updateACTNDpMetrics(double area, double leakPower);

  void dump();

  unsigned getEquivalentBW(unsigned oriBW);

  double *getOrGenCopyMetric(unsigned bitwidth, unsigned numOut);

  double *getSinkMetric(unsigned bitwidth);

  double *getOrGenInitMetric(unsigned bitwidth);

  double *getBuffMetric(unsigned nBuff, unsigned bw);

  double *getOrGenFUMetric(const char *instName,
                           StringMap<unsigned> &inBW,
                           StringMap<unsigned> &hiddenBW,
                           Map<const char *, Expr *> &exprMap,
                           Map<Expr *, Expr *> &hiddenExprs,
                           Map<unsigned int, unsigned int> &outRecord,
                           UIntVec &outWidthList);

  double *getSourceMetric(const char *instance, unsigned int bitwidth);

  double *getOrGenMergeMetric(unsigned guardBW,
                              unsigned inBW,
                              unsigned numIn,
                              bool actnCp,
                              bool actnDp);

  double *getOrGenSplitMetric(unsigned guardBW,
                              unsigned inBW,
                              unsigned numOut,
                              bool actnCp,
                              bool actnDp);

  double *getArbiterMetric(unsigned numInputs,
                           unsigned inBW,
                           unsigned coutBW,
                           bool actnCp,
                           bool actnDp);

 private:
  /* operator, (leak power (nW), dyn energy (e-15J), delay (ps), area (um^2)) */
  Map<const char *, double *> opMetrics;

  /* copy bitwidth,< # of output, # of instances of this COPY> */
  Map<int, Map<int, int>> copyStatistics;

  double totalArea = 0;

  /* instanceName, area (um^2) of all of the instances of the process */
  Map<const char *, double> areaStatistics;

  double totalLeakPowewr = 0;

  /* instanceName, LeakPower (nW) of all of the instances of the process */
  Map<const char *, double> leakpowerStatistics;

  double mergeArea = 0;

  double splitArea = 0;

  double actnCpArea = 0;

  double actnDpArea = 0;

  double mergeLeakPower = 0;

  double splitLeakPower = 0;

  double actnCpLeakPower = 0;

  double actnDpLeakPower = 0;

  /* instanceName, # of instances */
  Map<const char *, int> instanceCnt;

  const char *metricFilePath;

  const char *statisticsFilePath;

  void printLeakpowerStatistics(FILE *statisticsFP);

  void printAreaStatistics(FILE *statisticsFP);

  void printCopyStatistics(FILE *statisticsFP);

  void printStatistics();

  double getArea(double metric[4]);

  double getLP(double metric[4]);

  double getEnergy(double metric[4]);

  double getDelay(double metric[4]);
};

#endif //DFLOWMAP_METRICS_H

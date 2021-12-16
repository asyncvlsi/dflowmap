#ifndef DFLOWMAP_METRICS_H
#define DFLOWMAP_METRICS_H

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <act/act.h>
#include "common.h"
#include "Constant.h"

class Metrics {
 public:
  Metrics(const char *metricFP, const char *statisticsFP);

  void updateMetrics(const char *op, double *metric);

  void updateCopyStatistics(unsigned bitwidth, unsigned numOutputs);

  void updateStatistics(const char *instance, double area, double leakPower);

  void printOpMetrics();

  static void getNormalizedOpName(const char *op, char *normalizedOp);

  static void normalizeName(char *src, char toDel, char newChar);

  double *getOpMetric(const char *op);

  int getInstanceCnt(const char *instance);

  double getInstanceArea(const char *instance);

  void readMetricsFile();

  void writeMetricsFile(char *opName, double metric[4]);

  void updateMergeMetrics(double area, double leakPower);

  void updateSplitMetrics(double area, double leakPower);

  void updateACTNCpMetrics(double area, double leakPower);

  void updateACTNDpMetrics(double area, double leakPower);

  void dump();

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
};

#endif //DFLOWMAP_METRICS_H

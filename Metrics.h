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

  void updateMetrics(const char *op, long *metric);

  void updateCopyStatistics(unsigned bitwidth, unsigned numOutputs);

  void updateStatistics(const char *instance, long area, long leakPower);

  void printOpMetrics();

  static void getNormalizedOpName(const char *op, char *normalizedOp);

  static void normalizeName(char *src, char toDel, char newChar);

  long *getOpMetric(const char *op);

  int getInstanceCnt(const char *instance);

  long getInstanceArea(const char *instance);

  void readMetricsFile();

  void writeMetricsFile(char *opName, long metric[4]);

  void updateMergeMetrics(long area, long leakPower);

  void updateSplitMetrics(long area, long leakPower);

  void updateACTNCpMetrics(long area, long leakPower);

  void updateACTNDpMetrics(long area, long leakPower);

  void dump();

private:
  /* operator, (leak power (nW), dyn energy (e-15J), delay (ps), area (um^2)) */
  Map<const char *, long *> opMetrics;

  /* copy bitwidth,< # of output, # of instances of this COPY> */
  Map<int, Map<int, int>> copyStatistics;

  long totalArea = 0;

  /* instanceName, area (um^2) of all of the instances of the process */
  Map<const char *, long> areaStatistics;

  long totalLeakPowewr = 0;

  /* instanceName, LeakPower (nW) of all of the instances of the process */
  Map<const char *, long> leakpowerStatistics;

  long mergeArea = 0;

  long splitArea = 0;

  long actnCpArea = 0;

  long actnDpArea = 0;

  long mergeLeakPower = 0;

  long splitLeakPower = 0;

  long actnCpLeakPower = 0;

  long actnDpLeakPower = 0;

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

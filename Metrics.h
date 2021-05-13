#ifndef DFLOWMAP_METRICS_H
#define DFLOWMAP_METRICS_H

#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <act/act.h>
#include "common.h"

class Metrics {
public:
  Metrics(char *metricFP);

  void updateMetrics(const char *op, int *metric);

  void updateCopyStatistics(unsigned bitwidth, unsigned numOutputs);

  void printCopyStatistics();

  void updateAreaStatistics(const char *instance, int area);

  void printAreaStatistics();

  void printOpMetrics();

  void getNormalizedOpName(const char *op, char *normalizedOp);

  int *getOpMetric(const char *op);

  void readMetricsFile();

  void writeMetricsFile(char *opName, int metric[4]);

  void dump();

private:
  /* operator, (leak power (nW), dyn energy (e-15J), delay (ps), area (um^2)) */
  Map<const char *, int *> opMetrics;

  /* copy bitwidth,< # of output, # of instances of this COPY> */
  Map<int, Map<int, int>> copyStatistics;

  int totalArea;
  /* procName, area of all of the instances of the process */
  Map<const char *, int> areaStatistics;

  void normalizeName(char *src, char toDel, char newChar);

  char *metricFilePath;
};


#endif //DFLOWMAP_METRICS_H

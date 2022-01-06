#ifndef DFLOWMAP_CHPLIBGENERATOR_H
#define DFLOWMAP_CHPLIBGENERATOR_H

#include <cstring>
#include <algorithm>
#include <act/act.h>
#include <fstream>
#include "common.h"

class ChpLibGenerator {
 public:
  ChpLibGenerator(FILE *libFp, FILE *confFp);

  void genMemConfiguration(const char *procName);

  void createConf(const char *instance, unsigned numOutputs, double *metric);

  void createConf(const char *instance, double *metric);

  void createFU(const char *procName,
                const char *calc,
                unsigned int numArgs,
                unsigned int numOuts,
                const char *instance,
                double *fuMetric,
                UIntVec &resBW,
                UIntVec &outBW,
                StringVec &outSendStr,
                IntVec &outResSuffixs,
                Vector<BuffInfo> &buffInfos);

  void createFULib(const char *procName,
                   const char *calc,
                   const char *outSend,
                   unsigned int numArgs,
                   unsigned int numOuts,
                   const char *instance,
                   double *metric,
                   UIntVec &resBW,
                   UIntVec &outBW);

  void createMerge(const char *procName,
                   const char *instance,
                   double *metric,
                   int numInputs);

  void createSplit(const char *procName,
                   const char *instance,
                   double *metric,
                   int numOutputs);

  void createArbiter(const char *procName,
                     const char *instance,
                     double *metric,
                     int numInputs);

  void createSource(const char *instance, double *metric);

  void createInit(const char *instance, double *metric);

  void createOneBuff(const char *instance, double *metric);

  void createBuff(Vector<BuffInfo> &buffInfos);

  void createSink(const char *instance, double *metric);

  void createCopy(const char *instance, double *metric);

  void createChpBlock(Process *p);

 private:
  const char *processes[MAX_PROCESSES];
  const char *instances[MAX_PROCESSES];

  FILE *libFp;
  FILE *confFp;

  bool hasInstance(const char *instance);

  bool hasProcess(const char *process);

};

#endif //DFLOWMAP_CHPLIBGENERATOR_H

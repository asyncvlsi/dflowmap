#ifndef DFLOWMAP_CHPPROCGENERATOR_H
#define DFLOWMAP_CHPPROCGENERATOR_H

#include <cstring>
#include <algorithm>
#include <fstream>
#include "common.h"

class ChpProcGenerator {
 public:
  void initialize();

  void genMemConfiguration(FILE *confFp, const char *procName);

  void createConf(FILE *confFp,
                  const char *instance,
                  unsigned numOutputs,
                  double *metric);

  void createConf(FILE *confFp,
                  const char *instance,
                  double *metric);

  void createFULib(FILE *libFp,
                   FILE *confFp,
                   const char *procName,
                   const char *calc,
                   const char *def,
                   const char *outSend,
                   int numArgs,
                   int numOuts,
                   const char *instance,
                   double *metric,
                   UIntVec &resBW,
                   UIntVec &outBW,
                   IntVec &queryResSuffixs,
                   IntVec &queryResSuffixs2);

  void createMerge(FILE *libFp,
                   FILE *confFp,
                   const char *procName,
                   const char *instance,
                   double *metric,
                   int numInputs);

  void createSplit(FILE *libFp,
                   FILE *confFp,
                   const char *procName,
                   const char *instance,
                   double *metric,
                   int numOutputs);

  void createArbiter(FILE *libFp,
                     FILE *confFp,
                     const char *procName,
                     const char *instance,
                     double *metric,
                     int numInputs);

  void createSource(FILE *libFp,
                    FILE *confFp,
                    const char *instance,
                    double *metric);

  void createInit(FILE *libFp,
                  FILE *confFp,
                  const char *instance,
                  double *metric);

  void createBuff(FILE *libFp,
                  FILE *confFp,
                  const char *instance,
                  double *metric);

  void createSink(FILE *libFp,
                  FILE *confFp,
                  const char *instance,
                  double *metric);

  void createCopy(FILE *libFp,
                  FILE *confFp,
                  const char *instance,
                  double *metric);

 private:
  const char *processes[MAX_PROCESSES];
  const char *instances[MAX_PROCESSES];

  bool hasInstance(const char *instance);

  bool hasProcess(const char *process);

};

#endif //DFLOWMAP_CHPPROCGENERATOR_H

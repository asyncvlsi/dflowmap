#ifndef DFLOWMAP_CHPPROCGENERATOR_H
#define DFLOWMAP_CHPPROCGENERATOR_H

#include <string.h>
#include <algorithm>
#include <fstream>
#include "common.h"

class ChpProcGenerator {
public:
  void initialize();

  void genMemConfiguration(FILE *confFp, const char *procName);

  void
  createFULib(FILE *libFp, FILE *confFp, const char *procName, const char *calc,
              const char *def, const char *outSend, int numArgs,
              int numOuts, int numRes, const char *instance, int *metric,
              IntVec &boolRes);

  void
  createMerge(FILE *libFp, FILE *confFp, const char *procName, const char *instance,
              int *metric, int numInputs);

  void
  createSplit(FILE *libFp, FILE *confFp, const char *procName, const char *instance,
              int *metric, int numOutputs);

  void createSource(FILE *libFp, FILE *confFp, const char *instance, int *metric);

  void createInit(FILE *libFp, FILE *confFp, const char *instance, int *metric);

  void createBuff(FILE *libFp, FILE *confFp, const char *instance, int *metric);

  void createSink(FILE *libFp, FILE *confFp, const char *instance, int *metric);

  void createCopy(FILE *libFp, FILE *confFp, const char *instance, int *metric);

private:
  const char *processes[MAX_PROCESSES];
  const char *instances[MAX_PROCESSES];

  bool hasInstance(const char *instance);

  bool hasProcess(const char *process);

};


#endif //DFLOWMAP_CHPPROCGENERATOR_H

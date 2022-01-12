//
// Created by ruile on 12/31/2021.
//

#include "ChpBackend.h"

void ChpBackend::createCopyProcs(const char *inName,
                                 unsigned bw,
                                 unsigned numOut,
                                 double *metric) {
  circuitGenerator->printCopy(inName, bw, numOut);
  char *instName = new char[1500];
  sprintf(instName, "copy<%u,%u>", bw, numOut);
  libGenerator->createCopy(instName, metric);
}

void ChpBackend::printSink(const char *inName, unsigned bw, double metric[4]) {
  circuitGenerator->printSink(inName, bw);
  char *instName = new char[1500];
  sprintf(instName, "sink<%u>", bw);
  libGenerator->createSink(instName, metric);
}

void ChpBackend::printBuff(Vector<BuffInfo> &buffInfos) {
  circuitGenerator->printBuff(buffInfos);
  libGenerator->createBuff(buffInfos);
}

void ChpBackend::printChannel(const char *chanName, unsigned int bitwidth) {
  circuitGenerator->printChannel(chanName, bitwidth);
}

void ChpBackend::printSource(const char *outName,
                             const char *instance,
                             double metric[4]) {
  circuitGenerator->printSource(instance, outName);
  libGenerator->createSource(instance, metric);
}

void ChpBackend::printFU(const char *procName,
                         const char *instName,
                         StringVec &argList,
                         UIntVec &argBWList,
                         UIntVec &resBWList,
                         UIntVec &outBWList,
                         const char *calc,
                         StringVec &outList,
                         Map<unsigned int, unsigned int> &outRecord,
                         Vector<BuffInfo> &buffInfos,
                         double *fuMetric) {
  /* handle normal fu */
  circuitGenerator->printFunc(instName,
                              argList,
                              argBWList,
                              resBWList,
                              outBWList,
                              outList,
                              buffInfos);
  unsigned numArgs = argList.size();
  unsigned numOuts = outList.size();
  libGenerator->createFU(procName,
                         calc,
                         numArgs,
                         numOuts,
                         instName,
                         fuMetric,
                         resBWList,
                         outBWList,
                         outRecord,
                         buffInfos);
}

void ChpBackend::printSplit(const char *splitName,
                            const char *guardStr,
                            const char *inputStr,
                            unsigned guardBW,
                            unsigned outBW,
                            CharPtrVec &outNameVec,
                            int numOut,
                            double *metric) {
  char *procName = new char[MAX_PROC_NAME_LEN];
  if (PIPELINE) {
    sprintf(procName, "pipe%s_%d", Constant::SPLIT_PREFIX, numOut);
  } else {
    sprintf(procName, "unpipe%s_%d", Constant::SPLIT_PREFIX, numOut);
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<%d,%d>", procName, guardBW, outBW);
  circuitGenerator->printSplit(procName,
                               splitName,
                               guardStr,
                               inputStr,
                               guardBW,
                               outBW,
                               outNameVec);
  libGenerator->createSplit(procName, instance, metric, numOut);
}

void ChpBackend::printMerge(const char *outName,
                            const char *guardStr,
                            unsigned guardBW,
                            unsigned inBW,
                            CharPtrVec &inNameVec,
                            int numIn,
                            double *metric) {
  char *procName = new char[MAX_PROC_NAME_LEN];
  if (PIPELINE) {
    sprintf(procName, "pipe%s_%d", Constant::MERGE_PREFIX, numIn);
  } else {
    sprintf(procName, "unpipe%s_%d", Constant::MERGE_PREFIX, numIn);
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<%d,%d>", procName, guardBW, inBW);
  circuitGenerator->printMerge(procName,
                               outName,
                               guardStr,
                               guardBW,
                               inBW,
                               inNameVec);
  libGenerator->createMerge(procName, instance, metric, numIn);
}

void ChpBackend::printArbiter(const char *procName,
                              const char *instance,
                              const char *outName,
                              const char *coutName,
                              unsigned outBW,
                              unsigned coutBW,
                              int numIn,
                              CharPtrVec &inNameVec,
                              double *metric) {
  circuitGenerator->printArbiter(outName,
                                 coutName,
                                 outBW,
                                 coutBW,
                                 numIn,
                                 inNameVec);
  libGenerator->createArbiter(procName, instance, metric, numIn);
}

void ChpBackend::printProcHeader(Process *p) {
  circuitGenerator->printProcHeader(p);
}

void ChpBackend::printProcEnding() {
  circuitGenerator->printProcEnding();
}

void ChpBackend::createChpBlock(Process *p) {
  libGenerator->createChpBlock(p);
}

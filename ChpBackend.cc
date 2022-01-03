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

void ChpBackend::printInit(const char *outName,
                           unsigned int bitwidth,
                           unsigned long initVal,
                           double metric[4]) {
  circuitGenerator->printInit(outName, bitwidth, initVal);
  char *instName = new char[100];
  sprintf(instName, "init%u", bitwidth);
  libGenerator->createInit(instName, metric);
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
                         const char *def,
                         const char *calc,
                         StringVec &outSendStr,
                         IntVec &outResSuffixs,
                         StringVec &normalizedOutList,
                         StringVec &outList,
                         Map<unsigned, unsigned long> &initMap,
                         Map<unsigned, unsigned long> &buffMap,
                         double *metric) {
  circuitGenerator->printFunc(instName,
                              argList,
                              argBWList,
                              resBWList,
                              outBWList,
                              normalizedOutList,
                              outList,
                              initMap,
                              buffMap);
  unsigned numArgs = argList.size();
  unsigned numOuts = outList.size();
  libGenerator->createFU(procName,
                         calc,
                         def,
                         numArgs,
                         numOuts,
                         instName,
                         metric,
                         resBWList,
                         outBWList,
                         outSendStr,
                         outResSuffixs);
}

void ChpBackend::printSplit(const char *procName,
                            const char *splitName,
                            const char *guardStr,
                            const char *inputStr,
                            unsigned guardBW,
                            unsigned outBW,
                            CharPtrVec &outNameVec,
                            const char *instance,
                            int numOut,
                            double *metric) {
  circuitGenerator->printSplit(procName,
                               splitName,
                               guardStr,
                               inputStr,
                               guardBW,
                               outBW,
                               outNameVec);
  libGenerator->createSplit(procName, instance, metric, numOut);
}

void ChpBackend::printMerge(const char *procName,
                            const char *outName,
                            const char *guardStr,
                            unsigned guardBW,
                            unsigned inBW,
                            CharPtrVec &inNameVec,
                            const char *instance,
                            int numIn,
                            double *metric) {
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
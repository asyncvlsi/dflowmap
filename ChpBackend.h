//
// Created by ruile on 12/31/2021.
//

#ifndef DFLOWMAP__CHPBACKEND_H_
#define DFLOWMAP__CHPBACKEND_H_

#include "ChpCircuitGenerator.h"
#include "ChpLibGenerator.h"

class ChpBackend {
 public:
  ChpBackend(ChpCircuitGenerator *circuitGenerator,
             ChpLibGenerator *libGenerator) {
    this->circuitGenerator = circuitGenerator;
    this->libGenerator = libGenerator;
  }

  void createCopyProcs(const char *inName,
                       unsigned bw,
                       unsigned numOut,
                       double *metric);

  void printSink(const char *inName, unsigned bw, double metric[4]);

  void printBuff(Vector<BuffInfo> &buffInfos);

  void printChannel(const char *chanName, unsigned bitwidth);

  void printSource(const char *outName,
                   const char *instance,
                   double metric[4]);

  void printFU(const char *procName,
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
               Vector<BuffInfo> &buffInfos,
               double fuMetric[4]);

  void printSplit(const char *procName,
                  const char *splitName,
                  const char *guardStr,
                  const char *inputStr,
                  unsigned guardBW,
                  unsigned outBW,
                  CharPtrVec &outNameVec,
                  const char *instance,
                  int numOut,
                  double metric[4]);

  void printMerge(const char *procName,
                  const char *outName,
                  const char *guardStr,
                  unsigned guardBW,
                  unsigned inBW,
                  CharPtrVec &inNameVec,
                  const char *instance,
                  int numIn,
                  double metric[4]);

  void printArbiter(const char *procName,
                    const char *instance,
                    const char *outName,
                    const char *coutName,
                    unsigned outBW,
                    unsigned coutBW,
                    int numIn,
                    CharPtrVec &inNameVec,
                    double *metric);

  void printProcHeader(Process *p);

  void printProcEnding();

  void createChpBlock(Process *p);

 private:
  ChpCircuitGenerator *circuitGenerator;
  ChpLibGenerator *libGenerator;
};

#endif //DFLOWMAP__CHPBACKEND_H_

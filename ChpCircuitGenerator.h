//
// Created by ruile on 12/31/2021.
//

#ifndef DFLOWMAP__CHPCIRCUITGENERATOR_H_
#define DFLOWMAP__CHPCIRCUITGENERATOR_H_

#include <act/act.h>
#include <fstream>
#include "Helper.h"

class ChpCircuitGenerator {
 private:
  FILE *resFp;
 public:
  explicit ChpCircuitGenerator(FILE *resFp);

  void printSink(const char *inName, unsigned bitwidth);

  void printCopy(const char *inputName,
                 unsigned int bw,
                 unsigned int numOut);

  void printEmptyLine();

  void printInit(const char *inName,
                 const char *outName,
                 unsigned bitwidth,
                 unsigned long initVal);

  void printBuff(const char *inName, const char *outName, unsigned bitwidth);

  void printChannel(const char* chanName, unsigned bitwidth);

  void printSource(const char *instance, const char *outName);

  void printFunc(const char *instance,
                 StringVec &argList,
                 UIntVec &argBWList,
                 UIntVec &resBWList,
                 UIntVec &outWidthList,
                 StringVec &normalizedOutList,
                 StringVec &outList,
                 Map<unsigned, unsigned long> &initMap,
                 Map<unsigned, unsigned long> &buffMap);

  void printSplit(const char *procName,
                  const char *splitName,
                  const char *guardStr,
                  const char *inputStr,
                  unsigned guardBW,
                  unsigned outBW,
                  CharPtrVec &outNameVec);

  void printMerge(const char *procName,
                  const char *outName,
                  const char *guardStr,
                  unsigned guardBW,
                  unsigned inBW,
                  CharPtrVec &inNameVec);

  void printArbiter(const char *outName,
                    const char *coutName,
                    unsigned outBW,
                    unsigned coutBW,
                    int numIn,
                    CharPtrVec &inNameVec);

  void printProcHeader(Process *p);

  void printProcEnding();
};

#endif //DFLOWMAP__CHPCIRCUITGENERATOR_H_

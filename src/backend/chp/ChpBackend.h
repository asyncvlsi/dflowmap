/*
 * This file is part of the ACT library
 *
 * Copyright (c) 2021 Rui Li
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef DFLOWMAP__CHPBACKEND_H_
#define DFLOWMAP__CHPBACKEND_H_

#include "src/backend/chp/ChpCircuitGenerator.h"
#include "src/backend/chp/ChpLibGenerator.h"

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
               const char *calc,
               StringVec &outList,
               Map<unsigned int, unsigned int> &outRecord,
               Vector<BuffInfo> &buffInfos,
               double *fuMetric);

  void printSplit(const char *splitName,
                  const char *guardStr,
                  const char *inputStr,
                  unsigned guardBW,
                  unsigned outBW,
                  CharPtrVec &outNameVec,
                  int numOut,
                  double *metric);

  void printMerge(const char *outName,
                  const char *guardStr,
                  unsigned guardBW,
                  unsigned inBW,
                  CharPtrVec &inNameVec,
                  int numIn,
                  double *metric);

  void printArbiter(const char *procName,
                    const char *instance,
                    const char *outName,
                    const char *coutName,
                    unsigned outBW,
                    unsigned coutBW,
                    int numIn,
                    CharPtrVec &inNameVec,
                    double *metric);

  void printMixer(const char *outName,
                  unsigned dataBW,
                  CharPtrVec &inNameVec,
                  double *metric);

  void printProcHeader(Process *p);

  void printProcDeclaration(Process *p);

  void printProcEnding();

  void createChpBlock(Process *p);

  void printCustomNamespace(ActNamespace *ns);

  void printFileEnding();

 private:
  ChpCircuitGenerator *circuitGenerator;
  ChpLibGenerator *libGenerator;
};

#endif //DFLOWMAP__CHPBACKEND_H_

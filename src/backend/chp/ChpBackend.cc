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

void ChpBackend::printProcDeclaration(Process *p) {
  circuitGenerator->printProcDeclaration(p);
}

void ChpBackend::printProcEnding() {
  circuitGenerator->printProcEnding();
}

void ChpBackend::createChpBlock(Process *p) {
  libGenerator->createChpBlock(p);
}

void ChpBackend::printCustomNamespace(ActNamespace *ns) {

}

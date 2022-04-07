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
  char *procName = new char[1024];
  sprintf(procName, "dflowstd::copy");
  char *instance = new char[1024];
  sprintf(instance, "%s<%u,%u>", procName, bw, numOut);
  circuitGenerator->printCopy(instance, inName);
  libGenerator->createCopy(instance, metric);
}

void ChpBackend::printSink(const char *inName, unsigned bw, double metric[4]) {
  char *instance = new char[1500];
  sprintf(instance, "dflowstd::sink<%u>", bw);
  circuitGenerator->printSink(instance, inName);
  libGenerator->createSink(instance, metric);
}

void ChpBackend::printBuff(Vector<BuffInfo> &buffInfos) {
  circuitGenerator->printBuff(buffInfos);
  libGenerator->createBuff(buffInfos);
}

void ChpBackend::printChannel(const char *chanName, unsigned int bitwidth) {
  circuitGenerator->printChannel(chanName, bitwidth);
}

void ChpBackend::printSource(const char *instance,
                             const char *outName,
                             double metric[4]) {
  circuitGenerator->printSource(instance, outName);
  libGenerator->createSource(instance, metric);
}

void ChpBackend::printFU(const char *procName,
                         const char *instance,
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
  circuitGenerator->printFunc(instance,
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
                         instance,
                         fuMetric,
                         resBWList,
                         outBWList,
                         outRecord,
                         buffInfos);
}

void ChpBackend::printSplit(const char *splitName,
                            const char *guardName,
                            const char *inputName,
                            unsigned guardBW,
                            unsigned outBW,
                            CharPtrVec &outNameVec,
                            int numOut,
                            double *metric) {
  char *procName = new char[MAX_PROC_NAME_LEN];
  if (PIPELINE) {
    sprintf(procName, "dflowstd::pipe_%s%d", Constant::SPLIT_PREFIX, numOut);
  } else {
    sprintf(procName, "dflowstd::unpipe_%s%d", Constant::SPLIT_PREFIX, numOut);
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<%d,%d>", procName, guardBW, outBW);
  circuitGenerator->printSplit(instance,
                               splitName,
                               guardName,
                               inputName,
                               outNameVec);
  libGenerator->createSplit(instance, metric, numOut);
}

void ChpBackend::printMerge(const char *outName,
                            const char *guardName,
                            unsigned guardBW,
                            unsigned inBW,
                            CharPtrVec &inNameVec,
                            int numInputs,
                            double *metric) {
  char *procName = new char[MAX_PROC_NAME_LEN];
  if (PIPELINE) {
    sprintf(procName, "dflowstd::pipe_%s%d", Constant::MERGE_PREFIX, numInputs);
  } else {
    sprintf(procName,
            "dflowstd::unpipe_%s%d",
            Constant::MERGE_PREFIX,
            numInputs);
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<%d,%d>", procName, guardBW, inBW);
  circuitGenerator->printMerge(instance,
                               outName,
                               guardName,
                               inNameVec);
  libGenerator->createMerge(instance, metric);
}

void ChpBackend::printMixer(const char *outName,
                            unsigned dataBW,
                            CharPtrVec &inNameVec,
                            double *metric) {
  unsigned numInputs = inNameVec.size();
  char *procName = new char[MAX_PROC_NAME_LEN];
  if (PIPELINE) {
    sprintf(procName, "dflowstd::pipe_%s%d", Constant::MIXER_PREFIX, numInputs);
  } else {
    sprintf(procName,
            "dflowstd::unpipe_%s%d",
            Constant::MIXER_PREFIX,
            numInputs);
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<%d>", procName, dataBW);
  circuitGenerator->printMixer(instance,
                               outName,
                               inNameVec);
  libGenerator->createMixer(instance, metric);
}

void ChpBackend::printArbiter(const char *outName,
                              const char *coutName,
                              unsigned outBW,
                              unsigned coutBW,
                              int numInputs,
                              CharPtrVec &inNameVec,
                              double *metric) {
  char *procName = new char[MAX_PROC_NAME_LEN];
  if (PIPELINE) {
    sprintf(procName,
            "dflowstd::pipe_%s%d",
            Constant::ARBITER_PREFIX,
            numInputs);
  } else {
    sprintf(procName,
            "dflowstd::unpipe_%s%d",
            Constant::ARBITER_PREFIX,
            numInputs);
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<%d,%d>", procName, outBW, coutBW);
  circuitGenerator->printArbiter(instance,
                                 outName,
                                 coutName,
                                 inNameVec);
  libGenerator->createArbiter(instance, metric);
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
  circuitGenerator->printCustomNamespace(ns);
  libGenerator->printCustomNamespace(ns);
}

void ChpBackend::printFileEnding() {
  circuitGenerator->printFileEnding();
  libGenerator->printFileEnding();
}

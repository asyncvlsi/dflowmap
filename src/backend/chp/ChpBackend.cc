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

void ChpBackend::createCopyProcs(const char *instance,
                                 const char *inName,
                                 double *metric) {
  circuitGenerator->printCopy(instance, inName);
  libGenerator->createCopy(instance, metric);
}

void ChpBackend::printSink(const char *instance,
                           const char *inName,
                           double metric[4]) {
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

void ChpBackend::printFU(const char *instance,
                         const char *procName,
                         StringVec &argList,
                         UIntVec &resBWList,
                         const char *calc,
                         StringVec &outList,
                         Map<unsigned int, unsigned int> &outRecord,
                         Vector<BuffInfo> &buffInfos,
                         double *fuMetric) {
  /* handle normal fu */
  circuitGenerator->printFunc(instance,
                              argList,
                              outList,
                              buffInfos);
  unsigned numArgs = argList.size();
  unsigned numOuts = outList.size();
  libGenerator->createFU(instance,
                         procName,
                         calc,
                         numArgs,
                         numOuts,
                         fuMetric,
                         resBWList,
                         outRecord,
                         buffInfos);
}

void ChpBackend::printSplit(const char *instance,
                            const char *splitName,
                            const char *guardName,
                            const char *inputName,
                            CharPtrVec &outNameVec,
                            int numOut,
                            double *metric) {
  circuitGenerator->printSplit(instance,
                               splitName,
                               guardName,
                               inputName,
                               outNameVec);
  libGenerator->createSplit(instance, metric, numOut);
}

void ChpBackend::printMerge(const char *instance,
                            const char *outName,
                            const char *guardName,
                            CharPtrVec &inNameVec,
                            double *metric) {
  circuitGenerator->printMerge(instance,
                               outName,
                               guardName,
                               inNameVec);
  libGenerator->createMerge(instance, metric);
}

void ChpBackend::printMixer(const char *instance,
                            const char *outName,
                            CharPtrVec &inNameVec,
                            double *metric) {
  circuitGenerator->printMixer(instance,
                               outName,
                               inNameVec);
  libGenerator->createMixer(instance, metric);
}

void ChpBackend::printArbiter(const char *instance,
                              const char *outName,
                              const char *coutName,
                              CharPtrVec &inNameVec,
                              double *metric) {
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

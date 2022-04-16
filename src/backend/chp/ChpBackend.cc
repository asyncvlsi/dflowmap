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

ChpBackend::ChpBackend(ChpGenerator *chpGenerator,
                       ChpLibGenerator *chpLibGenerator) {
  this->chpGenerator = chpGenerator;
  this->chpLibGenerator = chpLibGenerator;
}

ChpBackend::ChpBackend(ChpGenerator *chpGenerator,
                       ChpLibGenerator *chpLibGenerator,
                       NetlistBackend *netlistBackend) {
  this->chpGenerator = chpGenerator;
  this->chpLibGenerator = chpLibGenerator;
  this->netlistBackend = netlistBackend;
}

void ChpBackend::printCopyProcs(const char *instance,
                                const char *inName,
                                double *metric) {
  chpGenerator->printCopyChp(instance, inName);
  chpLibGenerator->printCopyChpLib(instance, metric);
  netlistBackend->printCopyNetlist(inName);
}

void ChpBackend::printSink(const char *instance,
                           const char *inName,
                           double metric[4]) {
  chpGenerator->printSinkChp(instance, inName);
  chpLibGenerator->printSinkChpLib(instance, metric);
  netlistBackend->printSinkNetlist(inName);
}

void ChpBackend::printBuff(Vector<BuffInfo> &buffInfos) {
  chpGenerator->printBuffChp(buffInfos);
  chpLibGenerator->printBuffChpLib(buffInfos);
  netlistBackend->printBuffNetlist(buffInfos);
}

void ChpBackend::printChannel(const char *chanName, unsigned int bitwidth) {
  chpGenerator->printChannelChp(chanName, bitwidth);
  netlistBackend->printChanNetlist(chanName, bitwidth);
}

void ChpBackend::printSource(const char *instance,
                             const char *outName,
                             double metric[4]) {
  chpGenerator->printSourceChp(instance, outName);
  chpLibGenerator->printSourceChpLib(instance, metric);
  netlistBackend->printSourceNetlist(outName);
}

void ChpBackend::printFU(const char *instance,
                         const char *procName,
                         StringVec &argList,
                         StringVec &outList,
                         UIntVec &argBWList,
                         UIntVec &resBWList,
                         UIntVec &outBWList,
                         const char *calc,
                         Map<unsigned int, unsigned int> &outRecord,
                         Vector<BuffInfo> &buffInfos,
                         double *fuMetric) {
  const char *fuInstName =
      chpGenerator->printFUChp(instance, argList, outList, buffInfos);
  unsigned numArgs = argList.size();
  unsigned numOuts = outList.size();
  chpLibGenerator->printFUChpLib(instance,
                                 procName,
                                 calc,
                                 numArgs,
                                 numOuts,
                                 fuMetric,
                                 resBWList,
                                 outRecord);
  netlistBackend->printFUNetlist(instance, fuInstName, argBWList, outBWList);
}

void ChpBackend::printSplit(const char *instance,
                            const char *procName,
                            const char *splitName,
                            const char *guardName,
                            const char *inputName,
                            CharPtrVec &outNameVec,
                            unsigned int dataBW,
                            double *metric) {
  unsigned numOutputs = outNameVec.size();
  chpGenerator->printSplitChp(instance,
                              splitName,
                              guardName,
                              inputName,
                              dataBW,
                              outNameVec);
  chpLibGenerator->printSplitChpLib(instance, metric, numOutputs);
  netlistBackend->printSplitNetlist(procName, splitName, dataBW, numOutputs);
}

void ChpBackend::printMerge(const char *instance,
                            const char *procName,
                            const char *outName,
                            const char *guardName,
                            CharPtrVec &inNameVec,
                            unsigned dataBW,
                            double *metric) {
  chpGenerator->printMergeChp(instance,
                              outName,
                              guardName,
                              dataBW,
                              inNameVec);
  chpLibGenerator->printMergeChpLib(instance, metric);
  unsigned numInputs = inNameVec.size();
  netlistBackend->printMergeNetlist(procName, outName, dataBW, numInputs);
}

void ChpBackend::printMixer(const char *instance,
                            const char *procName,
                            const char *outName,
                            const char *coutName,
                            unsigned dataBW,
                            CharPtrVec &inNameVec,
                            double *metric) {
  chpGenerator->printMixerChp(instance,
                              outName,
                              coutName,
                              dataBW,
                              inNameVec);
  chpLibGenerator->printMixerChpLib(instance, metric);
  unsigned numInputs = inNameVec.size();
  netlistBackend->printMixerNetlist(procName, outName, dataBW, numInputs);
}

void ChpBackend::printArbiter(const char *instance,
                              const char *procName,
                              const char *outName,
                              const char *coutName,
                              unsigned dataBW,
                              CharPtrVec &inNameVec,
                              double *metric) {
  chpGenerator->printArbiterChp(instance,
                                outName,
                                coutName,
                                dataBW,
                                inNameVec);
  chpLibGenerator->printArbiterChpLib(instance, metric);
  unsigned numInputs = inNameVec.size();
  netlistBackend->printArbiterNetlist(procName, outName, dataBW, numInputs);
}

void ChpBackend::printProcHeader(Process *p) {
  chpGenerator->printProcChpHeader(p);
}

void ChpBackend::printProcDeclaration(Process *p) {
  chpGenerator->printProcDeclaration(p);
}

void ChpBackend::printProcEnding() {
  chpGenerator->printProcEnding();
}

void ChpBackend::createChpBlock(Process *p) {
  chpLibGenerator->printChpBlock(p);
}

void ChpBackend::printCustomNamespace(ActNamespace *ns) {
  chpGenerator->printCustomNamespace(ns);
  chpLibGenerator->printCustomNamespace(ns);
}

void ChpBackend::printFileEnding() {
  chpGenerator->printChpFileEnding();
  chpLibGenerator->printFileEnding();
}

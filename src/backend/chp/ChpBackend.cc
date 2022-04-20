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

#if GEN_NETLIST
ChpBackend::ChpBackend(ChpGenerator *chpGenerator,
                       ChpLibGenerator *chpLibGenerator,
                       DflowNetBackend *dflowNetBackend) {
  this->chpGenerator = chpGenerator;
  this->chpLibGenerator = chpLibGenerator;
  this->dflowNetBackend = dflowNetBackend;
}
#else
ChpBackend::ChpBackend(ChpGenerator *chpGenerator,
                       ChpLibGenerator *chpLibGenerator) {
  this->chpGenerator = chpGenerator;
  this->chpLibGenerator = chpLibGenerator;
}
#endif

void ChpBackend::printCopyProcs(const char *instance,
                                const char *inName,
                                double *metric) {
  chpGenerator->printCopyChp(instance, inName);
  chpLibGenerator->printCopyChpLib(instance, metric);
#if GEN_NETLIST
  dflowNetBackend->printCopyNetlist(inName);
#endif
}

void ChpBackend::printSink(const char *instance,
                           const char *inName,
                           double metric[4]) {
  chpGenerator->printSinkChp(instance, inName);
  chpLibGenerator->printSinkChpLib(instance, metric);
#if GEN_NETLIST
  dflowNetBackend->printSinkNetlist(inName);
#endif
}

void ChpBackend::printBuff(Vector<BuffInfo> &buffInfos) {
  chpGenerator->printBuffChp(buffInfos);
  chpLibGenerator->printBuffChpLib(buffInfos);
#if GEN_NETLIST
  dflowNetBackend->printBuffNetlist(buffInfos);
#endif
}

void ChpBackend::printChannel(const char *chanName, unsigned int bitwidth) {
  chpGenerator->printChannelChp(chanName, bitwidth);
#if GEN_NETLIST
  dflowNetBackend->printChanNetlist(chanName, bitwidth);
#endif
}

void ChpBackend::printSource(const char *instance,
                             const char *outName,
                             double metric[4]) {
  chpGenerator->printSourceChp(instance, outName);
  chpLibGenerator->printSourceChpLib(instance, metric);
#if GEN_NETLIST
  dflowNetBackend->printSourceNetlist(outName);
#endif
}

void ChpBackend::printFU(const char *instance,
                         const char *procName,
                         StringVec &argList,
                         StringVec &outList,
                         UIntVec &resBWList,
#if GEN_NETLIST
                         UIntVec &argBWList,
                         UIntVec &outBWList,
#endif
                         const char *calc,
                         Map<unsigned int, unsigned int> &outRecord,
                         Vector<BuffInfo> &buffInfos,
                         double *fuMetric) {
#if GEN_NETLIST
  const char *fuInstName =
      chpGenerator->printFUChp(instance, argList, outList, buffInfos);
#else
  chpGenerator->printFUChp(instance, argList, outList, buffInfos);
#endif
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
#if GEN_NETLIST
  dflowNetBackend->printFUNetlist(instance, fuInstName, argBWList, outBWList);
#endif
}

void ChpBackend::printSplit(const char *instance,
#if GEN_NETLIST
                            const char *procName,
#endif
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
#if GEN_NETLIST
  dflowNetBackend->printSplitNetlist(procName, splitName, dataBW, numOutputs);
#endif
}

void ChpBackend::printMerge(const char *instance,
#if GEN_NETLIST
                            const char *procName,
#endif
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
#if GEN_NETLIST
  unsigned numInputs = inNameVec.size();
  dflowNetBackend->printMergeNetlist(procName, outName, dataBW, numInputs);
#endif
}

void ChpBackend::printMixer(const char *instance,
#if GEN_NETLIST
                            const char *procName,
#endif
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
#if GEN_NETLIST
  unsigned numInputs = inNameVec.size();
  dflowNetBackend->printMixerNetlist(procName, outName, dataBW, numInputs);
#endif
}

void ChpBackend::printArbiter(const char *instance,
#if GEN_NETLIST
                              const char *procName,
#endif
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
#if GEN_NETLIST
  unsigned numInputs = inNameVec.size();
  dflowNetBackend->printArbiterNetlist(procName, outName, dataBW, numInputs);
#endif
}

void ChpBackend::printProcHeader(Process *p) {
  chpGenerator->printProcChpHeader(p);
#if GEN_NETLIST
  dflowNetBackend->printProcNetListHeader(p);
#endif
}

void ChpBackend::printProcDeclaration(Process *p) {
  chpGenerator->printProcDeclaration(p);
}

void ChpBackend::printProcEnding() {
  chpGenerator->printProcEnding();
#if GEN_NETLIST
  dflowNetBackend->printProcNetListEnding();
#endif
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
#if GEN_NETLIST
  dflowNetBackend->printNetlistFileEnding();
#endif
}

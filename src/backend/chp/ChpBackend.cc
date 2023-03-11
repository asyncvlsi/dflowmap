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

  /* 
   * This needs to be initialized based on the process technology
   */
  PW = 4;
  CD = 2;
  PD_SPLIT = 1;
  PD_MERGE = 2;
}
#else
ChpBackend::ChpBackend(ChpGenerator *chpGenerator,
                       ChpLibGenerator *chpLibGenerator) {
  this->chpGenerator = chpGenerator;
  this->chpLibGenerator = chpLibGenerator;
}
#endif

void ChpBackend::printCopyProcs(double *metric,
                                const char *instance,
				const char *leafinst,
                                const char *inName,
#if GEN_NETLIST
    unsigned bw,
#endif
                                unsigned numOut) {
  chpGenerator->printCopyChp(instance, inName);
  chpLibGenerator->printCopyChpLib(instance, leafinst, metric, numOut);
#if GEN_NETLIST
  dflowNetBackend->printCopyNetlist(inName, bw, numOut, CD, PW);
#endif
}

void ChpBackend::printSink(
#if GEN_NETLIST
    unsigned bw,
#endif
    double metric[4],
    const char *instance,
    const char *inName) {
  chpGenerator->printSinkChp(instance, inName);
  chpLibGenerator->printSinkChpLib(instance, metric);
#if GEN_NETLIST
  dflowNetBackend->printSinkNetlist(inName, bw);
#endif
}

void ChpBackend::printBuff(Vector<BuffInfo> &buffInfos) {
  chpGenerator->printBuffChp(buffInfos);
  chpLibGenerator->printBuffChpLib(buffInfos);
#if GEN_NETLIST
  dflowNetBackend->printBuffNetlist(buffInfos, CD, PW);
#endif
}

void ChpBackend::printChannel(const char *chanName, unsigned int bitwidth) {
  chpGenerator->printChannelChp(chanName, bitwidth);
#if GEN_NETLIST
  dflowNetBackend->printChanNetlist(chanName, bitwidth);
#endif
}

void ChpBackend::printSource(
#if GEN_NETLIST
    unsigned long val,
    unsigned bw,
#endif
    double metric[4],
    const char *instance,
    const char *outName) {
  chpGenerator->printSourceChp(instance, outName);
  chpLibGenerator->printSourceChpLib(instance, metric);
#if GEN_NETLIST
  dflowNetBackend->printSourceNetlist(outName, val, bw);
#endif
}

void ChpBackend::printFU(
    double *metric,
    const char *instance,
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
    Vector<BuffInfo> &buffInfos) {
#if GEN_NETLIST
  const char *fuInstName =
      chpGenerator->printFUChp(instance, argList, outList, buffInfos);
#else
  chpGenerator->printFUChp(instance, argList, outList, buffInfos);
#endif
  unsigned numArgs = argList.size();
  unsigned numOuts = outList.size();

  double unit_delay;
  if (config_exists ("expropt.unit_delay")) {
    unit_delay = config_get_real ("expropt.unit_delay");
  }
  else {
    unit_delay = 100.0; // ps default units
  }
  
  chpLibGenerator->printFUChpLib(instance,
                                 procName,
                                 calc,
                                 numArgs,
                                 numOuts,
                                 metric,
                                 resBWList,
                                 outRecord);
#if GEN_NETLIST

  unsigned int delay_units;
  if (!metric) {
    delay_units = 1;
  }
  else {
    delay_units = metric[2]/unit_delay;
    if (delay_units < metric[2]/unit_delay) {
      delay_units++;
    }
    if (delay_units == 0) {
      delay_units = 1;
    }
  }
  
  dflowNetBackend->printFUNetlist(instance,
                                  fuInstName,
                                  argBWList,
                                  outBWList,
                                  CD, 
                                  PW,
				  delay_units);
#endif
}

void ChpBackend::printSplit(double *metric,
                            const char *instance,
#if GEN_NETLIST
    unsigned guardBW,
#endif
                            const char *splitName,
                            const char *guardName,
                            const char *inputName,
                            CharPtrVec &outNameVec,
                            unsigned int dataBW) {
  unsigned numOutputs = outNameVec.size();
  chpGenerator->printSplitChp(instance,
                              splitName,
                              guardName,
                              inputName,
                              dataBW,
                              outNameVec);
  chpLibGenerator->printSplitChpLib(instance, metric, numOutputs);
#if GEN_NETLIST
  unsigned ADDR = 1;
#if PIPELINE
  unsigned C_CD = CD;
  unsigned C_PW = PW;
  unsigned D_CD = CD;
  unsigned D_PW = PW;
  dflowNetBackend->printPipeSplitNetlist(splitName,
                                         C_CD,
                                         C_PW,
                                         ADDR,
                                         D_CD,
                                         D_PW,
                                         numOutputs,
                                         guardBW,
                                         dataBW);
#else
  dflowNetBackend->printUnpipeSplitNetlist(splitName,
                                           ADDR,
                                           PD_SPLIT,
                                           numOutputs,
                                           guardBW,
                                           dataBW);
#endif
#endif
}

void ChpBackend::printMerge(double *metric,
                            const char *instance,
#if GEN_NETLIST
    unsigned guardBW,
#endif
                            const char *outName,
                            const char *guardName,
                            CharPtrVec &inNameVec,
                            unsigned dataBW) {
  chpGenerator->printMergeChp(instance,
                              outName,
                              guardName,
                              dataBW,
                              inNameVec);
  chpLibGenerator->printMergeChpLib(instance, metric);
#if GEN_NETLIST
  size_t numInputs = inNameVec.size();
  unsigned ADDR = 1;
#if PIPELINE
  unsigned C_CD = CD;
  unsigned D_CD = CD;
  unsigned SEL = 1;
  dflowNetBackend->printPipeMergeNetlist(outName,
                                         C_CD,
                                         D_CD,
                                         PW,
                                         ADDR,
                                         SEL,
                                         numInputs,
                                         guardBW,
                                         dataBW);
#else
  dflowNetBackend->printUnpipeMergeNetlist(outName,
                                           ADDR,
                                           PD_MERGE,
                                           numInputs,
                                           guardBW,
                                           dataBW);
#endif
#endif
}

void ChpBackend::printMixer(double *metric,
                            const char *instance,
#if GEN_NETLIST
    unsigned guardBW,
#endif
                            const char *outName,
                            const char *ctrlOutName,
                            unsigned dataBW,
                            CharPtrVec &inNameVec) {
  chpGenerator->printMixerChp(instance,
                              outName,
			      ctrlOutName,
                              dataBW,
                              inNameVec);
  chpLibGenerator->printMixerChpLib(instance, metric);
#if GEN_NETLIST
  size_t numInputs = inNameVec.size();
  unsigned ADDR = 1;
#if PIPELINE
  unsigned D_CD = CD;
  unsigned D_PW = PW;
  dflowNetBackend->printPipeMixerNetlist(outName,
                                         ADDR,
                                         D_CD,
                                         D_PW,
                                         numInputs,
                                         dataBW,
                                         guardBW);
#else
  dflowNetBackend->printUnpipeMixerNetlist(outName,
                                           ADDR,
                                           numInputs,
                                           dataBW,
                                           guardBW);
#endif
#endif
}

void ChpBackend::printArbiter(double *metric,
                              const char *instance,
#if GEN_NETLIST
    unsigned guardBW,
#endif
                              const char *outName,
                              const char *coutName,
                              unsigned dataBW,
                              CharPtrVec &inNameVec) {
  chpGenerator->printArbiterChp(instance,
                                outName,
                                coutName,
                                dataBW,
                                inNameVec);
  chpLibGenerator->printArbiterChpLib(instance, metric);
#if GEN_NETLIST
  size_t numInputs = inNameVec.size();
#if PIPELINE
  dflowNetBackend->printPipeArbiterNetlist(outName,
                                           dataBW,
                                           guardBW,
                                           numInputs);
#else
  dflowNetBackend->printUnpipeArbiterNetlist(outName,
                                             dataBW,
                                             guardBW,
                                             numInputs);
#endif
#endif
}

void ChpBackend::printProcHeader(Process *p) {
  chpGenerator->printProcChpHeader(p);
#if GEN_NETLIST
  dflowNetBackend->printProcNetListHeader(p);
#endif
}

void ChpBackend::printFreshChannel(char *name, int bw) {
  chpGenerator->printChpFreshChannel(name, bw);
#if GEN_NETLIST
  dflowNetBackend->printDflowFreshChannel(name, bw);
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

void ChpBackend::createChpBlock(Process *p, int where) {
  chpLibGenerator->printChpBlock(p, where);
#if GEN_NETLIST
  dflowNetBackend->printDflowBlock(p);
#endif  
}

void ChpBackend::printCustomNamespace(ActNamespace *ns) {
  auto stdNS = ActNamespace::Global()->findNS(Constant::STD_NAMESPACE);
  if (ns == stdNS) return;
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

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

#include "NetlistBackend.h"

NetlistBackend::NetlistBackend(NetlistGenerator *netlistGenerator,
                               NetlistLibGenerator *netlistLibGenerator) {
  this->netlistGenerator = netlistGenerator;
  this->netlistLibGenerator = netlistLibGenerator;
}

void NetlistBackend::printCopyNetlist(const char *inName) {
  netlistGenerator->printCopyNetlist(inName);
}

void NetlistBackend::printSinkNetlist(const char *inName) {
  netlistGenerator->printSinkNetlist(inName);
}

void NetlistBackend::printBuffNetlist(Vector<BuffInfo> &buffInfos) {
  netlistGenerator->printBuffNetlist(buffInfos);
}

void NetlistBackend::printChanNetlist(const char *chanName,
                                      unsigned int bitwidth) {
  netlistGenerator->printChanNetlist(chanName, bitwidth);
}

void NetlistBackend::printSourceNetlist(const char *outName) {
  netlistGenerator->printSourceNetlist(outName);
}

void NetlistBackend::printFUNetlist(const char *instance,
                                    const char *fuInstName,
                                    UIntVec &argBWList,
                                    UIntVec &outBWList) {
  /* handle normal fu */
  netlistGenerator->printFUNetlist(instance, fuInstName);
  netlistLibGenerator->printFUNetListLib(instance, argBWList, outBWList);
}

void NetlistBackend::printSplitNetlist(const char *procName,
                                       const char *splitName,
                                       unsigned int dataBW,
                                       size_t numOutputs) {
  netlistGenerator->printSplitNetlist(procName,
                                      splitName,
                                      dataBW,
                                      numOutputs);
}

void NetlistBackend::printMergeNetlist(const char *procName,
                                       const char *outName,
                                       unsigned dataBW,
                                       size_t numInputs) {
  netlistGenerator->printMergeNetlist(procName,
                                      outName,
                                      dataBW,
                                      numInputs);
}

void NetlistBackend::printMixerNetlist(const char *procName,
                                       const char *outName,
                                       unsigned dataBW,
                                       size_t numInputs) {
  netlistGenerator->printMixerNetlist(procName,
                                      outName,
                                      dataBW,
                                      numInputs);
}

void NetlistBackend::printArbiterNetlist(const char *procName,
                                         const char *outName,
                                         unsigned dataBW,
                                         size_t numInputs) {
  netlistGenerator->printArbiterNetlist(procName,
                                        outName,
                                        dataBW,
                                        numInputs);
}

void NetlistBackend::printProcNetListHeader(Process *p) {
  netlistGenerator->printProcNetListHeader(p);
}

void NetlistBackend::printProcNetListEnding() {
  netlistGenerator->printProcNetListEnding();
}

void NetlistBackend::printNetlistFileEnding() {
  netlistGenerator->printNetlistFileEnding();
}

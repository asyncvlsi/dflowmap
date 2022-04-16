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

#ifndef DFLOWMAP__NETLISTBACKEND_H_
#define DFLOWMAP__NETLISTBACKEND_H_

#include "src/backend/netlist/NetlistGenerator.h"
#include "src/backend/netlist/NetlistLibGenerator.h"

class NetlistBackend {
 public:
  NetlistBackend(NetlistGenerator *netlistGenerator,
                 NetlistLibGenerator *netlistLibGenerator);

  void printCopyNetlist(const char *inName);

  void printSinkNetlist(const char *inName);

  void printBuffNetlist(Vector<BuffInfo> &buffInfos);

  void printChanNetlist(const char *chanName, unsigned bitwidth);

  void printSourceNetlist(const char *outName);

  void printFUNetlist(const char *instance,
                      const char *fuInstName,
                      UIntVec &argBWList,
                      UIntVec &outBWList);

  void printSplitNetlist(const char *procName,
                         const char *splitName,
                         unsigned int dataBW,
                         size_t numOutputs);

  void printMergeNetlist(const char *procName,
                         const char *outName,
                         unsigned dataBW,
                         size_t numInputs);

  void printMixerNetlist(const char *procName,
                         const char *outName,
                         unsigned dataBW,
                         size_t numInputs);

  void printArbiterNetlist(const char *procName,
                           const char *outName,
                           unsigned dataBW,
                           size_t numInputs);

  void printProcNetListHeader(Process *p);

  void printProcNetListEnding();

  void printNetlistFileEnding();

 private:
  NetlistGenerator *netlistGenerator;
  NetlistLibGenerator *netlistLibGenerator;
};

#endif //DFLOWMAP__NETLISTBACKEND_H_

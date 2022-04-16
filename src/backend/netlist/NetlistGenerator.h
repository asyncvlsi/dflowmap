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

#ifndef DFLOWMAP__NETLISTGENERATOR_H_
#define DFLOWMAP__NETLISTGENERATOR_H_

#include <act/act.h>
#include <act/iter.h>
#include <fstream>
#include "src/common/Helper.h"

class NetlistGenerator {
 private:
  FILE *netlistFp;

 public:
  explicit NetlistGenerator(FILE *netlistFp);

  void printSinkNetlist(const char *inName);

  void printCopyNetlist(const char *inputName);

  void printInitNetlist(const char *outName);

  void printOneBuffNetlist(const char *outName);

  void printChanNetlist(const char *chanName, unsigned int bitwidth);

  void printBuffNetlist(Vector<BuffInfo> &buffInfos);

  void printSourceNetlist(const char *outName);

  void printFUNetlist(const char *instance, const char *fuInstName);

  void printSplitNetlist(const char *procName,
                         const char *splitName,
                         unsigned dataBW,
                         unsigned numOutputs);

  void printMergeNetlist(const char *procName,
                         const char *outName,
                         unsigned dataBW,
                         unsigned numInputs);

  void printArbiterNetlist(const char *procName,
                           const char *outName,
                           unsigned dataBW,
                           unsigned numInputs);

  void printMixerNetlist(const char *procName,
                         const char *outName,
                         unsigned dataBW,
                         unsigned numInputs);

  void printProcNetListHeader(Process *p);

  void printProcNetListEnding();

  void printNetlistFileEnding();
};

#endif //DFLOWMAP__NETLISTGENERATOR_H_

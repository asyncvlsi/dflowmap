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

#ifndef DFLOWMAP_CHPLIBGENERATOR_H
#define DFLOWMAP_CHPLIBGENERATOR_H

#include <cstring>
#include <algorithm>
#include <act/act.h>
#include <act/iter.h>
#include <fstream>
#include "src/common/common.h"
#include "src/common/Helper.h"

class ChpLibGenerator {
 public:
  ChpLibGenerator(FILE *chpLibFp, FILE *confFp);

  void printMemConfig(const char *procName);

  void printConf(const char *instance, unsigned numOutputs, double *metric);

  void printConf(const char *instance, double *metric);

  void printFUChpLib(const char *instance,
                     const char *procName,
                     const char *calc,
                     unsigned int numArgs,
                     unsigned int numOuts,
                     double *fuMetric,
                     UIntVec &resBWList,
                     Map<unsigned int,
                         unsigned int> &outRecord);

  void printFUChpLib(const char *instance,
                     const char *procName,
                     const char *calc,
                     const char *outSend,
                     unsigned int numArgs,
                     unsigned int numOuts,
                     double *metric,
                     UIntVec &resBW);

  void printMergeChpLib(const char *instance, double *metric);

  void printSplitChpLib(const char *instance, double *metric, int numOutputs);

  void printArbiterChpLib(const char *instance, double *metric);

  void printMixerChpLib(const char *instance, double *metric);

  void printSourceChpLib(const char *instance, double *metric);

  void printInitChpLib(const char *instance, double *metric);

  void printOneBuffChpLib(const char *instance, double *metric);

  void printBuffChpLib(Vector<BuffInfo> &buffInfos);

  void printSinkChpLib(const char *instance, double *metric);

  void printCopyChpLib(const char *instance, double *metric);

  void printChpBlock(Process *p);

  void printCustomNamespace(ActNamespace *ns);

  void printFileEnding();

 private:
  const char *processes[MAX_PROCESSES];
  const char *instances[MAX_PROCESSES];
  FILE *chpLibFp;
  FILE *confFp;

  bool checkAndUpdateInstance(const char *instance);

  bool checkAndUpdateProcess(const char *process);

};

#endif //DFLOWMAP_CHPLIBGENERATOR_H

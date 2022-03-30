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
#include "common.h"

class ChpLibGenerator {
 public:
  ChpLibGenerator(FILE *libFp, FILE *confFp);

  void genMemConfiguration(const char *procName);

  void createConf(const char *instance, unsigned numOutputs, double *metric);

  void createConf(const char *instance, double *metric);

  void createFU(const char *procName,
                const char *calc,
                unsigned int numArgs,
                unsigned int numOuts,
                const char *instance,
                double *fuMetric,
                UIntVec &resBW,
                UIntVec &outBW,
                Map<unsigned int, unsigned int> &outRecord,
                Vector<BuffInfo> &buffInfos);

  void createFULib(const char *procName,
                   const char *calc,
                   const char *outSend,
                   unsigned int numArgs,
                   unsigned int numOuts,
                   const char *instance,
                   double *metric,
                   UIntVec &resBW,
                   UIntVec &outBW);

  void createMerge(const char *procName,
                   const char *instance,
                   double *metric,
                   int numInputs);

  void createSplit(const char *procName,
                   const char *instance,
                   double *metric,
                   int numOutputs);

  void createArbiter(const char *procName,
                     const char *instance,
                     double *metric,
                     int numInputs);

  void createMixer(const char *procName,
                     const char *instance,
                     double *metric,
                     int numInputs);

  void createSource(const char *instance, double *metric);

  void createInit(const char *instance, double *metric);

  void createOneBuff(const char *instance, double *metric);

  void createBuff(Vector<BuffInfo> &buffInfos);

  void createSink(const char *instance, double *metric);

  void createCopy(const char *instance, double *metric);

  void createChpBlock(Process *p);

  void printCustomNamespace(ActNamespace *ns);

  void printFileEnding();

 private:
  const char *processes[MAX_PROCESSES];
  const char *instances[MAX_PROCESSES];

  FILE *libFp;
  FILE *confFp;

  bool hasInstance(const char *instance);

  bool hasProcess(const char *process);

};

#endif //DFLOWMAP_CHPLIBGENERATOR_H

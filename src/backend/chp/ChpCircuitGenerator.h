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

#ifndef DFLOWMAP__CHPCIRCUITGENERATOR_H_
#define DFLOWMAP__CHPCIRCUITGENERATOR_H_

#include <act/act.h>
#include <fstream>
#include "Helper.h"

class ChpCircuitGenerator {
 private:
  FILE *resFp;
 public:
  explicit ChpCircuitGenerator(FILE *resFp);

  void printSink(const char *inName, unsigned bitwidth);

  void printCopy(const char *inputName,
                 unsigned int bw,
                 unsigned int numOut);

  void printEmptyLine();

  void printInit(const char *inName,
                 const char *outName,
                 unsigned bitwidth,
                 unsigned long initVal);

  void printOneBuff(const char *inName, const char *outName, unsigned bitwidth);

  void printBuff(Vector<BuffInfo> &buffInfos);

  void printChannel(const char* chanName, unsigned bitwidth);

  void printSource(const char *instance, const char *outName);

  void printFunc(const char *instance,
                 StringVec &argList,
                 UIntVec &argBWList,
                 UIntVec &resBWList,
                 UIntVec &outBWList,
                 StringVec &outList,
                 Vector<BuffInfo> &buffInfos);

  void printSplit(const char *procName,
                  const char *splitName,
                  const char *guardStr,
                  const char *inputStr,
                  unsigned guardBW,
                  unsigned outBW,
                  CharPtrVec &outNameVec);

  void printMerge(const char *procName,
                  const char *outName,
                  const char *guardStr,
                  unsigned guardBW,
                  unsigned inBW,
                  CharPtrVec &inNameVec);

  void printArbiter(const char *outName,
                    const char *coutName,
                    unsigned outBW,
                    unsigned coutBW,
                    int numIn,
                    CharPtrVec &inNameVec);

  void printProcHeader(Process *p);

  void printProcDeclaration(Process *p);

  void printProcEnding();
};

#endif //DFLOWMAP__CHPCIRCUITGENERATOR_H_

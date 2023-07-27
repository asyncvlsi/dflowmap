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
#include <act/iter.h>
#include <fstream>
#include "src/common/Helper.h"
#include "src/common/config.h"

class ChpGenerator {
 private:
  FILE *chpFp;
#if GEN_NETLIST
#endif

 public:
  explicit ChpGenerator(FILE *chpFp) {
    this->chpFp = chpFp;
  }

  void printSinkChp(const char *instance, const char *inName);

  void printCopyChp(const char *instance,
                    const char *inputName);

  void printEmptyLine();

  void printInitChp(const char *instance,
                    const char *inName,
                    const char *outName);

  void printOneBuffChp(const char *instance,
                       const char *inName,
                       const char *outName);

  void printBuffChp(Vector<BuffInfo> &buffInfos);

  void printChannelChp(const char *chanName, unsigned bitwidth);

  void printSourceChp(const char *instance, const char *outName);

  const char *printFUChp(const char *instance,
                         StringVec &argList,
                         StringVec &outList,
                         Vector<BuffInfo> &buffInfos);

  void printSplitChp(const char *instance,
                     const char *splitName,
                     const char *guardName,
                     const char *inputName,
                     unsigned dataBW,
                     CharPtrVec &outNameVec);

  void printMergeChp(const char *instance,
                     const char *outName,
                     const char *guardStr,
                     unsigned dataBW,
                     CharPtrVec &inNameVec);

  void printArbiterChp(const char *instance,
                       const char *outName,
                       const char *coutName,
                       unsigned dataBW,
                       CharPtrVec &inNameVec);

  void printMixerChp(const char *instance,
                     const char *outName,
                     const char *ctrlName,
                     unsigned dataBW,
                     CharPtrVec &inNameVec);

  void printProcChpHeader(Process *p);

  void printBoolToIntDecl (act_connection *c, int val);
  void printBoolToIntConv (act_connection *c, int val);

  void printProcDeclaration(Process *p);

  void printProcEnding();

  void printCustomNamespace(ActNamespace *ns);

  void printChpFileEnding();

  void printChpFreshChannel (char *name, int bw);
};

#endif //DFLOWMAP__CHPCIRCUITGENERATOR_H_

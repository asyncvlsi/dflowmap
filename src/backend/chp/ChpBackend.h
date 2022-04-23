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

#ifndef DFLOWMAP__CHPBACKEND_H_
#define DFLOWMAP__CHPBACKEND_H_

#include "src/backend/chp/ChpGenerator.h"
#include "src/backend/chp/ChpLibGenerator.h"
#if GEN_NETLIST
#include <act/dflow/backend/netlist/DflowNetBackend.h>
#endif
class ChpBackend {
 public:
#if GEN_NETLIST
  ChpBackend(ChpGenerator *chpGenerator,
             ChpLibGenerator *chpLibGenerator,
             DflowNetBackend *dflowNetBackend);
#else
  ChpBackend(ChpGenerator *chpGenerator,
             ChpLibGenerator *chpLibGenerator);
#endif

  void printCopyProcs(double *metric,
                      const char *instance,
                      const char *inName,
#if GEN_NETLIST
      unsigned bw,
#endif
                      unsigned numOut);

  void printSink(
#if GEN_NETLIST
      unsigned bw,
#endif
      double metric[4],
      const char *instance,
      const char *inName
  );

  void printBuff(Vector<BuffInfo> &buffInfos);

  void printChannel(const char *chanName, unsigned bitwidth);

  void printSource(
#if GEN_NETLIST
      unsigned long val,
      unsigned bw,
#endif
      double metric[4],
      const char *instance,
      const char *outName
  );

  void printFU(
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
      Vector<BuffInfo> &buffInfos);

  void printSplit(double *metric,
                  const char *instance,
#if GEN_NETLIST
      const char *procName,
      unsigned guardBW,
#endif
                  const char *splitName,
                  const char *guardName,
                  const char *inputName,
                  CharPtrVec &outNameVec,
                  unsigned int dataBW);

  void printMerge(double *metric,
                  const char *instance,
#if GEN_NETLIST
      const char *procName,
      unsigned guardBW,
#endif
                  const char *outName,
                  const char *guardName,
                  CharPtrVec &inNameVec,
                  unsigned dataBW);

  void printMixer(double *metric,
                  const char *instance,
#if GEN_NETLIST
      const char *procName,
      unsigned guardBW,
#endif
                  const char *outName,
                  const char *coutName,
                  unsigned dataBW,
                  CharPtrVec &inNameVec);

  void printArbiter(double *metric,
                    const char *instance,
#if GEN_NETLIST
      const char *procName,
      unsigned guardBW,
#endif
                    const char *outName,
                    const char *coutName,
                    unsigned dataBW,
                    CharPtrVec &inNameVec);

  void printProcHeader(Process *p);

  void printProcDeclaration(Process *p);

  void printProcEnding();

  void createChpBlock(Process *p);

  void printCustomNamespace(ActNamespace *ns);

  void printFileEnding();

 private:
  ChpGenerator *chpGenerator;
  ChpLibGenerator *chpLibGenerator;
#if GEN_NETLIST
  DflowNetBackend *dflowNetBackend;
#endif
};

#endif //DFLOWMAP__CHPBACKEND_H_

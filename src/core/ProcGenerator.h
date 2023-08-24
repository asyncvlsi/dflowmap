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

#ifndef DFLOWMAP_CHPPROCGENERATOR_H
#define DFLOWMAP_CHPPROCGENERATOR_H

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <act/act.h>
#include <act/passes.h>
#include <act/lang.h>
#include <act/types.h>
#include <act/expr.h>
#include <act/act.h>
#include "src/backend/chp/ChpBackend.h"
#include "src/core/Metrics.h"
#include "src/core/DflowGenerator.h"
#include "src/core/NameGenerator.h"
#include "src/common/common.h"
#include "src/common/Helper.h"
#include "src/common/config.h"

#if LOGIC_OPTIMIZER
#include <act/expropt.h>
#endif

class ProcGenerator {
 public:
  ProcGenerator(Metrics *metrics,
                ChpBackend *chpBackend);

  const char *getActIdOrCopyName(ActId *actId);

  void collectBitwidthInfo();

  void printBitwidthInfo();

  unsigned getActIdBW(ActId *actId);

  unsigned getBitwidth(act_connection *actConnection);

  const char *EMIT_QUERY(DflowGenerator *dflowGenerator,
                         Expr *expr,
                         int &resSuffix,
                         unsigned &resBW);

  const char *EMIT_BIN(DflowGenerator *dflowGenerator,
                       Expr *expr,
                       const char *op,
                       int type,
                       int &resSuffix,
                       unsigned &resBW);

  const char *EMIT_UNI(DflowGenerator *dflowGenerator,
                       Expr *expr,
                       const char *op,
                       int &resSuffix,
                       unsigned &resBW);

  const char *EMIT_CONCAT(DflowGenerator *dflowGenerator,
                          Expr *expr,
                          int &resSuffix,
                          unsigned &resBW);

  const char *EMIT_BITFIELD(DflowGenerator *dflowGenerator,
			    Expr *expr,
			    int &resSuffix,
			    unsigned &resBW);

  const char *printExpr(DflowGenerator *dflowGenerator,
                        Expr *expr,
                        int &resSuffix,
                        unsigned &resBW);

  unsigned getCopyUses(ActId *actId);

  void updateOpUses(ActId *actId);

  void updateOpUses(act_connection *actConnection);

  void recordOpUses(ActId *actId, ActConnectVec &actConnectVec);

  void printOpUses();

  unsigned getOpUses(ActId *actId);

  void collectUniOpUses(Expr *expr, StringVec &recordedOps);

  void collectBinOpUses(Expr *expr, StringVec &recordedOps);

  void recordUniOpUses(Expr *expr, ActConnectVec &actConnectVec);

  void recordBinOpUses(Expr *expr, ActConnectVec &actConnectVec);

  void collectExprUses(Expr *expr, StringVec &recordedOps);

  void recordExprUses(Expr *expr, ActConnectVec &actConnectVec);

  void collectDflowClusterUses(list_t *dflow, ActConnectVec &actConnectVec);

  void collectOpUses();

  void createCopyProcs();

  void printDFlowFunc(DflowGenerator *dflowGenerator,
                      const char *procName,
                      UIntVec &outBWList,
                      StringVec &outList,
                      Map<unsigned int, unsigned int> &outRecord,
                      Vector<BuffInfo> &buffInfos);

  void handleDFlowFunc(DflowGenerator *dflowGenerator,
                       act_dataflow_element *d,
                       int &resSuffix,
                       StringVec &outList,
                       UIntVec &outBWList,
                       Map<unsigned int, unsigned int> &outRecord,
                       Vector<BuffInfo> &buffInfos);

  void handleSelectionUnit(act_dataflow_element *d,
                           CharPtrVec &inNameVec,
                           char *&outputName,
                           unsigned &dataBW,
                           int &numInputs);

  void handleNormDflowElement(act_dataflow_element *d, unsigned &sinkCnt);

  void handleDFlowCluster(list_t *dflow_cluster);

  bool isOpUsed(ActId *actId);

  void handleBuff(Expr *bufExpr,
                  Expr *initExpr,
                  const char *outName,
                  unsigned outID,
                  unsigned outBW,
                  Vector<BuffInfo> &buffInfos);

  static void handlePort(const Expr *expr,
                         int &resSuffix,
                         unsigned resBW,
                         const char *exprName,
                         DflowGenerator *dflowGenerator);

  int run(Process *p);

  char *nextAnon ();

 private:
  /* op, its bitwidth */
  Map<act_connection *, unsigned> bitwidthMap;
  Map<act_connection *, int> isBoolMap;
  /* operator, # of times it is used (if it is used for more than once, then we create COPY for it) */
  Map<act_connection *, unsigned> opUses;
  /* copy operator, # of times it has already been used */
  Map<act_connection *, unsigned> copyUses;
  Metrics *metrics;
  ChpBackend *chpBackend;
  Process *p;
  Scope *sc;

  int _anon_names;

  void createSink(const char *name, unsigned bitwidth);

  void createSource(const char *outName,
                    unsigned long val,
                    unsigned bitwidth);

  const char *getOutputNormId (ActId *id);

};

#endif //DFLOWMAP_CHPPROCGENERATOR_H

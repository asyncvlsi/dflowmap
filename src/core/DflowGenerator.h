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

#ifndef DFLOWMAP__DFLOWGENERATOR_H_
#define DFLOWMAP__DFLOWGENERATOR_H_
#include <act/act.h>
#include <cstring>
#include "src/common/common.h"
#include "src/common/Helper.h"
#include "src/common/config.h"

class DflowGenerator {
 public:
  DflowGenerator(StringVec &argList,
                 StringVec &oriArgList,
                 UIntVec &argBWList,
                 UIntVec &resBWList,
                 Map<const char *, Expr *> &exprMap,
                 StringMap<unsigned> &inBW,
                 StringMap<unsigned> &hiddenBW,
                 Map<Expr *, Expr *> &hiddenExprs);

  bool isNewArg(const char *arg);

  const char *handleEVar(const char *oriArgName,
                         const char *mappedVarName,
                         unsigned argBW);

  void printChpPort(const char *exprName,
                    int resSuffix,
                    unsigned resBW);

  void printChpConcatExpr(StringVec &operandList,
                          int resSuffix,
                          unsigned resBW);

  void printChpBitfieldExpr(const char *exprName,
			    unsigned hi, unsigned lo,
			    const int resSuffix,
			    unsigned resBW);
  
  void printChpUniExpr(const char *op,
                       const char *exprName,
                       int resSuffix,
                       unsigned resBW);

  void printChpBinExpr(const char *op,
                       const char *lexpr_name,
                       const char *rexpr_name,
                       int exprType,
                       int resSuffix,
                       unsigned resBW);

  void printChpQueryExpr(const char *cexpr_name,
                         const char *lexpr_name,
                         const char *rexpr_name,
                         int resSuffix,
                         unsigned resBW);

  void preparePortForOpt(const char *expr_name,
                         const char *portName,
                         unsigned bw);

  void prepareQueryExprForOpt(const char *cexpr_name,
                              int cexpr_type,
                              const char *lexpr_name,
                              int lexpr_type,
                              const char *rexpr_name,
                              int rexpr_type,
                              const char *expr_name,
                              int expr_type,
                              int body_expr_type,
                              unsigned bw);

  void prepareBinExprForOpt(const char *lexpr_name,
                            int lexpr_type,
                            const char *rexpr_name,
                            int rexpr_type,
                            const char *expr_name,
                            int expr_type,
                            unsigned bw);

  void prepareUniExprForOpt(const char *lexpr_name,
                            int lexpr_type,
                            const char *expr_name,
                            int expr_type,
                            unsigned bw);

  void prepareConcatExprForOpt(StringVec &operandList,
                               IntVec &opTypeList,
                               const char *expr_name,
                               unsigned bw);

  void prepareBitfieldExprForOpt(const char *lexpr_name,
                                 const char *expr_name,
                                 unsigned hi, unsigned lo,
                                 unsigned bw);

  const char *getCalc();

  StringVec &getArgList();

  UIntVec &getArgBWList();

  UIntVec &getResBWList();

  Map<const char *, Expr *> &getExprMap();

  StringMap<unsigned> &getInBW();

  StringMap<unsigned> &getHiddenBWs();

  Map<Expr *, Expr *> &getHiddenExprs();

  void dump();

 private:
  char *calc;
  StringVec argList;
  StringVec oriArgList;
  UIntVec argBWList;
  UIntVec resBWList;
  Map<const char *, Expr *> exprMap;
  StringMap<unsigned> inBWMap;
  StringMap<unsigned> hiddenBWMap;
  Map<Expr *, Expr *> hiddenExprs;
};

#endif //DFLOWMAP__DFLOWGENERATOR_H_

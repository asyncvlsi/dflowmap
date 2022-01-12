//
// Created by ruile on 1/6/2022.
//

#ifndef DFLOWMAP__DFLOWGENERATOR_H_
#define DFLOWMAP__DFLOWGENERATOR_H_
#include <act/act.h>
#include <cstring>
#include "common.h"
#include "Helper.h"

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
                    int result_suffix,
                    unsigned result_bw);

  void printChpUniExpr(const char *op,
                       const char *exprName,
                       int result_suffix,
                       unsigned result_bw);

  void printChpBinExpr(const char *op,
                       const char *lexpr_name,
                       const char *rexpr_name,
                       int exprType,
                       int result_suffix,
                       unsigned result_bw);

  void printChpQueryExpr(const char *cexpr_name,
                         const char *lexpr_name,
                         const char *rexpr_name,
                         int result_suffix,
                         unsigned result_bw);

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
  StringMap<unsigned> inBW;
  StringMap<unsigned> hiddenBW;
  Map<Expr *, Expr *> hiddenExprs;
};

#endif //DFLOWMAP__DFLOWGENERATOR_H_

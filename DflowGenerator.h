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
                 Map<Expr *, Expr *> &hiddenExprs) {
    this->argList = argList;
    this->oriArgList = oriArgList;
    this->argBWList = argBWList;
    this->resBWList = resBWList;
    this->exprMap = exprMap;
    this->inBW = inBW;
    this->hiddenBW = hiddenBW;
    this->hiddenExprs = hiddenExprs;
    calc = new char[MAX_CALC_LEN];
    calc[0] = '\0';
  }

  const char *handleEVar(const char *oriArgName,
                         const char *mappedVarName,
                         unsigned argBW) {
    char *curArg = new char[10240];
    int idx = searchStringVec(oriArgList, oriArgName);
    if (idx == -1) {
      int numArgs = argList.size();
      oriArgList.push_back(oriArgName);
      argList.push_back(mappedVarName);
      if (debug_verbose) {
        printf("oriArgName: %s, mappedVarName: %s\n", oriArgName,
               mappedVarName);
      }
      sprintf(curArg, "x%d", numArgs);
      argBWList.push_back(argBW);
    } else {
      sprintf(curArg, "x%d", idx);
    }
    inBW.insert({curArg, argBW});
    return curArg;
  }

  void printPort(const char *exprName,
                 const int result_suffix,
                 unsigned result_bw) {
    resBWList.push_back(result_bw);
    char *subCalc = new char[1500];
    sprintf(subCalc, "      res%d := %s;\n", result_suffix, exprName);
    strcat(calc, subCalc);
  }

  void printUniExpr(const char *op,
                    const char *exprName,
                    const int result_suffix,
                    unsigned result_bw) {
    char *curCal = new char[128 + strlen(exprName)];
    sprintf(curCal, "      res%d := %s %s;\n", result_suffix, op, exprName);
    if (debug_verbose) {
      printf("%s\n", curCal);
    }
    strcat(calc, curCal);
    if (result_bw == 0) {
      printf("result_bw is 0!\n");
      exit(-1);
    }
    resBWList.push_back(result_bw);
  }

  void printBinExpr(const char *op,
                    const char *lexpr_name,
                    const char *rexpr_name,
                    int exprType,
                    const int result_suffix,
                    unsigned result_bw) {
    if (result_bw == 0) {
      printf("result_bw is 0!\n");
      exit(-1);
    }
    resBWList.push_back(result_bw);

    char *curCal = new char[300];
    bool binType = isBinType(exprType);
    if (binType) {
      sprintf(curCal, "      res%d := int(%s %s %s);\n",
              result_suffix, lexpr_name, op, rexpr_name);
    } else {
      sprintf(curCal, "      res%d := %s %s %s;\n",
              result_suffix, lexpr_name, op, rexpr_name);
    }
    strcat(calc, curCal);
  }

  void printQueryExpr(const char *cexpr_name,
                      const char *lexpr_name,
                      const char *rexpr_name,
                      const int result_suffix,
                      unsigned result_bw) {
    char *curCal = new char[128 + strlen(cexpr_name) + strlen(lexpr_name)
        + strlen(rexpr_name)];
    sprintf(curCal, "      res%d := bool(%s) ? %s : %s;\n",
            result_suffix, cexpr_name, lexpr_name, rexpr_name);
    strcat(calc, curCal);
    if (result_bw == 0) {
      printf("result_bw is 0!\n");
      exit(-1);
    }
    resBWList.push_back(result_bw);
  }

  void preparePortForOpt(const char *expr_name,
                         const char *portName,
                         unsigned bw) {
    Expr *rhs = getExprFromName(expr_name, exprMap, false, E_VAR);
    Expr *expr = getExprFromName(portName, exprMap, false, E_VAR);
    hiddenBW.insert({expr_name, bw});
    hiddenExprs.insert({rhs, expr});
  }

  void prepareQueryExprForOpt(const char *cexpr_name,
                              int cexpr_type,
                              const char *lexpr_name,
                              int lexpr_type,
                              const char *rexpr_name,
                              int rexpr_type,
                              const char *expr_name,
                              int expr_type,
                              int body_expr_type,
                              unsigned bw) {
    Expr *cExpr = getExprFromName(cexpr_name, exprMap, false, cexpr_type);
    Expr *lExpr = getExprFromName(lexpr_name, exprMap, false, lexpr_type);
    Expr *rExpr = getExprFromName(rexpr_name, exprMap, false, rexpr_type);
    Expr *rhs = getExprFromName(expr_name, exprMap, false, E_VAR);
    Expr *expr = new Expr;
    expr->type = expr_type;
    expr->u.e.l = cExpr;
    Expr *body_expr = new Expr;
    body_expr->type = body_expr_type;
    body_expr->u.e.l = lExpr;
    body_expr->u.e.r = rExpr;
    expr->u.e.r = body_expr;
    hiddenBW.insert({expr_name, bw});
    hiddenExprs.insert({rhs, expr});
    if (debug_verbose) {
      printf("rhs: ");
      print_expr(stdout, rhs);
      printf(", resExpr: ");
      print_expr(stdout, expr);
      printf(".\n");
    }
  }

  void prepareBinExprForOpt(const char *lexpr_name,
                            int lexpr_type,
                            const char *rexpr_name,
                            int rexpr_type,
                            const char *expr_name,
                            int expr_type,
                            unsigned bw) {
    Expr *lExpr = getExprFromName(lexpr_name, exprMap, false, lexpr_type);
    Expr *rExpr = getExprFromName(rexpr_name, exprMap, false, rexpr_type);
    Expr *rhs = getExprFromName(expr_name, exprMap, false, E_VAR);
    Expr *expr = new Expr;
    expr->type = expr_type;
    expr->u.e.l = lExpr;
    expr->u.e.r = rExpr;
    hiddenBW.insert({expr_name, bw});
    hiddenExprs.insert({rhs, expr});
    if (debug_verbose) {
      printf("rhs: ");
      print_expr(stdout, rhs);
      printf(", resExpr: ");
      print_expr(stdout, expr);
      printf(".\n");
    }
  }

  void prepareUniExprForOpt(const char *lexpr_name,
                            int lexpr_type,
                            const char *expr_name,
                            int expr_type,
                            unsigned bw) {
    Expr *lExpr = getExprFromName(lexpr_name, exprMap, false, lexpr_type);
    Expr *rhs = getExprFromName(expr_name, exprMap, false, E_VAR);
    Expr *expr = new Expr;
    expr->type = expr_type;
    expr->u.e.l = lExpr;
    hiddenBW.insert({expr_name, bw});
    hiddenExprs.insert({rhs, expr});
    if (debug_verbose) {
      printf("rhs: ");
      print_expr(stdout, rhs);
      printf(", resExpr: ");
      print_expr(stdout, expr);
      printf(".\n");
    }
  }

  const char *getCalc() {
    calc[strlen(calc) - 2] = ';';
    return calc;
  }

  StringVec &getArgList() {
    return argList;
  }

  UIntVec &getArgBWList() {
    return argBWList;
  }

  UIntVec &getResBWList() {
    return resBWList;
  }

  Map<const char *, Expr *> &getExprMap() {
    return exprMap;
  }

  StringMap<unsigned> &getInBW() {
    return inBW;
  }

  StringMap<unsigned> &getHiddenBWs() {
    return hiddenBW;
  }

  Map<Expr *, Expr *> &getHiddenExprs() {
    return hiddenExprs;
  }

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

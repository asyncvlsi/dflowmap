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

#include "DflowGenerator.h"

DflowGenerator::DflowGenerator(StringVec &argList,
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
  this->inBWMap = inBW;
  this->hiddenBWMap = hiddenBW;
  this->hiddenExprs = hiddenExprs;
  calc = new char[MAX_CALC_LEN];
  calc[0] = '\0';
}

bool DflowGenerator::isNewArg(const char *arg) {
  int idx = searchStringVec(oriArgList, arg);
  return (idx == -1);
}

const char *DflowGenerator::handleEVar(const char *oriArgName,
                                       const char *mappedVarName,
                                       unsigned argBW) {
  char *curArg = new char[10240];
  int idx = searchStringVec(oriArgList, oriArgName);
  if (idx == -1) {
    unsigned numArgs = argList.size();
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
  inBWMap.insert({std::string(curArg), argBW});
  return curArg;
}

void DflowGenerator::printChpPort(const char *exprName,
                                  const int resSuffix,
                                  unsigned resBW) {
  resBWList.push_back(resBW);
  char *subCalc = new char[1500];
  sprintf(subCalc, "      res%d := %s;\n", resSuffix, exprName);
  strcat(calc, subCalc);
}

void DflowGenerator::printChpConcatExpr(StringVec &operandList,
                                        const int resSuffix,
                                        unsigned resBW) {
  char *curCal = new char[MAX_CALC_LEN];
  sprintf(curCal, "      res%d := {", resSuffix);
  size_t numOps = operandList.size();
  for (size_t i = 0; i < numOps; i++) {
    strcat(curCal, operandList[i].c_str());
    if (i != numOps - 1) {
      strcat(curCal, ", ");
    }
  }
  strcat(curCal, "};\n");
  strcat(calc, curCal);
  if (debug_verbose) {
    printf("concat expr res%d has bw %u\n", resSuffix, resBW);
    printf("%s\n", curCal);
  }
  if (resBW == 0) {
    printf("resBW is 0!\n");
    exit(-1);
  }
  resBWList.push_back(resBW);
}

void DflowGenerator::printChpUniExpr(const char *op,
                                     const char *exprName,
                                     const int resSuffix,
                                     unsigned resBW) {
  char *curCal = new char[128 + strlen(exprName)];
  sprintf(curCal, "      res%d := %s %s;\n", resSuffix, op, exprName);
  strcat(calc, curCal);
  if (debug_verbose) {
    printf("uni res%d has bw %u\n", resSuffix, resBW);
    printf("%s\n", curCal);
  }
  if (resBW == 0) {
    printf("resBW is 0!\n");
    exit(-1);
  }
  resBWList.push_back(resBW);
}

void DflowGenerator::printChpBitfieldExpr(const char *exprName,
                                     unsigned hi, unsigned lo,
                                     const int resSuffix,
                                     unsigned resBW) {
  char *curCal = new char[128 + strlen(exprName)];
  sprintf(curCal, "      res%d := %s{%d..%d};\n", resSuffix, exprName, hi, lo);
  strcat(calc, curCal);
  if (debug_verbose) {
    printf("uni res%d has bw %u\n", resSuffix, resBW);
    printf("%s\n", curCal);
  }
  if (resBW == 0) {
    printf("resBW is 0!\n");
    exit(-1);
  }
  resBWList.push_back(resBW);
}


void DflowGenerator::printChpBinExpr(const char *op,
                                     const char *lexpr_name,
                                     const char *rexpr_name,
                                     int exprType,
                                     const int resSuffix,
                                     unsigned resBW) {
  if (resBW == 0) {
    printf("resBW is 0!\n");
    exit(-1);
  }
  resBWList.push_back(resBW);

  char *curCal = new char[300];
  bool binType = isBinType(exprType);
  if (binType) {
    sprintf(curCal, "      res%d := int(%s %s %s);\n",
            resSuffix, lexpr_name, op, rexpr_name);
  } else {
    sprintf(curCal, "      res%d := %s %s %s;\n",
            resSuffix, lexpr_name, op, rexpr_name);
  }
  strcat(calc, curCal);
  if (debug_verbose) {
    printf("bin res%d has bw %u\n", resSuffix, resBW);
    printf("%s\n", curCal);
  }
}

void DflowGenerator::printChpQueryExpr(const char *cexpr_name,
                                       const char *lexpr_name,
                                       const char *rexpr_name,
                                       const int resSuffix,
                                       unsigned resBW) {
  char *curCal = new char[128 + strlen(cexpr_name) + strlen(lexpr_name)
      + strlen(rexpr_name)];
  sprintf(curCal, "      res%d := bool(%s) ? %s : %s;\n",
          resSuffix, cexpr_name, lexpr_name, rexpr_name);
  strcat(calc, curCal);
  if (debug_verbose) {
    printf("query res%d has bw %u\n", resSuffix, resBW);
    printf("%s\n", curCal);
  }
  if (resBW == 0) {
    printf("resBW is 0!\n");
    exit(-1);
  }
  resBWList.push_back(resBW);
}

void DflowGenerator::preparePortForOpt(const char *expr_name,
                                       const char *portName,
                                       unsigned bw) {
  Expr *rhs = getExprFromName(expr_name, exprMap, false, E_VAR);
  Expr *expr = getExprFromName(portName, exprMap, false, E_VAR);
  hiddenBWMap.insert({expr_name, bw});
  hiddenExprs.insert({rhs, expr});
}

void DflowGenerator::prepareQueryExprForOpt(const char *cexpr_name,
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
  hiddenBWMap.insert({expr_name, bw});
  hiddenExprs.insert({rhs, expr});
  if (debug_verbose) {
    printf("rhs: ");
    print_expr(stdout, rhs);
    printf(", resExpr: ");
    print_expr(stdout, expr);
    printf(".\n");
  }
}

void DflowGenerator::prepareBinExprForOpt(const char *lexpr_name,
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
  hiddenBWMap.insert({expr_name, bw});
  hiddenExprs.insert({rhs, expr});
  if (debug_verbose) {
    printf("rhs: ");
    print_expr(stdout, rhs);
    printf(", resExpr: ");
    print_expr(stdout, expr);
    printf(".\n");
  }
}

void DflowGenerator::prepareUniExprForOpt(const char *lexpr_name,
                                          int lexpr_type,
                                          const char *expr_name,
                                          int expr_type,
                                          unsigned bw) {
  Expr *lExpr = getExprFromName(lexpr_name, exprMap, false, lexpr_type);
  Expr *rhs = getExprFromName(expr_name, exprMap, false, E_VAR);
  Expr *expr = new Expr;
  expr->type = expr_type;
  expr->u.e.l = lExpr;
  hiddenBWMap.insert({expr_name, bw});
  hiddenExprs.insert({rhs, expr});
  if (debug_verbose) {
    printf("rhs: ");
    print_expr(stdout, rhs);
    printf(", resExpr: ");
    print_expr(stdout, expr);
    printf(".\n");
  }
}

void DflowGenerator::prepareBitfieldExprForOpt(const char *lexpr_name,
                                          const char *expr_name,
                                          unsigned hi, unsigned lo,
                                          unsigned bw) {

  Expr *lExpr = getExprFromName(lexpr_name, exprMap, false, E_VAR);
  Expr *rhs = getExprFromName(expr_name, exprMap, false, E_VAR);
  
  Expr *expr = new Expr;
  expr->type = E_BITFIELD;
  expr->u.e.l = lExpr->u.e.l;
  expr->u.e.r = new Expr;
  expr->u.e.r->type = E_BITFIELD;
  expr->u.e.r->u.e.l = new Expr;
  expr->u.e.r->u.e.r = new Expr;
  expr->u.e.r->u.e.r->type = E_INT;
  expr->u.e.r->u.e.r->u.ival.v = hi;
  expr->u.e.r->u.e.r->u.ival.v_extra = NULL;
  expr->u.e.r->u.e.l->type = E_INT;
  expr->u.e.r->u.e.l->u.ival.v = lo;
  expr->u.e.r->u.e.r->u.ival.v_extra = NULL;
  
  hiddenBWMap.insert({expr_name, bw});
  hiddenExprs.insert({rhs, expr});
  
  if (debug_verbose) {
    printf("rhs: ");
    print_expr(stdout, rhs);
    printf(", resExpr: ");
    print_expr(stdout, expr);
    printf(".\n");
  }
}


void DflowGenerator::prepareConcatExprForOpt(StringVec &operandList,
                                             IntVec &opTypeList,
                                             const char *expr_name,
                                             unsigned bw) {
  Expr *rhs = getExprFromName(expr_name, exprMap, false, E_VAR);
  Expr *expr = new Expr;
  expr->type = E_CONCAT;
  Expr *rootExpr = expr;
  expr->u.e.l = NULL;
  expr->u.e.r = NULL;
  size_t numOps = operandList.size();
  for (size_t i = 0; i < numOps; i++) {
    Expr *opExpr =
      getExprFromName(Strdup (operandList[i].c_str()), exprMap, false, opTypeList[i]);
    expr->u.e.l = opExpr;
    if (i < numOps-1) {
      expr->u.e.r = new Expr;
      expr->u.e.r->type = E_CONCAT;
      expr->u.e.r->u.e.l = NULL;
      expr->u.e.r->u.e.r = NULL;
      expr = expr->u.e.r;
    }
  }
  hiddenBWMap.insert({expr_name, bw});
  hiddenExprs.insert({rhs, rootExpr});
  if (debug_verbose) {
    printf("rhs: ");
    print_expr(stdout, rhs);
    printf(", resExpr: ");
    print_expr(stdout, rootExpr);
    printf(".\n");
  }
}

const char *DflowGenerator::getCalc() {
  return calc;
}

StringVec &DflowGenerator::getArgList() {
  return argList;
}

UIntVec &DflowGenerator::getArgBWList() {
  return argBWList;
}

UIntVec &DflowGenerator::getResBWList() {
  return resBWList;
}

Map<const char *, Expr *> &DflowGenerator::getExprMap() {
  return exprMap;
}

StringMap<unsigned> &DflowGenerator::getInBW() {
  return inBWMap;
}

StringMap<unsigned> &DflowGenerator::getHiddenBWs() {
  return hiddenBWMap;
}

Map<Expr *, Expr *> &DflowGenerator::getHiddenExprs() {
  return hiddenExprs;
}

void DflowGenerator::dump() {
  printf("arg list:\n");
  for (auto &arg: argList) {
    printf("%s ", arg.c_str());
  }
  printf("\n");
  printf("arg bw list:\n");
  for (auto &bw: argBWList) {
    printf("%u ", bw);
  }
  printf("\n");
  printf("res bw list:\n");
  for (auto &resBW: resBWList) {
    printf("%u ", resBW);
  }
  printf("\n");
  printf("calc: %s\n", calc);
}


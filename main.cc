/*************************************************************************
 *
 *  This file is part of the ACT library
 *
 *  Copyright (c) 2020 Rajit Manohar
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *
 **************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <act/act.h>
#include <act/passes.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <act/lang.h>
#include <act/types.h>
#include <act/expr.h>
#include <algorithm>
#include "lib.cc"
#include "common.h"

int getBitwidth(const char *varName);

unsigned getOpUses(const char *op);

unsigned getCopyUses(const char *copyOp);

void recordExprUses(Expr *expr, CharPtrVec &charPtrVec);

const char *printExpr(Expr *expr, char *procName, char *calc, char *def,
                      StringVec &argList, StringVec &oriArgList, IntVec &argBWList,
                      IntVec &resBWList,
                      int &result_suffix, int result_bw, bool &constant, char *calcStr);

void collectExprUses(Expr *expr);

template<typename A, typename B>
std::pair<B, A> flip_pair(const std::pair<A, B> &p) {
  return std::pair<B, A>(p.second, p.first);
}

template<typename A, typename B>
std::multimap<B, A> flip_map(const std::map<A, B> &src) {
  std::multimap<B, A> dst;
  std::transform(src.begin(), src.end(), std::inserter(dst, dst.begin()),
                 flip_pair<A, B>);
  return dst;
}

void updateCopyStatistics(int bitwidth, int numOutputs) {
  auto copyStatisticsIt = copyStatistics.find(bitwidth);
  if (copyStatisticsIt != copyStatistics.end()) {
    Map<int, int> &record = copyStatisticsIt->second;
    auto recordIt = record.find(numOutputs);
    if (recordIt != record.end()) {
      recordIt->second++;
    } else {
      record.insert(GenPair(numOutputs, 1));
    }
  } else {
    Map<int, int> record = {{numOutputs, 1}};
    copyStatistics.insert(GenPair(bitwidth, record));
  }
}

void printCopyStatistics() {
  printf("COPY STATISTICS:\n");
  for (auto &copyStatisticsIt : copyStatistics) {
    printf("%d-bit COPY:\n", copyStatisticsIt.first);
    Map<int, int> &record = copyStatisticsIt.second;
    for (auto &recordIt : record) {
      printf("  %d outputs: %d\n", recordIt.first, recordIt.second);
    }
  }
  printf("\n");
}

void updateAreaStatistics(const char *instance, int area) {
  totalArea += area;
  for (auto &areaStatisticsIt : areaStatistics) {
    if (!strcmp(areaStatisticsIt.first, instance)) {
      areaStatisticsIt.second += area;
      return;
    }
  }
  areaStatistics.insert(GenPair(instance, area));
}

void printAreaStatistics() {
  printf("Area Statistics:\n");
  std::multimap<int, const char*> sortedAreas = flip_map(areaStatistics);
  std::multimap<int, const char*>::iterator sortedAreasIt;
  for (auto iter = sortedAreas.rbegin(); iter != sortedAreas.rend(); ++iter) {
    int area = iter->first;
    double ratio = (double) area / totalArea * 100;
//    if (ratio > 1) {
      printf("%40.30s %5d %5.1f\%\n", iter->second, area, ratio);
//    }
  }
  printf("\n");
}

int searchStringVec(StringVec &strVec, const char *str) {
  auto it = std::find(strVec.begin(), strVec.end(), str);
  if (it != strVec.end()) {
    return (it - strVec.begin());
  } else {
    return -1;
  }
}

const char *removeDot(const char *src) {
  int len = strlen(src);
  char *result = new char[len + 1];
  int cnt = 0;
  for (int i = 0; i < len; i++) {
    if (src[i] != '.') {
      result[cnt] = src[i];
      cnt++;
    }
  }
  result[cnt] = '\0';
  return result;
}

void printOpMetrics() {
  printf("Info in opMetrics:\n");
  for (auto &opMetricsIt : opMetrics) {
    printf("%s ", opMetricsIt.first);
  }
  printf("\n");
}

void normalizeName(char *src, char toDel, char newChar) {
  char *pos = strchr(src, toDel);
  while (pos) {
    *pos = newChar;
    pos = strchr(pos + 1, toDel);
  }
}

int *getOpMetric(const char *op) {
  if (DEBUG_VERBOSE) {
    printf("get op metric for %s\n", op);
  }
  if (op == nullptr) {
    fatal_error("op is NULL\n");
  }
  char *normalizedOp = new char[1500];
  normalizedOp[0] = '\0';
  strcat(normalizedOp, op);
  normalizeName(normalizedOp, '<', '_');
  normalizeName(normalizedOp, '>', '_');
  normalizeName(normalizedOp, ',', '_');
  for (auto &opMetricsIt : opMetrics) {
    if (!strcmp(opMetricsIt.first, normalizedOp)) {
      return opMetricsIt.second;
    }
  }
  printf("\n\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n"
         "We could not find metric info for %s\n", normalizedOp);
  printOpMetrics();
  printf("\n\n\n\n\n");
  return nullptr;
}

static void usage(char *name) {
  fprintf(stderr, "Usage: %s <actfile>\n", name);
  exit(1);
}

const char *getActIdName(ActId *actId) {
  char *str = new char[100];
  if (actId) {
    const char *actName = actId->getName();
    unsigned outUses = getOpUses(actName);
    if (outUses) {
      unsigned copyUse = getCopyUses(actName);
      if (copyUse <= outUses) {
        sprintf(str, "%scopy.out[%u]", actId->getName(), copyUse);
      } else {
        sprintf(str, "%s", actId->getName());
      }
    } else {
      sprintf(str, "%s", actId->getName());
    }
  } else {
    sprintf(str, "*");
  }
  return str;
}

void printSink(const char *name, int bitwidth) {
  if (name == nullptr) {
    fatal_error("sink name is NULL!\n");
  }
  fprintf(resFp, "sink<%d> %s_sink(%s);\n", bitwidth, name, name);
  char *instance = new char[1500];
  sprintf(instance, "sink<%d>", bitwidth);
  int *metric = getOpMetric(instance);
  createSink(instance, metric);
  updateAreaStatistics(instance, metric[3]);
}

void printInt(const char *out, const char *normalizedOut, unsigned val, int outWidth) {
  fprintf(resFp, "source<%u,%d> %s_inst(%s);\n", val, outWidth, normalizedOut, out);
  char *instance = new char[1500];
  sprintf(instance, "source<%u,%d>", val, outWidth);
  char *opName = new char[1500];
  sprintf(opName, "source%d", outWidth);
  int *metric = getOpMetric(opName);
  createSource(instance, metric);
  updateAreaStatistics(instance, metric[3]);
}

void collectBitwidthInfo(Process *p) {
  ActInstiter inst(p->CurScope());
  for (inst = inst.begin(); inst != inst.end(); inst++) {
    ValueIdx *vx = *inst;
    const char *varName = vx->getName();
    int bitwidth = TypeFactory::bitWidth(vx->t);
    bitwidthMap.insert(std::make_pair(varName, bitwidth));
  }
}

void printBitwidthInfo() {
  printf("bitwidth info:\n");
  for (auto bitwidthMapIt = bitwidthMap.begin(); bitwidthMapIt != bitwidthMap.end();
       bitwidthMapIt++) {
    printf("(%s, %d) ", bitwidthMapIt->first, bitwidthMapIt->second);
  }
  printf("\n");
}

int getActIdBW(ActId *actId, Process *p) {
  Array **aref = nullptr;
  InstType *instType = p->CurScope()->FullLookup(actId, aref);
  return TypeFactory::bitWidth(instType);
}

int getBitwidth(const char *varName) {
  for (auto &bitwidthMapIt : bitwidthMap) {
    const char *record = bitwidthMapIt.first;
    if (strcmp(record, varName) == 0) {
      return bitwidthMapIt.second;
    }
  }
  printf("We could not find bitwidth info for %s\n", varName);
  printBitwidthInfo();
  exit(-1);
}

void getCurProc(const char *str, char *val) {
  char curProc[100];
  if (strstr(str, "res")) {
    sprintf(curProc, "r%s", str + 3);
  } else if (strstr(str, "x")) {
    sprintf(curProc, "%s", str + 1);
  } else {
    sprintf(curProc, "c%s", str);
  }
  strcpy(val, curProc);
}

const char *
EMIT_BIN(Expr *expr, const char *sym, const char *op, int type, const char *metricSym,
         char *procName, char *calc, char *def, StringVec &argList, StringVec
         &oriArgList, IntVec &argBWList,
         IntVec &resBWList, int &result_suffix, int result_bw, char *calcStr) {
  Expr *lExpr = expr->u.e.l;
  Expr *rExpr = expr->u.e.r;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  bool lConst = false;
  char *lCalcStr = new char[1500];
  const char *lStr = printExpr(lExpr, procName, calc, def, argList, oriArgList, argBWList,
                               resBWList,
                               result_suffix, result_bw, lConst, lCalcStr);
  bool rConst = false;
  char *rCalcStr = new char[1500];
  const char *rStr = printExpr(rExpr, procName, calc, def, argList, oriArgList, argBWList,
                               resBWList,
                               result_suffix, result_bw, rConst, rCalcStr);
  if (lConst && rConst) {
    print_expr(stdout, expr);
    printf(" has both const operands!\n");
    printf("lExpr: ");
    print_expr(stdout, lExpr);
    printf(", rExpr: ");
    print_expr(stdout, rExpr);
    printf("\n");
    exit(-1);
  }
  char *newExpr = new char[100];
  result_suffix++;
  sprintf(newExpr, "res%d", result_suffix);
  char *curCal = new char[300];
  sprintf(curCal, "      res%d := %s %s %s;\n", result_suffix, lStr, op, rStr);
  strcat(calc, curCal);
  resBWList.push_back(result_bw);

  char *lVal = new char[100];
  getCurProc(lStr, lVal);
  char *rVal = new char[100];
  getCurProc(rStr, rVal);

  char *subProcName = new char[1500];
  sprintf(subProcName, "_%s%s%s", lVal, sym, rVal);
  strcat(procName, subProcName);
  if (DEBUG_VERBOSE) {
    printf("binary expr: ");
    print_expr(stdout, expr);
    printf("\ndflowmap generates calc: %s\n", calc);
    printf("arg list: ");
    for (auto &arg : argList) {
      printf("%s ", arg.c_str());
    }
    printf("\n");
    printf("arg bw list: ");
    for (auto &bw : argBWList) {
      printf("%d ", bw);
    }
    printf("\n");
    printf("res bw list: ");
    for (auto &bw2:resBWList) {
      printf("%d ", bw2);
    }
    printf("\n");
  }
  sprintf(calcStr, "%s", newExpr);
  return newExpr;
}

const char *
EMIT_UNI(Expr *expr, const char *sym, const char *op, int type, const char *metricSym,
         char *procName, char *calc, char *def, StringVec &argList, StringVec &oriArgList,
         IntVec &argBWList,
         IntVec &resBWList, int &result_suffix, int result_bw, char *calcStr) {
  /* collect bitwidth info */
  Expr *lExpr = expr->u.e.l;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  bool lConst;
  char *lCalcStr = new char[1500];
  const char *lStr = printExpr(lExpr, procName, calc, def, argList, oriArgList, argBWList,
                               resBWList,
                               result_suffix, result_bw, lConst, lCalcStr);
  if (lConst) {
    print_expr(stdout, expr);
    printf(" has const operands!\n");
    exit(-1);
  }
  char *val = new char[100];
  getCurProc(lStr, val);
  sprintf(procName, "%s_%s%s", procName, sym, val);
  char *newExpr = new char[100];
  result_suffix++;
  sprintf(newExpr, "res%d", result_suffix);
  char *curCal = new char[300];
  sprintf(curCal, "      res%d := %s %s;\n", result_suffix, op, lCalcStr);
  resBWList.push_back(result_bw);
  strcat(calc, curCal);
  if (DEBUG_VERBOSE) {
    printf("unary expr: ");
    print_expr(stdout, expr);
    printf("\ndflowmap generates calc: %s\n", calc);
  }
  sprintf(calcStr, "%s", newExpr);
  return newExpr;
}

const char *
printExpr(Expr *expr, char *procName, char *calc, char *def, StringVec &argList,
          StringVec &oriArgList, IntVec &argBWList,
          IntVec &resBWList, int &result_suffix, int result_bw, bool &constant,
          char *calcStr) {
  int type = expr->type;
  switch (type) {
    case E_INT: {
      if (procName[0] == '\0') {
        fatal_error("we should NOT process Source here!\n");
      }
      unsigned int val = expr->u.v;
      const char *valStr = strdup(std::to_string(val).c_str());
      sprintf(calcStr, "%s", valStr);
      constant = true;
      return valStr;
    }
    case E_VAR: {
      int numArgs = argList.size();
      auto actId = (ActId *) expr->u.e.l;
      const char *oriVarName = actId->getName();
      char *curArg = new char[1000];
      const char *mappedVarName = getActIdName(actId);
      int idx = searchStringVec(oriArgList, oriVarName);
      if (idx == -1) {
        oriArgList.push_back(oriVarName);
        argList.push_back(mappedVarName);
        sprintf(calcStr, "%s_%d", oriVarName, numArgs);
        sprintf(curArg, "x%d", numArgs);
      } else {
        sprintf(calcStr, "%s_%d", oriVarName, idx);
        sprintf(curArg, "x%d", idx);
      }
      int argBW = getBitwidth(oriVarName);
      argBWList.push_back(argBW);
      if (procName[0] == '\0') {
        resBWList.push_back(result_bw);
      }
      return curArg;
    }
    case E_AND: {
      return EMIT_BIN(expr, "and", "&", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_OR: {
      return EMIT_BIN(expr, "or", "|", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_NOT: {
      return EMIT_UNI(expr, "not", "~", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_PLUS: {
      return EMIT_BIN(expr, "add", "+", type, "add", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_MINUS: {
      return EMIT_BIN(expr, "minus", "-", type, "add", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_MULT: {
      return EMIT_BIN(expr, "mul", "*", type, "mul", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_DIV: {
      return EMIT_BIN(expr, "div", "/", type, "div", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_MOD: {
      return EMIT_BIN(expr, "mod", "%", type, "rem", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_LSL: {
      return EMIT_BIN(expr, "lsl", "<<", type, "lshift", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_LSR: {
      return EMIT_BIN(expr, "lsr", ">>", type, "lshift", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_ASR: {
      return EMIT_BIN(expr, "asr", ">>>", type, "lshift", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_UMINUS: {
      return EMIT_UNI(expr, "neg", "-", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_XOR: {
      return EMIT_BIN(expr, "xor", "^", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_LT: {
      return EMIT_BIN(expr, "lt", "<", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_GT: {
      return EMIT_BIN(expr, "gt", ">", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_LE: {
      return EMIT_BIN(expr, "le", "<=", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_GE: {
      return EMIT_BIN(expr, "ge", ">=", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_EQ: {
      return EMIT_BIN(expr, "eq", "=", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_NE: {
      return EMIT_BIN(expr, "ne", "!=", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_COMPLEMENT: {
      return EMIT_UNI(expr, "compl", "~", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr);
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      Expr *rExpr = expr->u.e.r;
      unsigned int bw;
      if (rExpr) {
        bw = rExpr->u.v;
      } else {
        bw = 1;  //TODO: double check
      }
      return printExpr(lExpr, procName, calc, def, argList, oriArgList, argBWList,
                       resBWList,
                       result_suffix, bw, constant, calcStr);
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      int bw = 1;
      return printExpr(lExpr, procName, calc, def, argList, oriArgList, argBWList,
                       resBWList,
                       result_suffix, bw, constant, calcStr);
      break;
    }
    default: {
      print_expr(stdout, expr);
      printf("\n");
      fatal_error("when printing expression, encounter unknown expression type %d\n",
                  type);
      break;
    }
  }
  fatal_error("Shouldn't be here");
  return "-should-not-be-here-";
}

unsigned getCopyUses(const char *op) {
  auto copyUsesIt = copyUses.find(op);
  if (copyUsesIt == copyUses.end()) {
    printf("We don't know how many times %s is used as COPY!\n", op);
    exit(-1);
  }
  unsigned uses = copyUsesIt->second;
  copyUsesIt->second++;
  return uses;
}

void updateOpUses(const char *op) {
  auto opUsesIt = opUses.find(op);
  if (opUsesIt == opUses.end()) {
    opUses.insert(std::make_pair(op, 0));
    copyUses.insert(std::make_pair(op, 0));
  } else {
    opUsesIt->second++;
  }
}

void recordOpUses(const char *op, CharPtrVec &charPtrVec) {
  if (std::find(charPtrVec.begin(), charPtrVec.end(), op) == charPtrVec.end()) {
    charPtrVec.push_back(op);
  }
}

void printOpUses() {
  printf("OP USES:\n");
  for (auto &opUsesIt : opUses) {
    printf("(%s, %u) ", opUsesIt.first, opUsesIt.second);
  }
  printf("\n");
}

unsigned getOpUses(const char *op) {
  auto opUsesIt = opUses.find(op);
  if (opUsesIt == opUses.end()) {
    printf("We don't know how many times %s is used!\n", op);
    printOpUses();
    exit(-1);
  }
  return opUsesIt->second;
}

void collectUniOpUses(Expr *expr) {
  Expr *lExpr = expr->u.e.l;
  collectExprUses(lExpr);
}

void collectBinOpUses(Expr *expr) {
  Expr *lExpr = expr->u.e.l;
  collectExprUses(lExpr);
  Expr *rExpr = expr->u.e.r;
  collectExprUses(rExpr);
}

void recordUniOpUses(Expr *expr, CharPtrVec &charPtrVec) {
  Expr *lExpr = expr->u.e.l;
  recordExprUses(lExpr, charPtrVec);
}

void recordBinOpUses(Expr *expr, CharPtrVec &charPtrVec) {
  Expr *lExpr = expr->u.e.l;
  recordExprUses(lExpr, charPtrVec);
  Expr *rExpr = expr->u.e.r;
  recordExprUses(rExpr, charPtrVec);
}

void collectExprUses(Expr *expr) {
  int type = expr->type;
  switch (type) {
    case E_AND:
    case E_OR:
    case E_PLUS:
    case E_MINUS:
    case E_MULT:
    case E_DIV:
    case E_MOD:
    case E_LSL:
    case E_LSR:
    case E_ASR:
    case E_XOR:
    case E_LT:
    case E_GT:
    case E_LE:
    case E_GE:
    case E_EQ:
    case E_NE: {
      collectBinOpUses(expr);
      break;
    }
    case E_NOT:
    case E_UMINUS:
    case E_COMPLEMENT: {
      collectUniOpUses(expr);
      break;
    }
    case E_INT: {
      break;
    }
    case E_VAR: {
      auto actId = (ActId *) expr->u.e.l;
      updateOpUses(actId->getName());
      break;
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      collectExprUses(lExpr);
      break;
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      collectExprUses(lExpr);
      break;
    }
    default: {
      fatal_error("Unknown expression type %d\n", type);
    }
  }
}

void recordExprUses(Expr *expr, CharPtrVec &charPtrVec) {
  int type = expr->type;
  switch (type) {
    case E_AND:
    case E_OR:
    case E_PLUS:
    case E_MINUS:
    case E_MULT:
    case E_DIV:
    case E_MOD:
    case E_LSL:
    case E_LSR:
    case E_ASR:
    case E_XOR:
    case E_LT:
    case E_GT:
    case E_LE:
    case E_GE:
    case E_EQ:
    case E_NE: {
      recordBinOpUses(expr, charPtrVec);
      break;
    }
    case E_NOT:
    case E_UMINUS:
    case E_COMPLEMENT: {
      recordUniOpUses(expr, charPtrVec);
      break;
    }
    case E_INT: {
      break;
    }
    case E_VAR: {
      auto actId = (ActId *) expr->u.e.l;
      recordOpUses(actId->getName(), charPtrVec);
      break;
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      recordExprUses(lExpr, charPtrVec);
      break;
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      recordExprUses(lExpr, charPtrVec);
      break;
    }
    default: {
      fatal_error("Unknown expression type %d\n", type);
    }
  }
}

void collectDflowClusterUses(list_t *dflow, CharPtrVec &charPtrVec) {
  listitem_t *li;
  for (li = list_first (dflow); li; li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
      case ACT_DFLOW_FUNC: {
        Expr *expr = d->u.func.lhs;
        recordExprUses(expr, charPtrVec);
        break;
      }
      case ACT_DFLOW_SPLIT: {
        ActId *input = d->u.splitmerge.single;
        recordOpUses(input->getName(), charPtrVec);
        ActId *guard = d->u.splitmerge.guard;
        recordOpUses(guard->getName(), charPtrVec);
        break;
      }
      case ACT_DFLOW_MERGE: {
        ActId *guard = d->u.splitmerge.guard;
        recordOpUses(guard->getName(), charPtrVec);
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs != 2) {
          fatal_error("Merge does not have TWO outputs!\n");
        }
        ActId **inputs = d->u.splitmerge.multi;
        ActId *lIn = inputs[0];
        recordOpUses(lIn->getName(), charPtrVec);
        ActId *rIn = inputs[1];
        recordOpUses(rIn->getName(), charPtrVec);
        break;
      }
      case ACT_DFLOW_MIXER: {
        fatal_error("We don't support MIXER for now!\n");
        break;
      }
      case ACT_DFLOW_ARBITER: {
        fatal_error("We don't support ARBITER for now!\n");
        break;
      }
      case ACT_DFLOW_CLUSTER: {
        fatal_error("Do not support nested dflow_cluster!\n");
        break;
      }
      default: {
        fatal_error("Unknown dataflow type %d\n", d->t);
        break;
      }
    }
  }
}

void collectOpUses(Process *p) {
  listitem_t *li;
  for (li = list_first (p->getlang()->getdflow()->dflow);
       li;
       li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
      case ACT_DFLOW_FUNC: {
        Expr *expr = d->u.func.lhs;
        collectExprUses(expr);
        break;
      }
      case ACT_DFLOW_SPLIT: {
        ActId *input = d->u.splitmerge.single;
        updateOpUses(input->getName());
        ActId *guard = d->u.splitmerge.guard;
        updateOpUses(guard->getName());
        break;
      }
      case ACT_DFLOW_MERGE: {
        ActId *guard = d->u.splitmerge.guard;
        updateOpUses(guard->getName());
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs != 2) {
          fatal_error("Merge does not have TWO outputs!\n");
        }
        ActId **inputs = d->u.splitmerge.multi;
        ActId *lIn = inputs[0];
        updateOpUses(lIn->getName());
        ActId *rIn = inputs[1];
        updateOpUses(rIn->getName());
        break;
      }
      case ACT_DFLOW_MIXER: {
        fatal_error("We don't support MIXER for now!\n");
        break;
      }
      case ACT_DFLOW_ARBITER: {
        fatal_error("We don't support ARBITER for now!\n");
        break;
      }
      case ACT_DFLOW_CLUSTER: {
        CharPtrVec charPtrVec;
        collectDflowClusterUses(d->u.dflow_cluster, charPtrVec);
        for (auto &charPtr : charPtrVec) {
          updateOpUses(charPtr);
        }
        break;
      }
      default: {
        fatal_error("Unknown dataflow type %d\n", d->t);
        break;
      }
    }
  }
}

void createCopyProcs() {
  fprintf(resFp, "/* copy processes */\n");
  for (auto &opUsesIt : opUses) {
    unsigned uses = opUsesIt.second;
    if (uses) {
      int N = uses + 1;
      const char *opName = opUsesIt.first;
      int bitwidth = getBitwidth(opName);
      fprintf(resFp, "copy<%d,%u> %scopy(%s);\n", bitwidth, N, opName, opName);
      char *instance = new char[1500];
      sprintf(instance, "copy<%d,%u>", bitwidth, N);
      int *metric = getOpMetric(instance);
      createCopy(instance, metric);
      updateCopyStatistics(bitwidth, N);
      updateAreaStatistics(instance, metric[3]);
    }
  }
  fprintf(resFp, "\n");
}

void printDFlowFunc(const char *procName, StringVec &argList, IntVec &argBWList,
                    IntVec &resBWList, IntVec &outWidthList, const char *def, char *calc,
                    int result_suffix, StringVec &outSendStr,
                    IntVec &outResSuffixs,
                    StringVec &normalizedOutList, StringVec &outList, StringVec
                    &initStrs) {
  calc[strlen(calc) - 2] = ';';
  if (DEBUG_CLUSTER) {
    printf("PRINT DFLOW FUNCTION\n");
    printf("procName: %s\n", procName);
    printf("arg list:\n");
    for (auto &arg : argList) {
      printf("%s ", arg.c_str());
    }
    printf("\n");
    printf("arg bw list:\n");
    for (auto &bw : argBWList) {
      printf("%d ", bw);
    }
    printf("\n");
    printf("res bw list:\n");
    for (auto &resBW : resBWList) {
      printf("%d ", resBW);
    }
    printf("\n");
    printf("outWidthList:\n");
    for (auto &outBW : outWidthList) {
      printf("%d ", outBW);
    }
    printf("\n");
    printf("def: %s\n", def);
    printf("calc: %s\n", calc);
    printf("result_suffix: %d\n", result_suffix);
    printf("outSendStr:\n");
    for (auto &outStr : outSendStr) {
      printf("%s\n", outStr.c_str());
    }
    printf("normalizedOutList:\n");
    for (auto &out : normalizedOutList) {
      printf("%s ", out.c_str());
    }
    printf("\n");
    printf("outList:\n");
    for (auto &out : outList) {
      printf("%s ", out.c_str());
    }
    printf("\n");
    printf("initStrs:\n");
    for (auto &initStr : initStrs) {
      printf("%s ", initStr.c_str());
    }
    printf("\n");
  }

  char *instance = new char[2000];
  sprintf(instance, "%s<", procName);
  int numArgs = argList.size();
  int i = 0;
  for (; i < numArgs; i++) {
    char *subInstance = new char[100];
    sprintf(subInstance, "%d,", argBWList[i]);
    strcat(instance, subInstance);
  }
  for (auto &outWidth : outWidthList) {
    char *subInstance = new char[100];
    sprintf(subInstance, "%d,", outWidth);
    strcat(instance, subInstance);
  }
  int numRes = resBWList.size();
  for (i = 0; i < numRes - 1; i++) {
    char *subInstance = new char[100];
    sprintf(subInstance, "%d,", resBWList[i]);
    strcat(instance, subInstance);
  }
  char *subInstance = new char[100];
  sprintf(subInstance, "%d>", resBWList[i]);
  strcat(instance, subInstance);

  fprintf(resFp, "%s ", instance);
  for (auto &normalizedOut : normalizedOutList) {
    fprintf(resFp, "%s_", normalizedOut.c_str());
  }
  fprintf(resFp, "inst(");
  for (auto &arg : argList) {
    fprintf(resFp, "%s, ", arg.c_str());
  }
  int numOuts = outList.size();
  if (numOuts < 1) {
    fatal_error("No output is found!\n");
  }
  for (i = 0; i < numOuts - 1; i++) {
    fprintf(resFp, "%s, ", outList[i].c_str());
  }
  fprintf(resFp, "%s);\n", outList[i].c_str());
  /* create chp library */
  if (strlen(instance) < 5) {
    fatal_error("Invalid instance name %s\n", instance);
  }
  char *opName = instance + 5;
  int *metric = getOpMetric(opName);
  char *outSend = new char[10240];
  sprintf(outSend, "      ");
  for (i = 0; i < numOuts - 1; i++) {
    char *subSend = new char[1500];
    sprintf(subSend, "%s, ", outSendStr[i].c_str());
    strcat(outSend, subSend);
  }
  char *subSend = new char[1500];
  sprintf(subSend, "%s;\n", outSendStr[i].c_str());
  strcat(outSend, subSend);
  char *log = new char[1500];
  sprintf(log, "      log(\"send (\", ");
  for (auto &outResSuffix : outResSuffixs) {
    char *subLog = new char[100];
    sprintf(subLog, "res%d, \",\", ", outResSuffix);
    strcat(log, subLog);
  }
  char *subLog = new char[100];
  sprintf(subLog, "\")\")");
  strcat(log, subLog);
  strcat(outSend, log);
  char *initSend = nullptr;
  int numInitStrs = initStrs.size();
  if (DEBUG_CLUSTER) {
    printf("numInitStrs: %d\n", numInitStrs);
  }
  if (numInitStrs > 0) {
    initSend = new char[10240];
    sprintf(initSend, "    ");
    for (i = 0; i < numInitStrs - 1; i++) {
      char *subInitSend = new char[1500];
      sprintf(subInitSend, "%s, ", initStrs[i].c_str());
      strcat(initSend, subInitSend);
    }
    char *subInitSend = new char[1500];
    sprintf(subInitSend, "%s;\n", initStrs[i].c_str());
    strcat(initSend, subInitSend);
  }
  createFULib(procName, calc, def, outSend, initSend, numArgs, numOuts, numRes, instance,
              metric);
  updateAreaStatistics(procName, metric[3]);
}

void handleDFlowFunc(Process *p, act_dataflow_element *d, char *procName, char *calc,
                     char *def, StringVec &argList, StringVec &oriArgList,
                     IntVec &argBWList,
                     IntVec &resBWList, int &result_suffix, StringVec &outSendStr,
                     IntVec &outResSuffixs,
                     StringVec &outList, StringVec &normalizedOutList,
                     IntVec &outWidthList, StringVec &initStrs) {
  if (d->t != ACT_DFLOW_FUNC) {
    dflow_print(stdout, d);
    printf("This is not dflow_func!\n");
    exit(-1);
  }
  /* handle left hand side */
  Expr *expr = d->u.func.lhs;
  int type = expr->type;
  /* handle right hand side */
  ActId *rhs = d->u.func.rhs;
  char out[10240];
  out[0] = '\0';
  rhs->sPrint(out, 10240, NULL, 0);
  const char *normalizedOut = removeDot(out);
  int outWidth = getActIdBW(rhs, p);
  if (DEBUG_VERBOSE) {
    printf("%%%%%%%%%%%%%%%%%%%%%\nHandle expr ");
    print_expr(stdout, expr);
    printf("\n%%%%%%%%%%%%%%%%%%%%%\n");
  }
  Expr *initExpr = d->u.func.init;
  Expr *nbufs = d->u.func.nbufs;
  if (initExpr) {
    if (initExpr->type != E_INT) {
      print_expr(stdout, initExpr);
      printf("The init value is not E_INT type!\n");
      exit(-1);
    }
  }
  if (type == E_INT) {
    unsigned int val = expr->u.v;
    printInt(out, normalizedOut, val, outWidth);
    if (initExpr) {
      print_expr(stdout, expr);
      printf(" has const lOp, but its rOp has init token!\n");
      exit(-1);
    }
  } else {
    bool constant = false;
    char *calcStr = new char[1500];
    calcStr[0] = '\0';
    printExpr(expr, procName, calc, def, argList, oriArgList, argBWList, resBWList,
              result_suffix, outWidth, constant, calcStr);
    if (constant) {
      print_expr(stdout, expr);
      printf("=> we should not process constant lhs here!\n");
      exit(-1);
    }
    int numArgs = argList.size();
    if (procName[0] == '\0') {
      sprintf(procName, "func_port");
      if ((numArgs != 1) || (result_suffix != -1) || (calc[0] != '\n')) {
        printf("We are processing expression ");
        print_expr(stdout, expr);
        printf(", the procName is empty, "
               "but the argList or result_suffix or calc is abnormal!\n");
        exit(-1);
      }
      result_suffix++;
      sprintf(calc, "%s      res0 := x0;\n", calc);
    }
    if (initExpr) {
      int initVal = initExpr->u.v;
      char *subProcName = new char[1500];
      sprintf(subProcName, "_init%d", initVal);
      strcat(procName, subProcName);
    }
    if (DEBUG_CLUSTER) {
      printf("___________________________________\nFor dataflow element: ");
      dflow_print(stdout, d);
      printf("\n___________________________________________\n\n\n\n\n\n");
      printf("procName: %s\n", procName);
      printf("arg list:\n");
      for (auto &arg : argList) {
        printf("%s ", arg.c_str());
      }
      printf("\n");
      printf("oriArgList:\n");
      for (auto &oriArg : oriArgList) {
        printf("%s ", oriArg.c_str());
      }
      printf("\n");
      printf("arg bw list:\n");
      for (auto &bw : argBWList) {
        printf("%d ", bw);
      }
      printf("\n");
      printf("res bw list:\n");
      for (auto &resBW : resBWList) {
        printf("%d ", resBW);
      }
      printf("\n");
      printf("out bw: %d\n", outWidth);
      printf("def: %s\n", def);
      printf("calc: %s\n", calc);
      printf("result_suffix: %d\n", result_suffix);
      printf("normalizedOut: %s, out: %s\n", normalizedOut, out);
      printf("init expr: ");
      print_expr(stdout, initExpr);
      printf("\n");
    }
    outList.push_back(out);
    normalizedOutList.push_back(normalizedOut);
    outWidthList.push_back(outWidth);
    char *outStr = new char[10240];
    outStr[0] = '\0';
    int numOuts = outList.size();
    sprintf(outStr, "out%d!res%d", (numOuts - 1), result_suffix);
    char *ase = new char[1500];
    if (initExpr) {
      int initVal = initExpr->u.v;
      sprintf(ase, "out%d!%d", (numOuts - 1), initVal);
      initStrs.push_back(ase);
    }
    if (DEBUG_CLUSTER) {
      printf("@@@@@@@@@@@@@@@@ generate %s\n", outStr);
    }
    outSendStr.push_back(outStr);
    outResSuffixs.push_back(result_suffix);
  }
}

void handleNormalDflowElement(Process *p, act_dataflow_element *d) {
  switch (d->t) {
    case ACT_DFLOW_FUNC: {
      char *procName = new char[200];
      procName[0] = '\0';
      char *calc = new char[10240];
      calc[0] = '\0';
      sprintf(calc, "\n");
      char *def = new char[10240];
      def[0] = '\0';
      sprintf(def, "\n");
      StringVec argList;
      StringVec oriArgList;
      IntVec argBWList;
      IntVec resBWList;
      int result_suffix = -1;
      StringVec outSendStr;
      IntVec outResSuffixs;
      StringVec outList;
      StringVec normalizedOutList;
      IntVec outWidthList;
      StringVec initStrs;
      handleDFlowFunc(p, d, procName, calc, def, argList, oriArgList, argBWList,
                      resBWList, result_suffix, outSendStr, outResSuffixs, outList,
                      normalizedOutList, outWidthList, initStrs);
      if (DEBUG_CLUSTER) {
        printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        printf("Process normal dflow:\n");
        dflow_print(stdout, d);
        printf("\n");
      }
      if (strlen(procName)) {
        printDFlowFunc(procName, argList, argBWList, resBWList, outWidthList, def, calc,
                       result_suffix, outSendStr, outResSuffixs, normalizedOutList,
                       outList,
                       initStrs);
      }
      break;
    }
    case ACT_DFLOW_SPLIT: {
      ActId *input = d->u.splitmerge.single;
      const char *inputName = input->getName();
      int inputSize = strlen(inputName);
      int bitwidth = getActIdBW(input, p);
      ActId **outputs = d->u.splitmerge.multi;
      ActId *lOut = outputs[0];
      ActId *rOut = outputs[1];
      const char *outName = nullptr;
      char *splitName = nullptr;
      ActId *guard = d->u.splitmerge.guard;
      const char *guardName = guard->getName();
      int guardSize = strlen(guardName);
      if (!lOut && !rOut) {  // split has empty target for both ports
        splitName = new char[inputSize + guardSize + 8];
        printf("no target. iS: %d, gS: %d\n", inputSize, guardSize);
        strcpy(splitName, inputName);
        strcat(splitName, "_");
        strcat(splitName, guardName);
        strcat(splitName, "_SPLIT");
      } else {
        if (lOut) {
          outName = lOut->getName();
        } else {
          outName = rOut->getName();
        }
        size_t splitSize = strlen(outName) - 1;
        splitName = new char[splitSize];
        memcpy(splitName, outName, splitSize - 1);
        splitName[splitSize - 1] = '\0';
      }
      if (!lOut) {
        char *sinkName = new char[1500];
        sprintf(sinkName, "%s_L", splitName);
        printSink(sinkName, bitwidth);
//        printf("in: %s, c: %s, split: %s, L\n", inputName, guardName, splitName);
      }
      if (!rOut) {
        char *sinkName = new char[1500];
        sprintf(sinkName, "%s_R", splitName);
        printSink(sinkName, bitwidth);
//        printf("in: %s, c: %s, split: %s, R\n", inputName, guardName, splitName);
      }
      fprintf(resFp, "control_split<%d> %s_inst(", bitwidth, splitName);
      const char *guardStr = getActIdName(guard);
      const char *inputStr = getActIdName(input);
      fprintf(resFp, "%s, %s, %s_L, %s_R);\n", guardStr, inputStr, splitName,
              splitName);
      char *instance = new char[1500];
      sprintf(instance, "control_split<%d>", bitwidth);
      int *metric = getOpMetric(instance);
      createSplit(instance, metric);
      updateAreaStatistics(instance, metric[3]);
      break;
    }
    case ACT_DFLOW_MERGE: {
      ActId *output = d->u.splitmerge.single;
      const char *outputName = output->getName();
      int bitwidth = getActIdBW(output, p);
      fprintf(resFp, "control_merge<%d> %s_inst(", bitwidth, outputName);
      ActId *guard = d->u.splitmerge.guard;
      const char *guardStr = getActIdName(guard);
      ActId **inputs = d->u.splitmerge.multi;
      ActId *lIn = inputs[0];
      const char *lInStr = getActIdName(lIn);
      ActId *rIn = inputs[1];
      const char *rInStr = getActIdName(rIn);
      fprintf(resFp, "%s, %s, %s, %s);\n", guardStr, lInStr, rInStr, outputName);
      char *instance = new char[1500];
      sprintf(instance, "control_merge<%d>", bitwidth);
      int *metric = getOpMetric(instance);
      createMerge(instance, metric);
      updateAreaStatistics(instance, metric[3]);
      break;
    }
    case ACT_DFLOW_MIXER: {
      fatal_error("We don't support MIXER for now!\n");
      break;
    }
    case ACT_DFLOW_ARBITER: {
      fatal_error("We don't support ARBITER for now!\n");
      break;
    }
    case ACT_DFLOW_CLUSTER: {
      dflow_print(stdout, d);
      fatal_error("We should not process dflow_clsuter here!");
    }
    default: {
      fatal_error("Unknown dataflow type %d\n", d->t);
      break;
    }
  }
}

void dflow_print(FILE *fp, list_t *dflow) {
  listitem_t *li;
  act_dataflow_element *e;

  for (li = list_first (dflow); li; li = list_next (li)) {
    e = (act_dataflow_element *) list_value (li);
    dflow_print(fp, e);
    if (list_next (li)) {
      fprintf(fp, ";");
    }
    fprintf(fp, "\n");
  }
}

void handleDFlowCluster(Process *p, list_t *dflow) {
  listitem_t *li;
  char *procName = new char[200];
  procName[0] = '\0';
  char *calc = new char[10240];
  calc[0] = '\0';
  sprintf(calc, "\n");
  char *def = new char[10240];
  def[0] = '\0';
  sprintf(def, "\n");
  StringVec argList;
  StringVec oriArgList;
  IntVec argBWList;
  IntVec resBWList;
  int result_suffix = -1;
  StringVec outSendStr;
  IntVec outResSuffixs;
  StringVec outList;
  StringVec normalizedOutList;
  IntVec outWidthList;
  StringVec initStrs;
  for (li = list_first (dflow); li; li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    if (d->t == ACT_DFLOW_FUNC) {
      handleDFlowFunc(p, d, procName, calc, def, argList, oriArgList, argBWList,
                      resBWList, result_suffix, outSendStr, outResSuffixs, outList,
                      normalizedOutList, outWidthList, initStrs);
    } else {
      dflow_print(stdout, d);
      fatal_error("This dflow statement should not appear in dflow-cluster!\n");
    }
  }
  if (DEBUG_CLUSTER) {
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("Process cluster dflow:\n");
    dflow_print(stdout, dflow);
    printf("\n");
  }
  if (strlen(procName)) {
    printDFlowFunc(procName, argList, argBWList, resBWList, outWidthList, def, calc,
                   result_suffix, outSendStr, outResSuffixs, normalizedOutList, outList,
                   initStrs);
  }
}

void handleProcess(Process *p) {
  const char *pName = p->getName();
  printf("processing %s\n", pName);
  bool mainProc = (strcmp(pName, "main<>") == 0);
  if (!p->getlang()->getdflow()) {
    fatal_error("Process `%s': no dataflow body", p->getName());
  }
  p->PrintHeader(resFp, "defproc");
  fprintf(resFp, "\n{");
  p->CurScope()->Print(resFp);
  bitwidthMap.clear();
  opUses.clear();
  copyUses.clear();
  collectBitwidthInfo(p);
  collectOpUses(p);
  createCopyProcs();
  listitem_t *li;
  for (li = list_first (p->getlang()->getdflow()->dflow);
       li;
       li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    if (d->t == ACT_DFLOW_CLUSTER) {
      list_t *dflow_cluster = d->u.dflow_cluster;
      if (DEBUG_CLUSTER) {
        dflow_print(stdout, dflow_cluster);
      }
      handleDFlowCluster(p, dflow_cluster);
    } else {
      handleNormalDflowElement(p, d);
    }
  }
  if (mainProc) {
    const char *outPort = "main_out";
    int outWidth = getBitwidth(outPort);
    printSink(outPort, outWidth);
  }
  fprintf(resFp, "}\n\n");
}

void initialize() {
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    processes[i] = nullptr;
  }
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    instances[i] = nullptr;
  }
}

void updateMetrics(const char *op, int *metric) {
  if (DEBUG_VERBOSE) {
    printf("Update metrics for %s\n", op);
  }
  for (auto &opMetricsIt : opMetrics) {
    if (!strcmp(opMetricsIt.first, op)) {
      fatal_error("We already have metric info for %s", op);
    }
  }
  opMetrics.insert(std::make_pair(op, metric));
}

int main(int argc, char **argv) {
  /* initialize ACT library */
  Act::Init(&argc, &argv);

  /* some usage check */
  if (argc != 2) {
    usage(argv[0]);
  }

  /* read in the ACT file */
  Act *a = new Act(argv[1]);
  a->Expand();
  a->mangle(NULL);
  fprintf(stdout, "Processing ACT file %s!\n", argv[1]);
  if (DEBUG_VERBOSE) {
    printf("------------------ACT FILE--------------------\n");
    a->Print(stdout);
    printf("\n\n\n");
  }
  /* create output file */
  char *result_file = new char[8 + strlen(argv[1])];
  strcpy(result_file, "result_");
  strcat(result_file, argv[1]);
  resFp = fopen(result_file, "w");

  char *lib_file = new char[5 + strlen(argv[1])];
  strcpy(lib_file, "lib_");
  strcat(lib_file, argv[1]);
  libFp = fopen(lib_file, "w");
  fprintf(resFp, "import \"%s\";\n\n", lib_file);

  char *conf_file = new char[6 + strlen(argv[1])];
  strcpy(conf_file, "conf_");
  strcat(conf_file, argv[1]);
  confFp = fopen(conf_file, "w");
  fprintf(confFp, "begin sim.chp\n");

  /* read in the Metric file */
  char *metricFilePath = new char[1000];
  sprintf(metricFilePath, "metrics/fluid.metrics");
  printf("Read metric file: %s\n", metricFilePath);
  std::ifstream metricFp(metricFilePath);
  std::string line;
  while (std::getline(metricFp, line)) {
    std::istringstream iss(line);
    char *instance = new char[2000];
    int metricCount = -1;
    int *metric = new int[4];
    bool emptyLine = true;
    do {
      std::string numStr;
      iss >> numStr;
      if (!numStr.empty()) {
        emptyLine = false;
        if (metricCount >= 0) {
          metric[metricCount] = std::atoi(numStr.c_str());
        } else {
          sprintf(instance, "%s", numStr.c_str());
        }
        metricCount++;
      }
    } while (iss);
    if (!emptyLine && (metricCount != 4)) {
      printf("%s has %d metrics!\n", metricFilePath, metricCount);
      exit(-1);
    }
    if (DEBUG_VERBOSE) {
      printf("Add metrics for %s\n", instance);
    }
    updateMetrics(instance, metric);
  }
  ActTypeiter it(a->Global());
  initialize();
  Process *procArray[MAX_PROCESSES];
  unsigned index = 0;
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      procArray[index] = p;
      index++;
    }
  }
  for (int i = 0; i < index; i++) {
//  for (int i = index - 1; i >= 0; i--) {
    Process *p = procArray[i];
    handleProcess(p);
  }
  fprintf(resFp, "main m;\n");
  fprintf(confFp, "end\n");
  fclose(resFp);
  fclose(confFp);

  printCopyStatistics();
  printAreaStatistics();
  return 0;
}


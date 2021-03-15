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

const char *printExpr(Expr *expr, char *procName, char *calc, char *def,
                      StringVec &argList, IntVec &argBWList, IntVec &resBWList,
                      int &result_suffix, int result_bw, bool &constant);

void collectExprUses(Expr *expr);

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
  char *normalizedOp = new char[1500];
  sprintf(normalizedOp, "%s", op);
//  strcat(normalizedOp, op);
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
}

void printInt(const char *out, const char *normalizedOut, unsigned val, int outWidth) {
  fprintf(resFp, "source<%u,%d> %s_inst(%s);\n", val, outWidth, normalizedOut, out);
  char *instance = new char[1500];
  sprintf(instance, "source<%u,%d>", val, outWidth);
  char *opName = new char[1500];
  sprintf(opName, "source%d", outWidth);
  int *metric = getOpMetric(opName);
  createSource(instance, metric);
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
//  curProc[0]='\0';
  if (strstr(str, "res")) {
//    strncpy(curProc, str + 3, strlen(str) - 3);
    sprintf(curProc, "r%s", str + 3);
  } else if (strstr(str, "x")) {
    sprintf(curProc, "%s", str + 1);
//    printf("111: %s\n", str);
//    strncpy(curProc, str + 1, strlen(str) - 1);
//    printf("222: %s, %d\n", curProc, strlen(str));
  } else {
    sprintf(curProc, "c%s", str);
  }
//  curProc[strlen(curProc)] = '\0';
  strcpy(val, curProc);
}

const char *
EMIT_BIN(Expr *expr, const char *sym, const char *op, int type, const char *metricSym,
         char *procName, char *calc, char *def, StringVec &argList, IntVec &argBWList,
         IntVec &resBWList, int &result_suffix, int result_bw) {
  Expr *lExpr = expr->u.e.l;
  Expr *rExpr = expr->u.e.r;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  bool lConst;
  const char *lStr = printExpr(lExpr, procName, calc, def, argList, argBWList, resBWList,
                               result_suffix, result_bw, lConst);
  bool rConst;
  const char *rStr = printExpr(rExpr, procName, calc, def, argList, argBWList, resBWList,
                               result_suffix, result_bw, rConst);
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

  sprintf(procName, "%s_%s%s%s", procName, lVal, sym, rVal);
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
  return newExpr;
}

const char *
EMIT_UNI(Expr *expr, const char *sym, const char *op, int type, const char *metricSym,
         char *procName, char *calc, char *def, StringVec &argList, IntVec &argBWList,
         IntVec &resBWList, int &result_suffix, int result_bw) {
  /* collect bitwidth info */
  Expr *lExpr = expr->u.e.l;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  bool lConst;
  const char *lStr = printExpr(lExpr, procName, calc, def, argList, argBWList, resBWList,
                               result_suffix, result_bw, lConst);
  if (lConst) {
    print_expr(stdout, expr);
    printf(" has const operands!\n");
    exit(-1);
  }
  char *val = new char[100];
  getCurProc(lStr, val);
  sprintf(procName, "%s_%s%s", procName, sym, val);
//  if (lConst) {
//    sprintf(procName, "%s_%s%s", procName, sym, lStr);
//  } else {
//    sprintf(procName, "%s_%s", procName, sym);
//  }
  char *newExpr = new char[100];
  result_suffix++;
  sprintf(newExpr, "res%d", result_suffix);
  char *curCal = new char[300];
  sprintf(curCal, "      res%d := %s %s;\n", result_suffix, lStr, op);
  resBWList.push_back(result_bw);
  strcat(calc, curCal);
  if (DEBUG_VERBOSE) {
    printf("unary expr: ");
    print_expr(stdout, expr);
    printf("\ndflowmap generates calc: %s\n", calc);
  }
  return newExpr;
}

const char *
printExpr(Expr *expr, char *procName, char *calc, char *def, StringVec &argList,
          IntVec &argBWList,
          IntVec &resBWList, int &result_suffix, int result_bw, bool &constant) {
  int type = expr->type;
  switch (type) {
    case E_INT: {
      if (procName[0] == '\0') {
        fatal_error("we should NOT process Source here!\n");
      }
      unsigned int val = expr->u.v;
      const char *valStr = strdup(std::to_string(val).c_str());
      constant = true;
      return valStr;
    }
    case E_VAR: {
      int numArgs = argList.size();
      char *curArg = new char[10];
      strcpy(curArg, "x");
      strcat(curArg, strdup(std::to_string(numArgs).c_str()));
      auto actId = (ActId *) expr->u.e.l;
      int argBW = getBitwidth(actId->getName());
      const char *varStr = getActIdName(actId);
      argList.push_back(varStr);
      argBWList.push_back(argBW);
      if (procName[0] == '\0') {
        resBWList.push_back(result_bw);
      }
      return curArg;
    }
    case E_AND: {
      return EMIT_BIN(expr, "and", "&", type, "and", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_OR: {
      return EMIT_BIN(expr, "or", "|", type, "and", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_NOT: {
      return EMIT_UNI(expr, "not", "~", type, "and", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_PLUS: {
      return EMIT_BIN(expr, "add", "+", type, "add", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_MINUS: {
      return EMIT_BIN(expr, "minus", "-", type, "add", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_MULT: {
      return EMIT_BIN(expr, "mul", "*", type, "mul", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_DIV: {
      return EMIT_BIN(expr, "div", "/", type, "div", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_MOD: {
      return EMIT_BIN(expr, "mod", "%", type, "rem", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_LSL: {
      return EMIT_BIN(expr, "lsl", "<<", type, "lshift", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_LSR: {
      return EMIT_BIN(expr, "lsr", ">>", type, "lshift", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_ASR: {
      return EMIT_BIN(expr, "asr", ">>>", type, "lshift", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_UMINUS: {
      return EMIT_UNI(expr, "neg", "-", type, "and", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_XOR: {
      return EMIT_BIN(expr, "xor", "^", type, "and", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_LT: {
      return EMIT_BIN(expr, "lt", "<", type, "icmp", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_GT: {
      return EMIT_BIN(expr, "gt", ">", type, "icmp", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_LE: {
      return EMIT_BIN(expr, "le", "<=", type, "icmp", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_GE: {
      return EMIT_BIN(expr, "ge", ">=", type, "icmp", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_EQ: {
      return EMIT_BIN(expr, "eq", "=", type, "icmp", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_NE: {
      return EMIT_BIN(expr, "ne", "!=", type, "icmp", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
    }
    case E_COMPLEMENT: {
      return EMIT_UNI(expr, "compl", "~", type, "and", procName, calc, def, argList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw);
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
      return printExpr(lExpr, procName, calc, def, argList, argBWList, resBWList,
                       result_suffix, bw, constant);
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      int bw = 1;
      return printExpr(lExpr, procName, calc, def, argList, argBWList, resBWList,
                       result_suffix, bw, constant);
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
      printf("handle op %s\n", opName);
      int bitwidth = getBitwidth(opName);
      char *instance = new char[1500];
      sprintf(instance, "copy<%d,%u>", bitwidth, N);
      int *metric = getOpMetric(instance);
      createCopy(instance, metric);
      fprintf(resFp, "copy<%d,%u> %scopy(%s);\n", bitwidth, N, opName, opName);
    }
  }
  fprintf(resFp, "\n");
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
    switch (d->t) {
      case ACT_DFLOW_FUNC: {
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
          char *procName = new char[200];
          procName[0] = '\0';
          char *calc = new char[10240];
          calc[0] = '\0';
          sprintf(calc, "\n");
          char *def = new char[10240];
          def[0] = '\0';
          sprintf(def, "\n");
          StringVec argList;
          IntVec argBWList;
          IntVec resBWList;
          int result_suffix = -1;
          bool constant = false;
          printExpr(expr, procName, calc, def, argList, argBWList, resBWList,
                    result_suffix, outWidth, constant);
          if (constant) {
            print_expr(stdout, expr);
            printf("=> we should not process constant lhs here!\n");
            exit(-1);
          }
          int numArgs = argList.size();
          if (DEBUG_VERBOSE) {
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
            printf("out bw: %d\n", outWidth);
            printf("calc: %s\n", calc);
            printf("result_suffix: %d\n", result_suffix);
          }
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
            sprintf(procName, "%s_init%d", procName,
                    initVal);  //TODO: collect metric for init
          }
          char *instance = new char[2000];
          sprintf(instance, "%s<", procName);
          int i = 0;
          for (; i < numArgs; i++) {
            sprintf(instance, "%s%d,", instance, argBWList[i]);
          }
          sprintf(instance, "%s%d,", instance, outWidth);
          int numRes = resBWList.size();
          if (DEBUG_VERBOSE) {
            printf("numRes: %d\n", numRes);
            for (i = 0; i < numRes; i++) {
              printf("%d ", resBWList[i]);
            }
            printf("\n");
          }
          for (i = 0; i < numRes - 1; i++) {
            sprintf(instance, "%s%d,", instance, resBWList[i]);
          }
          sprintf(instance, "%s%d>", instance, resBWList[i]);
          printf("instance: %s", instance);


          fprintf(resFp, "%s %s_inst(", instance, normalizedOut);
          for (auto &arg : argList) {
            fprintf(resFp, "%s, ", arg.c_str());
          }
          fprintf(resFp, "%s);\n", out);
          /* create chp library */
          char *opName = new char[1500];
          strncpy(opName, instance + 5, strlen(instance) - 5);
          int *metric = getOpMetric(opName);
          if (initExpr) {
            int initVal = initExpr->u.v;
            int calcLen = strlen(calc);
            calc[calcLen - 2] = '\0';
            createFUInitLib(procName, calc, def, numArgs, result_suffix, numRes, initVal);
          } else {
            createFULib(procName, calc, def, numArgs, result_suffix, numRes, instance,
                        metric);
          }
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
          printf("in: %s, c: %s, split: %s, L\n", inputName, guardName, splitName);
        }
        if (!rOut) {
          char *sinkName = new char[1500];
          sprintf(sinkName, "%s_R", splitName);
          printSink(sinkName, bitwidth);
          printf("in: %s, c: %s, split: %s, R\n", inputName, guardName, splitName);
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
      default: {
        fatal_error("Unknown dataflow type %d\n", d->t);
        break;
      }
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
  auto opMetricsIt = opMetrics.find(op);
  if (opMetricsIt != opMetrics.end()) {
    fatal_error("We already have metric info for %s", op);
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
  const char *metricPath = "metrics/";
  DIR *dir = opendir(metricPath);
  if (!dir) {
    fatal_error("Could not find metric path %s\n", metricPath);
  }
  struct dirent *entry;
  entry = readdir(dir);
  while (entry) {
    char *metricFile = entry->d_name;
    if (strcmp(metricFile, ".") != 0 && strcmp(metricFile, "..") != 0) {
      char *metricFilePath = new char[1000];
      sprintf(metricFilePath, "%s%s", metricPath, metricFile);
      printf("Read metric file: %s\n", metricFilePath);
      std::ifstream metricFp(metricFilePath);
      std::string line;
      std::getline(metricFp, line);
      std::istringstream iss(line);
      int metricCount = 0;
      int *metric = new int[4];
      do {
        std::string numStr;
        iss >> numStr;
        if (!numStr.empty()) {
          metric[metricCount] = std::atoi(numStr.c_str());
          metricCount++;
        }
      } while (iss);
      if (metricCount != 4) {
        printf("%s has %d metrics!\n", metricFile, metricCount);
        exit(-1);
      }
      char *op = new char[1500];
      int opLen = strlen(metricFile) - 7;
      strncpy(op, metricFile, opLen);
      op[opLen] = '\0';
      updateMetrics(op, metric);
    }
    entry = readdir(dir);
  }
  closedir(dir);





//  for (int i = 0; i < numOps; i++) {
//    const char *op = ops[i];
//    printf("Read metric file for %s\n", op);
//    char *metricFile = new char[strlen(op) + 16];
//    strcpy(metricFile, "metrics/");
//    strcat(metricFile, op);
//    strcat(metricFile, ".metric");
//    std::ifstream metricFp(metricFile);
//    std::string line;
//    int bwCount = 0;
//    int **metric = new int *[numBWs];
//
//    while (std::getline(metricFp, line)) {
//      std::istringstream iss(line);
//      int metricCount = 0;
//      metric[bwCount] = new int[4];
//      do {
//        std::string numStr;
//        iss >> numStr;
//        if (!numStr.empty()) {
//          metric[bwCount][metricCount] = std::atoi(numStr.c_str());
//          metricCount++;
//        }
//      } while (iss);
//      if (metricCount != 4) {
//        printf("%s has %d metrics for the %d-th bitwidth info!\n", metricFile, metricCount,
//               bwCount);
//        exit(-1);
//      }
//      bwCount++;
//    }
//    if (bwCount != numBWs) {
//      printf("%s has %d bitwidth info!\n", metricFile, bwCount);
//      exit(-1);
//    }
//    opMetrics.insert(std::make_pair(op, metric));
//  }




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
  return 0;
}


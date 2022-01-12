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

#include "Helper.h"

bool isActnCp(const char *instance) {
  return std::string(instance).find(Constant::ACTN_CP_PREFIX) == 0;
}

bool isActnDp(const char *instance) {
  return std::string(instance).find(Constant::ACTN_DP_PREFIX) == 0;
}

void normalizeName(char *src, char toDel, char newChar) {
  char *pos = strchr(src, toDel);
  while (pos) {
    *pos = newChar;
    pos = strchr(pos + 1, toDel);
  }
}

const char *getNormInstanceName(const char *src) {
  char *result = new char[1 + strlen(src)];
  sprintf(result, "%s", src);
  normalizeName(result, '<', '_');
  normalizeName(result, '>', '_');
  normalizeName(result, ',', '_');
  return result;
}
const char *getNormActIdName(const char *src) {
  char *result = new char[1 + strlen(src)];
  sprintf(result, "%s", src);
  normalizeName(result, '.', '_');
  return result;
}

void printIntVec(IntVec &intVec) {
  for (auto &val : intVec) {
    printf("%d ", val);
  }
  printf("\n");
}

void printULongVec(ULongVec &longVec) {
  for (auto &val : longVec) {
    printf("%lu ", val);
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

Expr *genExprFromStr(const char *str,
                     int exprType) {
  Expr *expr = new Expr;
  auto newLActId = ActId::parseId(str);
  expr->type = exprType;
  expr->u.e.l = (Expr *) (newLActId);
  return expr;
}

Expr *genExprFromInt(unsigned long val) {
  Expr *expr = new Expr;
  expr->type = E_INT;
  expr->u.v = val;
  expr->u.v_extra = nullptr;
  return expr;
}

Expr *getExprFromName(const char *name,
                      Map<const char *, Expr *> &exprMap,
                      bool exitOnMissing,
                      int exprType) {
  for (auto &exprMapIt : exprMap) {
    if (strcmp(name, exprMapIt.first) == 0) {
      return exprMapIt.second;
    }
  }
  if (exitOnMissing) {
    printf("We could not find the expr for %s!\n", name);
    exit(-1);
  }
  Expr *newExpr = nullptr;
  if (exprType == E_INT) {
    newExpr = genExprFromInt(std::stoul(std::string(name)));
  } else {
    newExpr = genExprFromStr(name, exprType);
  }
  exprMap.insert({name, newExpr});
  return newExpr;
}

bool isBinType(int exprType) {
  return (exprType == E_LT) || (exprType == E_GT) || (exprType == E_LE)
      || (exprType == E_GE) || (exprType == E_EQ) || (exprType == E_NE);
}

void getActIdName(Scope* sc, ActId *actId, char *buff, int sz) {
  ActId *uid = actId->Canonical(sc)->toid();
  uid->sPrint(buff, sz);
  delete uid;
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

void getActConnectionName(act_connection *actConnection,
                                         char *buff,
                                         int sz) {
  if (actConnection == nullptr) {
    printf("Try to get the name of NULL act connection!\n");
    exit(-1);
  }
  ActId *uid = actConnection->toid();
  uid->sPrint(buff, sz);
  delete uid;
}

void checkACTN(const char *channel,
                              bool &actnCp,
                              bool &actnDp) {
  if (isActnCp(channel)) {
    actnCp = true;
  }
  if (isActnDp(channel)) {
    actnDp = true;
  }
  if (actnCp && actnDp) {
    printf("%s is both actnCp and actnDp!\n", channel);
    exit(-1);
  }
}

void print_dflow(FILE *fp, list_t *dflow) {
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

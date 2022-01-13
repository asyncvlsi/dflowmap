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

#ifndef DFLOWMAP__HELPER_H_
#define DFLOWMAP__HELPER_H_
#include <act/act.h>
#include <cstring>
#include "Constant.h"
#include "common.h"

bool isActnCp(const char *instance);

bool isActnDp(const char *instance);

void normalizeName(char *src, char toDel, char newChar);

const char *getNormInstanceName(const char *src);

const char *getNormActIdName(const char *src);

void printIntVec(IntVec &ULongVec);

void printULongVec(ULongVec &longVec);

int searchStringVec(StringVec &strVec, const char *str);

void genExprFromInt(unsigned long val, Expr *expr);

void genExprFromStr(const char *str, Expr *expr, int exprType);

Expr *getExprFromName(const char *name,
                      Map<const char *, Expr *> &exprMap,
                      bool exitOnMissing,
                      int exprType);

bool isBinType(int exprType);

void getActIdName(Scope *sc, ActId *actId, char *buff, int sz);

void getCurProc(const char *str, char *val);

void getActConnectionName(act_connection *actConnection, char *buff, int sz);

void checkACTN(const char *channel,bool &actnCp,bool &actnDp);

void print_dflow(FILE *fp, list_t *dflow);

template<class T>
bool hasInVector(Vector<T> &vector, T &elem) {
  return std::find(vector.begin(), vector.end(), elem) != vector.end();
}

#endif //DFLOWMAP__HELPER_H_

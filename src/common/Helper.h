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
#include <filesystem>
#include <fstream>
#include "Constant.h"
#include "common.h"

void normalizeName(char *src, char toDel, char newChar);

const char *getNormInstanceName(const char *src);

const char *getNormActIdName(const char *src);
char *_getNormActIdName(const char *src);

void printIntVec(IntVec &ULongVec);

void printULongVec(ULongVec &longVec);

int searchStringVec(StringVec &strVec, const char *str);

Expr *genExprFromInt(unsigned long val);

Expr *genExprFromIntName(const char *name);

Expr *genExprFromStr(const char *str, int exprType);

Expr *getExprFromName(const char *name,
                      Map<const char *, Expr *> &exprMap,
                      bool exitOnMissing,
                      int exprType);

bool isBinType(int exprType);

void getActIdName(Scope *sc, ActId *actId, char *buff, int sz);

void getCurProc(const char *str, char *val);

void getActConnectionName(act_connection *actConnection, char *buff, int sz);

void printActConnectionName (FILE *fp, act_connection *c);

void print_dflow(FILE *fp, list_t *dflow);

void removeDirectoryIfExist(const char *dir);

void createDirectoryIfNotExist(const char *dir);

void createFileIfNotExist(const char *file, std::ios_base::openmode mode);
void createMetricsFileIfNotExist(const char *file, std::ios_base::openmode mode);

void copyFileToTargetDir(const char *srcFile,
                         const char *targetDir,
                         const char *errMsg);

/*
 * Structs and namespaces in the CHP model, which is one flat file of
 * definitions with no namespaces and no struct types: text, ACT's printing of
 * process p (its header, its scope, or its whole definition), with p named
 * by its mangled name (namespace included, as the netlist names it), every
 * user process it instantiates likewise, and every channel carrying a struct
 * a channel of the int the struct packs into.
 */
std::string modelText(Process *p, const std::string &text);

/*
 * text, ACT's printing of p's scope, with a declaration for each process
 * instance and channel it leaves out -- those of the process p is derived
 * from (synth2's df_ <: the source's), or of a refine block's overrides
 * (refine +{ ... }) -- which the connections it prints use
 */
std::string modelInstances(Process *p, const std::string &text);

/* what print(fp) writes */
template<class F>
std::string printToString(F print) {
  char *buf = nullptr;
  size_t len = 0;
  FILE *mf = open_memstream(&buf, &len);
  print(mf);
  fclose(mf);
  std::string s(buf ? buf : "");
  free(buf);
  return s;
}

template<class T>
bool hasInVector(Vector<T> &vector, T &elem) {
  return std::find(vector.begin(), vector.end(), elem) != vector.end();
}

#endif //DFLOWMAP__HELPER_H_

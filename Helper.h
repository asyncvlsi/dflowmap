#ifndef DFLOWMAP__HELPER_H_
#define DFLOWMAP__HELPER_H_
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

template<class T>
bool hasInVector(Vector<T> &vector, T &elem) {
  return std::find(vector.begin(), vector.end(), elem) != vector.end();
}

#endif //DFLOWMAP__HELPER_H_

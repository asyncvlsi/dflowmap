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

Expr * genExprFromStr(const char *str,
                    int exprType) {
  Expr *expr = new Expr;
  auto newLActId = ActId::parseId(str);
  expr->type = exprType;
  expr->u.e.l = (Expr *) (newLActId);
  return expr;
}

Expr * genExprFromInt(unsigned long val) {
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
  exprMap.insert(GenPair(name, newExpr));
  return newExpr;
}

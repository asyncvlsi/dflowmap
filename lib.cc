#include <stdio.h>
#include <act/act.h>

#define MAX_OP_TYPE_NUM 10
#define MAX_EXPR_TYPE_NUM 100

int opTypes[MAX_OP_TYPE_NUM];
int exprTypes[MAX_EXPR_TYPE_NUM];

bool hasExpr(int typeId) {
  for (unsigned i = 0; i < MAX_EXPR_TYPE_NUM; i++) {
    if (exprTypes[i] == -1) {
      exprTypes[i] = typeId;
      return false;
    } else if (exprTypes[i] == typeId) {
      return true;
    }
  }
  fatal_error("We have reached the end of exprTypes array!\n");
}

bool hasOp(int typeId) {
  for (unsigned i = 0; i < MAX_OP_TYPE_NUM; i++) {
    if (opTypes[i] == -1) {
      opTypes[i] = typeId;
      return false;
    } else if (opTypes[i] == typeId) {
      return true;
    }
  }
  fatal_error("We have reached the end of opTypes array!\n");
}

void createBinLib(FILE *libFp, const char *sym, const char *op, int type) {
  if (!hasExpr(type)) {
    fprintf(libFp, "defproc func_%s(a, b, c) {\n", sym);
    fprintf(libFp, "  chp {\n    *[a?x, b?y; c!(x%sy) ]\n  }\n}\n\n", op);
  }
}

void createUniLib(FILE *libFp, const char *sym, const char *op, int type) {
  if (!hasExpr(type)) {
    fprintf(libFp, "defproc func_%s(a, b) {\n", sym);
    fprintf(libFp, "  chp {\n    *[a?x; b!(%sx) ]\n  }\n}\n\n", op);
  }
}

void createMerge(FILE *libFp) {
  if (!hasOp(ACT_DFLOW_MERGE)) {
    fprintf(libFp, "defproc control_merge(ctrl, lIn, rIn, out) {\n");
    fprintf(libFp, "  chp {\n");
    fprintf(libFp, "    [~ctrl -> out!lIn [] ctrl -> out!rIn]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createSplit(FILE *libFp) {
  if (!hasOp(ACT_DFLOW_SPLIT)) {
    fprintf(libFp, "defproc control_split(ctrl, in, lOut, rOut) {\n");
    fprintf(libFp, "  chp {\n");
    fprintf(libFp, "    [~ctrl -> lOut!in [] ctrl -> rOut!in]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createSource(FILE *libFp) {
  if (!hasExpr(E_INT)) {
    fprintf(libFp, "defproc source(in, out) {\n");
    fprintf(libFp, "  chp {\n  }\n}\n\n");
  }
}

void createBuff(FILE *libFp) {
  if (!hasExpr(E_VAR)) {
    fprintf(libFp, "defproc buff(in, out) {\n");
    fprintf(libFp, "  chp {\n    out!in\n  }\n}\n\n");
  }
}

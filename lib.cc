#include <stdio.h>
#include <act/act.h>

#define MAX_OP_TYPE_NUM 10
#define MAX_EXPR_TYPE_NUM 100
#define MAX_SOURCE_VAL_NUM 100

int opTypes[MAX_OP_TYPE_NUM];
int exprTypes[MAX_EXPR_TYPE_NUM];
unsigned sourceVals[MAX_SOURCE_VAL_NUM];

bool hasSourceVal(unsigned sourceVal) {
  for (unsigned i = 0; i < MAX_SOURCE_VAL_NUM; i++) {
    if (sourceVals[i] == UINT32_MAX) {
      sourceVals[i] = sourceVal;
      return false;
    } else if (sourceVals[i] == sourceVal) {
      return true;
    }
  }
  fatal_error("We have reached the end of sourceVals array!\n");
}

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

void createBinLib(FILE *libFp,
                  const char *sym,
                  const char *op,
                  int type) {
  if (!hasExpr(type)) {
    fprintf(libFp,
            "defproc func_%s(chan?(int) a, b; chan!(int) c) {\n",
            sym);
    fprintf(libFp, "  int x, y;\n");
    fprintf(libFp,
            "  chp {\n    *[a?x, b?y; c!(x%sy) ]\n  }\n}\n\n",
            op);
  }
}

void createUniLib(FILE *libFp,
                  const char *sym,
                  const char *op,
                  int type) {
  if (!hasExpr(type)) {
    fprintf(libFp,
            "defproc func_%s(chan?(int)a; chan!(int) b) {\n",
            sym);
    fprintf(libFp, "  int x;\n");
    fprintf(libFp, "  chp {\n    *[a?x; b!(%sx) ]\n  }\n}\n\n", op);
  }
}

void createMerge(FILE *libFp) {
  if (!hasOp(ACT_DFLOW_MERGE)) {
    fprintf(libFp,
            "defproc control_merge(chan?(bool)ctrl, chan?(int)lIn, rIn; chan!(int) out) {\n");
    fprintf(libFp, "  int x;\n");
    fprintf(libFp, "  chp {\n");
    fprintf(libFp, "    *[~ctrl -> lIn?x [] ctrl -> rIn?x; out!x]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createSplit(FILE *libFp) {
  if (!hasOp(ACT_DFLOW_SPLIT)) {
    fprintf(libFp,
            "defproc control_split(chan?(bool)ctrl, chan?(int)in; chan!(int) lOut, rOut) {\n");
    fprintf(libFp, "  int x;\n");
    fprintf(libFp, "  chp {\n");
    fprintf(libFp,
            "    *[in?x; ~ctrl -> lOut!x [] ctrl -> rOut!x]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createSource(FILE *libFp, unsigned sourceVal) {
  if (!hasSourceVal(sourceVal)) {
    fprintf(libFp, "defproc source%u(chan!(int)x) {\n", sourceVal);
    fprintf(libFp, "  chp {\n");
    fprintf(libFp, "    *[x!%u]\n", sourceVal);
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createBuff(FILE *libFp) {
  if (!hasExpr(E_VAR)) {
    fprintf(libFp,
            "defproc buffer(chan?(int)in; chan!(int) out) {\n");
    fprintf(libFp, "  int x;\n");
    fprintf(libFp, "  chp {\n    *[in?x;out!x]\n  }\n}\n\n");
  }
}

void createSink(FILE *libFp) {
  fprintf(libFp, "defproc sink(chan?(int) x) {\n");
  fprintf(libFp, "  int t;");
  fprintf(libFp, "  chp {\n");
  fprintf(libFp, "  *[x?t; log (\"got \", t)]\n");
  fprintf(libFp, "  }\n}\n");
}
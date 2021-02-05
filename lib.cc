#include <stdio.h>
#include <act/act.h>

#define MAX_OP_TYPE_NUM 10
#define MAX_EXPR_TYPE_NUM 100
#define MAX_SOURCE_VAL_NUM 100

int opTypes[MAX_OP_TYPE_NUM];
int exprTypes[MAX_EXPR_TYPE_NUM];
std::pair<unsigned, int> sourcePairs[MAX_SOURCE_VAL_NUM];

bool hasSourceVal(unsigned sourceVal, int bitwidth) {
  std::pair<unsigned, int> sourcePair = std::make_pair(sourceVal, bitwidth);
  for (unsigned i = 0; i < MAX_SOURCE_VAL_NUM; i++) {
    if (sourcePairs[i].first == UINT32_MAX) {
      sourcePairs[i] = sourcePair;
      return false;
    } else if (sourcePairs[i] == sourcePair) {
      return true;
    }
  }
  fatal_error("We have reached the end of sourcePairs array!\n");
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

void createBinLib(FILE *libFp, const char *sym, const char *op, int type,
                  int inWidth, int outWidth) {
  if (!hasExpr(type)) {
    fprintf(libFp,
            "defproc func_%s(chan?(int<%d>) a, b; chan!(int<%d>) c) {\n",
            sym, inWidth, outWidth);
    fprintf(libFp, "  int<%d> x, y;\n", inWidth);
    fprintf(libFp,
            "  chp {\n    *[a?x, b?y; c!(x%sy) ]\n  }\n}\n\n",
            op);
  }
}

void createUniLib(FILE *libFp, const char *sym, const char *op, int type, int bitwidth) {
  if (!hasExpr(type)) {
    fprintf(libFp,
            "defproc func_%s(chan?(int<%d>)a; chan!(int<%d>) b) {\n",
            sym, bitwidth, bitwidth);
    fprintf(libFp, "  int<%d> x;\n", bitwidth);
    fprintf(libFp, "  chp {\n    *[a?x; b!(%sx) ]\n  }\n}\n\n", op);
  }
}

void createMerge(FILE *libFp, int bitwidth) {
  if (!hasOp(ACT_DFLOW_MERGE)) {
    fprintf(libFp,
            "defproc control_merge(chan?(int<1>)ctrl; chan?(int<%d>)lIn, "
            "rIn; chan!(int<%d>) out) {\n", bitwidth, bitwidth);
    fprintf(libFp, "  int<%d> x;\n  bool c;\n", bitwidth);
    fprintf(libFp, "  chp {\n");
    fprintf(libFp,
            "    *[ctrl?c; [~c -> lIn?x [] c -> rIn?x]; out!x]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createSplit(FILE *libFp, int bitwidth) {
  if (!hasOp(ACT_DFLOW_SPLIT)) {
    fprintf(libFp,
            "defproc control_split(chan?(int<1>)ctrl; chan?(int<%d>)in; "
            "chan!(int<%d>) lOut, rOut) {\n", bitwidth, bitwidth);
    fprintf(libFp, "  int<%d> x;\n  bool c;\n", bitwidth);
    fprintf(libFp, "  chp {\n");
    fprintf(libFp,
            "    *[in?x; ctrl?c; [~c -> lOut!x [] c -> rOut!x]]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createSource(FILE *libFp, unsigned sourceVal, int bitwidth) {
  if (!hasSourceVal(sourceVal, bitwidth)) {
    fprintf(libFp, "defproc source%u_%d(chan!(int<%d>)x) {\n", sourceVal, bitwidth, bitwidth);
    fprintf(libFp, "  chp {\n");
    fprintf(libFp, "    *[x!%u]\n", sourceVal);
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createBuff(FILE *libFp, int bitwidth) {
  if (!hasExpr(E_VAR)) {
    fprintf(libFp,
            "defproc buffer(chan?(int<%d>)in; chan!(int<%d>) out) {\n", bitwidth, bitwidth);
    fprintf(libFp, "  int<%d> x;\n", bitwidth);
    fprintf(libFp, "  chp {\n    *[in?x;out!x]\n  }\n}\n\n");
  }
}

void createSink(FILE *libFp, int bitwidth) {
  fprintf(libFp, "defproc sink(chan?(int<%d>) x) {\n", bitwidth);
  fprintf(libFp, "  int<%d> t;", bitwidth);
  fprintf(libFp, "  chp {\n");
  fprintf(libFp, "  *[x?t; log (\"got \", t)]\n");
  fprintf(libFp, "  }\n}\n");
}
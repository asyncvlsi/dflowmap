#include <stdio.h>
#include <act/act.h>

#define MAX_OP_TYPE_NUM 10
#define MAX_EXPR_TYPE_NUM 100
#define MAX_SOURCE_VAL_NUM 100

//int opTypes[MAX_OP_TYPE_NUM];
//int exprTypes[MAX_EXPR_TYPE_NUM];

/* opType, bitwidth */
std::pair<int, int> opTypePairs[MAX_SOURCE_VAL_NUM];
/* exprType, bitwidth */
std::pair<int, int> exprTypePairs[MAX_SOURCE_VAL_NUM];
/* sourceVal, bitwidth */
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

bool hasExpr(int typeId, int bitwidth) {
  std::pair<int, int> exprPair = std::make_pair(typeId, bitwidth);
  for (unsigned i = 0; i < MAX_EXPR_TYPE_NUM; i++) {
    if (exprTypePairs[i].first == INT32_MAX) {
      exprTypePairs[i] = exprPair;
      return false;
    } else if (exprTypePairs[i] == exprPair) {
      return true;
    }
  }
  fatal_error("We have reached the end of exprTypePairs array!\n");
}

bool hasOp(int typeId, int bitwidth) {
  std::pair<int, int> opPair = std::make_pair(typeId, bitwidth);
  for (unsigned i = 0; i < MAX_OP_TYPE_NUM; i++) {
    if (opTypePairs[i].first == INT32_MAX) {
      opTypePairs[i] = opPair;
      return false;
    } else if (opTypePairs[i] == opPair) {
      return true;
    }
  }
  fatal_error("We have reached the end of opTypePairs array!\n");
}

void createBinLib(FILE *libFp, const char *sym, const char *op, int typeId,
                  int inWidth, int outWidth) {
  if (!hasExpr(typeId, inWidth)) {
    fprintf(libFp,
            "defproc func_%s_%d(chan?(int<%d>) a, b; chan!(int<%d>) c) {\n",
            sym, inWidth, inWidth, outWidth);
    fprintf(libFp, "  int<%d> x, y;\n", inWidth);
    fprintf(libFp,
            "  chp {\n    *[a?x, b?y; c!(x%sy) ]\n  }\n}\n\n",
            op);
  }
}

void createUniLib(FILE *libFp, const char *sym, const char *op, int typeId, int bitwidth) {
  if (!hasExpr(typeId, bitwidth)) {
    fprintf(libFp,
            "defproc func_%s_%d(chan?(int<%d>)a; chan!(int<%d>) b) {\n",
            sym, bitwidth, bitwidth, bitwidth);
    fprintf(libFp, "  int<%d> x;\n", bitwidth);
    fprintf(libFp, "  chp {\n    *[a?x; b!(%sx) ]\n  }\n}\n\n", op);
  }
}

void createMerge(FILE *libFp, int bitwidth) {
  if (!hasOp(ACT_DFLOW_MERGE, bitwidth)) {
    fprintf(libFp,
            "defproc control_merge%d(chan?(int<1>)ctrl; chan?(int<%d>)lIn, "
            "rIn; chan!(int<%d>) out) {\n", bitwidth, bitwidth, bitwidth);
    fprintf(libFp, "  int<%d> x;\n  bool c;\n", bitwidth);
    fprintf(libFp, "  chp {\n");
    fprintf(libFp,
            "    *[ctrl?c; [~c -> lIn?x [] c -> rIn?x]; out!x]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createSplit(FILE *libFp, int bitwidth) {
  if (!hasOp(ACT_DFLOW_SPLIT, bitwidth)) {
    fprintf(libFp,
            "defproc control_split%d(chan?(int<1>)ctrl; chan?(int<%d>)in; "
            "chan!(int<%d>) lOut, rOut) {\n", bitwidth, bitwidth, bitwidth);
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
  if (!hasExpr(E_VAR, bitwidth)) {
    fprintf(libFp,
            "defproc buffer%d(chan?(int<%d>)in; chan!(int<%d>) out) {\n", bitwidth, bitwidth,
            bitwidth);
    fprintf(libFp, "  int<%d> x;\n", bitwidth, bitwidth);
    fprintf(libFp, "  chp {\n    *[in?x;out!x]\n  }\n}\n\n");
  }
}

void createSink(FILE *libFp, int bitwidth) {
  fprintf(libFp, "defproc sink%d(chan?(int<%d>) x) {\n", bitwidth, bitwidth);
  fprintf(libFp, "  int<%d> t;", bitwidth);
  fprintf(libFp, "  chp {\n");
  fprintf(libFp, "  *[x?t; log (\"got \", t)]\n");
  fprintf(libFp, "  }\n}\n");
}
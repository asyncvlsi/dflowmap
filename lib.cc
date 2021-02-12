#include <stdio.h>
#include <string.h>
#include <act/act.h>

#define MAX_EXPR_TYPE_NUM 100
#define MAX_PROCESSES 500

/* array of created FU ids */
int fuIDs[MAX_EXPR_TYPE_NUM];
/* array of created processes */
const char *processes[MAX_PROCESSES];

bool hasProcess(const char *process) {
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    if (processes[i] == nullptr) {
      processes[i] = process;
      return false;
    } else if (!strcmp(processes[i], process)) {
      return true;
    }
  }
}

bool hasExpr(int typeId) {
  for (unsigned i = 0; i < MAX_EXPR_TYPE_NUM; i++) {
    if (fuIDs[i] == -1) {
      fuIDs[i] = typeId;
      return false;
    } else if (fuIDs[i] == typeId) {
      return true;
    }
  }
  fatal_error("We have reached the end of exprTypePairs array!\n");
}

void createBinLib(FILE *libFp, const char *sym, const char *op, int typeId) {
  if (!hasExpr(typeId)) {
    fprintf(libFp, "template<pint W1, W2>\n");
    fprintf(libFp, "defproc func_%s(chan?(int<W1>) a, b; chan!(int<W2>) c) {\n", sym);
    fprintf(libFp, "  int<W1> x, y;\n");
    fprintf(libFp, "  int<W2> res;\n");
    fprintf(libFp,
            "  chp {\n    *[a?x, b?y; log(\"recv \", x, \", \", y); res := x%sy; c!res; "
            "log(\"send \", res)]\n  }\n}\n\n", op);
  }
}

void createUniLib(FILE *libFp, const char *sym, const char *op, int typeId) {
  if (!hasExpr(typeId)) {
    fprintf(libFp, "template<pint W>\n");
    fprintf(libFp,
            "defproc func_%s(chan?(int<W>)a; chan!(int<W>) b) {\n", sym);
    fprintf(libFp, "  int<W> x;\n");
    fprintf(libFp, "  chp {\n    *[a?x; log(\"recv \", x); b!(%sx); log(\"send \", x) "
                   "]\n  }\n}\n\n", op);
  }
}

void createMerge(FILE *libFp) {
  if (!hasProcess("control_merge")) {
    fprintf(libFp, "template<pint W>\n");
    fprintf(libFp,
            "defproc control_merge(chan?(int<1>)ctrl; chan?(int<W>)lIn, "
            "rIn; chan!(int<W>) out) {\n");
    fprintf(libFp, "  int<W> x;\n  bool c;\n");
    fprintf(libFp, "  chp {\n");
    fprintf(libFp,
            "    *[ctrl?c; log(\"receive \", c); [~c -> lIn?x [] c -> rIn?x]; out!x; log(\"send \", x)]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createSplit(FILE *libFp) {
  if (!hasProcess("control_split")) {
    fprintf(libFp, "template<pint W>\n");
    fprintf(libFp,
            "defproc control_split(chan?(int<1>)ctrl; chan?(int<W>)in; "
            "chan!(int<W>) lOut, rOut) {\n");
    fprintf(libFp, "  int<W> x;\n  bool c;\n");
    fprintf(libFp, "  chp {\n");
    fprintf(libFp,
            "    *[in?x, ctrl?c; log(\"receive \", c, \", \", x);"
            "  [~c -> lOut!x [] c -> rOut!x]; log(\"send \", x)]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createSource(FILE *libFp) {
  if (!hasProcess("source")) {
    fprintf(libFp, "template<pint V, W>\n");
    fprintf(libFp, "defproc source(chan!(int<W>)x) {\n");
    fprintf(libFp, "  chp {\n");
    fprintf(libFp, "    *[log(\"send \", V); x!V]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
}

void createInit(FILE *libFp) {
  if (!hasProcess("init")) {
    fprintf(libFp, "template<pint V, W>\n");
    fprintf(libFp,
            "defproc init(chan?(int<W>)in; chan!(int<W>) out) {\n");
    fprintf(libFp, "  int<W> x;\n");
    fprintf(libFp, "  bool b;\n");
    fprintf(libFp, "  chp {\n    b-;\n");
    fprintf(libFp, "    *[[~b->x:=V;b+ [] b->in?x]; out!x; log(\"send \", x)]\n  }\n}\n\n");
  }
}

void createBuff(FILE *libFp) {
  if (!hasProcess("buffer")) {
    fprintf(libFp, "template<pint W>\n");
    fprintf(libFp,
            "defproc buffer(chan?(int<W>)in; chan!(int<W>) out) {\n");
    fprintf(libFp, "  int<W> x;\n");
    fprintf(libFp, "  chp {\n    *[in?x; out!x; log(\"send \", x)]\n  }\n}\n\n");
  }
}

void createSink(FILE *libFp) {
  if (!hasProcess("sink")) {
    fprintf(libFp, "template<pint W>\n");
    fprintf(libFp, "defproc sink(chan?(int<W>) x) {\n");
    fprintf(libFp, "  int<W> t;");
    fprintf(libFp, "  chp {\n");
    fprintf(libFp, "  *[x?t; log (\"got \", t)]\n");
    fprintf(libFp, "  }\n}\n");
  }
}

void createCopy(FILE *libFp) {
  if (!hasProcess("copy")) {
    fprintf(libFp, "template<pint W, N>\n");
    fprintf(libFp, "defproc copy(chan?(int<W>) in; chan!(int<W>) out[N]) {\n");
    fprintf(libFp, "  int<W> x;\n  chp {\n");
    fprintf(libFp, "  *[ in?x; (,i:N: out[i]!x); log(\"send \", x) ]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
}

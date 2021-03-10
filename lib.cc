#include <stdio.h>
#include <string.h>
#include <act/act.h>
#include "common.h"

#define MAX_EXPR_TYPE_NUM 100
#define MAX_PROCESSES 500

/* array of created FU ids */
int fuIDs[MAX_EXPR_TYPE_NUM];
/* array of created processes */
const char *processes[MAX_PROCESSES];
const char *instances[MAX_PROCESSES];

bool hasInstance(const char *instance) {
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    if (instances[i] == nullptr) {
      instances[i] = instance;
      return false;
    } else if (!strcmp(instances[i], instance)) {
      return true;
    }
  }
  return false;
}

bool hasProcess(const char *process) {
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    if (processes[i] == nullptr) {
      processes[i] = process;
      return false;
    } else if (!strcmp(processes[i], process)) {
      return true;
    }
  }
  return false;
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
  return false;
}

void createBinLib(const char *sym, const char *op, int typeId, const char* instance, int *metric) {
  if (!hasExpr(typeId)) {
    fprintf(libFp, "template<pint W1, W2>\n");
    fprintf(libFp, "defproc func_%s(chan?(int<W1>) a, b; chan!(int<W2>) c) {\n", sym);
    fprintf(libFp, "  int<W1> x, y;\n");
    fprintf(libFp, "  int<W2> res;\n");
    fprintf(libFp,
            "  chp {\n    *[a?x, b?y; log(\"recv \", x, \", \", y); res := x%sy; c!res; "
            "log(\"send \", res)]\n  }\n}\n\n", op);
  }
  if (!hasInstance(instance)) {
    fprintf(confFp, "begin %s\n", instance);
    fprintf(confFp, "  begin a\n");
    fprintf(confFp, "    int D 0\n");
    fprintf(confFp, "    int E 0\n");
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin b\n");
    fprintf(confFp, "    int D 0\n");
    fprintf(confFp, "    int E 0\n");
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin c\n");
    fprintf(confFp, "    int D %d\n", metric[2]);
    fprintf(confFp, "    int E %d\n", metric[1]);
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    fprintf(confFp, "end\n");
  }
}

void createUniLib(const char *sym, const char *op, int typeId, const char* instance, int *metric) {
  if (!hasExpr(typeId)) {
    fprintf(libFp, "template<pint W>\n");
    fprintf(libFp,
            "defproc func_%s(chan?(int<W>)a; chan!(int<W>) b) {\n", sym);
    fprintf(libFp, "  int<W> x;\n");
    fprintf(libFp, "  chp {\n    *[a?x; log(\"recv \", x); b!(%sx); log(\"send \", x) "
                   "]\n  }\n}\n\n", op);
  }
  if (!hasInstance(instance)) {
    fprintf(confFp, "begin %s\n", instance);
    fprintf(confFp, "  begin a\n");
    fprintf(confFp, "    int D 0\n");
    fprintf(confFp, "    int E 0\n");
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin b\n");
    fprintf(confFp, "    int D %d\n", metric[2]);
    fprintf(confFp, "    int E %d\n", metric[1]);
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    fprintf(confFp, "end\n");
  }
}

void createMerge(const char* instance, int *metric) {
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
  if (!hasInstance(instance)) {
    fprintf(confFp, "begin %s\n", instance);
    fprintf(confFp, "  begin ctrl\n");
    fprintf(confFp, "    int D 0\n");
    fprintf(confFp, "    int E 0\n");
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin lIn\n");
    fprintf(confFp, "    int D 0\n");
    fprintf(confFp, "    int E 0\n");
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin rIn\n");
    fprintf(confFp, "    int D 0\n");
    fprintf(confFp, "    int E 0\n");
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin out\n");
    fprintf(confFp, "    int D %d\n", metric[2]);
    fprintf(confFp, "    int E %d\n", metric[1]);
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    fprintf(confFp, "end\n");
  }
}

void createSplit(const char* instance, int *metric) {
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
  if (!hasInstance(instance)) {
    fprintf(confFp, "begin %s\n", instance);
    fprintf(confFp, "  begin ctrl\n");
    fprintf(confFp, "    int D 0\n");
    fprintf(confFp, "    int E 0\n");
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin in\n");
    fprintf(confFp, "    int D 0\n");
    fprintf(confFp, "    int E 0\n");
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin lOut\n");
    fprintf(confFp, "    int D %d\n", metric[2]);
    fprintf(confFp, "    int E %d\n", metric[1]);
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin rOut\n");
    fprintf(confFp, "    int D %d\n", metric[2]);
    fprintf(confFp, "    int E %d\n", metric[1]);
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    fprintf(confFp, "end\n");
  }
}

void createSource(const char* instance, int *metric) {
  if (!hasProcess("source")) {
    fprintf(libFp, "template<pint V, W>\n");
    fprintf(libFp, "defproc source(chan!(int<W>)x) {\n");
    fprintf(libFp, "  chp {\n");
    fprintf(libFp, "    *[log(\"send \", V); x!V]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
  if (!hasInstance(instance)) {
    fprintf(confFp, "begin %s\n", instance);
    fprintf(confFp, "  begin x\n");
    fprintf(confFp, "    int D %d\n", metric[2]);
    fprintf(confFp, "    int E %d\n", metric[1]);
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    fprintf(confFp, "end\n");
  }
}

void createInit(const char* instance, int *metric) {
  if (!hasProcess("init")) {
    fprintf(libFp, "template<pint V, W>\n");
    fprintf(libFp,
            "defproc init(chan?(int<W>)in; chan!(int<W>) out) {\n");
    fprintf(libFp, "  int<W> x;\n");
    fprintf(libFp, "  bool b;\n");
    fprintf(libFp, "  chp {\n    b-;\n");
    fprintf(libFp, "    *[[~b->x:=V;b+ [] b->in?x]; out!x; log(\"send \", x)]\n  }\n}\n\n");
  }
  if (!hasInstance(instance)) {
    fprintf(confFp, "begin %s\n", instance);
    fprintf(confFp, "  begin in\n");
    fprintf(confFp, "    int D 0\n");
    fprintf(confFp, "    int E 0\n");
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin out\n");
    fprintf(confFp, "    int D %d\n", metric[2]);
    fprintf(confFp, "    int E %d\n", metric[1]);
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    fprintf(confFp, "end\n");
  }
}

void createBuff(const char* instance, int *metric) {
  if (!hasProcess("buffer")) {
    fprintf(libFp, "template<pint W>\n");
    fprintf(libFp,
            "defproc buffer(chan?(int<W>)in; chan!(int<W>) out) {\n");
    fprintf(libFp, "  int<W> x;\n");
    fprintf(libFp, "  chp {\n    *[in?x; out!x; log(\"send \", x)]\n  }\n}\n\n");
  }
  if (!hasInstance(instance)) {
    fprintf(confFp, "begin %s\n", instance);
    fprintf(confFp, "  begin in\n");
    fprintf(confFp, "    int D 0\n");
    fprintf(confFp, "    int E 0\n");
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin out\n");
    fprintf(confFp, "    int D %d\n", metric[2]);
    fprintf(confFp, "    int E %d\n", metric[1]);
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    fprintf(confFp, "end\n");
  }
}

void createSink(const char* instance, int *metric) {
  if (!hasProcess("sink")) {
    fprintf(libFp, "template<pint W>\n");
    fprintf(libFp, "defproc sink(chan?(int<W>) x) {\n");
    fprintf(libFp, "  int<W> t;");
    fprintf(libFp, "  chp {\n");
    fprintf(libFp, "  *[x?t; log (\"got \", t)]\n");
    fprintf(libFp, "  }\n}\n");
  }
  if (!hasInstance(instance)) {
    fprintf(confFp, "begin %s\n", instance);
    fprintf(confFp, "  begin x\n");
    fprintf(confFp, "    int D %d\n", metric[2]);
    fprintf(confFp, "    int E %d\n", metric[1]);
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    fprintf(confFp, "end\n");
  }
}

void createCopy(int N, const char* instance, int *metric) {
  if (!hasProcess("copy")) {
    fprintf(libFp, "template<pint W, N>\n");
    fprintf(libFp, "defproc copy(chan?(int<W>) in; chan!(int<W>) out[N]) {\n");
    fprintf(libFp, "  int<W> x;\n  chp {\n");
    fprintf(libFp, "  *[ in?x; (,i:N: out[i]!x); log(\"send \", x) ]\n");
    fprintf(libFp, "  }\n}\n\n");
  }
  if (!hasInstance(instance)) {
    fprintf(confFp, "begin %s\n", instance);
    fprintf(confFp, "  begin in\n");
    fprintf(confFp, "    int D 0\n");
    fprintf(confFp, "    int E 0\n");
    fprintf(confFp, "  end\n");
    for (int i = 0; i < N; i++) {
      fprintf(confFp, "  begin out[%d]\n", i);
      fprintf(confFp, "    int D %d\n", metric[2]);
      fprintf(confFp, "    int E %d\n", metric[1]);
      fprintf(confFp, "  end\n");
    }
    fprintf(confFp, "  real leakage %de-9\n", metric[0] * N);
    fprintf(confFp, "end\n");
  }
}

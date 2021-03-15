#include <stdio.h>
#include <string.h>
#include <act/act.h>
#include "common.h"

#define MAX_EXPR_TYPE_NUM 100
#define MAX_PROCESSES 500

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

void createFULib(const char *procName, const char *calc, const char *def,
                 int numArgs, int result_suffix, int numRes, const char*instance, int* metric) {
  if (!hasProcess(procName)) {
    fprintf(libFp, "template<pint ");
    int i = 0;
    for (; i <= numArgs; i++) {
      fprintf(libFp, "W%d, ", i);
    }
    for (i = 0; i < numRes - 1; i++) {
      fprintf(libFp, "W%d, ", i + numArgs + 1);
    }
    fprintf(libFp, "W%d>\n", i + numArgs + 1);
    fprintf(libFp, "defproc %s(", procName);
    for (i = 0; i < numArgs; i++) {
      fprintf(libFp, "chan?(int<W%d>)arg%d; ", i, i);
    }
    fprintf(libFp, "chan!(int<W%d>) out) {\n", i);
    for (i = 0; i < numArgs; i++) {
      fprintf(libFp, "  int<W%d> x%d;\n", i, i);
    }
    for (i = 0; i < numRes; i++) {
      fprintf(libFp, "  int<W%d> res%d;\n", i + numArgs + 1, i);
    }
    fprintf(libFp, "%s", def);
    fprintf(libFp, "  chp {\n    *[");
    for (i = 0; i < numArgs - 1; i++) {
      fprintf(libFp, "arg%d?x%d, ", i, i);
    }
    fprintf(libFp, "arg%d?x%d; ", i, i);
    fprintf(libFp, "%s", calc);
    fprintf(libFp, "      out!res%d; log(\"send \", res%d)]\n  }\n}\n\n",
            result_suffix, result_suffix);
  }
}

void createFUInitLib(const char *procName, const char *calc, const char *def,
                 int numArgs, int result_suffix, int numRes, int initVal) {
  if (!hasProcess(procName)) {
    fprintf(libFp, "template<pint ");
    int i = 0;
    for (; i <= numArgs; i++) {
      fprintf(libFp, "W%d, ", i);
    }
    for (i = 0; i < numRes - 1; i++) {
      fprintf(libFp, "W%d, ", i + numArgs + 1);
    }
    fprintf(libFp, "W%d>\n", i + numArgs + 1);
    fprintf(libFp, "defproc %s(", procName);
    for (i = 0; i < numArgs; i++) {
      fprintf(libFp, "chan?(int<W%d>)arg%d; ", i, i);
    }
    fprintf(libFp, "chan!(int<W%d>) out) {\n", i);
    fprintf(libFp, "  bool b;\n");
    for (i = 0; i < numArgs; i++) {
      fprintf(libFp, "  int<W%d> x%d;\n", i, i);
    }
    for (i = 0; i < numRes; i++) {
      fprintf(libFp, "  int<W%d> res%d;\n", i + numArgs + 1, i);
    }
    fprintf(libFp, "%s", def);
    fprintf(libFp, "  chp {\n    b-;\n    *[");
    fprintf(libFp, "[~b->res%d:=%d;b+ [] b->", result_suffix, initVal);
    for (i = 0; i < numArgs - 1; i++) {
      fprintf(libFp, "arg%d?x%d, ", i, i);
    }
    fprintf(libFp, "arg%d?x%d; ", i, i);
    fprintf(libFp, "%s];\n", calc);
    fprintf(libFp, "      out!res%d; log(\"send \", res%d)]\n  }\n}\n\n",
            result_suffix, result_suffix);
  }
}

void createMerge(const char *instance, int *metric) {
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
    if (metric != nullptr) {
      fprintf(confFp, "    int D %d\n", metric[2]);
      fprintf(confFp, "    int E %d\n", metric[1]);
    }
    fprintf(confFp, "  end\n");
    if (metric != nullptr) {
      fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    }
    fprintf(confFp, "end\n");
  }
}

void createSplit(const char *instance, int *metric) {
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
    if (metric != nullptr) {
      fprintf(confFp, "    int D %d\n", metric[2]);
      fprintf(confFp, "    int E %d\n", metric[1]);
    }
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  begin rOut\n");
    if (metric != nullptr) {
      fprintf(confFp, "    int D %d\n", metric[2]);
      fprintf(confFp, "    int E %d\n", metric[1]);
    }
    fprintf(confFp, "  end\n");
    if (metric != nullptr) {
      fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    }
    fprintf(confFp, "end\n");
  }
}

void createSource(const char *instance, int *metric) {
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
    if (metric != nullptr) {
      fprintf(confFp, "    int D %d\n", metric[2]);
      fprintf(confFp, "    int E %d\n", metric[1]);
    }
    fprintf(confFp, "  end\n");
    if (metric != nullptr) {
      fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    }
    fprintf(confFp, "end\n");
  }
}

void createInit(const char *instance, int *metric) {
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
    if (metric != nullptr) {
      fprintf(confFp, "    int D %d\n", metric[2]);
      fprintf(confFp, "    int E %d\n", metric[1]);
    }
    fprintf(confFp, "  end\n");
    if (metric != nullptr) {
      fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    }
    fprintf(confFp, "end\n");
  }
}

void createBuff(const char *instance, int *metric) {
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
    if (metric != nullptr) {
      fprintf(confFp, "    int D %d\n", metric[2]);
      fprintf(confFp, "    int E %d\n", metric[1]);
    }
    fprintf(confFp, "  end\n");
    if (metric != nullptr) {
      fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    }
    fprintf(confFp, "end\n");
  }
}

void createSink(const char *instance, int *metric) {
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
    if (metric != nullptr) {
      fprintf(confFp, "    int D %d\n", metric[2]);
      fprintf(confFp, "    int E %d\n", metric[1]);
    }
    fprintf(confFp, "  end\n");
    if (metric != nullptr) {
      fprintf(confFp, "  real leakage %de-9\n", metric[0]);
    }
    fprintf(confFp, "end\n");
  }
}

void createCopy(int N, const char *instance, int *metric) {
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
    fprintf(confFp, "  begin out\n");
    if (metric != nullptr) {
      fprintf(confFp, "    int D %d\n", metric[2]);
      fprintf(confFp, "    int E %d\n", metric[1] * (N - 1));
    }
    fprintf(confFp, "  end\n");
    if (metric != nullptr) {
      fprintf(confFp, "  real leakage %de-9\n", metric[0] * (N - 1));
    }
    fprintf(confFp, "end\n");
  }
}

#include <stdio.h>

#define MAX_TYPE_NUM 100

int processedTypes[MAX_TYPE_NUM] = {-1};

bool hasType(int typeId) {
  for (unsigned i = 0; i < MAX_TYPE_NUM; i++) {
    if (processedTypes[i] == typeId) {
      return true;
    }
  }
  return false;
}

void createBinLib(FILE *libFp, const char *sym, const char *op, int type) {
  if (!hasType(type)) {
    fprintf(libFp, "defproc func_%s(a, b, c) {\n", sym);
    fprintf(libFp, "  chp {\n    *[a?x, b?y; c!(x%sy) ]\n  }\n}\n\n", op);
  }
}

void createUniLib(FILE *libFp, const char *sym, const char *op, int type) {
  if (!hasType(type)) {
    fprintf(libFp, "defproc func_%s(a, b) {\n", sym);
    fprintf(libFp, "  chp {\n    *[a?x; b!(%sx) ]\n  }\n}\n\n", op);
  }
}

void createMerge(FILE *libFp) {
  if (!hasType(ACT_DFLOW_MERGE)) {
    fprintf(libFp, "defproc control_merge(ctrl, lIn, rIn, out) {\n");
    fprintf(libFp, "  chp {\n  }\n}\n\n");
  }
}

void createSplit(FILE *libFp) {
  if (!hasType(ACT_DFLOW_SPLIT)) {
    fprintf(libFp, "defproc control_split(ctrl, in, lOut, rOut) {\n");
    fprintf(libFp, "  chp {\n  }\n}\n\n");
  }
}

void createSource(FILE *libFp) {
  if (!hasType(E_INT)) {
    fprintf(libFp, "defproc source(in, out) {\n");
    fprintf(libFp, "  chp {\n  }\n}\n\n");
  }
}

void createBuff(FILE *libFp) {
  if (!hasType(E_VAR)) {
    fprintf(libFp, "defproc buff(in, out) {\n");
    fprintf(libFp, "  chp {\n  }\n}\n\n");
  }
}

#include <stdio.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <act/act.h>

typedef std::string String;
typedef std::vector<String> StringVec;
StringVec createdProcs;

void createBinLib(FILE *libFp, const char *sym, const char *op) {
  String symbol(sym);
  if (std::find(createdProcs.begin(), createdProcs.end(), symbol)
      == createdProcs.end()) {
    createdProcs.push_back(symbol);
    fprintf(libFp, "defproc func_%s(a, b, c) {\n", sym);
    fprintf(libFp, "  chp {\n    *[a?x, b?y; c!(x%sy) ]\n  }\n}\n\n", op);
  }
}

void createUniLib(FILE *libFp, const char *sym, const char *op) {
  String symbol(sym);
  if (std::find(createdProcs.begin(), createdProcs.end(), symbol)
      == createdProcs.end()) {
    createdProcs.push_back(symbol);
    fprintf(libFp, "defproc func_%s(a, b) {\n", sym);
    fprintf(libFp, "  chp {\n    *[a?x; b!(%sx) ]\n  }\n}\n\n", op);
  }
}

void createMerge(FILE *libFp) {
  String symbol = "merge";
  if (std::find(createdProcs.begin(), createdProcs.end(), symbol)
      == createdProcs.end()) {
    createdProcs.push_back(symbol);
    fprintf(libFp, "defproc control_merge(ctrl, lIn, rIn, out) {\n");
    fprintf(libFp, "  chp {\n  }\n}\n\n");
  }
}

void createSplit(FILE *libFp) {
  String symbol = "split";
  if (std::find(createdProcs.begin(), createdProcs.end(), symbol)
      == createdProcs.end()) {
    createdProcs.push_back(symbol);
    fprintf(libFp, "defproc control_split(ctrl, in, lOut, rOut) {\n");
    fprintf(libFp, "  chp {\n  }\n}\n\n");
  }
}

void createSource(FILE *libFp) {
  String symbol = "source";
  if (std::find(createdProcs.begin(), createdProcs.end(), symbol)
      == createdProcs.end()) {
    createdProcs.push_back(symbol);
    fprintf(libFp, "defproc source(in, out) {\n");
    fprintf(libFp, "  chp {\n  }\n}\n\n");
  }
}

void createBuff(FILE *libFp) {
  String symbol = "buff";
  if (std::find(createdProcs.begin(), createdProcs.end(), symbol)
      == createdProcs.end()) {
    createdProcs.push_back(symbol);
    fprintf(libFp, "defproc buff(in, out) {\n");
    fprintf(libFp, "  chp {\n  }\n}\n\n");
  }
}

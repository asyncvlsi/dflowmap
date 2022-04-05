#include <cstdio>
#include <iostream>

FILE* outFp;

void createUnpipeSplit(int numOuts) {
  fprintf(outFp, "template<pint W1,W2>\n");
  fprintf(outFp, "defproc unpipe_split%d(chan?(int<W1>)ctrl; chan?(int<W2>)in; ",
         numOuts);
  int i = 0;
  for (i = 0; i < numOuts - 1; i++) {
    fprintf(outFp, "chan!(int<W2>) out%d; ", i);
  }
  fprintf(outFp, "chan!(int<W2>) out%d) {\n", i);
  fprintf(outFp, "  chp {\n");
  fprintf(outFp, "    *[[");
  for (i = 0; i < numOuts; i++) {
    fprintf(outFp, "ctrl=%d & #in -> out%d!in; in?,ctrl?\n", i, i);
    if (i < numOuts - 1) {
      fprintf(outFp, "      []");
    }
  }
  fprintf(outFp, "      ]\n    ]\n");
  fprintf(outFp, "  }\n}\n\n");
}

void createPipeSplit(int numOuts) {
  fprintf(outFp, "template<pint W1,W2>\n");
  fprintf(outFp, "defproc pipe_split%d(chan?(int<W1>)ctrl; chan?(int<W2>)in; ",
         numOuts);
  int i = 0;
  for (i = 0; i < numOuts - 1; i++) {
    fprintf(outFp, "chan!(int<W2>) out%d; ", i);
  }
  fprintf(outFp, "chan!(int<W2>) out%d) {\n", i);
  fprintf(outFp, "  int<W1> c;\n  int<W2> x;\n");
  fprintf(outFp, "  chp {\n");
  fprintf(outFp, R"(    *[in?x, ctrl?c; log("receive ", c, ", ", x);)");
  fprintf(outFp, "\n      [");
  for (i = 0; i < numOuts; i++) {
    fprintf(outFp, "c=%d -> out%d!x\n", i, i);
    if (i != (numOuts - 1)) {
      fprintf(outFp, "       [] ");
    }
  }
  fprintf(outFp, "      ];\n      log(\"send \", x)\n    ]\n");
  fprintf(outFp, "  }\n}\n\n");
}

void createUnpipeMerge(int N) {
  fprintf(outFp, "template<pint W1, W2>\n");
  fprintf(outFp, "defproc unpipe_merge%d(chan?(int<W1>)ctrl; ", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    fprintf(outFp, "chan?(int<W2>) in%d; ", i);
  }
  fprintf(outFp, "chan!(int<W2>) out) {\n");
  fprintf(outFp, "  chp {\n");
  fprintf(outFp, "    *[[");
  for (i = 0; i < N; i++) {
    fprintf(outFp, 
        "ctrl=%d & #in%d -> out!in%d; in%d?,ctrl?\n", i, i, i, i);
    if (i < N - 1) {
      fprintf(outFp, "      []");
    }
  }
  fprintf(outFp, "      ]\n    ]\n");

  fprintf(outFp, "  }\n}\n\n");
}

void createPipeMerge(int N) {
  fprintf(outFp, "template<pint W1, W2>\n");
  fprintf(outFp, "defproc pipe_merge%d(chan?(int<W1>)ctrl; ", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    fprintf(outFp, "chan?(int<W2>) in%d; ", i);
  }
  fprintf(outFp, "chan!(int<W2>) out) {\n");
  fprintf(outFp, "  int<W1> c;\n  int<W2> x;\n");
  fprintf(outFp, "  chp {\n");
  fprintf(outFp, "    *[ctrl?c; log(\"receive \", c);\n      [");
  for (i = 0; i < N; i++) {
    fprintf(outFp, "c=%d -> in%d?x\n", i, i);
    if (i != (N - 1)) {
      fprintf(outFp, "       [] ");
    }
  }
  fprintf(outFp, "      ];\n      log(\"receive x: \", x);\n");
  fprintf(outFp, "      out!x; log(\"send \", x)\n    ]\n");
  fprintf(outFp, "  }\n}\n\n");
}

void createUnpipeArbiter(int N) {
  fprintf(outFp, "template<pint W1, W2>\n");
  fprintf(outFp, "defproc unpipe_arbiter%d(", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    fprintf(outFp, "chan?(int<W1>) in%d; ", i);
  }
  fprintf(outFp, "chan!(int<W1>) out; chan!(int<W2>) cout) {\n");
  fprintf(outFp, "  chp {\n");
  fprintf(outFp, "    *[ [| ");
  for (i = 0; i < N; i++) {
    fprintf(outFp, "#in%d -> out!in%d,cout!%d; in%d?\n", i, i, i, i);
    if (i != (N - 1)) {
      fprintf(outFp, "       [] ");
    }
  }
  fprintf(outFp, "       |]\n    ]\n");
  fprintf(outFp, "  }\n}\n\n");
}

void createPipeArbiter(int N) {
  fprintf(outFp, "template<pint W1, W2>\n");
  fprintf(outFp, "defproc pipe_arbiter%d(", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    fprintf(outFp, "chan?(int<W1>) in%d; ", i);
  }
  fprintf(outFp, "chan!(int<W1>) out; chan!(int<W2>) cout) {\n");
  fprintf(outFp, "  int<W2> x;\n");
  fprintf(outFp, "  chp {\n");
  fprintf(outFp, "    *[ [| ");
  for (i = 0; i < N; i++) {
    fprintf(outFp, "#in%d -> in%d?x; out!x,cout!%d; in%d?\n", i, i, i, i);
    if (i != (N - 1)) {
      fprintf(outFp, "       [] ");
    }
  }
  fprintf(outFp, "       |];\n       log(\"send \", x)\n    ]\n");
  fprintf(outFp, "  }\n}\n\n");
}

void createUnpipeMixer(int N) {
  fprintf(outFp, "template<pint W1, W2>\n");
  fprintf(outFp, "defproc unpipe_mixer%d(", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    fprintf(outFp, "chan?(int<W1>) in%d; ", i);
  }
  fprintf(outFp, "chan!(int<W1>) out; chan!(int<W2>) cout) {\n");
  fprintf(outFp, "  chp {\n");
  fprintf(outFp, "    *[ [");
  for (i = 0; i < N; i++) {
    fprintf(outFp, "#in%d -> out!in%d,cout!%d; in%d?\n", i, i, i, i);
    if (i != (N - 1)) {
      fprintf(outFp, "       [] ");
    }
  }
  fprintf(outFp, "       ]\n    ]\n");
  fprintf(outFp, "  }\n}\n\n");
}

void createPipeMixer(int N) {
  fprintf(outFp, "template<pint W1, W2>\n");
  fprintf(outFp, "defproc pipe_mixer%d(", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    fprintf(outFp, "chan?(int<W1>) in%d; ", i);
  }
  fprintf(outFp, "chan!(int<W1>) out; chan!(int<W2>) cout) {\n");
  fprintf(outFp, "  int<W2> x;\n");
  fprintf(outFp, "  chp {\n");
  fprintf(outFp, "    *[ [");
  for (i = 0; i < N; i++) {
    fprintf(outFp, "#in%d -> in%d?x; out!x,cout!%d; in%d?\n", i, i, i, i);
    if (i != (N - 1)) {
      fprintf(outFp, "       [] ");
    }
  }
  fprintf(outFp, "       ];\n       log(\"send \", x)\n    ]\n");
  fprintf(outFp, "  }\n}\n\n");
}

int main() {
  outFp = fopen("dflow_stdlib", "w");
  if (!outFp) {
    fprintf(outFp, "Cannot open dflow_stdlib!\n");
    exit(-1);
  }
  for (int N = 2; N < 20; N++) {
    createUnpipeMixer(N);
    createPipeMixer(N);
    createUnpipeArbiter(N);
    createPipeArbiter(N);
    createUnpipeMerge(N);
    createPipeMerge(N);
    createUnpipeSplit(N);
    createPipeSplit(N);
  }

  return 0;
}
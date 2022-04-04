#include <stdio.h>
#include <string.h>

void createUnpipeSplit(int numOuts) {
  printf("template<pint W1,W2>\n");
  printf("defproc unpipe_split%d(chan?(int<W1>)ctrl; chan?(int<W2>)in; ",
         numOuts);
  int i = 0;
  for (i = 0; i < numOuts - 1; i++) {
    printf("chan!(int<W2>) out%d; ", i);
  }
  printf("chan!(int<W2>) out%d) {\n", i);
  printf("  chp {\n");
  printf("    *[[");
  for (i = 0; i < numOuts; i++) {
    printf("ctrl=%d & #in -> out%d!in; in?,ctrl?\n", i, i);
    if (i < numOuts - 1) {
      printf("      []");
    }
  }
  printf("      ]\n    ]\n");
  printf("  }\n}\n\n");
}

void createPipeSplit(int numOuts) {
  printf("template<pint W1,W2>\n");
  printf("defproc pipe_split%d(chan?(int<W1>)ctrl; chan?(int<W2>)in; ",
         numOuts);
  int i = 0;
  for (i = 0; i < numOuts - 1; i++) {
    printf("chan!(int<W2>) out%d; ", i);
  }
  printf("chan!(int<W2>) out%d) {\n", i);
  printf("  int<W1> c;\n  int<W2> x;\n");
  printf("  chp {\n");
  printf(R"(    *[in?x, ctrl?c; log("receive ", c, ", ", x);)");
  printf("\n      [");
  for (i = 0; i < numOuts; i++) {
    printf("c=%d -> out%d!x\n", i, i);
    if (i != (numOuts - 1)) {
      printf("       [] ");
    }
  }
  printf("      ];\n      log(\"send \", x)\n    ]\n");
  printf("  }\n}\n\n");
}

void createUnpipeMerge(int N) {
  printf("template<pint W1, W2>\n");
  printf("defproc unpipe_merge%d(chan?(int<W1>)ctrl; ", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    printf("chan?(int<W2>) in%d; ", i);
  }
  printf("chan!(int<W2>) out) {\n");
  printf("  chp {\n");
  printf("    *[[");
  for (i = 0; i < N; i++) {
    printf(
        "ctrl=%d & #in%d -> out!in%d; in%d?,ctrl?\n", i, i, i, i);
    if (i < N - 1) {
      printf("      []");
    }
  }
  printf("      ]\n    ]\n");

  printf("  }\n}\n\n");
}

void createPipeMerge(int N) {
  printf("template<pint W1, W2>\n");
  printf("defproc pipe_merge%d(chan?(int<W1>)ctrl; ", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    printf("chan?(int<W2>) in%d; ", i);
  }
  printf("chan!(int<W2>) out) {\n");
  printf("  int<W1> c;\n  int<W2> x;\n");
  printf("  chp {\n");
  printf("    *[ctrl?c; log(\"receive \", c);\n      [");
  for (i = 0; i < N; i++) {
    printf("c=%d -> in%d?x\n", i, i);
    if (i != (N - 1)) {
      printf("       [] ");
    }
  }
  printf("      ];\n      log(\"receive x: \", x);\n");
  printf("      out!x; log(\"send \", x)\n    ]\n");
  printf("  }\n}\n\n");
}

void createUnpipeArbiter(int N) {
  printf("template<pint W1, W2>\n");
  printf("defproc unpipe_arbiter%d(", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    printf("chan?(int<W1>) in%d; ", i);
  }
  printf("chan!(int<W1>) out; chan!(int<W2>) cout) {\n");
  printf("  chp {\n");
  printf("    *[ [| ");
  for (i = 0; i < N; i++) {
    printf("#in%d -> out!in%d,cout!%d; in%d?\n", i, i, i, i);
    if (i != (N - 1)) {
      printf("       [] ");
    }
  }
  printf("       |]\n    ]\n");
  printf("  }\n}\n\n");
}

void createPipeArbiter(int N) {
  printf("template<pint W1, W2>\n");
  printf("defproc pipe_arbiter%d(", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    printf("chan?(int<W1>) in%d; ", i);
  }
  printf("chan!(int<W1>) out; chan!(int<W2>) cout) {\n");
  printf("  int<W2> x;\n");
  printf("  chp {\n");
  printf("    *[ [| ");
  for (i = 0; i < N; i++) {
    printf("#in%d -> in%d?x; out!x,cout!%d; in%d?\n", i, i, i, i);
    if (i != (N - 1)) {
      printf("       [] ");
    }
  }
  printf("       |];\n       log(\"send \", x)\n    ]\n");
  printf("  }\n}\n\n");
}

void createUnpipeMixer(int N) {
  printf("template<pint W1, W2>\n");
  printf("defproc unpipe_mixer%d(", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    printf("chan?(int<W1>) in%d; ", i);
  }
  printf("chan!(int<W1>) out; chan!(int<W2>) cout) {\n");
  printf("  chp {\n");
  printf("    *[ [");
  for (i = 0; i < N; i++) {
    printf("#in%d -> out!in%d,cout!%d; in%d?\n", i, i, i, i);
    if (i != (N - 1)) {
      printf("       [] ");
    }
  }
  printf("       ]\n    ]\n");
  printf("  }\n}\n\n");
}

void createPipeMixer(int N) {
  printf("template<pint W1, W2>\n");
  printf("defproc pipe_mixer%d(", N);
  int i = 0;
  for (i = 0; i < N; i++) {
    printf("chan?(int<W1>) in%d; ", i);
  }
  printf("chan!(int<W1>) out; chan!(int<W2>) cout) {\n");
  printf("  int<W2> x;\n");
  printf("  chp {\n");
  printf("    *[ [");
  for (i = 0; i < N; i++) {
    printf("#in%d -> in%d?x; out!x,cout!%d; in%d?\n", i, i, i, i);
    if (i != (N - 1)) {
      printf("       [] ");
    }
  }
  printf("       ];\n       log(\"send \", x)\n    ]\n");
  printf("  }\n}\n\n");
}

int main() {
  int N = 3;
  createUnpipeMixer(N);
  createPipeMixer(N);
//  createUnpipeArbiter(N);
//  createPipeArbiter(N);
//  createUnpipeMerge(N);
//  createPipeMerge(N);
//  createUnpipeSplit(N);
//  createPipeSplit(N);

  return 0;
}
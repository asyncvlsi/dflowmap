#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <act/act.h>
#include <act/passes.h>
#include <iostream>
#include <act/lang.h>
#include <act/types.h>
#include <algorithm>
#include "src/core/dflowmap_main.cc"
#define GTEST_COUT std::cerr << "[          ] [ INFO ]"
int debug_verbose = 0;
TEST(IfTest, Rui) {
  char **argv = new char *[2];
  argv[0] = new char[1024];
  argv[1] = new char[1024];
  sprintf(argv[0], "build/src/dflowmap");
  sprintf(argv[1], "if.act");
  int argc = 2;
  Act::Init(&argc, &argv);
  char *metric_file = new char[1024];
  sprintf(metric_file, "../../../metrics/fluid.metrics");
  char *act_file = new char[1024];
  sprintf(act_file, "test/basic/if/if.act");
  add(act_file);
//  Act *a = new Act(act_file);
//  a->Expand();
//  a->mangle(nullptr);
//  FILE *resFp, *libFp, *confFp;
//  int res = 0;
//  res = dflowmap_main(a, metric_file, act_file, resFp, libFp, confFp);
//  EXPECT_EQ(res, 0);
}

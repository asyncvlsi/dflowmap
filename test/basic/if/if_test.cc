#include <gtest/gtest.h>
#include "src/main.cc"

// Demonstrate some basic assertions.
TEST(IfTest, BasicAssertions) {
  char** argv = new char*[2];
  argv[0] = new char[1024];
  argv[1] = new char[1024];
  sprintf(argv[0], "build/src/dflowmap");
  sprintf(argv[1], "if.act");
  int argc = 2;
  main(argc, argv);
  EXPECT_STRNE("hello", "world");
  EXPECT_EQ(7 * 6, 42);
}

/*************************************************************************
 *
 *  This file is part of the ACT library
 *
 *  Copyright (c) 2020 Rajit Manohar
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *
 **************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <act/act.h>
#include <act/passes.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <act/lang.h>
#include <act/types.h>
#include <act/expr.h>
#include <algorithm>
#include "common.h"
#include "ChpGenerator.h"
#include "Metrics.h"

static void usage(char *name) {
  fprintf(stderr, "Usage: %s <actfile>\n", name);
  exit(1);
}

void printCustomNamespace(ActNamespace *ns, FILE *resFp, FILE *libFp) {
  const char *nsName = ns->getName();
  fprintf(resFp, "namespace %s {\n", nsName);
  fprintf(libFp, "namespace %s {\n", nsName);
  ActTypeiter it(ns);
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
//    p->Print(libFp);
    if (p->isExpanded()) {
      p->PrintHeader(resFp, "defproc");
      fprintf(resFp, ";\n");
      p->Print(libFp);
    }
  }
  fprintf(resFp, "}\n\n");
  fprintf(libFp, "}\n\n");
}

int main(int argc, char **argv) {
  /* initialize ACT library */
  Act::Init(&argc, &argv);

  /* some usage check */
  if (argc != 2) {
    usage(argv[0]);
  }

  /* read in the ACT file */
  Act *a = new Act(argv[1]);
  a->Expand();
  a->mangle(NULL);
  fprintf(stdout, "Processing ACT file %s!\n", argv[1]);
  if (DEBUG_VERBOSE) {
    printf("------------------ACT FILE--------------------\n");
    a->Print(stdout);
    printf("\n\n\n");
  }
  /* create output file */
  char *result_file = new char[8 + strlen(argv[1])];
  strcpy(result_file, "result_");
  strcat(result_file, argv[1]);
  FILE *resFp = fopen(result_file, "w");

  char *lib_file = new char[5 + strlen(argv[1])];
  strcpy(lib_file, "lib_");
  strcat(lib_file, argv[1]);
  FILE *libFp = fopen(lib_file, "w");
  fprintf(resFp, "import \"%s\";\n\n", lib_file);

  char *conf_file = new char[6 + strlen(argv[1])];
  strcpy(conf_file, "conf_");
  strcat(conf_file, argv[1]);
  FILE *confFp = fopen(conf_file, "w");
  fprintf(confFp, "begin sim.chp\n");

  /* read in the Metric file */
  char *metricFilePath = new char[1000];
  sprintf(metricFilePath, "metrics/fluid.metrics");
  auto metrics = new Metrics(metricFilePath);
  metrics->readMetricsFile();

  /* declare custom namespace */
  ActNamespaceiter i(a->Global());
  for (i = i.begin(); i != i.end(); i++) {
    ActNamespace *n = *i;
    if (!n->isExported()) {
      printCustomNamespace(n, resFp, libFp);
    }
  }

  /* declare all of the act processes */
  ActTypeiter it(a->Global());
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      p->PrintHeader(resFp, "defproc");
      fprintf(resFp, ";\n");
    }
  }

  /* generate chp implementation for each act process */
  auto chpGenerator = new ChpGenerator(a, "ChpGenerator", metrics);
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      chpGenerator->handleProcess(resFp, libFp, confFp, p);
    }
  }
  fprintf(resFp, "main m;\n");
  fprintf(confFp, "end\n");
  fclose(resFp);
  fclose(confFp);
  if (DEBUG_VERBOSE) {
    metrics->dump();
  }

  return 0;
}


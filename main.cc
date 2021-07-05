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
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <act/act.h>
#include <act/passes.h>
#include <iostream>
#include <act/lang.h>
#include <act/types.h>
#include <algorithm>
#include "ChpGenerator.h"
#include "Metrics.h"

int quiet_mode;
int debug_verbose;

static void usage(char *name) {
  fprintf(stderr, "Usage: %s [-qv] [-m <metrics>] <actfile>\n", name);
  fprintf (stderr, " -m <metrics> : provide file name for energy/delay/area metrics\n");
  fprintf (stderr, " -q : quiet mode\n");
  fprintf (stderr, " -v : increase verbosity (default 1)\n");
  exit(1);
}

void printCustomNamespace(ChpGenerator *chpGenerator, ActNamespace *ns,
                          FILE *resFp, FILE *libFp, FILE *confFp) {
  const char *nsName = ns->getName();
  fprintf(resFp, "namespace %s {\n", nsName);
  fprintf(libFp, "namespace %s {\n", nsName);
  ActTypeiter it(ns);
  bool isMEM = strcmp(nsName, "mem") == 0;
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
//    p->Print(libFp);
    if (p->isExpanded()) {
      p->PrintHeader(resFp, "defproc");
      fprintf(resFp, ";\n");
      p->Print(libFp);
      if (isMEM) {
        const char* memProcName = p->getName();
        chpGenerator->genMemConfiguration(confFp, memProcName);
        if (debug_verbose) {
          unsigned len = strlen(memProcName);
          char *memName = new char[len - 1];
          strncpy(memName, memProcName, len - 2);
          memName[len - 2] = '\0';
          printf("memName: %s\n", memName);
          printf("[mem]: mem_%s_inst\n", memName);
        }
      }
    }
  }
  fprintf(resFp, "}\n\n");
  fprintf(libFp, "}\n\n");
}

int main(int argc, char **argv) {
  int ch;
  char *mfile = nullptr;
  /* initialize ACT library */
  Act::Init(&argc, &argv);

  debug_verbose = 1;
  quiet_mode = 0;

  while ((ch = getopt (argc, argv, "vqm:")) != -1) {
    switch (ch) {
    case 'v':
      debug_verbose++;
      break;
      
    case 'q':
      quiet_mode = 1;
      debug_verbose = 0;
      break;
      
    case 'm':
      if (mfile) {
	FREE (mfile);
      }
      mfile = Strdup (optarg);
      break;
      
    case '?':
    default:
      usage (argv[0]);
      break;
    }
  }
      
  if (optind != argc - 1) {
    usage(argv[0]);
  }

  char *act_file = argv[optind];
  
  /* read in the ACT file */
  Act *a = new Act(act_file);
  a->Expand();
  a->mangle(nullptr);
  fprintf(stdout, "Processing ACT file %s!\n", act_file);
  if (debug_verbose) {
    printf("------------------ACT FILE--------------------\n");
    a->Print(stdout);
    printf("\n\n\n");
  }
  /* create output file */
  char *result_file = new char[8 + strlen(act_file)];
  strcpy(result_file, "result_");
  strcat(result_file, act_file);
  FILE *resFp = fopen(result_file, "w");

  char *lib_file = new char[5 + strlen(act_file)];
  strcpy(lib_file, "lib_");
  strcat(lib_file, act_file);
  FILE *libFp = fopen(lib_file, "w");
  fprintf(resFp, "import \"%s\";\n\n", lib_file);

  char *conf_file = new char[6 + strlen(act_file)];
  strcpy(conf_file, "conf_");
  strcat(conf_file, act_file);
  FILE *confFp = fopen(conf_file, "w");
  fprintf(confFp, "begin sim.chp\n");

  /* read in the Metric file */
  char *metricFilePath = new char[1000];
  if (mfile) {
    snprintf(metricFilePath, 1000, "%s", mfile);
  } else {
    sprintf(metricFilePath, "metrics/fluid.metrics");
  }
  char *statisticsFilePath = new char[1000];
  sprintf(statisticsFilePath, "statistics");
  auto metrics = new Metrics(metricFilePath, statisticsFilePath);
  metrics->readMetricsFile();

  auto chpGenerator = new ChpGenerator(a, "ChpGenerator", metrics);
  /* declare custom namespace */
  ActNamespaceiter i(a->Global());
  for (i = i.begin(); i != i.end(); i++) {
    ActNamespace *n = *i;
    if (!n->isExported()) {
      printCustomNamespace(chpGenerator, n, resFp, libFp, confFp);
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
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      chpGenerator->handleProcess(resFp, libFp, confFp, p);
    }
  }
  fprintf(resFp, "main_test test;\n");
  fprintf(confFp, "end\n");
  fclose(resFp);
  fclose(confFp);
  metrics->dump();

  return 0;
}

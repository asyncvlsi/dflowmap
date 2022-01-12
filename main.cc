/*
 * This file is part of the ACT library
 *
 * Copyright (c) 2021 Rui Li
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
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
#include "ProcGenerator.h"
#include "Metrics.h"

int debug_verbose;

static void usage(char *name) {
  fprintf(stderr, "Usage: %s [-qv] [-m <metrics>] <actfile>\n", name);
  fprintf(stderr,
          " -m <metrics> : provide file name for energy/delay/area metrics\n");
  fprintf(stderr, " -v : increase verbosity (default 1)\n");
  exit(1);
}

void printCustomNamespace(ChpLibGenerator *libGenerator, ActNamespace *ns,
                          FILE *resFp, FILE *libFp, FILE *confFp) {
  const char *nsName = ns->getName();
  fprintf(resFp, "namespace %s {\n", nsName);
  fprintf(libFp, "namespace %s {\n", nsName);
  ActTypeiter it(ns);
  bool isMEM = strcmp(nsName, "mem") == 0;
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      p->PrintHeader(resFp, "defproc");
      fprintf(resFp, ";\n");
      p->Print(libFp);
      if (isMEM) {
        const char *memProcName = p->getName();
        libGenerator->genMemConfiguration(memProcName);
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

static void create_outfiles(const char *src,
                            FILE **resfp, FILE **libfp, FILE **conffp) {
  int i;
  i = strlen(src);
  while (i > 0) {
    if (src[i - 1] == '/') {
      break;
    }
    i--;
  }

  int len = strlen(src);
  char *tmpbuf = new char[8 + len];
  for (int j = 0; j < i; j++) {
    tmpbuf[j] = src[j];
  }
  snprintf(tmpbuf + i, 8 + len - i, "result_%s", src + i);
  *resfp = fopen(tmpbuf, "w");
  if (!*resfp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }

  snprintf(tmpbuf + i, 8 + len - i, "lib_%s", src + i);
  *libfp = fopen(tmpbuf, "w");
  if (!*libfp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }
  fprintf(*resfp, "import \"%s\";\n\n", tmpbuf);

  snprintf(tmpbuf + i, 8 + len - i, "conf_%s", src + i);
  *conffp = fopen(tmpbuf, "w");
  if (!*conffp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }
}

int main(int argc, char **argv) {
  int ch;
  char *mfile = nullptr;
  /* initialize ACT library */
  Act::Init(&argc, &argv);

  debug_verbose = 0;

  while ((ch = getopt(argc, argv, "vqm:")) != -1) {
    switch (ch) {
      case 'v':debug_verbose++;
        break;

      case 'm':
        if (mfile) {
          FREE (mfile);
        }
        mfile = Strdup(optarg);
        break;

      case '?':
      default:usage(argv[0]);
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

  if (debug_verbose) {
    fprintf(stdout, "Processing ACT file %s!\n", act_file);
    printf("------------------ACT FILE--------------------\n");
    a->Print(stdout);
    printf("\n\n\n");
  }

  /* open files */
  FILE *resFp, *libFp, *confFp;
  create_outfiles(act_file, &resFp, &libFp, &confFp);
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
  auto circuitGenerator = new ChpCircuitGenerator(resFp);
  auto libGenerator = new ChpLibGenerator(libFp, confFp);
  auto backend = new ChpBackend(circuitGenerator, libGenerator);
  auto procGenerator = new ProcGenerator(metrics, backend);
  /* declare custom namespace */
  ActNamespaceiter i(a->Global());
  for (i = i.begin(); i != i.end(); i++) {
    ActNamespace *n = *i;
    if (!n->isExported()) {
      printCustomNamespace(libGenerator, n, resFp, libFp, confFp);
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
  Map<const char *, unsigned> procCount;
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      procGenerator->handleProcess(p);
    }
  }
  fprintf(resFp, "main_test test;\n");
  fprintf(confFp, "end\n");
  fclose(resFp);
  fclose(confFp);
  metrics->dump();

  return 0;
}

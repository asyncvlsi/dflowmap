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
#include "src/core/ProcGenerator.h"
#include "src/core/Metrics.h"

static void create_outfiles(const char *src, char *&statsFilePath,
                            FILE **resfp, FILE **libfp, FILE **conffp) {
  /* "src" contains the path to the act file, which is in the form of
   * path1/path2/.../circuit.act. We need to extract the baseSrcName, which is
   * "circuit", as well as taking care of the file path.*/
  unsigned i = strlen(src);
  while (i > 0) {
    if (src[i - 1] == '/') {
      break;
    }
    i--;
  }
  unsigned len = strlen(src);
  char *basesrcName = new char[len - i - 3];
  snprintf(basesrcName, len - i - 3, "%s", src + i);
  char *tmpbuf = new char[8 + len];
  for (unsigned j = 0; j < i; j++) {
    tmpbuf[j] = src[j];
  }
  statsFilePath = new char[8 + len];
  sprintf(statsFilePath, "%s.stat", basesrcName);
  sprintf(tmpbuf + i, "%s_circuit.act", basesrcName);
  *resfp = fopen(tmpbuf, "w");
  if (!*resfp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }
  sprintf(tmpbuf + i, "%s_lib.act", basesrcName);
  *libfp = fopen(tmpbuf, "w");
  if (!*libfp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }
  fprintf(*resfp, "import \"%s\";\n\n", tmpbuf);
  sprintf(tmpbuf + i, "%s.conf", basesrcName);
  *conffp = fopen(tmpbuf, "w");
  if (!*conffp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }
}

Metrics *createMetrics(const char *metricFile, const char *statsFilePath) {
  char *metricFilePath = new char[1000];
  if (metricFile) {
    if (strlen(metricFile) > 1000) {
      printf("The metric file path %s is too long!\n", metricFile);
      exit(-1);
    }
    snprintf(metricFilePath, 1000, "%s", metricFile);
  } else {
    sprintf(metricFilePath, "metrics/fluid.metrics");
  }
  auto metrics = new Metrics(metricFilePath, statsFilePath);
  metrics->readMetricsFile();
  return metrics;
}

int add(const char* act_file) {
  Act *a = new Act(act_file);
  return 0;
}

int dflowmap_main(Act *a,
                  const char *metric_file,
                  const char *act_file,
                  FILE *&resFp,
                  FILE *&libFp,
                  FILE *&confFp) {
  char *statsFilePath = nullptr;
  create_outfiles(act_file, statsFilePath, &resFp, &libFp, &confFp);
  Metrics *metrics = createMetrics(metric_file, statsFilePath);
  auto circuitGenerator = new ChpCircuitGenerator(resFp);
  auto libGenerator = new ChpLibGenerator(libFp, confFp);
  auto backend = new ChpBackend(circuitGenerator, libGenerator);
  /* declare custom namespace */
  ActNamespaceiter i(a->Global());
  for (i = i.begin(); i != i.end(); i++) {
    ActNamespace *ns = *i;
    if (!ns->isExported()) {
      backend->printCustomNamespace(ns);
    }
  }
  /* declare all of the act processes */
  ActTypeiter it(a->Global());
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      backend->printProcDeclaration(p);
    }
  }
  /* generate chp implementation for each act process */
  Map<const char *, unsigned> procCount;
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      auto procGenerator = new ProcGenerator(metrics, backend, p);
      procGenerator->run();
    }
  }
  backend->printFileEnding();
  metrics->dump();

  return 0;
}

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
#include "core/ProcGenerator.h"
#include "core/Metrics.h"

int debug_verbose;

static void usage(char *name) {
  fprintf(stderr, "Usage: %s [-qv] [-m <metrics>] <actfile>\n", name);
  fprintf(stderr,
          " -m <metrics> : provide file name for energy/delay/area metrics\n");
  fprintf(stderr, " -v : increase verbosity (default 1)\n");
  exit(1);
}

static void create_outfiles(char *&statsFilePath,
                            FILE **chpFp,
                            FILE **chpLibfp,
                            FILE **conffp,
#if GEN_NETLIST
FILE **netlistFp,
FILE **netlistLibFp,
FILE **netlistIncludeFp,
#endif
                            const char *src) {
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
  char *tmpbuf = new char[64 + len];
  for (unsigned j = 0; j < i; j++) {
    tmpbuf[j] = src[j];
  }
  /* generate statistics file */
  statsFilePath = new char[8 + len];
  sprintf(statsFilePath, "%s.stat", basesrcName);
  /* generate chp file */
  sprintf(tmpbuf + i, "%s_chp.act", basesrcName);
  *chpFp = fopen(tmpbuf, "w");
  if (!*chpFp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }
  sprintf(tmpbuf + i, "%s_lib.act", basesrcName);
  *chpLibfp = fopen(tmpbuf, "w");
  if (!*chpLibfp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }
  fprintf(*chpFp, "import \"%s\";\n", tmpbuf);
  fprintf(*chpFp, "import \"dflow_stdlib.act\";\n\n");
#if GEN_NETLIST
  /* generate netlist file */
  sprintf(tmpbuf + i, "%s_netlib.act", basesrcName);
  *netlistLibFp = fopen(tmpbuf, "w");
  if (!*netlistLibFp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }
  fprintf(*netlistLibFp, "import \"%s_netlist_include.act\";\n\n", basesrcName);
  fprintf(*netlistLibFp, "open dflowstd;\n\n");

  sprintf(tmpbuf + i, "%s_netlist_include.act", basesrcName);
  *netlistIncludeFp = fopen(tmpbuf, "w");
  if (!*netlistIncludeFp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }
  fprintf(*netlistIncludeFp, "import \"%s_lib.act\";\n", basesrcName);
  fprintf(*netlistIncludeFp, R"(
import "dflow_stdlib_refine.act";
import globals;
import "syn/bdopt/stdcells.act";
open syn;
)");
  fprintf(*netlistIncludeFp, "import \"dflow_stdlib_refine.act\";\n");
  sprintf(tmpbuf + i, "%s_netlist.act", basesrcName);
  *netlistFp = fopen(tmpbuf, "w");
  if (!*netlistFp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }
  fprintf(*netlistFp, "import \"%s_chp.act\";\n", basesrcName);
  fprintf(*netlistFp, "import \"%s_netlib.act\";\n", basesrcName);
  fprintf(*netlistFp, "import \"dflow_stdlib_refine.act\";\n\n");
#endif
  /* create configuration file */
  sprintf(tmpbuf + i, "%s.conf", basesrcName);
  *conffp = fopen(tmpbuf, "w");
  if (!*conffp) {
    fatal_error("Could not open file `%s' for writing", tmpbuf);
  }
}

static Metrics *createMetrics(const char *metricFile,
                              const char *statsFilePath) {
  size_t metricFPLen = (metricFile) ? 1 + strlen(metricFile) : 1024;
  char *metricFilePath = new char[metricFPLen];
  if (metricFile) {
    sprintf(metricFilePath, "%s", metricFile);
  } else {
    sprintf(metricFilePath, "dflow-std/chp.metrics");
  }
  auto metrics = new Metrics(metricFilePath, statsFilePath);
  metrics->readMetricsFile();
  return metrics;
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
  FILE *chpFp, *chpLibFp, *confFp;
#if GEN_NETLIST
  FILE *netlistFp, *netlistLibFp, *netlistIncludeFp;
#endif
  char *statsFilePath = nullptr;

  create_outfiles(
      statsFilePath,
      &chpFp,
      &chpLibFp,
      &confFp,
#if GEN_NETLIST
  &netlistFp,
  &netlistLibFp,
  &netlistIncludeFp,
#endif
      act_file);

  Metrics *metrics = createMetrics(mfile, statsFilePath);
#if GEN_NETLIST
  auto chpGenerator = new ChpGenerator(chpFp, netlistFp);
  auto chpLibGenerator = new ChpLibGenerator(chpLibFp, confFp);
  auto netlistGenerator = new NetlistGenerator(netlistFp);
  auto netlistLibGenerator =
      new NetlistLibGenerator(netlistLibFp, netlistIncludeFp);
  auto netlistBackend =
      new NetlistBackend(netlistGenerator, netlistLibGenerator);
  auto backend = new ChpBackend(chpGenerator, chpLibGenerator, netlistBackend);
#else
  auto chpGenerator = new ChpGenerator(chpFp);
  auto chpLibGenerator = new ChpLibGenerator(chpLibFp, confFp);
  auto backend = new ChpBackend(chpGenerator, chpLibGenerator);
#endif

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

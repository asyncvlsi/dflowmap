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
#include <filesystem>
#include "DflowMapPass.h"
#include "src/core/Metrics.h"

int debug_verbose;
char *outputDir;
char *cache_dir;
char *cached_metrics;
char *custom_metrics;
char *custom_fu_dir;

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
   * path1/path2/.../circuit.act. We need to extract the workload_name, which is
   * "circuit", as well as taking care of the file path.*/
  unsigned i = strlen(src);
  char *baseDir = new char[i];
  while (i > 0) {
    if (src[i - 1] == '/') {
      break;
    }
    i--;
  }
  if (i == 0) {
    sprintf(baseDir, ".");
  } else {
    baseDir = (char *) std::string(src).substr(0, i).c_str();
  }
  size_t baseDirLen = strlen(baseDir);
  size_t len = strlen(src);
  char *workload_name = new char[len - i - 3];
  snprintf(workload_name, len - i - 3, "%s", src + i);
  size_t workloadNameLen = strlen(workload_name);
  /* create the default cache folder */
  cache_dir = new char[16 + baseDirLen];
  sprintf(cache_dir, "%s/.dflow_cache", baseDir);
  cached_metrics = new char[16 + strlen(cache_dir)];
  sprintf(cached_metrics, "%s/fu.metrics", cache_dir);
  if (!std::filesystem::is_directory(cache_dir)
      || !std::filesystem::exists(cache_dir)) {
    std::filesystem::create_directory(cache_dir);
    std::ofstream metric_cache;
    metric_cache.open(cached_metrics, std::fstream::app);
  }
  /* create output folder "${workload_name}_output" if it does not exist */
  outputDir = new char[baseDirLen + workloadNameLen + 8];
  sprintf(outputDir, "%s/%s_out", baseDir, workload_name);
  size_t outputPathLen = strlen(outputDir);
  custom_fu_dir = new char[16 + outputPathLen];
  sprintf(custom_fu_dir, "%s/customF", outputDir);
  custom_metrics = new char[16 + strlen(custom_fu_dir)];
  sprintf(custom_metrics, "%s/fu.metrics", custom_fu_dir);
  if (!std::filesystem::is_directory(outputDir)
      || !std::filesystem::exists(outputDir)) {
    std::filesystem::create_directory(outputDir);
  }
  if (!std::filesystem::is_directory(custom_fu_dir)
      || !std::filesystem::exists(custom_fu_dir)) {
    std::filesystem::create_directory(custom_fu_dir);
    std::ofstream custom_metric_file;
    custom_metric_file.open(custom_metrics, std::fstream::app);
  }
  /* generate statistics file */
  statsFilePath = new char[outputPathLen + workloadNameLen + 16];
  sprintf(statsFilePath, "%s/%s.stat", outputDir, workload_name);
  /* generate chp file */
  char *chp_file = new char[outputPathLen + workloadNameLen + 16];
  sprintf(chp_file, "%s/%s_chp.act", outputDir, workload_name);
  *chpFp = fopen(chp_file, "w");
  if (!*chpFp) {
    fatal_error("Could not open file `%s' for writing", chp_file);
  }
  fprintf(*chpFp, "import \"%s_lib.act\";\n\n", workload_name);
  /* generate chp lib file */
  char *chp_lib = new char[outputPathLen + workloadNameLen + 16];
  sprintf(chp_lib, "%s/%s_lib.act", outputDir, workload_name);
  *chpLibfp = fopen(chp_lib, "w");
  if (!*chpLibfp) {
    fatal_error("Could not open file `%s' for writing", chp_lib);
  }
  fprintf(*chpLibfp, R"(import globals;
import std;
import dflow_std::dflow_stdlib;

open dflow_std::dflow_stdlib;

)");
  /* create configuration file */
  char *conf_file = new char[outputPathLen + workloadNameLen + 16];
  sprintf(conf_file, "%s/%s.conf", outputDir, workload_name);
  *conffp = fopen(conf_file, "w");
  if (!*conffp) {
    fatal_error("Could not open file `%s' for writing", conf_file);
  }
#if GEN_NETLIST
  /* generate netlist file */
  char *netlist_lib = new char[outputPathLen + workloadNameLen + 16];
  sprintf(netlist_lib, "%s/%s_netlib.act", outputDir, workload_name);
  *netlistLibFp = fopen(netlist_lib, "w");
  if (!*netlistLibFp) {
    fatal_error("Could not open file `%s' for writing", netlist_lib);
  }
  fprintf(*netlistLibFp,
          "import \"%s_netlist_include.act\";\n\n",
          workload_name);
  /* generate netlist include file */
  char *netlist_include = new char[outputPathLen + workloadNameLen + 64];
  sprintf(netlist_include,
          "%s/%s_netlist_include.act",
          outputDir,
          workload_name);
  *netlistIncludeFp = fopen(netlist_include, "w");
  if (!*netlistIncludeFp) {
    fatal_error("Could not open file `%s' for writing", netlist_include);
  }
  fprintf(*netlistIncludeFp, R"(import globals;
import std;
import dflow_std::dflow_stdlib_refine;
import "syn/bdopt/stdcells.act";

open syn;
open dflow_std::dflow_stdlib_refine;

)");
  fprintf(*netlistIncludeFp, "import \"%s_lib.act\";\n", workload_name);

  /* generate netlist file */
  char *netlist_file = new char[outputPathLen + workloadNameLen + 16];
  sprintf(netlist_file, "%s/%s_netlist.act", outputDir, workload_name);
  *netlistFp = fopen(netlist_file, "w");
  if (!*netlistFp) {
    fatal_error("Could not open file `%s' for writing", netlist_file);
  }
  fprintf(*netlistFp, "import \"%s_chp.act\";\n", workload_name);
  fprintf(*netlistFp, "import \"%s_netlib.act\";\n\n", workload_name);
#endif
  if (debug_verbose) {
    printf("baseDir: %s, workload_name: %s, outputDir: %s, statsFilePath: %s, "
           "chp_file: %s, chp_lib: %s, conf_file: %s\n",
           baseDir,
           workload_name,
           outputDir,
           statsFilePath,
           chp_file,
           chp_lib,
           conf_file);
#if GEN_NETLIST
    printf("netlist_file: %s, netlist_lib: %s, netlist_include: %s\n",
           netlist_file,
           netlist_lib,
           netlist_include);
#endif
    printf(
        "custom_fu_dir: %s, custom_metrics: %s, cache_dir: %s, cached_metrics: %s\n",
        custom_fu_dir,
        custom_metrics,
        cache_dir,
        cached_metrics);
  }
}

static Metrics *createMetrics(const char *metricFile,
                              const char *statsFilePath) {
  size_t metricFPLen = (metricFile) ? 1 + strlen(metricFile) : MAX_INSTANCE_LEN;
  char *customFUMetricsFP = new char[metricFPLen];
  if (metricFile) {
    sprintf(customFUMetricsFP, "%s", metricFile);
  } else {
    sprintf(customFUMetricsFP, "%s/customF/fu.metrics", outputDir);
  }
  char *act_home = getenv("ACT_HOME");
  char *stdFUMetricsFP = new char[SHORT_STRING_LEN + strlen(act_home)];
  sprintf(stdFUMetricsFP,
          "%s/act/dflow_std/dflow_stdlib_refine.metrics",
          act_home);
  auto metrics = new Metrics(customFUMetricsFP,
                             stdFUMetricsFP,
                             statsFilePath);
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
  auto chpGenerator = new ChpGenerator(chpFp);
  auto chpLibGenerator = new ChpLibGenerator(chpLibFp, confFp);
#if GEN_NETLIST
  auto dflowNetGenerator = new DflowNetGenerator(netlistFp);
  auto dflowNetLibGenerator =
      new DflowNetLibGenerator(netlistLibFp, netlistIncludeFp);
  auto dflowNetBackend =
      new DflowNetBackend(dflowNetGenerator, dflowNetLibGenerator);
  auto backend = new ChpBackend(chpGenerator, chpLibGenerator, dflowNetBackend);
#else
  auto backend = new ChpBackend(chpGenerator, chpLibGenerator);
#endif

  /* declare custom namespace */
  ActNamespaceiter i(a->Global());
  for (i = i.begin(); i != i.end(); i++) {
    ActNamespace *ns = *i;
    if (!ns) continue;
    if (!ns->isExported()) {
      backend->printCustomNamespace(ns);
    }
  }
  /* generate chp implementation for each act process */
  auto dflowmap_pass = new DflowMapPass(a, "dflowmap", metrics, backend);
  dflowmap_pass->run(nullptr);
  backend->printFileEnding();
  metrics->dump();

  return 0;
}

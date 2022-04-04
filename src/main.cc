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
#include "src/core/dflowmap_main.cc"

int debug_verbose;

static void usage(char *name) {
  fprintf(stderr, "Usage: %s [-qv] [-m <metrics>] <actfile>\n", name);
  fprintf(stderr,
          " -m <metrics> : provide file name for energy/delay/area metrics\n");
  fprintf(stderr, " -v : increase verbosity (default 1)\n");
  exit(1);
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
  /* read in the ACT file */
  char *act_file = argv[optind];
  Act *a = new Act(act_file);
  a->Expand();
  a->mangle(nullptr);
  if (debug_verbose) {
    fprintf(stdout, "Processing ACT file %s!\n", act_file);
    printf("------------------ACT FILE--------------------\n");
    a->Print(stdout);
    printf("\n\n\n");
  }
  FILE *resFp, *libFp, *confFp;
  dflowmap_main(a, mfile, act_file, resFp, libFp, confFp);

  return 0;
}

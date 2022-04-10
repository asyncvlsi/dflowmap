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

#ifndef DFLOWMAP_SRC_CORE_NAMEGENERATOR_H_
#define DFLOWMAP_SRC_CORE_NAMEGENERATOR_H_

#include <act/act.h>
#include <act/lang.h>
#include "src/common/common.h"
#include "src/common/Constant.h"

class NameGenerator {
 public:
  static const char *genMergeInstName(unsigned guardBW,
                                      unsigned inBW,
                                      int numInputs);

  static const char *genMixerInstName(unsigned inBW, int numInputs);

  static const char *genArbiterInstName(unsigned guardBW,
                                        unsigned inBW,
                                        int numInputs);

  static const char *genSplitInstName(unsigned guardBW,
                                      unsigned outBW,
                                      int numOut);

  static const char *genCopyInstName(unsigned bw, unsigned numOut);

  static const char *genSinkInstName(unsigned bw);

  static const char *genSourceInstName(unsigned long val, unsigned bitwidth);

  static const char *genExprName(Expr *expr) {
    list_t *arg_list = list_new();
    act_expr_collect_ids(arg_list, expr);
    return act_expr_to_string(arg_list, expr);
  }

  static const char *genExprClusterName(list_t *dflow_cluster) {
    Vector<Expr *> exprList;
    for (listitem_t *li = list_first (dflow_cluster); li; li = list_next (li)) {
      auto *d = (act_dataflow_element *) list_value (li);
      if (d->t != ACT_DFLOW_FUNC) {
        dflow_print(stdout, d);
        printf("\nThis dflow statement should not appear in dflow-cluster!\n");
        exit(-1);
      }
      Expr *expr = d->u.func.lhs;
      exprList.push_back(expr);
    }
    list_t *argList = list_new();
    for (auto &e: exprList) {
      act_expr_collect_ids(argList, e);
    }
    char *name = new char[MAX_CLUSTER_PROC_NAME_LEN];
    name[0] = '\0';
    char *delimiter = new char[1];
    sprintf(delimiter, "_");
    for (auto &e: exprList) {
      if (name[0] != '\0') strcat(name, delimiter);
      strcat(name, act_expr_to_string(argList, e));
    }
    return name;
  }
};

#endif //DFLOWMAP_SRC_CORE_NAMEGENERATOR_H_

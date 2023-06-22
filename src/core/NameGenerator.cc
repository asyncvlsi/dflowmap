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

#include "NameGenerator.h"

const char *NameGenerator::genMergeInstName(unsigned guardBW,
                                            unsigned inBW,
                                            int numInputs,
                                            char *&procName) {
  if (PIPELINE) {
    sprintf(procName, "lib::pipe_%s", Constant::MERGE_PREFIX);
  } else {
    sprintf(procName, "lib::unpipe_%s", Constant::MERGE_PREFIX);
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<%d,%u,%u>", procName, numInputs, guardBW, inBW);
  return instance;
}

const char *NameGenerator::genMixerInstName(unsigned ctrlBW,
					    unsigned inBW,
                                            int numInputs,
                                            char *&procName) {
  if (PIPELINE) {
    sprintf(procName, "lib::pipe_%s", Constant::MIXER_PREFIX);
  } else {
    sprintf(procName, "lib::unpipe_%s", Constant::MIXER_PREFIX);
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<%d,%u,%u>", procName, numInputs, inBW, ctrlBW);
  return instance;
}

const char *NameGenerator::genArbiterInstName(unsigned ctrlBW,
                                              unsigned inBW,
                                              int numInputs,
                                              char *&procName) {
  if (PIPELINE) {
    sprintf(procName, "lib::pipe_%s", Constant::ARBITER_PREFIX);
  } else {
    sprintf(procName, "lib::unpipe_%s", Constant::ARBITER_PREFIX);
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<%d,%u,%u>", procName, numInputs, inBW, ctrlBW);
  return instance;
}

const char *NameGenerator::genSplitInstName(unsigned guardBW,
                                            unsigned outBW,
                                            int numOut,
                                            char *&procName) {
  if (PIPELINE) {
    sprintf(procName, "lib::pipe_%s", Constant::SPLIT_PREFIX);
  } else {
    sprintf(procName, "lib::unpipe_%s", Constant::SPLIT_PREFIX);
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<%d,%u,%u>", procName, numOut, guardBW, outBW);
  return instance;
}

const char *NameGenerator::genCopyInstName(unsigned bw, unsigned numOut) {
  char *procName = new char[1024];
  sprintf(procName, "lib::copy");
  char *instance = new char[1024];
  sprintf(instance, "%s<%u,%u>", procName, bw, numOut);
  return instance;
}

const char *NameGenerator::genCopyLeafInstName(unsigned bw, unsigned numOut) {
  char *procName = new char[1024];
  sprintf(procName, "lib::copy_leaf");
  char *instance = new char[1024];
  sprintf(instance, "%s<%u,%u>", procName, bw, numOut);
  return instance;
}

const char *NameGenerator::genSinkInstName(unsigned bw) {
  char *instance = new char[1500];
  sprintf(instance, "lib::sink<%u>", bw);
  return instance;
}

const char *NameGenerator::genSourceInstName(unsigned long val,
                                             unsigned bitwidth) {
  char *instance = new char[1500];
  sprintf(instance, "lib::source<%lu,%u>", val, bitwidth);
  return instance;
}

const char *NameGenerator::genExprName(Scope *sc, Expr *expr, ActId *rhs) {
  list_t *arg_list = list_new();
  char *instance = new char[MAX_INSTANCE_LEN];
  char buf[16];
  char *tmp;
  act_expr_collect_ids(arg_list, expr);
  tmp = act_expr_to_string(arg_list, expr);
  snprintf (instance, MAX_INSTANCE_LEN, tmp);
  FREE (tmp);
  listitem_t *li;
  list_append (arg_list, rhs);
  for (li = list_first (arg_list); li; li = list_next (li)) {
    InstType *it = sc->FullLookup ((ActId *) list_value (li), NULL);
    Assert (it, "Hmm");
    Assert (TypeFactory::bitWidth (it) > 0, "Non-positive bitwidth?");
    snprintf (buf, 16, "W%d", TypeFactory::bitWidth (it));
    strcat (instance, buf);
  }
  list_free (arg_list);

  return instance;
}

const char *NameGenerator::genExprClusterName(Scope *sc, list_t *dflow_cluster) {
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
  char *delimiter = new char[2];
  sprintf(delimiter, "_");
  for (auto &e: exprList) {
    if (name[0] != '\0') strcat(name, delimiter);
    strcat(name, act_expr_to_string(argList, e));
  }

  for (listitem_t *li = list_first (dflow_cluster); li; li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    Assert (d->t == ACT_DFLOW_FUNC, "What?");
    list_append (argList, d->u.func.rhs);
  }
  
  for (listitem_t *li = list_first (argList); li; li = list_next (li)) {
    char buf[16];
    InstType *it = sc->FullLookup ((ActId *) list_value (li), NULL);
    Assert (it, "Hmm");
    Assert (TypeFactory::bitWidth (it) > 0, "Non-positive bitwidth?");
    snprintf (buf, 16, "W%d", TypeFactory::bitWidth (it));
    strcat (name, buf);
  }
  list_free (argList);

  return name;
}

const char *NameGenerator::genFUName(const char *procName,
                                     StringVec &argList,
                                     UIntVec &outBWList,
                                     UIntVec &argBWList) {
  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s", procName);
  unsigned numArgs = argList.size();
  unsigned numOuts = outBWList.size();
#if 0  
  for (unsigned i = 0; i < numArgs; i++) {
    char *subInstance = new char[100];
    sprintf(subInstance, "W%u", argBWList[i]);
    strcat(instance, subInstance);
  }
  for (unsigned i = 0; i < numOuts; i++) {
    char *subInstance = new char[100];
    sprintf(subInstance, "W%u", outBWList[i]);
    strcat(instance, subInstance);
  }
#endif  
  return instance;
}

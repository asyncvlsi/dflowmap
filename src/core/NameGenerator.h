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
#include <cstring>
#include "src/common/common.h"
#include "src/common/Constant.h"
#include "src/common/config.h"

class NameGenerator {
 public:
  static const char *genMergeInstName(unsigned guardBW,
                                      unsigned inBW,
                                      int numInputs,
                                      char *&procName);

  static const char *genMixerInstName(unsigned ctrlBW,
				      unsigned inBW,
                                      int numInputs,
                                      char *&procName);

  static const char *genArbiterInstName(unsigned ctrlBW,
                                        unsigned inBW,
                                        int numInputs,
                                        char *&procName);

  static const char *genSplitInstName(unsigned guardBW,
                                      unsigned outBW,
                                      int numOut,
                                      char *&procName);

  static const char *genCopyInstName(unsigned bw, unsigned numOut);
  static const char *genCopyLeafInstName(unsigned bw, unsigned numOut);

  static const char *genSinkInstName(unsigned bw);

  static const char *genSourceInstName(unsigned long val, unsigned bitwidth);

  static const char *genExprName(Scope *sc, Expr *expr, ActId *rhs);

  static const char *genExprClusterName(Scope *sc, list_t *dflow_cluster);

  static const char *genFUName(const char *procName,
                               StringVec &argList,
                               UIntVec &outBWList,
                               UIntVec &argBWList);
};

#endif //DFLOWMAP_SRC_CORE_NAMEGENERATOR_H_

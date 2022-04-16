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

#ifndef DFLOWMAP_NETLISTLIBGENERATOR_H
#define DFLOWMAP_NETLISTLIBGENERATOR_H

#include <cstring>
#include <algorithm>
#include <act/act.h>
#include <act/iter.h>
#include <fstream>
#include "src/common/common.h"
#include "src/common/Helper.h"

class NetlistLibGenerator {
 public:
  NetlistLibGenerator(FILE *netlistLibFp,
                      FILE *netlistIncludeFp);

  bool checkAndUpdateInstance(const char *instance);

  void printFUNetListLib(const char *instance,
                         UIntVec &argBWList,
                         UIntVec &outBWList);

 private:
  const char *instances[MAX_PROCESSES];

  FILE *netlistLibFp;
  FILE *netlistIncludeFp;

};

#endif //DFLOWMAP_NETLISTLIBGENERATOR_H

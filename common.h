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
#ifndef DFLOWMAP_COMMON_H
#define DFLOWMAP_COMMON_H

#include <vector>

#define DEBUG_VERBOSE false
#define DEBUG_CLUSTER false

FILE *libFp;
FILE *resFp;
FILE *confFp;


typedef std::string String;
typedef std::vector<String> StringVec;
typedef std::vector<int> IntVec;
typedef std::vector<const char *> CharPtrVec;

template<class T1, class T2>
using Map = std::map<T1, T2>;

template<class T1, class T2>
std::pair<T1, T2> GenPair(T1 a, T2 b) {
  return std::make_pair(a, b);
}

/* op, its bitwidth */
Map<const char *, int> bitwidthMap;
/* operator, # of times it is used (if it is used for more than once, then we create COPY for it) */
Map<const char *, unsigned> opUses;
/* copy operator, # of times it has already been used */
Map<const char *, unsigned> copyUses;
unsigned sinkCnt = 0;
/* operator, (leak power (nW), dyn energy (e-15J), delay (ps), area (um^2)) */
Map<const char *, int *> opMetrics;

/* copy bitwidth,< # of output, # of instances of this COPY> */
Map<int, Map<int, int>> copyStatistics;

int totalArea = 0;
/* procName, area of all of the instances of the process */
Map<const char*, int> areaStatistics;

#endif //DFLOWMAP_COMMON_H

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
#define numOps 13
#define numBWs 5

FILE *libFp;
FILE *resFp;
FILE *confFp;

std::map<const char *, int> bitwidthMap;
/* operator, # of times it is used (if it is used for more than once, then we create COPY for it) */
std::map<const char *, unsigned> opUses;
/* copy operator, # of times it has already been used */
std::map<const char *, unsigned> copyUses;

const char *ops[numOps] = {"add", "and", "buff", "copy", "div", "icmp", "lshift", "merge", "mul",
                           "rem", "sink", "source", "split"};
const int opBWs[numBWs] = {1, 8, 16, 32, 64};
/* operator, <BW, (leak power (nW), dyn energy (e-15J), delay (ps), area (um^2))> */
std::map<const char *, int**> opMetrics;

#endif //DFLOWMAP_COMMON_H

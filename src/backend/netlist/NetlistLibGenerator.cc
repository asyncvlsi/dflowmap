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

#include "NetlistLibGenerator.h"

NetlistLibGenerator::NetlistLibGenerator(FILE *netlistLibFp,
                                         FILE *netlistIncludeFp) {
  for (auto i = 0; i < MAX_PROCESSES; i++) instances[i] = nullptr;
  this->netlistLibFp = netlistLibFp;
  this->netlistIncludeFp = netlistIncludeFp;
}

bool NetlistLibGenerator::checkAndUpdateInstance(const char *instance) {
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    if (instances[i] == nullptr) {
      instances[i] = instance;
      return false;
    } else if (!strcmp(instances[i], instance)) {
      return true;
    }
  }
  return false;
}

void NetlistLibGenerator::printFUNetListLib(const char *instance,
                                            UIntVec &argBWList,
                                            UIntVec &outBWList) {
  if (checkAndUpdateInstance(instance)) return;
  const char *normInstance = getNormInstanceName(instance);
  fprintf(netlistIncludeFp, "import \"%s.act\";\n", normInstance);
  fprintf(netlistLibFp, "defproc %simpl <: %s()\n", normInstance, instance);
  fprintf(netlistLibFp, "+{\n");
  unsigned numArgs = argBWList.size();
  unsigned numOuts = outBWList.size();
  for (unsigned i = 0; i < numArgs; i++) {
    fprintf(netlistLibFp, "  bd<%u> in%d;\n", argBWList[i], i);
  }
  for (unsigned i = 0; i < numOuts; i++) {
    fprintf(netlistLibFp, "  bd<%u> out%d;\n", outBWList[i], i);
  }
  fprintf(netlistLibFp, "}\n{\n");
  fprintf(netlistLibFp,
          "m_in_n_out_func_control<%d, %d> fc;\n",
          numArgs,
          numOuts);
  for (unsigned i = 0; i < numArgs; i++) {
    fprintf(netlistLibFp, "in%d.r = fc.in[%d].r;\n", i, i);
    fprintf(netlistLibFp, "in%d.a = fc.in[%d].a;\n", i, i);
  }
  for (unsigned i = 0; i < numOuts; i++) {
    fprintf(netlistLibFp, "out%d.r = fc.out[%d].r;\n", i, i);
    fprintf(netlistLibFp, "out%d.a = fc.out[%d].a;\n", i, i);
  }
  for (unsigned i = 0; i < numArgs; i++) {
    //TODO: generate CD and PW
    fprintf(netlistLibFp, "capture<%d, 1, 1> cap%d;\n", argBWList[i], i);
    fprintf(netlistLibFp, "cap%d.go = fc.dc[%d];\n", i, i);
    fprintf(netlistLibFp, "cap%d.din = in%d.d;\n", i, i);
  }
  fprintf(netlistLibFp, "%s fu;\n", normInstance);
  for (unsigned i = 0; i < numArgs; i++) {
    fprintf(netlistLibFp, "fu.x%d = cap%d.dout;\n", i, i);
  }
  //TODO: size of delay lines should be computed!
  fprintf(netlistLibFp, R"(delay_line<10> fu_delay;
fu_delay.in = fc.f.r;
fu_delay.out = fc.f.a;
)");
  for (unsigned i = 0; i < numOuts; i++) {
    fprintf(netlistLibFp, "fu.out%d = out%d.d;\n", i, i);
  }
  fprintf(netlistLibFp, "}\n");
}


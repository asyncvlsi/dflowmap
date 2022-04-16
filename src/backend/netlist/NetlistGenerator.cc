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

#include "NetlistGenerator.h"

NetlistGenerator::NetlistGenerator(FILE *netlistFp) {
  if (!netlistFp) {
    printf("Invalid file handler for netlist circuit!\n");
    exit(-1);
  }
  this->netlistFp = netlistFp;
}

void NetlistGenerator::printSinkNetlist(const char *inName) {
  if (inName == nullptr) {
    printf("sink name is NULL!\n");
    exit(-1);
  }
  const char *normalizedName = getNormActIdName(inName);
  fprintf(netlistFp, "dflowstd::sink_dflow %s_sink;\n", normalizedName);
}

void NetlistGenerator::printCopyNetlist(const char *inputName) {
  const char *normOutName = getNormActIdName(inputName);
  //TODO: CD and PW need to be computed
  fprintf(netlistFp, "dflowstd::copy_dflow<1,1> %scopy;\n", normOutName);
}

void NetlistGenerator::printInitNetlist(const char *outName) {
  const char *normOutput = getNormActIdName(outName);
  fprintf(netlistFp, "dflowstd::init_dflow %s_inst;\n", normOutput);
}

void NetlistGenerator::printOneBuffNetlist(const char *outName) {
  fprintf(netlistFp, "dflowstd::onebuf_dflow %s_inst;\n", outName);
}

void NetlistGenerator::printChanNetlist(const char *chanName,
                                        unsigned int bitwidth) {
  fprintf(netlistFp, "bd<%u> %s;\n", bitwidth, chanName);
}

void NetlistGenerator::printSourceNetlist(const char *outName) {
  const char *normOutName = getNormActIdName(outName);
  fprintf(netlistFp, "dflowstd::source_dflow %s_inst;\n", normOutName);
}

void NetlistGenerator::printBuffNetlist(Vector<BuffInfo> &buffInfos) {
  for (auto &buffInfo: buffInfos) {
    const char *finalOutput = buffInfo.finalOutput;
    unsigned bw = buffInfo.bw;
    unsigned long nBuff = buffInfo.nBuff;
    bool hasInitVal = buffInfo.hasInitVal;
    for (unsigned i = 0; i < nBuff - 1; i++) {
      char *chanName = new char[strlen(finalOutput) + 1024];
      sprintf(chanName, "%s_buf%u", finalOutput, i);
      printChanNetlist(chanName, bw);
      printOneBuffNetlist(chanName);
    }
    if (hasInitVal) {
      printInitNetlist(finalOutput);
    } else {
      printOneBuffNetlist(finalOutput);
    }
  }
}

void NetlistGenerator::printFUNetlist(const char *instance,
                                      const char *fuInstName) {
  const char *normInstance = getNormInstanceName(instance);
  fprintf(netlistFp, "  %simpl %s;\n", normInstance, fuInstName);
}

void NetlistGenerator::printSplitNetlist(const char *procName,
                                         const char *splitName,
                                         unsigned dataBW,
                                         unsigned numOutputs) {
  fprintf(netlistFp, "bd<%d> %s_out[%d];\n", dataBW, splitName, numOutputs);
  fprintf(netlistFp, "%s_dflow %s;\n", procName, splitName);
}

void NetlistGenerator::printMergeNetlist(const char *procName,
                                         const char *outName,
                                         unsigned dataBW,
                                         unsigned numInputs) {
  const char *normOutput = getNormActIdName(outName);
  fprintf(netlistFp, "bd<%d> %s_in[%d];\n", dataBW, normOutput, numInputs);
  fprintf(netlistFp, "%s_dflow %s_inst;\n", procName, normOutput);
}

void NetlistGenerator::printArbiterNetlist(const char *procName,
                                           const char *outName,
                                           unsigned dataBW,
                                           unsigned numInputs) {
  const char *normOutput = getNormActIdName(outName);
  fprintf(netlistFp, "bd<%d> %s_in[%d];\n", dataBW, normOutput, numInputs);
  fprintf(netlistFp, "%s_dflow %s_inst;\n", procName, normOutput);
}

void NetlistGenerator::printMixerNetlist(const char *procName,
                                         const char *outName,
                                         unsigned dataBW,
                                         unsigned numInputs) {
  const char *normOutput = getNormActIdName(outName);
  fprintf(netlistFp, "bd<%d> %s_in[%d];\n", dataBW, normOutput, numInputs);
  fprintf(netlistFp, "%s_dflow %s_inst;\n", procName, normOutput);
}

void NetlistGenerator::printProcNetListHeader(Process *p) {
  fprintf(netlistFp, "defproc ");
  p->printActName(netlistFp);
  fprintf(netlistFp, "_impl <: ");
  p->printActName(netlistFp);
  fprintf(netlistFp, "()\n");
  fprintf(netlistFp, "+{\n");
  ActInstiter iter(p->CurScope());
  for (iter = iter.begin(); iter != iter.end(); iter++) {
    ValueIdx *vx = *iter;
    if (TypeFactory::isChanType(vx->t)
        || TypeFactory::isIntType(vx->t)
        || TypeFactory::isBoolType(vx->t)) {
      int bw = TypeFactory::bitWidth(vx->t);
      fprintf(netlistFp, "  bd<%d> %s;\n", bw, vx->getName());
    } else if (TypeFactory::isProcessType(vx->t)) {
      auto proc = dynamic_cast <Process *> (vx->t->BaseType());
      Assert (proc, "Uncaptured exception!");
      ActNamespace::Act()->mfprintfproc(netlistFp, proc);
      fprintf(netlistFp, " %s;\n", vx->getName());
    } else if (TypeFactory::isStructure(vx->t)) {
      Data *d = dynamic_cast <Data *> (vx->t->BaseType());
      Assert (d, "Uncaptured exception!");
      ActNamespace::Act()->mfprintfproc(netlistFp, d);
      fprintf(netlistFp, " %s;\n", vx->getName());
    }
  }
}

void NetlistGenerator::printProcNetListEnding() {
  fprintf(netlistFp, "}\n{\n}\n\n");
}

void NetlistGenerator::printNetlistFileEnding() {
  fprintf(netlistFp, "/* end of generation */\n");
  fclose(netlistFp);
}

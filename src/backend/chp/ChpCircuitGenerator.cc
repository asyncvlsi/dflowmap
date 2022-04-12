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

#include "ChpCircuitGenerator.h"

ChpCircuitGenerator::ChpCircuitGenerator(FILE *chpFp, FILE* netlistFp) {
  if (!chpFp || !netlistFp) {
    printf("Invalid file handler for CHP circuit!\n");
    exit(-1);
  }
  this->chpFp = chpFp;
  this->netlistFp = netlistFp;
}

void ChpCircuitGenerator::printSink(const char *instance, const char *inName) {
  if (inName == nullptr) {
    printf("sink name is NULL!\n");
    exit(-1);
  }
  const char *normalizedName = getNormActIdName(inName);
  fprintf(chpFp, "%s %s_sink(%s);\n", instance, normalizedName, inName);
}

void ChpCircuitGenerator::printCopy(const char *instance,
                                    const char *inputName) {
  const char *normOutName = getNormActIdName(inputName);
  fprintf(chpFp,
          "%s %scopy(%s);\n",
          instance,
          normOutName,
          inputName);
  if (debug_verbose) {
    printf("[copy] %scopy\n", normOutName);
  }
}

void ChpCircuitGenerator::printEmptyLine() {
  fprintf(chpFp, "\n");
}

void ChpCircuitGenerator::printInit(const char *instance,
                                    const char *inName,
                                    const char *outName) {
  const char *normOutput = getNormActIdName(outName);
  fprintf(chpFp,
          "%s %s_inst(%s, %s);\n",
          instance,
          normOutput,
          inName,
          outName);
  if (debug_verbose) {
    printf("[buff] %s_init\n", outName);
  }
}

void ChpCircuitGenerator::printOneBuff(const char *instance,
                                       const char *inName,
                                       const char *outName) {
  fprintf(chpFp,
          "%s %s_inst(%s, %s);\n",
          instance,
          outName,
          inName,
          outName);
  if (debug_verbose) {
    printf("[buff] %s_init\n", outName);
  }
}

void ChpCircuitGenerator::printChannel(const char *chanName,
                                       unsigned int bitwidth) {
  fprintf(chpFp, "chan(int<%u>) %s;\n", bitwidth, chanName);
}

void ChpCircuitGenerator::printSource(const char *instance,
                                      const char *outName) {
  const char *normOutName = getNormActIdName(outName);
  fprintf(chpFp, "%s %s_inst(%s);\n", instance, normOutName, outName);
}

void ChpCircuitGenerator::printBuff(Vector<BuffInfo> &buffInfos) {
  for (auto &buffInfo: buffInfos) {
    const char *finalOutput = buffInfo.finalOutput;
    unsigned bw = buffInfo.bw;
    unsigned long nBuff = buffInfo.nBuff;
    unsigned long initVal = buffInfo.initVal;
    bool hasInitVal = buffInfo.hasInitVal;
    char *prevInName = new char[strlen(finalOutput) + 7];
    sprintf(prevInName, "%s_bufIn", finalOutput);
    char *onebufInstance = new char[1024];
    sprintf(onebufInstance, "dflowstd::onebuf<%u>", bw);
    for (unsigned i = 0; i < nBuff - 1; i++) {
      char *chanName = new char[strlen(finalOutput) + 1024];
      sprintf(chanName, "%s_buf%u", finalOutput, i);
      printChannel(chanName, bw);
      printOneBuff(onebufInstance, prevInName, chanName);
      prevInName = chanName;
    }
    if (hasInitVal) {
      char *initProcName = new char[1024];
      sprintf(initProcName, "dflowstd::init<%lu,%u>", initVal, bw);
      printInit(initProcName, prevInName, finalOutput);
    } else {
      printOneBuff(onebufInstance, prevInName, finalOutput);
    }
  }
}

void ChpCircuitGenerator::printFunc(const char *instance,
                                    StringVec &argList,
                                    StringVec &outList,
                                    Vector<BuffInfo> &buffInfos) {
  /* create port for BUFF first */
  //TODO: may not need to create the port! (if there is no func body!!!)
  for (auto &buffInfo: buffInfos) {
    const char *output = buffInfo.finalOutput;
    if (!output) {
      printf("Try to create BUFF for null output channel!\n");
      exit(-1);
    }
    size_t len = strlen(output);
    char *actualOut = new char[7 + len];
    sprintf(actualOut, "%s_bufIn", output);
    unsigned bw = buffInfo.bw;
    printChannel(actualOut, bw);
  }
  Vector<unsigned> buffOutIDs;
  for (auto &buffInfo: buffInfos) {
    buffOutIDs.push_back(buffInfo.outputID);
  }
  unsigned numOuts = outList.size();
  if (numOuts < 1) {
    printf("No output is found!\n");
    exit(-1);
  }
  Vector<const char *> outputVec;
  Vector<const char *> normOutputVec;
  for (unsigned i = 0; i < numOuts; i++) {
    const char *oriOut = outList[i].c_str();
    const char *normOut = getNormActIdName(oriOut);
    char *actualOut = new char[7 + strlen(oriOut)];
    char *actualNormOut = new char[7 + strlen(normOut)];
    if (hasInVector<unsigned>(buffOutIDs, i)) {
      sprintf(actualOut, "%s_bufIn", oriOut);
      sprintf(actualNormOut, "%s_bufIn", normOut);
    } else {
      sprintf(actualOut, "%s", oriOut);
      sprintf(actualNormOut, "%s", normOut);
    }
    outputVec.push_back(actualOut);
    normOutputVec.push_back(actualNormOut);
  }
  fprintf(chpFp, "%s ", instance);
  if (debug_verbose) {
    printf("[fu]: ");
  }
  for (auto &normOutput: normOutputVec) {
    fprintf(chpFp, "%s_", normOutput);
    if (debug_verbose) {
      printf("%s_", normOutput);
    }
  }
  fprintf(chpFp, "inst(");
  if (debug_verbose) {
    printf("inst\n");
  }
  for (auto &arg: argList) {
    fprintf(chpFp, "%s, ", arg.c_str());
  }
  for (unsigned i = 0; i < numOuts; i++) {
    const char *output = outputVec[i];
    fprintf(chpFp, "%s", output);
    if (i == (numOuts - 1)) {
      fprintf(chpFp, ");\n");
    } else {
      fprintf(chpFp, ", ");
    }
  }
}

void ChpCircuitGenerator::printSplit(const char *instance,
                                     const char *splitName,
                                     const char *guardName,
                                     const char *inputName,
                                     CharPtrVec &outNameVec) {
  fprintf(chpFp, "%s %s(", instance, splitName);
  fprintf(chpFp, "%s, %s", guardName, inputName);
  for (auto outName: outNameVec) {
    fprintf(chpFp, ", %s", outName);
  }
  fprintf(chpFp, ");\n");
}

void ChpCircuitGenerator::printMerge(const char *instance,
                                     const char *outName,
                                     const char *guardStr,
                                     CharPtrVec &inNameVec) {
  const char *normOutput = getNormActIdName(outName);
  fprintf(chpFp, "%s %s_inst(", instance, normOutput);
  fprintf(chpFp, "%s, ", guardStr);
  for (auto inName: inNameVec) {
    fprintf(chpFp, "%s, ", inName);
  }
  fprintf(chpFp, "%s);\n", outName);
}

void ChpCircuitGenerator::printArbiter(const char *instance,
                                       const char *outName,
                                       const char *coutName,
                                       CharPtrVec &inNameVec) {
  const char *normOutput = getNormActIdName(outName);
  fprintf(chpFp, "%s %s_inst(", instance, normOutput);
  for (auto inName: inNameVec) {
    fprintf(chpFp, "%s, ", inName);
  }
  fprintf(chpFp, "%s, %s);\n", outName, coutName);
}

void ChpCircuitGenerator::printMixer(const char *instance,
                                     const char *outName,
                                     CharPtrVec &inNameVec) {
  const char *normOutput = getNormActIdName(outName);
  fprintf(chpFp, "%s %s_inst(", instance, normOutput);
  for (auto inName: inNameVec) {
    fprintf(chpFp, "%s, ", inName);
  }
  fprintf(chpFp, "%s);\n", outName);
}

void ChpCircuitGenerator::printProcHeader(Process *p) {
  p->PrintHeader(chpFp, "defproc");
  fprintf(chpFp, "\n{");
  p->CurScope()->Print(chpFp);
}

void ChpCircuitGenerator::printProcDeclaration(Process *p) {
  p->PrintHeader(chpFp, "defproc");
  fprintf(chpFp, ";\n");
}

void ChpCircuitGenerator::printProcEnding() {
  fprintf(chpFp, "}\n\n");
}

void ChpCircuitGenerator::printCustomNamespace(ActNamespace *ns) {
  const char *nsName = ns->getName();
  fprintf(chpFp, "namespace %s {\n", nsName);
  ActTypeiter it(ns);
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      p->PrintHeader(chpFp, "defproc");
      fprintf(chpFp, ";\n");
    }
  }
  fprintf(chpFp, "}\n\n");
}

void ChpCircuitGenerator::printFileEnding() {
  fprintf(chpFp, "/* end of generation */\n");
  fclose(chpFp);
}

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

#include "ChpGenerator.h"

void ChpGenerator::printSinkChp(const char *instance, const char *inName) {
  if (inName == nullptr) {
    printf("sink name is NULL!\n");
    exit(-1);
  }
  const char *normalizedName = getNormActIdName(inName);
  fprintf(chpFp, "%s %s_sink(%s);\n", instance, normalizedName, inName);
}

void ChpGenerator::printCopyChp(const char *instance,
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

void ChpGenerator::printEmptyLine() {
  fprintf(chpFp, "\n");
}

void ChpGenerator::printInitChp(const char *instance,
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

void ChpGenerator::printOneBuffChp(const char *instance,
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

void ChpGenerator::printChannelChp(const char *chanName,
                                   unsigned int bitwidth) {
  fprintf(chpFp, "chan(int<%u>) %s;\n", bitwidth, chanName);
}

void ChpGenerator::printSourceChp(const char *instance,
                                  const char *outName) {
  const char *normOutName = getNormActIdName(outName);
  fprintf(chpFp, "%s %s_inst(%s);\n", instance, normOutName, outName);
}

void ChpGenerator::printBuffChp(Vector<BuffInfo> &buffInfos) {
  for (auto &buffInfo: buffInfos) {
    const char *finalOutput = buffInfo.finalOutput;
    unsigned bw = buffInfo.bw;
    unsigned long nBuff = buffInfo.nBuff;
    unsigned long initVal = buffInfo.initVal;
    bool hasInitVal = buffInfo.hasInitVal;
    char *prevInName = new char[strlen(finalOutput) + 7];
    sprintf(prevInName, "%s_bufIn", finalOutput);
    char *onebufInstance = new char[1024];
    sprintf(onebufInstance, "lib::onebuf<%u>", bw);
    for (unsigned i = 0; i < nBuff - 1; i++) {
      char *chanName = new char[strlen(finalOutput) + 1024];
      sprintf(chanName, "%s_buf%u", finalOutput, i);
      printChannelChp(chanName, bw);
      printOneBuffChp(onebufInstance, prevInName, chanName);
      prevInName = chanName;
    }
    if (hasInitVal) {
      char *initProcName = new char[1024];
      sprintf(initProcName, "lib::init<%lu,%u>", initVal, bw);
      printInitChp(initProcName, prevInName, finalOutput);
    } else {
      printOneBuffChp(onebufInstance, prevInName, finalOutput);
    }
  }
}

const char *ChpGenerator::printFUChp(const char *instance,
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
    printChannelChp(actualOut, bw);
  }
  Vector<unsigned> buffOutIDs;
  for (auto &buffInfo: buffInfos) {
    buffOutIDs.push_back(buffInfo.outputID);
  }
  /* preprocessing */
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
  /* calculate instance name */
  char *instOutCollect = new char[MAX_INSTANCE_LEN];
  instOutCollect[0] = '\0';
  for (auto &normOutput: normOutputVec) {
    char *subInstName = new char[2 + strlen(normOutput)];
    sprintf(subInstName, "%s_", normOutput);
    strcat(instOutCollect, subInstName);
  }
  char *fuInstName = new char[5 + strlen(instOutCollect)];
  sprintf(fuInstName, "%sinst", instOutCollect);
  if (debug_verbose) {
    printf("[fu]: %s\n", fuInstName);
  }
  /* print args for the instance */
  fprintf(chpFp, "%s(", fuInstName);
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
  return fuInstName;
}

void ChpGenerator::printSplitChp(const char *instance,
                                 const char *splitName,
                                 const char *guardName,
                                 const char *inputName,
                                 unsigned dataBW,
                                 CharPtrVec &outNameVec) {
  unsigned numOutputs = outNameVec.size();
  fprintf(chpFp, "chan(int<%u>) %s_out[%u];\n", dataBW, splitName, numOutputs);
  for (size_t i = 0; i < numOutputs; i++) {
    fprintf(chpFp, "%s_out[%zd] = %s;\n", splitName, i, outNameVec[i]);
  }
  fprintf(chpFp,
          "%s %s(%s, %s, %s_out);\n",
          instance,
          splitName,
          guardName,
          inputName,
          splitName);
}

void ChpGenerator::printMergeChp(const char *instance,
                                 const char *outName,
                                 const char *guardStr,
                                 unsigned dataBW,
                                 CharPtrVec &inNameVec) {
  const char *normOutput = getNormActIdName(outName);
  unsigned numInputs = inNameVec.size();
  fprintf(chpFp, "chan(int<%u>) %s_in[%u];\n", dataBW, normOutput, numInputs);
  for (size_t i = 0; i < numInputs; i++) {
    fprintf(chpFp, "%s_in[%zd] = %s;\n", normOutput, i, inNameVec[i]);
  }
  fprintf(chpFp,
          "%s %s_inst(%s, %s_in, %s);\n",
          instance,
          normOutput,
          guardStr,
          normOutput,
          outName);
}

void ChpGenerator::printArbiterChp(const char *instance,
                                   const char *outName,
                                   const char *coutName,
                                   unsigned dataBW,
                                   CharPtrVec &inNameVec) {
  const char *normOutput = getNormActIdName(outName);
  size_t numInputs = inNameVec.size();
  fprintf(chpFp, "chan(int<%u>) %s_in[%zd];\n", dataBW, normOutput, numInputs);
  for (size_t i = 0; i < numInputs; i++) {
    fprintf(chpFp, "%s_in[%zd] = %s;\n", normOutput, i, inNameVec[i]);
  }
  fprintf(chpFp,
          "%s %s_inst(%s_in, %s, %s);\n",
          instance,
          normOutput,
          normOutput,
          outName,
          coutName);
}

void ChpGenerator::printMixerChp(const char *instance,
                                 const char *outName,
                                 const char *coutName,
                                 unsigned dataBW,
                                 CharPtrVec &inNameVec) {
  const char *normOutput = getNormActIdName(outName);
  size_t numInputs = inNameVec.size();
  fprintf(chpFp, "chan(int<%u>) %s_in[%zd];\n", dataBW, normOutput, numInputs);
  for (size_t i = 0; i < numInputs; i++) {
    fprintf(chpFp, "%s_in[%zd] = %s;\n", normOutput, i, inNameVec[i]);
  }
  fprintf(chpFp,
          "%s %s_inst(%s_in, %s, %s);\n",
          instance,
          normOutput,
          normOutput,
          outName,
          coutName);
}

void ChpGenerator::printProcChpHeader(Process *p) {
  p->PrintHeader(chpFp, "defproc");
  fprintf(chpFp, "\n{");
  p->CurScope()->Print(chpFp);
}

void ChpGenerator::printProcDeclaration(Process *p) {
  p->PrintHeader(chpFp, "defproc");
  fprintf(chpFp, ";\n");
}

void ChpGenerator::printProcEnding() {
  fprintf(chpFp, "}\n\n");
}

void ChpGenerator::printCustomNamespace(ActNamespace *ns) {
  const char *nsName = ns->getName();
  fprintf(chpFp, "namespace %s {\n", nsName);
  ActTypeiter it(ns);
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (!p) continue;
    if (p->isExpanded()) {
      p->PrintHeader(chpFp, "defproc");
      fprintf(chpFp, ";\n");
    }
  }
  fprintf(chpFp, "}\n\n");
}

void ChpGenerator::printChpFileEnding() {
  fprintf(chpFp, "/* end of generation */\n");
  fclose(chpFp);
}

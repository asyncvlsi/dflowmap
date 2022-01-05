//
// Created by ruile on 12/31/2021.
//

#include "ChpCircuitGenerator.h"

ChpCircuitGenerator::ChpCircuitGenerator(FILE *resFp) {
  if (!resFp) {
    printf("Invalid file handler for CHP circuit generator!\n");
    exit(-1);
  }
  this->resFp = resFp;
}

void ChpCircuitGenerator::printSink(const char *inName, unsigned bitwidth) {
  if (inName == nullptr) {
    printf("sink name is NULL!\n");
    exit(-1);
  }
  const char *normalizedName = getNormActIdName(inName);
  fprintf(resFp, "sink<%u> %s_sink(%s);\n", bitwidth, normalizedName, inName);
}

void ChpCircuitGenerator::printCopy(const char *inputName,
                                    unsigned int bw,
                                    unsigned int numOut) {
  const char *normOutName = getNormActIdName(inputName);
  fprintf(resFp,
          "copy<%u,%u> %scopy(%s);\n",
          bw,
          numOut,
          normOutName,
          inputName);
  if (debug_verbose) {
    printf("[copy] %scopy\n", normOutName);
  }
}

void ChpCircuitGenerator::printEmptyLine() {
  fprintf(resFp, "\n");
}

void ChpCircuitGenerator::printInit(const char *inName,
                                    const char *outName,
                                    unsigned bitwidth,
                                    unsigned long initVal) {
//  char *inName = new char[4 + strlen(outName)];
//  sprintf(inName, "%s_in", outName);
//  fprintf(resFp, "chan(int<%u>) %s;\n", bitwidth, inName);
  fprintf(resFp,
          "init<%lu,%u> %s_inst(%s, %s);\n",
          initVal,
          bitwidth,
          outName,
          inName,
          outName);
  if (debug_verbose) {
    printf("[buff] %s_init\n", outName);
  }
}

void ChpCircuitGenerator::printOneBuff(const char *inName,
                                       const char *outName,
                                       unsigned bitwidth) {
  fprintf(resFp,
          "onebuf<%u> %s_inst(%s, %s);\n",
          bitwidth,
          outName,
          inName,
          outName);
  if (debug_verbose) {
    printf("[buff] %s_init\n", outName);
  }
}

void ChpCircuitGenerator::printChannel(const char *chanName,
                                       unsigned int bitwidth) {
  fprintf(resFp, "chan(int<%u>) %s;\n", bitwidth, chanName);
}

void ChpCircuitGenerator::printSource(const char *instance,
                                      const char *outName) {
  const char *normOutName = getNormActIdName(outName);
  fprintf(resFp, "%s %s_inst(%s);\n", instance, normOutName, outName);
}

void ChpCircuitGenerator::printBuff(Vector<BuffInfo> &buffInfos) {
  for (auto &buffInfo : buffInfos) {
    const char *finalOutput = buffInfo.finalOutput;
    unsigned bw = buffInfo.bw;
    unsigned long nBuff = buffInfo.nBuff;
    unsigned long initVal = buffInfo.initVal;
    bool hasInitVal = buffInfo.hasInitVal;
    char *prevInName = new char[strlen(finalOutput) + 7];
    sprintf(prevInName, "%s_bufIn", finalOutput);
    for (unsigned i = 0; i < nBuff - 1; i++) {
      char *chanName = new char[strlen(finalOutput) + 1024];
      sprintf(chanName, "%s_buf%u", finalOutput, i);
      printChannel(chanName, bw);
      printOneBuff(prevInName, chanName, bw);
      prevInName = chanName;
    }
    if (hasInitVal) {
      printInit(prevInName, finalOutput, bw, initVal);
    } else {
      printOneBuff(prevInName, finalOutput, bw);
    }
  }
}

void ChpCircuitGenerator::printFunc(const char *instance,
                                    StringVec &argList,
                                    UIntVec &argBWList,
                                    UIntVec &resBWList,
                                    UIntVec &outWidthList,
                                    StringVec &normalizedOutList,
                                    StringVec &outList,
                                    Vector<BuffInfo> &buffInfos) {
  /* create port for BUFF first */
  //TODO: may not need to create the port! (if there is no func body!!!)
  for (auto &buffInfo : buffInfos) {
    const char *output = buffInfo.finalOutput;
    char *actualOut = new char[7 + strlen(output)];
    sprintf(actualOut, "%s_bufIn", output);
    unsigned bw = buffInfo.bw;
    printChannel(actualOut, bw);
  }

  Vector<unsigned> buffOutIDs;
  for (auto &buffInfo : buffInfos) {
    buffOutIDs.push_back(buffInfo.outputID);
  }
  unsigned numOuts = outList.size();
  if (numOuts < 1) {
    printf("No output is found!\n");
    exit(-1);
  }
  Vector<const char*> outputVec;
  Vector<const char*> normOutputVec;
  for (unsigned i = 0; i < numOuts; i++) {
    const char *oriOut = outList[i].c_str();
    const char* normOut = getNormActIdName(oriOut);
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
  fprintf(resFp, "%s ", instance);
  if (debug_verbose) {
    printf("[fu]: ");
  }
  for (auto &normOutput : normOutputVec) {
    fprintf(resFp, "%s_", normOutput);
    if (debug_verbose) {
      printf("%s_", normOutput);
    }
  }
  fprintf(resFp, "inst(");
  if (debug_verbose) {
    printf("inst\n");
  }
  for (auto &arg : argList) {
    fprintf(resFp, "%s, ", arg.c_str());
  }
  for (unsigned i = 0; i < numOuts; i++) {
    const char* output = outputVec[i];
    fprintf(resFp, "%s", output);
    if (i == (numOuts - 1)) {
      fprintf(resFp, ");\n");
    } else {
      fprintf(resFp, ", ");
    }
  }
}

void ChpCircuitGenerator::printSplit(const char *procName,
                                     const char *splitName,
                                     const char *guardStr,
                                     const char *inputStr,
                                     unsigned guardBW,
                                     unsigned outBW,
                                     CharPtrVec &outNameVec) {
  fprintf(resFp, "%s<%d,%d> %s(", procName, guardBW, outBW, splitName);
  fprintf(resFp, "%s, %s", guardStr, inputStr);
  for (auto outName : outNameVec) {
    fprintf(resFp, ", %s", outName);
  }
  fprintf(resFp, ");\n");
}

void ChpCircuitGenerator::printMerge(const char *procName,
                                     const char *outName,
                                     const char *guardStr,
                                     unsigned guardBW,
                                     unsigned inBW,
                                     CharPtrVec &inNameVec) {
  const char *normOutput = getNormActIdName(outName);
  fprintf(resFp, "%s<%d,%d> %s_inst(", procName, guardBW, inBW, normOutput);
  fprintf(resFp, "%s, ", guardStr);
  for (auto inName : inNameVec) {
    fprintf(resFp, "%s, ", inName);
  }
  fprintf(resFp, "%s);\n", outName);
}

void ChpCircuitGenerator::printArbiter(const char *outName,
                                       const char *coutName,
                                       unsigned outBW,
                                       unsigned coutBW,
                                       int numIn,
                                       CharPtrVec &inNameVec) {
  const char *normOutput = getNormActIdName(outName);
  fprintf(resFp,
          "%s_%d<%d,%d> %s_inst(",
          Constant::ARBITER_PREFIX,
          numIn,
          outBW,
          coutBW,
          normOutput);
  for (auto inName : inNameVec) {
    fprintf(resFp, "%s, ", inName);
  }
  fprintf(resFp, "%s, %s);\n", outName, coutName);
}

void ChpCircuitGenerator::printProcHeader(Process *p) {
  p->PrintHeader(resFp, "defproc");
  fprintf(resFp, "\n{");
  p->CurScope()->Print(resFp);
}

void ChpCircuitGenerator::printProcEnding() {
  fprintf(resFp, "}\n\n");
}

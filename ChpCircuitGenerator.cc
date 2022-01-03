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

//void ChpCircuitGenerator::printInt(const char *out,
//                                   const char *normalizedOut,
//                                   unsigned long val,
//                                   unsigned outWidth,
//                                   const double metric[4]) {
//  fprintf(resFp,
//          "source<%lu,%u> %s_inst(%s);\n",
//          val,
//          outWidth,
//          normalizedOut,
//          out);
//}

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

void ChpCircuitGenerator::printInit(const char *outName,
                                    unsigned bitwidth,
                                    unsigned long initVal) {
  char *inName = new char[4 + strlen(outName)];
  sprintf(inName, "%s_in", outName);
  fprintf(resFp, "chan(int<%u>) %s;\n", bitwidth, inName);
  fprintf(resFp,
          "init<%lu,%u> %s_init(%s, %s);\n",
          initVal,
          bitwidth,
          outName,
          inName,
          outName);
  if (debug_verbose) {
    printf("[init] %s_init\n", outName);
  }
}

void ChpCircuitGenerator::printSource(const char *instance,
                                      const char *outName) {
  const char *normOutName = getNormActIdName(outName);
  fprintf(resFp, "%s %s_inst(%s);\n", instance, normOutName, outName);
}

void ChpCircuitGenerator::printFunc(const char *instance,
                                    StringVec &argList,
                                    UIntVec &argBWList,
                                    UIntVec &resBWList,
                                    UIntVec &outWidthList,
                                    StringVec &normalizedOutList,
                                    StringVec &outList,
                                    Map<unsigned, unsigned long> &initMap,
                                    Map<unsigned, unsigned long> &buffMap) {
  fprintf(resFp, "%s ", instance);
  if (debug_verbose) {
    printf("[fu]: ");
  }
  for (auto &normalizedOut : normalizedOutList) {
    fprintf(resFp, "%s_", normalizedOut.c_str());
    if (debug_verbose) {
      printf("%s_", normalizedOut.c_str());
    }
  }
  fprintf(resFp, "inst(");
  if (debug_verbose) {
    printf("inst\n");
  }
  for (auto &arg : argList) {
    fprintf(resFp, "%s, ", arg.c_str());
  }
  int numOuts = outList.size();
  if (numOuts < 1) {
    printf("No output is found!\n");
    exit(-1);
  }
  int i = 0;
  for (i = 0; i < numOuts; i++) {
    char *actualOut = new char[10240];
    const char *oriOut = outList[i].c_str();
    if (initMap.find(i) != initMap.end()) {
      sprintf(actualOut, "%s_in", oriOut);
    } else if (buffMap.find(i) != buffMap.end()) {
      sprintf(actualOut, "%s_buf0", oriOut);
    } else {
      sprintf(actualOut, "%s", oriOut);
    }
    fprintf(resFp, "%s", actualOut);
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

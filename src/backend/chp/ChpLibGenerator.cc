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

#include "ChpLibGenerator.h"

ChpLibGenerator::ChpLibGenerator(FILE *libFp, FILE *confFp) {
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    processes[i] = nullptr;
  }
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    instances[i] = nullptr;
  }
  if (!libFp || !confFp) {
    printf("Invalid file handler for CHP lib generator!\n");
    exit(-1);
  }
  fprintf(confFp, "begin sim.chp\n");
  this->libFp = libFp;
  this->confFp = confFp;

}

bool ChpLibGenerator::hasInstance(const char *instance) {
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

bool ChpLibGenerator::hasProcess(const char *process) {
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    if (processes[i] == nullptr) {
      processes[i] = process;
      return false;
    } else if (!strcmp(processes[i], process)) {
      return true;
    }
  }
  return false;
}

void ChpLibGenerator::genMemConfiguration(const char *procName) {
  fprintf(confFp, "begin mem::%s\n", procName);
  fprintf(confFp, "  begin DO\n");
  fprintf(confFp, "    int D 0\n");
  fprintf(confFp, "    int E 0\n");
  fprintf(confFp, "  end\n");
  fprintf(confFp, "  real leakage 0\n");
  fprintf(confFp, "  int area 0\n");
  fprintf(confFp, "end\n");
}

void ChpLibGenerator::createConf(const char *instance,
                                 unsigned numOutputs,
                                 double *metric) {
  if (!hasInstance(instance)) {
    if (!metric) {
      if (LOGIC_OPTIMIZER) {
        printf("We could not find metrics for %s\n", instance);
        exit(-1);
      }
      return;
    }
    fprintf(confFp, "begin %s\n", instance);
    for (int i = 0; i < numOutputs; i++) {
      fprintf(confFp, "  begin out%d\n", i);
      fprintf(confFp, "    int D %ld\n", (long) metric[2]);
      fprintf(confFp, "    int E %ld\n", (long) metric[1]);
      fprintf(confFp, "  end\n");
    }
    fprintf(confFp, "  real leakage %lde-9\n", (long) metric[0]);
    fprintf(confFp, "  int area %ld\n", (long) metric[3]);
    fprintf(confFp, "end\n");
  }
}

void ChpLibGenerator::createConf(const char *instance, double *metric) {
  if (!hasInstance(instance)) {
    if (!metric) {
      if (LOGIC_OPTIMIZER) {
        printf("We could not find metrics for %s\n", instance);
        exit(-1);
      }
      return;
    }
    fprintf(confFp, "begin %s\n", instance);
    fprintf(confFp, "  begin out\n");
    fprintf(confFp, "    int D %ld\n", (long) metric[2]);
    fprintf(confFp, "    int E %ld\n", (long) metric[1]);
    fprintf(confFp, "  end\n");
    fprintf(confFp, "  real leakage %lde-9\n", (long) metric[0]);
    fprintf(confFp, "  int area %ld\n", (long) metric[3]);
    fprintf(confFp, "end\n");
  }
}

void ChpLibGenerator::createFU(const char *procName,
                               const char *calc,
                               unsigned int numArgs,
                               unsigned int numOuts,
                               const char *instance,
                               double *fuMetric,
                               UIntVec &resBW,
                               UIntVec &outBW,
                               Map<unsigned int, unsigned int> &outRecord,
                               Vector<BuffInfo> &buffInfos) {
  if (strlen(instance) < 5) {
    printf("Invalid instance name %s\n", instance);
    exit(-1);
  }
  char *outSend = new char[10240];
  sprintf(outSend, "      ");
  int i = 0;
  Vector<unsigned> resSuffixVec;
  for (auto &outRecordIt : outRecord) {
    unsigned outID = outRecordIt.first;
    unsigned resSuffix = outRecordIt.second;
    resSuffixVec.push_back(resSuffix);
    char *subSend = new char[1024];
    if (i < numOuts - 1) {
      sprintf(subSend, "out%u!res%u, ", outID, resSuffix);
    } else {
      sprintf(subSend, "out%u!res%u;\n", outID, resSuffix);
    }
    strcat(outSend, subSend);
    i++;
  }
  char *log = new char[1500];
  sprintf(log, "      log(\"send (\", ");
  for (auto &outResSuffix : resSuffixVec) {
    char *subLog = new char[100];
    sprintf(subLog, "res%u, \",\", ", outResSuffix);
    strcat(log, subLog);
  }
  char *subLog = new char[100];
  sprintf(subLog, "\")\")");
  strcat(log, subLog);
  strcat(outSend, log);
  createFULib(procName,
              calc,
              outSend,
              numArgs,
              numOuts,
              instance,
              fuMetric,
              resBW,
              outBW);
//  createBuff(buffInfos, buffMetric);
}

void ChpLibGenerator::createFULib(const char *procName,
                                  const char *calc,
                                  const char *outSend,
                                  unsigned int numArgs,
                                  unsigned int numOuts,
                                  const char *instance,
                                  double *metric,
                                  UIntVec &resBW,
                                  UIntVec &outBW) {
  if (!hasProcess(procName)) {
    fprintf(libFp, "template<pint ");
    int i = 0;
    // generate template for input channels
    for (; i < numArgs; i++) {
      fprintf(libFp, "W%d", i);
      if (i == (numArgs - 1)) {
        fprintf(libFp, ">\n");
      } else {
        fprintf(libFp, ", ");
      }
    }
    // generate defproc
    fprintf(libFp, "defproc %s(", procName);
    for (i = 0; i < numArgs; i++) {
      fprintf(libFp, "chan?(int<W%d>)arg%d; ", i, i);
    }
    for (i = 0; i < numOuts; i++) {
      fprintf(libFp, "chan!(int<%d>) out%d", outBW[i], i);
      if (i == (numOuts - 1)) {
        fprintf(libFp, ") {\n");
      } else {
        fprintf(libFp, "; ");
      }
    }
    // define internal variables for the input channel
    for (i = 0; i < numArgs; i++) {
      fprintf(libFp, "  int<W%d> x%d;\n", i, i);
    }
    // define intermediate variables
    unsigned numRes = resBW.size();
    for (i = 0; i < numRes; i++) {
      unsigned int resbw = resBW[i];
      fprintf(libFp, "  int<%u> res%d;\n", resbw, i);
    }
    // generate CHP's actual code
    fprintf(libFp, "  chp {\n");
    fprintf(libFp, "    *[\n      ");
    for (i = 0; i < numArgs - 1; i++) {
      fprintf(libFp, "arg%d?x%d, ", i, i);
    }
    fprintf(libFp, "arg%d?x%d;\n", i, i);
    fprintf(libFp, "      log(\"receive (\", ");
    for (i = 0; i < numArgs - 1; i++) {
      fprintf(libFp, "x%d, \",\", ", i);
    }
    fprintf(libFp, "x%d, \")\");\n", i);

    fprintf(libFp, "%s", calc);
    fprintf(libFp, "%s", outSend);
    fprintf(libFp, "\n    ]\n  }\n}\n\n");
  }
  createConf(instance, numOuts, metric);
}

void ChpLibGenerator::createMerge(const char *instance, double *metric) {
  createConf(instance, metric);
}

void ChpLibGenerator::createSplit(const char *instance,
                                  double *metric,
                                  int numOutputs) {
  createConf(instance, numOutputs, metric);
}

void ChpLibGenerator::createArbiter(const char *instance, double *metric) {
  createConf(instance, metric);
}

void ChpLibGenerator::createMixer(const char *instance, double *metric) {
  createConf(instance, metric);
}

void ChpLibGenerator::createSource(const char *instance, double *metric) {
  createConf(instance, metric);
}

void ChpLibGenerator::createInit(const char *instance, double *metric) {
  createConf(instance, metric);
}

void ChpLibGenerator::createOneBuff(const char *instance, double *metric) {
  createConf(instance, metric);
}

void ChpLibGenerator::createBuff(Vector<BuffInfo> &buffInfos) {
  for (auto &buffInfo : buffInfos) {
    unsigned numBuff = buffInfo.nBuff;
    unsigned bw = buffInfo.bw;
    bool hasInitVal = buffInfo.hasInitVal;
    double *metric = buffInfo.metric;
    if ((numBuff > 1) || (!hasInitVal)) {
      char *buffInstance = new char[1024];
      sprintf(buffInstance, "dflowstd::onebuf<%u>", bw);
      createOneBuff(buffInstance, metric);
    }
    if (hasInitVal) {
      char *initInstance = new char[1024];
      unsigned long initVal = buffInfo.initVal;
      sprintf(initInstance, "dflowstd::init<%lu,%u>", initVal, bw);
      createInit(initInstance, metric);
    }
  }
}

void ChpLibGenerator::createSink(const char *instance, double *metric) {
  createConf(instance, metric);
}

void ChpLibGenerator::createCopy(const char *instance, double *metric) {
  createConf(instance, metric);
}

void ChpLibGenerator::createChpBlock(Process *p) {
  if (!p->getlang()->getchp()) {
    printf("Process %s does NOT have CHP block!\n", p->getName());
    exit(-1);
  }
  p->Print(libFp);
}

void ChpLibGenerator::printCustomNamespace(ActNamespace *ns) {
  const char *nsName = ns->getName();
  fprintf(libFp, "namespace %s {\n", nsName);
  ActTypeiter it(ns);
  bool isMEM = strcmp(nsName, "mem") == 0;
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      p->Print(libFp);
      if (isMEM) {
        const char *memProcName = p->getName();
        genMemConfiguration(memProcName);
        if (debug_verbose) {
          unsigned len = strlen(memProcName);
          char *memName = new char[len - 1];
          strncpy(memName, memProcName, len - 2);
          memName[len - 2] = '\0';
          printf("memName: %s\n", memName);
          printf("[mem]: mem_%s_inst\n", memName);
        }
      }
    }
  }
  fprintf(libFp, "}\n\n");
}

void ChpLibGenerator::printFileEnding() {
  fprintf(confFp, "end\n");
  fclose(confFp);
}

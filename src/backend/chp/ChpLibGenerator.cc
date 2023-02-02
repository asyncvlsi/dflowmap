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
#include <assert.h>

ChpLibGenerator::ChpLibGenerator(FILE *chpLibFp, FILE *chpFp,
                                 FILE *confFp) {
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    processes[i] = nullptr;
  }
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    instances[i] = nullptr;
  }
  if (!chpLibFp || !confFp) {
    printf("Invalid file handler for CHP lib generator!\n");
    exit(-1);
  }
  fprintf(confFp, "begin sim.chp\n");
  this->chpLibFp = chpLibFp;
  this->confFp = confFp;
  this->chpFp = chpFp;
}

bool ChpLibGenerator::checkAndUpdateInstance(const char *instance) {
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    if (instances[i] == nullptr) {
      instances[i] = Strdup (instance);
      return false;
    } else if (!strcmp(instances[i], instance)) {
      return true;
    }
  }
  return false;
}

bool ChpLibGenerator::checkAndUpdateProcess(const char *process) {
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

void ChpLibGenerator::printMemConfig(const char *procName) {
  fprintf(confFp, "begin mem::%s\n", procName);
  fprintf(confFp, "  begin DO\n");
  fprintf(confFp, "    int D 0\n");
  fprintf(confFp, "    int E 0\n");
  fprintf(confFp, "  end\n");
  fprintf(confFp, "  real leakage 0\n");
  fprintf(confFp, "  int area 0\n");
  fprintf(confFp, "end\n");
}

void ChpLibGenerator::printConf(double *metric,
                                const char *instance,
                                unsigned numOutputs,
                                bool exitOnMissing) {
  if (!metric) {
    if (exitOnMissing) {
      printf("We could not find metrics for %s\n", instance);
      exit(-1);
    }
    return;
  }
  if (!checkAndUpdateInstance(instance)) {
    fprintf(confFp, "begin %s\n", instance);
    for (unsigned i = 0; i < numOutputs; i++) {
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

void ChpLibGenerator::printConf(double *metric, const char *instance) {
  if (!metric) {
    if (LOGIC_OPTIMIZER) {
      printf("We could not find metrics for %s\n", instance);
      exit(-1);
    }
    return;
  }
  if (!checkAndUpdateInstance(instance)) {
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

void ChpLibGenerator::printFUChpLib(const char *instance,
                                    const char *procName,
                                    const char *calc,
                                    unsigned int numArgs,
                                    unsigned int numOuts,
                                    double *fuMetric,
                                    UIntVec &resBWList,
                                    Map<unsigned int,
                                        unsigned int> &outRecord) {
  if (strlen(instance) < 5) {
    printf("Invalid instance name %s\n", instance);
    exit(-1);
  }
  char *outSend = new char[10240];
  sprintf(outSend, "      ");
  unsigned i = 0;
  Vector<unsigned> resSuffixVec;
  for (auto &outRecordIt: outRecord) {
    unsigned outID = outRecordIt.first;
    unsigned resSuffix = outRecordIt.second;
    resSuffixVec.push_back(resSuffix);
    char *subSend = new char[1024];
    if (i < numOuts - 1) {
      sprintf(subSend, "out%u!res%u, ", outID, resSuffix);
    } else {
      sprintf(subSend, "out%u!res%u%c\n", outID, resSuffix, quiet_mode ? ' ' : ';');
    }
    strcat(outSend, subSend);
    i++;
  }
  char *log = new char[1500];
  if (!quiet_mode) {
  sprintf(log, "      log(\"send (\", ");
  for (auto &outResSuffix: resSuffixVec) {
    char *subLog = new char[100];
    sprintf(subLog, "res%u, \",\", ", outResSuffix);
    strcat(log, subLog);
  }
  char *subLog = new char[100];
  sprintf(subLog, "\")\")");
  strcat(log, subLog);
  }
  else {
  log[0] = '\0';
  }
  strcat(outSend, log);
  printFUChpLib(procName,
                calc,
                outSend,
                numArgs,
                numOuts,
                instance,
                fuMetric,
                resBWList);
}

void ChpLibGenerator::printFUChpLib(const char *procName,
                                    const char *calc,
                                    const char *outSend,
                                    unsigned int numArgs,
                                    unsigned int numOuts,
                                    const char *instance,
                                    double *metric,
                                    UIntVec &resBW) {
  if (!checkAndUpdateProcess(procName)) {
    fprintf(chpLibFp, "template<pint ");
    unsigned numTemplateVars = numArgs + numOuts;
    /* generate template for input/output channels */
    for (unsigned i = 0; i < numTemplateVars; i++) {
      fprintf(chpLibFp, "W%d", i);
      if (i == (numTemplateVars - 1)) {
        fprintf(chpLibFp, ">\n");
      } else {
        fprintf(chpLibFp, ", ");
      }
    }
    /* generate defproc */
    fprintf(chpLibFp, "defproc %s(", procName);
    for (unsigned i = 0; i < numArgs; i++) {
      fprintf(chpLibFp, "chan?(int<W%d>)in%d; ", i, i);
    }
    for (unsigned i = 0; i < numOuts; i++) {
      fprintf(chpLibFp, "chan!(int<W%d>) out%d", (i + numArgs), i);
      if (i == (numOuts - 1)) {
        fprintf(chpLibFp, ") {\n");
      } else {
        fprintf(chpLibFp, "; ");
      }
    }
    /* define internal variables for the input channel */
    for (unsigned i = 0; i < numArgs; i++) {
      fprintf(chpLibFp, "  int<W%d> x%d;\n", i, i);
    }
    /* define intermediate variables */
    unsigned numRes = resBW.size();
    for (unsigned i = 0; i < numRes; i++) {
      unsigned int resbw = resBW[i];
      //TODO: this needs to be parameterized!!!
      fprintf(chpLibFp, "  int<%u> res%d;\n", resbw, i);
    }
    /* generate CHP block */
    fprintf(chpLibFp, "  chp {\n");
    fprintf(chpLibFp, "    *[\n      ");
    for (unsigned i = 0; i < numArgs; i++) {
      if (i == numArgs - 1) {
        fprintf(chpLibFp, "in%d?x%d;\n", i, i);
      } else {
        fprintf(chpLibFp, "in%d?x%d, ", i, i);
      }
    }
    if (!quiet_mode) {
    fprintf(chpLibFp, "      log(\"receive (\", ");
    for (unsigned i = 0; i < numArgs; i++) {
      if (i == numArgs - 1) {
        fprintf(chpLibFp, "x%d, \")\");\n", i);
      } else {
        fprintf(chpLibFp, "x%d, \",\", ", i);
      }
    }
    }
    fprintf(chpLibFp, "%s", calc);
    fprintf(chpLibFp, "%s", outSend);
    fprintf(chpLibFp, "\n    ]\n  }\n}\n\n");
  }
  printConf(metric, instance, numOuts, LOGIC_OPTIMIZER);
}

static char *_add_ns (const char *s)
{
  char *tmp;
  tmp = (char *) malloc (strlen (s) + 13);
  assert (tmp);
  sprintf (tmp, "std::dflow::%s", s);
  return tmp;
}

void ChpLibGenerator::printMergeChpLib(const char *instance, double *metric) {
  char *tmp = _add_ns (instance);
  printConf(metric, tmp);
  free (tmp);
}

void ChpLibGenerator::printSplitChpLib(const char *instance,
                                       double *metric,
                                       unsigned numOutputs) {
  char *tmp = _add_ns (instance);
  printConf(metric, tmp, numOutputs, LOGIC_OPTIMIZER);
  free (tmp);
}

void ChpLibGenerator::printArbiterChpLib(const char *instance, double *metric) {
  char *tmp = _add_ns (instance);
  printConf(metric, tmp);
  free (tmp);
}

void ChpLibGenerator::printMixerChpLib(const char *instance, double *metric) {
  char *tmp = _add_ns (instance);
  printConf(metric, tmp);
  free (tmp);
}

void ChpLibGenerator::printSourceChpLib(const char *instance, double *metric) {
  char *tmp = _add_ns (instance);
  printConf(metric, tmp);
  free (tmp);
}

void ChpLibGenerator::printInitChpLib(const char *instance, double *metric) {
  char *tmp = _add_ns (instance);
  printConf(metric, tmp);
  free (tmp);
}

void ChpLibGenerator::printOneBuffChpLib(const char *instance, double *metric) {
  char *tmp = _add_ns (instance);
  printConf(metric, tmp);
  free (tmp);
}

void ChpLibGenerator::printBuffChpLib(Vector<BuffInfo> &buffInfos) {
  for (auto &buffInfo: buffInfos) {
    unsigned numBuff = buffInfo.nBuff;
    unsigned bw = buffInfo.bw;
    bool hasInitVal = buffInfo.hasInitVal;
    double *metric = buffInfo.metric;
    if ((numBuff > 1) || (!hasInitVal)) {
      char *buffInstance = new char[1024];
      sprintf(buffInstance, "lib::onebuf<%u>", bw);
      printOneBuffChpLib(buffInstance, metric);
    }
    if (hasInitVal) {
      char *initInstance = new char[1024];
      unsigned long initVal = buffInfo.initVal;
      sprintf(initInstance, "lib::init<%lu,%u>", initVal, bw);
      printInitChpLib(initInstance, metric);
    }
  }
}

void ChpLibGenerator::printSinkChpLib(const char *instance, double *metric) {
  char *tmp = _add_ns (instance);
  printConf(metric, tmp);
  free (tmp);
}

void ChpLibGenerator::printCopyChpLib(const char *instance,
                                      double *metric,
                                      unsigned numOuts) {
  char *tmp = _add_ns (instance);
  printConf(metric, tmp, numOuts, LOGIC_OPTIMIZER);
  free (tmp);
}

void ChpLibGenerator::printChpBlock(Process *p, int where) {
#if 0
  if (!p->getlang()->getchp()) {
    printf("Process %s does NOT have CHP block!\n", p->getName());
    exit(-1);
  }
#endif
  if (p->getlang()->getchp()) {
    p->Print(chpLibFp);
  }
  else {
    p->Print(chpFp);
  }
}

void ChpLibGenerator::printCustomNamespace(ActNamespace *ns) {
  const char *nsName = ns->getName();
  fprintf(chpLibFp, "namespace %s {\n", nsName);
  ActTypeiter it(ns);
  bool isMEM = strcmp(nsName, "mem") == 0;
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (!p) continue;
    if (p->isExpanded()) {
      p->Print(chpLibFp);
      if (isMEM) {
        const char *memProcName = p->getName();
        printMemConfig(memProcName);
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
  fprintf(chpLibFp, "}\n\n");
}

void ChpLibGenerator::printFileEnding() {
  fprintf(confFp, "end\n");
  fclose(confFp);
}

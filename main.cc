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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <act/act.h>
#include <act/passes.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include "lib.cc"
#include "common.h"

int getBitwidth(const char *varName);

unsigned getOpUses(const char *op);

unsigned getCopyUses(const char *copyOp);

int* getOpMetric(const char *op, int bitwidth) {
  auto opMetricsIt = opMetrics.find(op);
  if (opMetricsIt == opMetrics.end()) {
    printf("We could not find metric info for %s\n", op);
    exit(-1);
  }
  int **metrics = opMetricsIt->second;
  int row = -1;
  switch (bitwidth) {
    case 1: {
      row = 0;
      break;
    }
    case 8: {
      row = 1;
      break;
    }
    case 16: {
      row = 2;
      break;
    }
    case 32: {
      row = 3;
      break;
    }
    case 64: {
      row = 4;
      break;
    }
    default: {
      fatal_error("Unknown bitwdith %d\n", bitwidth);
    }
  }
  return metrics[row];
}

static void usage(char *name) {
  fprintf(stderr, "Usage: %s <actfile>\n", name);
  exit(1);
}

void printActId(ActId *actId) {
  if (actId) {
    const char *actName = actId->getName();
    unsigned outUses = getOpUses(actName);
    if (outUses) {
      unsigned copyUse = getCopyUses(actName);
      if (copyUse <= outUses) {
        fprintf(resFp, "%scopy.out[%u]", actId->getName(), copyUse);
      } else {
        fprintf(resFp, "%s", actId->getName());
      }
    } else {
      fprintf(resFp, "%s", actId->getName());
    }
    fprintf(resFp, ", ");
  } else {
    fprintf(resFp, "*, ");
  }
}

int getExprBitwidth(Expr *expr) {
  int type = expr->type;
  if (type == E_VAR) {
    auto actId = (ActId *) expr->u.e.l;
    return getBitwidth(actId->getName());
  } else if (type == E_INT) {
    return -1;
  } else {
    printf("Try to get bitwidth for invalid expr type: %d!\n", type);
    print_expr(stdout, expr);
    exit(-1);
  }
}

void printSink(const char* name, int bitwidth) {
  fprintf(resFp, "sink<%d> %s_sink(%s);\n", bitwidth, name, name);
  char *instance = new char[50];
  sprintf(instance, "sink<%d>", bitwidth);
  int *metric = getOpMetric("sink", bitwidth);
  createSink(instance, metric);
}

void printInt(const char *out, unsigned val, int outWidth) {
  fprintf(resFp, "source<%u,%d> %s_inst(%s);\n", val, outWidth, out, out);
  char* instance = new char[50];
  sprintf(instance, "source<%u,%d>", val, outWidth);
  int *metric = getOpMetric("source", outWidth);
  createSource(instance, metric);
}

void printExpr(Expr *expr, const char *out, int bitwidth) {
  int type = expr->type;
  if (type == E_VAR) {
    auto actId = (ActId *) expr->u.e.l;
    printActId(actId);
  } else if (type == E_INT) {
    unsigned intVal = expr->u.v;
    fprintf(resFp, "%u, ", intVal);
  } else {
    fprintf(resFp, "Try to get name for invalid expr type: %d!\n", type);
    print_expr(stdout, expr);
    exit(-1);
  }
}

void collectBitwidthInfo(Process *p) {
  ActInstiter inst(p->CurScope());
  for (inst = inst.begin(); inst != inst.end(); inst++) {
    ValueIdx *vx = *inst;
    const char *varName = vx->getName();
    int bitwidth = TypeFactory::bitWidth(vx->t);
    bitwidthMap.insert(std::make_pair(varName, bitwidth));
  }
}

void printBitwidthInfo() {
  printf("bitwidth info:\n");
  for (auto bitwidthMapIt = bitwidthMap.begin(); bitwidthMapIt != bitwidthMap.end();
       bitwidthMapIt++) {
    printf("(%s, %d) ", bitwidthMapIt->first, bitwidthMapIt->second);
  }
  printf("\n");
}

int getBitwidth(const char *varName) {
  for (auto &bitwidthMapIt : bitwidthMap) {
    const char *record = bitwidthMapIt.first;
    if (strcmp(record, varName) == 0) {
      return bitwidthMapIt.second;
    }
  }
  printf("We could not find bitwidth info for %s\n", varName);
  printBitwidthInfo();
  exit(-1);
}

void EMIT_BIN(Expr *expr, const char *out, const char *sym, int outWidth,
              const char *op, int type, const char *metricSym) {
  /* collect bitwidth info */
  Expr *lExpr = expr->u.e.l;
  int lWidth = getExprBitwidth(lExpr);
  Expr *rExpr = expr->u.e.r;
  int rWidth = getExprBitwidth(rExpr);
  int inWidth;
  if (lWidth != -1) {
    inWidth = lWidth;
  } else if (rWidth != -1) {
    inWidth = rWidth;
  } else {
    printf("Expression has constants for both operands, which should be optimized away by "
           "compiler!\n");
    print_expr(stdout, expr);
    exit(-1);
  }
  /* print */
  fprintf(resFp, "func_%s<%d,%d> %s_inst(", sym, inWidth, outWidth, out);
  printExpr(lExpr, out, inWidth);
  printExpr(rExpr, out, inWidth);
  fprintf(resFp, "%s);\n", out);
  char *instance = new char[50];
  sprintf(instance, "func_%s<%d,%d>", sym, inWidth, outWidth);
  int *metric = getOpMetric(metricSym, outWidth);
  createBinLib(sym, op, type, instance, metric);

}

void EMIT_UNI(Expr *expr, const char *out, const char *sym, const char *op, int type,
              const char *metricSym) {
  /* collect bitwidth info */
  Expr *lExpr = expr->u.e.l;
  int lWidth = getExprBitwidth(lExpr);
  if (lWidth == -1) {
    printf("UniExpr has constant for input, which should be optimized by compiler!\n");
    print_expr(stdout, expr);
    exit(-1);
  }
  fprintf(resFp, "func_%s<%d> %s_uniinst(", sym, lWidth, out);
  printExpr(lExpr, out, lWidth);
  fprintf(resFp, "%s);\n", out);
  char *instance = new char[50];
  sprintf(instance, "func_%s<%d>", sym, lWidth);
  int *metric = getOpMetric(metricSym, lWidth);
  createUniLib(sym, op, type, instance, metric);
}

unsigned getCopyUses(const char *op) {
  auto copyUsesIt = copyUses.find(op);
  if (copyUsesIt == copyUses.end()) {
    printf("We don't know how many times %s is used as COPY!\n", op);
    exit(-1);
  }
  unsigned uses = copyUsesIt->second;
  copyUsesIt->second++;
  return uses;
}

void updateOpUses(const char *op) {
  auto opUsesIt = opUses.find(op);
  if (opUsesIt == opUses.end()) {
    opUses.insert(std::make_pair(op, 0));
    copyUses.insert(std::make_pair(op, 0));
  } else {
    opUsesIt->second++;
  }
}

void printOpUses() {
  printf("OP USES:\n");
  for (auto &opUsesIt : opUses) {
    printf("(%s, %u) ", opUsesIt.first, opUsesIt.second);
  }
  printf("\n");
}

unsigned getOpUses(const char *op) {
  auto opUsesIt = opUses.find(op);
  if (opUsesIt == opUses.end()) {
    printf("We don't know how many times %s is used!\n", op);
    printOpUses();
    return 0;
//    exit(-1);
  }
  return opUsesIt->second;
}

void collectUniOpUses(Expr *expr) {
  Expr *lExpr = expr->u.e.l;
  if (lExpr->type == E_VAR) {
    auto actId = (ActId *) lExpr->u.e.l;
    updateOpUses(actId->getName());
  }
}

void collectBinOpUses(Expr *expr) {
  Expr *lExpr = expr->u.e.l;
  if (lExpr->type == E_VAR) {
    auto actId = (ActId *) lExpr->u.e.l;
    updateOpUses(actId->getName());
  }
  Expr *rExpr = expr->u.e.r;
  if (rExpr->type == E_VAR) {
    auto actId = (ActId *) rExpr->u.e.l;
    updateOpUses(actId->getName());
  }
}

void collectOpUses(Process *p) {
  listitem_t *li;
  for (li = list_first (p->getlang()->getdflow()->dflow);
       li;
       li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
      case ACT_DFLOW_FUNC: {
        Expr *expr = d->u.func.lhs;
        ActId *rhs = d->u.func.rhs;
        if (!rhs) {
          fprintf(stdout, "dflow function has empty RHS!\n");
          print_expr(stdout, expr);
          exit(-1);
        }
        int type = expr->type;
        switch (type) {
          case E_AND:
          case E_OR:
          case E_PLUS:
          case E_MINUS:
          case E_MULT:
          case E_DIV:
          case E_MOD:
          case E_LSL:
          case E_LSR:
          case E_ASR:
          case E_XOR:
          case E_LT:
          case E_GT:
          case E_LE:
          case E_GE:
          case E_EQ:
          case E_NE: {
            collectBinOpUses(expr);
            break;
          }
          case E_NOT:
          case E_UMINUS:
          case E_COMPLEMENT: {
            collectUniOpUses(expr);
            break;
          }
          case E_INT: {
            break;
          }
          case E_VAR: {
            auto actId = (ActId *) expr->u.e.l;
            updateOpUses(actId->getName());
            break;
          }
          default: {
            fatal_error("Unknown expression type %d\n", type);
          }
        }
        break;
      }
      case ACT_DFLOW_SPLIT: {
        ActId *input = d->u.splitmerge.single;
        updateOpUses(input->getName());
        ActId *guard = d->u.splitmerge.guard;
        updateOpUses(guard->getName());
        break;
      }
      case ACT_DFLOW_MERGE: {
        ActId *guard = d->u.splitmerge.guard;
        updateOpUses(guard->getName());
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs != 2) {
          fatal_error("Merge does not have TWO outputs!\n");
        }
        ActId **inputs = d->u.splitmerge.multi;
        ActId *lIn = inputs[0];
        updateOpUses(lIn->getName());
        ActId *rIn = inputs[1];
        updateOpUses(rIn->getName());
        break;
      }
      case ACT_DFLOW_MIXER: {
        fatal_error("We don't support MIXER for now!\n");
        break;
      }
      case ACT_DFLOW_ARBITER: {
        fatal_error("We don't support ARBITER for now!\n");
        break;
      }
      default: {
        fatal_error("Unknown dataflow type %d\n", d->t);
        break;
      }
    }
  }
}

void createCopyProcs() {
  fprintf(resFp, "/* copy processes */\n");
  for (auto &opUsesIt : opUses) {
    unsigned uses = opUsesIt.second;
    if (uses) {
      int N = uses + 1;
      const char *opName = opUsesIt.first;
      int bitwidth = getBitwidth(opName);
      char *instance = new char[50];
      sprintf(instance, "copy<%d,%u>", bitwidth, N);
      int *metric = getOpMetric("copy", bitwidth);
      createCopy(N, instance, metric);
      fprintf(resFp, "copy<%d,%u> %scopy(%s);\n", bitwidth, N, opName, opName);
    }
  }
  fprintf(resFp, "\n");
}

void handleProcess(Process *p) {
  const char *procName = p->getName();
  fprintf(stdout, "processing %s\n", procName);
  bool mainProc = (strcmp(procName, "main<>") == 0);
  if (!p->getlang()->getdflow()) {
    fatal_error("Process `%s': no dataflow body", p->getName());
  }
  p->PrintHeader(resFp, "defproc");
  fprintf(resFp, "\n{");
  p->CurScope()->Print(resFp);
  bitwidthMap.clear();
  opUses.clear();
  copyUses.clear();
  collectBitwidthInfo(p);
  collectOpUses(p);
  createCopyProcs();
  listitem_t *li;
  for (li = list_first (p->getlang()->getdflow()->dflow);
       li;
       li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
      case ACT_DFLOW_FUNC: {
        /* handle left hand side */
        Expr *expr = d->u.func.lhs;
        int type = expr->type;
        /* handle right hand side */
        ActId *rhs = d->u.func.rhs;
        const char *out = rhs->getName();
        int outWidth = getBitwidth(out);
        Expr *init = d->u.func.init;
        if (init) {
          if (type != E_VAR) {
            printf("We have init in ACT code, but the left side is not E_VAR!\n");
            print_expr(stdout, expr);
            printf(" => ");
            printActId(rhs);
            printf("\n");
            exit(-1);
          }
        }
        switch (type) {
          case E_AND: {
            EMIT_BIN(expr, out, "and", outWidth, "&", type, "and");
            break;
          }
          case E_OR: {
            EMIT_BIN(expr, out, "or", outWidth, "|", type, "and");
            break;
          }
          case E_NOT: {
            EMIT_UNI(expr, out, "not", "~", type, "and");
            break;
          }
          case E_PLUS: {
            EMIT_BIN(expr, out, "add", outWidth, "+", type, "add");
            break;
          }
          case E_MINUS: {
            EMIT_BIN(expr, out, "minus", outWidth, "-", type, "add");
            break;
          }
          case E_MULT: {
            EMIT_BIN(expr, out, "multi", outWidth, "*", type, "mul");
            break;
          }
          case E_DIV: {
            EMIT_BIN(expr, out, "div", outWidth, "/", type, "div");
            break;
          }
          case E_MOD: {
            EMIT_BIN(expr, out, "mod", outWidth, "%", type, "rem");
            break;
          }
          case E_LSL: {
            EMIT_BIN(expr, out, "lsl", outWidth, "<<", type, "lshift");
            break;
          }
          case E_LSR: {
            EMIT_BIN(expr, out, "lsr", outWidth, ">>", type, "lshift");
            break;
          }
          case E_ASR: {
            EMIT_BIN(expr, out, "asr", outWidth, ">>>", type, "lshift");
            break;
          }
          case E_UMINUS: {
            EMIT_UNI(expr, out, "neg", "-", type, "and");
            break;
          }
          case E_INT: {
            unsigned int val = expr->u.v;
            printInt(out, val, outWidth);
            break;
          }
          case E_VAR: {
            unsigned initVal = 0;
            auto actId = (ActId *) expr->u.e.l;
            if (init) {
              int initValType = init->type;
              if (initValType != E_INT) {
                print_expr(stdout, init);
                printf("The init value is not E_INT type!\n");
                exit(-1);
              }
              initVal = init->u.v;
              fprintf(resFp, "init<%u,%d> %s_inst(", initVal, outWidth, out);
              printActId(actId);
              char *instance = new char[50];
              sprintf(instance, "init<%u,%d>", initVal, outWidth);
              int *metric = getOpMetric("buff", outWidth);
              createInit(instance, metric);
            } else {
              fprintf(resFp, "buffer<%d> %s_inst(", outWidth, out);
              printActId(actId);
              char *instance = new char[50];
              sprintf(instance, "buffer<%d>", outWidth);
              int *metric = getOpMetric("buff", outWidth);
              createBuff(instance, metric);
            }
            fprintf(resFp, "%s);\n", out);
            break;
          }
          case E_XOR: {
            EMIT_BIN(expr, out, "xor", outWidth, "^", type, "and");
            break;
          }
          case E_LT: {
            EMIT_BIN(expr, out, "lt", outWidth, "<", type, "icmp");
            break;
          }
          case E_GT: {
            EMIT_BIN(expr, out, "gt", outWidth, ">", type, "icmp");
            break;
          }
          case E_LE: {
            EMIT_BIN(expr, out, "le", outWidth, "<=", type, "icmp");
            break;
          }
          case E_GE: {
            EMIT_BIN(expr, out, "ge", outWidth, ">=", type, "icmp");
            break;
          }
          case E_EQ: {
            EMIT_BIN(expr, out, "eq", outWidth, "=", type, "icmp");
            break;
          }
          case E_NE: {
            EMIT_BIN(expr, out, "ne", outWidth, "!=", type, "icmp");
            break;
          }
          case E_COMPLEMENT: {
            EMIT_UNI(expr, out, "comple", "~", type, "and");
            break;
          }
          default: {
            fatal_error("Unknown expression type %d\n", type);
          }
        }
//        checkAndCreateCopy(resFp, out, outWidth);
        break;
      }
      case ACT_DFLOW_SPLIT: {
        ActId *input = d->u.splitmerge.single;
        const char *inputName = input->getName();
        size_t inputSize = strlen(inputName);
        int bitwidth = getBitwidth(inputName);
        ActId **outputs = d->u.splitmerge.multi;
        ActId *lOut = outputs[0];
        ActId *rOut = outputs[1];
        const char *outName = nullptr;
        char *splitName = nullptr;
        ActId *guard = d->u.splitmerge.guard;
        const char *guardName = guard->getName();
        size_t guardSize = strlen(guardName);
        if (!lOut && !rOut) {  // split has empty target for both ports
          splitName = new char[inputSize + guardSize + 8];
          printf("no target. iS: %d, gS: %d\n", inputSize, guardSize);
          strcpy(splitName, inputName);
          strcat(splitName, "_");
          strcat(splitName, guardName);
          strcat(splitName, "_SPLIT");
        } else {
          if (lOut) {
            outName = lOut->getName();
          } else {
            outName = rOut->getName();
          }
          size_t splitSize = strlen(outName) - 1;
          splitName = new char[splitSize];
          memcpy(splitName, outName, splitSize - 1);
          splitName[splitSize - 1] = '\0';
        }
        if (!lOut) {
          char *sinkName = new char[50];
          sprintf(sinkName, "%s_L", splitName);
          printSink(sinkName, bitwidth);

//          fprintf(resFp, "sink<%d> %s_L_sink(%s_L);\n", bitwidth, splitName, splitName);
          printf("in: %s, c: %s, split: %s, L\n", inputName, guardName, splitName);
//          fprintf(resFp, "sink<%d> %s_%s_L_sink(%s_%s_L);\n", bitwidth, splitName,
//                  guardName, splitName, guardName);
        }
        if (!rOut) {
          char *sinkName = new char[50];
          sprintf(sinkName, "%s_R", splitName);
          printSink(sinkName, bitwidth);

//          fprintf(resFp, "sink<%d> %s_R_sink(%s_R);\n", bitwidth, splitName, splitName);
          printf("in: %s, c: %s, split: %s, R\n", inputName, guardName, splitName);
//          fprintf(resFp, "sink<%d> %s_%s_R_sink(%s_%s_R);\n", bitwidth, splitName,
//                  guardName, splitName, guardName);
        }
        fprintf(resFp, "control_split<%d> %s_inst(", bitwidth, splitName);
        printActId(guard);
        printActId(input);
        fprintf(resFp, "%s_L, %s_R);\n", splitName, splitName);
        char *instance = new char[50];
        sprintf(instance, "control_split<%d>", bitwidth);
        int *metric = getOpMetric("split", bitwidth);
        createSplit(instance, metric);
        break;
      }
      case ACT_DFLOW_MERGE: {
        ActId *output = d->u.splitmerge.single;
        const char *outputName = output->getName();
        int bitwidth = getBitwidth(outputName);
        fprintf(resFp, "control_merge<%d> %s_inst(", bitwidth, outputName);
        ActId *guard = d->u.splitmerge.guard;
        printActId(guard);
        ActId **inputs = d->u.splitmerge.multi;
        ActId *lIn = inputs[0];
        printActId(lIn);
        ActId *rIn = inputs[1];
        printActId(rIn);
        fprintf(resFp, "%s);\n", outputName);
        char *instance = new char[50];
        sprintf(instance, "control_merge<%d>", bitwidth);
        int *metric = getOpMetric("merge", bitwidth);
        createMerge(instance, metric);
        break;
      }
      case ACT_DFLOW_MIXER: {
        fatal_error("We don't support MIXER for now!\n");
        break;
      }
      case ACT_DFLOW_ARBITER: {
        fatal_error("We don't support ARBITER for now!\n");
        break;
      }
      default: {
        fatal_error("Unknown dataflow type %d\n", d->t);
        break;
      }
    }
  }
  if (mainProc) {
    const char *outPort = "main_out";
    int outWidth = getBitwidth(outPort);
    printSink(outPort, outWidth);
//    fprintf(resFp, "sink<%d> main_out_sink(main_out);\n", outWidth);
//    createSink();
  }
  fprintf(resFp, "}\n\n");
}

void initialize() {
  for (unsigned i = 0; i < MAX_EXPR_TYPE_NUM; i++) {
    fuIDs[i] = -1;
  }
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    processes[i] = nullptr;
  }
  for (unsigned i = 0; i < MAX_PROCESSES; i++) {
    instances[i] = nullptr;
  }
}

int main(int argc, char **argv) {
  /* initialize ACT library */
  Act::Init(&argc, &argv);

  /* some usage check */
  if (argc != 2) {
    usage(argv[0]);
  }

  /* read in the ACT file */
  Act *a = new Act(argv[1]);
  a->Expand();
  a->mangle(NULL);
  fprintf(stdout, "Processing ACT file %s!\n", argv[1]);

  /* create output file */
  char *result_file = new char[8 + strlen(argv[1])];
  strcpy(result_file, "result_");
  strcat(result_file, argv[1]);
  resFp = fopen(result_file, "w");

  char *lib_file = new char[5 + strlen(argv[1])];
  strcpy(lib_file, "lib_");
  strcat(lib_file, argv[1]);
  libFp = fopen(lib_file, "w");
  fprintf(resFp, "import \"%s\";\n\n", lib_file);

  char *conf_file = new char[6 + strlen(argv[1])];
  strcpy(conf_file, "conf_");
  strcat(conf_file, argv[1]);
  confFp = fopen(conf_file, "w");
  fprintf(confFp, "begin sim.chp\n");

  /* read in the Metric file */
  for (int i = 0; i < numOps; i++) {
    const char *op = ops[i];
    printf("Read metric file for %s\n", op);
    char *metricFile = new char[strlen(op) + 16];
    strcpy(metricFile, "metrics/");
    strcat(metricFile, op);
    strcat(metricFile, "_metric");
    std::ifstream metricFp(metricFile);
    std::string line;
    int bwCount = 0;
    int **metric = new int *[numBWs];
    while (std::getline(metricFp, line)) {
      printf("%s\n", line.c_str());
      std::istringstream iss(line);
      int metricCount = 0;
      metric[bwCount] = new int[4];
      do {
        std::string numStr;
        iss >> numStr;
        if (!numStr.empty()) {
          metric[bwCount][metricCount] = std::atoi(numStr.c_str());
          metricCount++;
        }
      } while (iss);
      if (metricCount != 4) {
        printf("%s has %d metrics for the %d-th bitwidth info!\n", metricFile, metricCount,
               bwCount);
        exit(-1);
      }
      bwCount++;
    }
    if (bwCount != numBWs) {
      printf("%s has %d bitwidth info!\n", metricFile, bwCount);
      exit(-1);
    }
    opMetrics.insert(std::make_pair(op, metric));
  }


  ActTypeiter it(a->Global());
  initialize();
  Process *procArray[MAX_PROCESSES];
  unsigned index = 0;
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    auto p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      procArray[index] = p;
      index++;
    }
  }
  for (int i = 0; i < index; i++) {
//  for (int i = index - 1; i >= 0; i--) {
    Process *p = procArray[i];
    handleProcess(p);
  }
  fprintf(resFp, "main m;\n");
  fprintf(confFp, "end\n");
  fclose(resFp);
  fclose(confFp);
  return 0;
}


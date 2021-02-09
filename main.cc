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
#include "lib.cc"

std::map<const char *, int> bitwidthMap;
/* operator, # of times it is used (if it is used for more than once, then we create COPY for it) */
std::map<const char *, unsigned> opUses;
/* copy operator, # of times it has already been used */
std::map<const char *, unsigned> copyUses;

int getBitwidth(const char *varName);

unsigned getOpUses(const char *op);

unsigned getCopyUses(const char *copyOp);

static void usage(char *name) {
  fprintf(stderr, "Usage: %s <actfile>\n", name);
  exit(1);
}

void printActId(FILE *resFp, ActId *actId, bool printComma = true) {
  if (actId) {
    const char *actName = actId->getName();
    unsigned outUses = getOpUses(actName);
    if (outUses) {
      unsigned copyUse = getCopyUses(actName);
      fprintf(resFp, "%scopy.out[%u]", actId->getName(), copyUse);
    } else {
      fprintf(resFp, "%s", actId->getName());
    }
    if (printComma) {
      fprintf(resFp, ", ");
    }
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

void printExpr(FILE *resFp, Expr *expr, bool printComma = true) {
  int type = expr->type;
  if (type == E_VAR) {
    auto actId = (ActId *) expr->u.e.l;
    printActId(resFp, actId, printComma);
  } else if (type == E_INT) {
    fprintf(resFp, "%d", expr->u.v);
    if (printComma) {
      fprintf(resFp, ",");
    }
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

void EMIT_BIN(FILE *resFp, Expr *expr, const char *out, const char *sym, int outWidth) {
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
  fprintf(resFp, "func_%s<%d, %d> %sinst(", sym, inWidth, outWidth, out);
  printExpr(resFp, lExpr);
  printExpr(resFp, rExpr);
  fprintf(resFp, "%s);\n", out);
}

void EMIT_UNI(FILE *resFp, Expr *expr, const char *out, const char *sym) {
  /* collect bitwidth info */
  Expr *lExpr = expr->u.e.l;
  int lWidth = getExprBitwidth(lExpr);
  if (lWidth == -1) {
    printf("UniExpr has constant for input, which should be optimized by compiler!\n");
    print_expr(stdout, expr);
    exit(-1);
  }
  fprintf(resFp, "func_%s<%d> %sinst(", sym, lWidth, out);
  fprintf(resFp, "%s);\n", out);
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

void checkAndCreateCopy(FILE *libFp, FILE *resFp, const char *name, int bitwidth) {
  unsigned outUses = getOpUses(name);
  if (outUses) {
    createCopy(libFp);
    fprintf(resFp, "copy<%d, %u> %scopy(%s);\n", bitwidth, outUses + 1, name, name);
  }
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

void handleProcess(FILE *resFp, FILE *libFp, Process *p) {
  const char *procName = p->getName();
  bool mainProc = (strcmp(procName, "main<>") == 0);
  if (!p->getlang()->getdflow()) {
    fatal_error("Process `%s': no dataflow body", p->getName());
  }
  p->PrintHeader(resFp, "defproc");
  fprintf(resFp, "\n{");
  p->CurScope()->Print(resFp);
  collectBitwidthInfo(p);
  collectOpUses(p);
  listitem_t *li;
  for (li = list_first (p->getlang()->getdflow()->dflow);
       li;
       li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
      case ACT_DFLOW_FUNC: {
        Expr *expr = d->u.func.lhs;
        ActId *rhs = d->u.func.rhs;
        const char *out = rhs->getName();
        int outWidth = getBitwidth(out);
        int type = expr->type;
        switch (type) {
          case E_AND: {
            EMIT_BIN(resFp, expr, out, "and", outWidth);
            createBinLib(libFp, "and", "&", type);
            break;
          }
          case E_OR: {
            EMIT_BIN(resFp, expr, out, "or", outWidth);
            createBinLib(libFp, "or", "|", type);
            break;
          }
          case E_NOT: {
            createUniLib(libFp, "not", "~", type);
            break;
          }
          case E_PLUS: {
            EMIT_BIN(resFp, expr, out, "add", outWidth);
            createBinLib(libFp, "add", "+", type);
            break;
          }
          case E_MINUS: {
            EMIT_BIN(resFp, expr, out, "minus", outWidth);
            createBinLib(libFp, "minus", "-", type);
            break;
          }
          case E_MULT: {
            EMIT_BIN(resFp, expr, out, "multi", outWidth);
            createBinLib(libFp, "multi", "*", type);
            break;
          }
          case E_DIV: {
            EMIT_BIN(resFp, expr, out, "div", outWidth);
            createBinLib(libFp, "div", "/", type);
            break;
          }
          case E_MOD: {
            EMIT_BIN(resFp, expr, out, "mod", outWidth);
            createBinLib(libFp, "mod", "%", type);
            break;
          }
          case E_LSL: {
            EMIT_BIN(resFp, expr, out, "lsl", outWidth);
            createBinLib(libFp, "lsl", "<<", type);
            break;
          }
          case E_LSR: {
            EMIT_BIN(resFp, expr, out, "lsr", outWidth);
            createBinLib(libFp, "lsr", ">>", type);
            break;
          }
          case E_ASR: {
            EMIT_BIN(resFp, expr, out, "asr", outWidth);
            createBinLib(libFp, "asr", ">>>", type);
            break;
          }
          case E_UMINUS: {
            createUniLib(libFp, "neg", "-", type);
            break;
          }
          case E_INT: {
            unsigned int val = expr->u.v;
            fprintf(resFp, "source<%u, %d> %sinst(%s);\n", val, outWidth, out, out);
            createSource(libFp);
            break;
          }
          case E_VAR: {
            bool hasInitVal = false;
            unsigned initVal = 0;
            Expr *init = d->u.func.init;
            if (init) {
              hasInitVal = true;
              int initValType = init->type;
              if (initValType != E_INT) {
                print_expr(stdout, init);
                printf("The init value is not E_INT type!\n");
                exit(-1);
              }
              initVal = init->u.v;
            }
            fprintf(resFp, "buffer<%d> %s_inst(", outWidth, out);
            auto actId = (ActId *) expr->u.e.l;
            printActId(resFp, actId);
            fprintf(resFp, "%s);\n", out);
            createBuff(libFp, hasInitVal, initVal);
            break;
          }
          case E_XOR: {
            EMIT_BIN(resFp, expr, out, "xor", outWidth);
            createBinLib(libFp, "xor", "^", type);
            break;
          }
          case E_LT: {
            EMIT_BIN(resFp, expr, out, "lt", outWidth);
            createBinLib(libFp, "lt", "<", type);
            break;
          }
          case E_GT: {
            EMIT_BIN(resFp, expr, out, "gt", outWidth);
            createBinLib(libFp, "gt", ">", type);
            break;
          }
          case E_LE: {
            EMIT_BIN(resFp, expr, out, "le", outWidth);
            createBinLib(libFp, "le", "<=", type);
            break;
          }
          case E_GE: {
            EMIT_BIN(resFp, expr, out, "ge", outWidth);
            createBinLib(libFp, "ge", ">=", type);
            break;
          }
          case E_EQ: {
            EMIT_BIN(resFp, expr, out, "eq", outWidth);
            createBinLib(libFp, "eq", "==", type);
            break;
          }
          case E_NE: {
            EMIT_BIN(resFp, expr, out, "ne", outWidth);
            createBinLib(libFp, "ne", "!=", type);
            break;
          }
          case E_COMPLEMENT: {
            createUniLib(libFp, "comple", "~", type);
            break;
          }
          default: {
            fatal_error("Unknown expression type %d\n", type);
          }
        }
        checkAndCreateCopy(libFp, resFp, out, outWidth);
        break;
      }
      case ACT_DFLOW_SPLIT: {
        ActId *input = d->u.splitmerge.single;
        int bitwidth = getBitwidth(input->getName());
        ActId **outputs = d->u.splitmerge.multi;
        ActId *lOut = outputs[0];
        ActId *rOut = outputs[1];
        const char *outName;
        /**
         * if SPLIT has empty left outPort, then emptyPort = 1;
         * if SPLIT has empty right outPort, then emptyPort = 2;
         * o/w, emptyPort = 0;
         */
        int emptyPort = 0;
        if (!lOut) {
          emptyPort = 1;
          if (!rOut) {
            fatal_error("SPLIT has null outputs for both ports!\n");
          }
          outName = rOut->getName();
        } else {
          outName = lOut->getName();
          if (!rOut) {
            emptyPort = 2;
          }
        }
        size_t splitSize = strlen(outName) - 2;
        char *splitName = new char[splitSize];
        memcpy(splitName, outName, splitSize);
        splitName[splitSize] = '\0';
        if (emptyPort == 1) {
          fprintf(resFp, "sink<%d> %s_sink(%s_L);\n", bitwidth, splitName, splitName);
        } else if (emptyPort == 2) {
          fprintf(resFp, "sink<%d> %s_sink(%s_R);\n", bitwidth, splitName, splitName);
        }
        fprintf(resFp, "control_split<%d> %sinst(", bitwidth, splitName);
        ActId *guard = d->u.splitmerge.guard;
        printActId(resFp, guard);
        printActId(resFp, input);
        fprintf(resFp, "%s_L, %s_R);\n", splitName, splitName);
        createSplit(libFp);
        if (lOut) {
          checkAndCreateCopy(libFp, resFp, lOut->getName(), bitwidth);
        }
        if (rOut) {
          checkAndCreateCopy(libFp, resFp, rOut->getName(), bitwidth);
        }

        break;
      }
      case ACT_DFLOW_MERGE: {
        ActId *output = d->u.splitmerge.single;
        const char *outputName = output->getName();
        int bitwidth = getBitwidth(outputName);
        fprintf(resFp, "control_merge<%d> %sinst(", bitwidth, outputName);
        ActId *guard = d->u.splitmerge.guard;
        printActId(resFp, guard);
        ActId **inputs = d->u.splitmerge.multi;
        ActId *lIn = inputs[0];
        printActId(resFp, lIn);
        ActId *rIn = inputs[1];
        printActId(resFp, rIn);
        bool printComma = false;
        printActId(resFp, output, printComma);
        fprintf(resFp, ");\n");
        createMerge(libFp);
        checkAndCreateCopy(libFp, resFp, outputName, bitwidth);
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
    fprintf(resFp, "sink<%d> main_out_sink(main_out);\n", outWidth);
    createSink(libFp);
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
  char *result_file = new char[8 + strlen(argv[1])];
  strcpy(result_file, "result_");
  strcat(result_file, argv[1]);
  FILE *resFp = fopen(result_file, "w");
  char *lib_file = new char[5 + strlen(argv[1])];
  strcpy(lib_file, "lib_");
  strcat(lib_file, argv[1]);
  FILE *libFp = fopen(lib_file, "w");
  fprintf(resFp, "import \"%s\";\n\n", lib_file);
  ActTypeiter it(a->Global());
  initialize();
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    Process *p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      const char *procName = p->getName();
      fprintf(stdout, "processing %s\n", procName);
      handleProcess(resFp, libFp, p);
    }
  }
  fprintf(resFp, "main m;\n");
  fclose(resFp);
  fclose(libFp);
  return 0;
}


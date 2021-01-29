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

static void usage(char *name) {
  fprintf(stderr, "Usage: %s <actfile>\n", name);
  exit(1);
}

void printActId(FILE *resFp, ActId *actId, bool printComma = true) {
  if (actId) {
    fprintf(resFp, "%s", actId->getName());
    if (printComma) {
      fprintf(resFp, ", ");
    }
  } else {
    fprintf(resFp, "*, ");
  }
}

void printExpr(FILE *resFp, Expr *expr, bool printComma = true) {
  int type = expr->type;
  if (type == E_VAR) {
    printActId(resFp, (ActId *) expr->u.e.l, printComma);
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

void EMIT_BIN(FILE *resFp, Expr *expr, const char *out, const char *sym,
              unsigned instCnt) {
  fprintf(resFp, "func_%s i%u(", sym, instCnt);
  Expr *lExpr = expr->u.e.l;
  printExpr(resFp, lExpr);
  Expr *rExpr = expr->u.e.r;
  printExpr(resFp, rExpr);
  fprintf(resFp, "%s);\n", out);
}

void EMIT_UNI(FILE *resFp, Expr *expr, const char *out, const char *sym,
              unsigned instCnt) {
  fprintf(resFp, "func_%s i%u(", sym, instCnt);
  printExpr(resFp, expr->u.e.l);
  fprintf(resFp, "%s);\n", out);
}

void handleProcess(FILE *resFp, FILE *libFp, Process *p) {
  if (!p->getlang()->getdflow()) {
    fatal_error("Process `%s': no dataflow body", p->getName());
  }
  unsigned instCnt = 0;
  p->PrintHeader(resFp, "defproc");
  fprintf(resFp, "\n{");
  p->CurScope()->Print(resFp);
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
          fprintf(resFp, "dflow function has empty RHS!\n");
          print_expr(stdout, expr);
          exit(-1);
        }
        const char *out = rhs->getName();
        int type = expr->type;
        switch (type) {
          case E_AND: {
            EMIT_BIN(resFp, expr, out, "and", instCnt);
            createBinLib(libFp, "and", "&", type);
            break;
          }
          case E_OR: {
            EMIT_BIN(resFp, expr, out, "or", instCnt);
            createBinLib(libFp, "or", "|", type);
            break;
          }
          case E_NOT: {
            EMIT_UNI(resFp, expr, out, "not", instCnt);
            createUniLib(libFp, "not", "~", type);
            break;
          }
          case E_PLUS: {
            EMIT_BIN(resFp, expr, out, "add", instCnt);
            createBinLib(libFp, "add", "+", type);
            break;
          }
          case E_MINUS: {
            EMIT_BIN(resFp, expr, out, "minus", instCnt);
            createBinLib(libFp, "minus", "-", type);
            break;
          }
          case E_MULT: {
            EMIT_BIN(resFp, expr, out, "multi", instCnt);
            createBinLib(libFp, "multi", "*", type);
            break;
          }
          case E_DIV: {
            EMIT_BIN(resFp, expr, out, "div", instCnt);
            createBinLib(libFp, "div", "/", type);
            break;
          }
          case E_MOD: {
            EMIT_BIN(resFp, expr, out, "mod", instCnt);
            createBinLib(libFp, "mod", "%", type);
            break;
          }
          case E_LSL: {
            EMIT_BIN(resFp, expr, out, "lsl", instCnt);
            createBinLib(libFp, "lsl", "<<", type);
            break;
          }
          case E_LSR: {
            EMIT_BIN(resFp, expr, out, "lsr", instCnt);
            createBinLib(libFp, "lsr", ">>", type);
            break;
          }
          case E_ASR: {
            EMIT_BIN(resFp, expr, out, "asr", instCnt);
            createBinLib(libFp, "asr", ">>>", type);
            break;
          }
          case E_UMINUS: {
            EMIT_UNI(resFp, expr, out, "neg", instCnt);
            createUniLib(libFp, "neg", "-", type);
            break;
          }
          case E_INT: {
            unsigned int val = expr->u.v;
            fprintf(resFp, "source<%u> i%u(%s);\n", val, instCnt, out);
            createSource(libFp);
            break;
          }
          case E_VAR: {
            fprintf(resFp, "buffer<");
            Expr *buff = d->u.func.nbufs;
            if (!buff) {
              fprintf(resFp, "1");
            } else {
              print_expr(resFp, buff);
            }
            Expr *init = d->u.func.init;
            if (init) {
              fprintf(resFp, ", ");
              print_expr(resFp, init);
            }
            fprintf(resFp, "> i%u(", instCnt);
            auto actId = (ActId *) expr->u.e.l;
            printActId(resFp, actId);
            fprintf(resFp, "%s);\n", out);
            createBuff(libFp);
            break;
          }
          case E_XOR: {
            EMIT_BIN(resFp, expr, out, "xor", instCnt);
            createBinLib(libFp, "xor", "^", type);
            break;
          }
          case E_LT: {
            EMIT_BIN(resFp, expr, out, "lt", instCnt);
            createBinLib(libFp, "lt", "<", type);
            break;
          }
          case E_GT: {
            EMIT_BIN(resFp, expr, out, "gt", instCnt);
            createBinLib(libFp, "gt", ">", type);
            break;
          }
          case E_LE: {
            EMIT_BIN(resFp, expr, out, "le", instCnt);
            createBinLib(libFp, "le", "<=", type);
            break;
          }
          case E_GE: {
            EMIT_BIN(resFp, expr, out, "ge", instCnt);
            createBinLib(libFp, "ge", ">=", type);
            break;
          }
          case E_EQ: {
            EMIT_BIN(resFp, expr, out, "eq", instCnt);
            createBinLib(libFp, "eq", "==", type);
            break;
          }
          case E_NE: {
            EMIT_BIN(resFp, expr, out, "ne", instCnt);
            createBinLib(libFp, "ne", "!=", type);
            break;
          }
          case E_COMPLEMENT: {
            EMIT_UNI(resFp, expr, out, "comple", instCnt);
            createUniLib(libFp, "comple", "~", type);
            break;
          }
          default: {
            fatal_error("Unknown expression type %d\n", type);
          }
        }
        break;
      }
      case ACT_DFLOW_SPLIT: {
        fprintf(resFp, "control_split i%d(", instCnt);
        ActId *guard = d->u.splitmerge.guard;
        printActId(resFp, guard);
        ActId *input = d->u.splitmerge.single;
        printActId(resFp, input);
        int numOutputs = d->u.splitmerge.nmulti;
        if (numOutputs != 2) {
          fatal_error("Split i%d does not have TWO outputs!\n", instCnt);
        }
        ActId **outputs = d->u.splitmerge.multi;
        ActId *lOut = outputs[0];
        printActId(resFp, lOut);
        ActId *rOut = outputs[1];
        bool printComma = false;
        printActId(resFp, rOut, printComma);
        fprintf(resFp, ");\n");
        createSplit(libFp);
        break;
      }
      case ACT_DFLOW_MERGE: {
        fprintf(resFp, "control_merge i%d(", instCnt);
        ActId *guard = d->u.splitmerge.guard;
        printActId(resFp, guard);
        ActId *output = d->u.splitmerge.single;
        printActId(resFp, output);
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs != 2) {
          fatal_error("Merge i%d does not have TWO outputs!\n", instCnt);
        }
        ActId **inputs = d->u.splitmerge.multi;
        ActId *lIn = inputs[0];
        printActId(resFp, lIn);
        ActId *rIn = inputs[1];
        bool printComma = false;
        printActId(resFp, rIn, printComma);
        fprintf(resFp, ");\n");
        createMerge(libFp);
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
    instCnt++;
  }
  fprintf(resFp, "}\n\n");
}

void initialize() {
  for (unsigned i = 0; i < MAX_OP_TYPE_NUM; i++) {
    opTypes[i] = -1;
  }
  for (unsigned i = 0; i < MAX_EXPR_TYPE_NUM; i++) {
    exprTypes[i] = -1;
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
      fprintf(stdout, "processing %s\n", p->getName());
      handleProcess(resFp, libFp, p);
    }
  }
  fclose(resFp);
  fclose(libFp);
  return 0;
}

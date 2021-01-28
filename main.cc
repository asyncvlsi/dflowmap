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

static void usage(char *name) {
  fprintf(stderr, "Usage: %s <actfile> <process>\n", name);
  exit(1);
}

void printActId(FILE *fp, ActId *actId, bool printComma = true) {
  if (actId) {
    fprintf(fp, "%s", actId->getName());
    if (printComma) {
      fprintf(fp, ", ");
    }
  } else {
    fprintf(fp, "*, ");
  }
}

void printExpr(FILE *fp, Expr *expr, bool printComma = true) {
  int type = expr->type;
  if (type == E_VAR) {
    printActId(fp, (ActId *) expr->u.e.l, printComma);
  } else if (type == E_INT) {
    fprintf(fp, "%d", expr->u.v);
    if (printComma) {
      fprintf(fp, ",");
    }
  } else {
    fprintf(fp, "Try to get name for invalid expr type: %d!\n", type);
    print_expr(stdout, expr);
    exit(-1);
  }
}

void EMIT_BIN(FILE *fp, Expr *expr, const char *out, const char *sym, unsigned instCnt) {
  fprintf(fp, "func_%s i%u(", sym, instCnt);
  Expr *lExpr = expr->u.e.l;
  printExpr(fp, lExpr);
  Expr *rExpr = expr->u.e.r;
  printExpr(fp, rExpr);
  fprintf(fp, "%s);\n", out);
}

void EMIT_UNI(FILE *fp, Expr *expr, const char *out, const char *sym, unsigned instCnt) {
  fprintf(fp, "func_%s i%u(", sym, instCnt);
  printExpr(fp, expr->u.e.l);
  fprintf(fp, "%s);\n", out);
}

void handleProcess(FILE *fp, Process *p) {
  if (!p->getlang()->getdflow()) {
    fatal_error("Process `%s': no dataflow body", p->getName());
  }
  unsigned instCnt = 0;
  p->PrintHeader(fp, "defproc");
  fprintf(fp, "\n{");
  p->CurScope()->Print(fp);
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
          fprintf(fp, "dflow function has empty RHS!\n");
          print_expr(stdout, expr);
          exit(-1);
        }
        const char *out = rhs->getName();
        int type = expr->type;
        switch (type) {
          case E_AND: {
            EMIT_BIN(fp, expr, out, "and", instCnt);
            break;
          }
          case E_OR: {
            EMIT_BIN(fp, expr, out, "or", instCnt);
            break;
          }
          case E_NOT: {
            EMIT_UNI(fp, expr, out, "not", instCnt);
            break;
          }
          case E_PLUS: {
            EMIT_BIN(fp, expr, out, "add", instCnt);
            break;
          }
          case E_MINUS: {
            EMIT_BIN(fp, expr, out, "minus", instCnt);
            break;
          }
          case E_MULT: {
            EMIT_BIN(fp, expr, out, "multi", instCnt);
            break;
          }
          case E_DIV: {
            EMIT_BIN(fp, expr, out, "div", instCnt);
            break;
          }
          case E_MOD: {
            EMIT_BIN(fp, expr, out, "mod", instCnt);
            break;
          }
          case E_LSL: {
            EMIT_BIN(fp, expr, out, "lsl", instCnt);
            break;
          }
          case E_LSR: {
            EMIT_BIN(fp, expr, out, "lsr", instCnt);
            break;
          }
          case E_ASR: {
            EMIT_BIN(fp, expr, out, "asr", instCnt);
            break;
          }
          case E_UMINUS: {
            EMIT_UNI(fp, expr, out, "neg", instCnt);
            break;
          }
          case E_INT: {
            unsigned int val = expr->u.v;
            fprintf(fp, "source<%u> i%u(%s);\n", val, instCnt, out);
            break;
          }
          case E_VAR: {
            fprintf(fp, "buffer<");
            Expr *buff = d->u.func.nbufs;
            print_expr(fp, buff);
            fprintf(fp, ", ");
            Expr *init = d->u.func.init;
            print_expr(fp, init);
            fprintf(fp, "> i%u(", instCnt);
            auto actId = (ActId *) expr->u.e.l;
            printActId(fp, actId);
            fprintf(fp, "%s);\n", out);
            break;
          }
          case E_XOR: {
            EMIT_BIN(fp, expr, out, "xor", instCnt);
            break;
          }
          case E_LT: {
            EMIT_BIN(fp, expr, out, "lt", instCnt);
            break;
          }
          case E_GT: {
            EMIT_BIN(fp, expr, out, "gt", instCnt);
            break;
          }
          case E_LE: {
            EMIT_BIN(fp, expr, out, "le", instCnt);
            break;
          }
          case E_GE: {
            EMIT_BIN(fp, expr, out, "ge", instCnt);
            break;
          }
          case E_EQ: {
            EMIT_BIN(fp, expr, out, "eq", instCnt);
            break;
          }
          case E_NE: {
            EMIT_BIN(fp, expr, out, "ne", instCnt);
            break;
          }
          case E_COMPLEMENT: {
            EMIT_UNI(fp, expr, out, "comple", instCnt);
            break;
          }
          default: {
            fatal_error("Unknown expression type %d\n", type);
          }
        }
        break;
      }
      case ACT_DFLOW_SPLIT: {
        fprintf(fp, "control_split i%d(", instCnt);
        ActId *guard = d->u.splitmerge.guard;
        printActId(fp, guard);
        ActId *input = d->u.splitmerge.single;
        printActId(fp, input);
        int numOutputs = d->u.splitmerge.nmulti;
        if (numOutputs != 2) {
          fatal_error("Split i%d does not have TWO outputs!\n", instCnt);
        }
        ActId **outputs = d->u.splitmerge.multi;
        ActId *lOut = outputs[0];
        printActId(fp, lOut);
        ActId *rOut = outputs[1];
        bool printComma = false;
        printActId(fp, rOut, printComma);
        fprintf(fp, ");\n");
        break;
      }
      case ACT_DFLOW_MERGE: {
        fprintf(fp, "control_merge i%d(", instCnt);
        ActId *guard = d->u.splitmerge.guard;
        printActId(fp, guard);
        ActId *output = d->u.splitmerge.single;
        printActId(fp, output);
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs != 2) {
          fatal_error("Merge i%d does not have TWO outputs!\n", instCnt);
        }
        ActId **inputs = d->u.splitmerge.multi;
        ActId *lIn = inputs[0];
        printActId(fp, lIn);
        ActId *rIn = inputs[1];
        bool printComma = false;
        printActId(fp, rIn, printComma);
        fprintf(fp, ");\n");
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
      default:
        fatal_error("Unknown dataflow type %d\n", d->t);
        break;
    }
    instCnt++;
  }
  fprintf(fp, "}\n\n");
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
  char *result_file = new char;
  strcpy(result_file, "result_");
  strcat(result_file, argv[1]);
  FILE *fp = fopen(result_file, "w");
  ActTypeiter it(a->Global());
  for (it = it.begin(); it != it.end(); it++) {
    Type *t = *it;
    Process *p = dynamic_cast<Process *>(t);
    if (p->isExpanded()) {
      fprintf(stdout, "processing %s\n", p->getName());
      handleProcess(fp, p);
    }
  }
  return 0;
}

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

#ifndef DFLOWMAP_SRC_CORE_NAMEGENERATOR_H_
#define DFLOWMAP_SRC_CORE_NAMEGENERATOR_H_

#include <act/act.h>
#include <act/lang.h>
#include "src/common/common.h"
#include "src/common/Constant.h"

class NameGenerator {
 public:
  static const char *genMergeInstName(unsigned guardBW,
                                      unsigned inBW,
                                      int numInputs);

  static const char *genMixerInstName(unsigned inBW, int numInputs);

  static const char *genArbiterInstName(unsigned guardBW,
                                        unsigned inBW,
                                        int numInputs);

  static const char *genSplitInstName(unsigned guardBW,
                                      unsigned outBW,
                                      int numOut);

  static const char *genCopyInstName(unsigned bw, unsigned numOut);

  static const char *genSinkInstName(unsigned bw);

  static const char *genSourceInstName(unsigned long val, unsigned bitwidth);

  static const char *genExprName(Expr *expr) {
    list_t *in_expr_list = list_new();
    parseExpr(expr, in_expr_list);
    return act_expr_to_string(in_expr_list, expr);
  }

 private:
  static void parseQueryExpr(const Expr *expr, list_t *in_expr_list) {
    Expr *cExpr = expr->u.e.l;
    parseExpr(cExpr, in_expr_list);
    Expr *lExpr = expr->u.e.r->u.e.l;
    parseExpr(lExpr, in_expr_list);
    Expr *rExpr = expr->u.e.r->u.e.r;
    parseExpr(rExpr, in_expr_list);
  }

  static void parseBinExpr(const Expr *expr, list_t *in_expr_list) {
    Expr *lExpr = expr->u.e.l;
    parseExpr(lExpr, in_expr_list);
    Expr *rExpr = expr->u.e.r;
    parseExpr(rExpr, in_expr_list);
  }

  static void parseUniExpr(const Expr *expr, list_t *in_expr_list) {
    const Expr *lExpr = expr->u.e.l;
    parseExpr(lExpr, in_expr_list);
  }

  static void parseExpr(const Expr *expr, list_t *in_expr_list) {
    int type = expr->type;
    switch (type) {
      case E_VAR: {
        //TODO: duplicate variables in the expr, e.g., "x + x * y"
        bool duplicate = false;
        listitem_t *li;
        for (li = list_first (in_expr_list); li; li = list_next (li)) {
          if ((long) list_value(li) == (long) expr) {
            duplicate = true;
            break;
          }
        }
        if (!duplicate) list_append(in_expr_list, expr);
        break;
      }
      case E_NOT:
      case E_UMINUS:
      case E_COMPLEMENT: {
        parseUniExpr(expr, in_expr_list);
        break;
      }
      case E_QUERY: {
        parseQueryExpr(expr, in_expr_list);
        break;
      }
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
        parseBinExpr(expr, in_expr_list);
        break;
      }
      case E_INT: {
        break;
      }
      case E_BUILTIN_INT:
      case E_BUILTIN_BOOL: {
        Expr *lExpr = expr->u.e.l;
        parseExpr(lExpr, in_expr_list);
        break;
      }
      default: {
        printf("Encountered unsupported expr (%d) for generating name\n", type);
        print_expr(stdout, expr);
        printf("\n");
        exit(-1);
      }
    }
  }

};

#endif //DFLOWMAP_SRC_CORE_NAMEGENERATOR_H_

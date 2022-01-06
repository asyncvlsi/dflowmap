//
// Created by ruile on 12/31/2021.
//

#ifndef DFLOWMAP__CHPBACKEND_H_
#define DFLOWMAP__CHPBACKEND_H_

#include "ChpCircuitGenerator.h"
#include "ChpLibGenerator.h"

class ChpBackend {
 public:
  ChpBackend(ChpCircuitGenerator *circuitGenerator,
             ChpLibGenerator *libGenerator) {
    this->circuitGenerator = circuitGenerator;
    this->libGenerator = libGenerator;
  }

  void createCopyProcs(const char *inName,
                       unsigned bw,
                       unsigned numOut,
                       double *metric);

  void printSink(const char *inName, unsigned bw, double metric[4]);

  void printBuff(Vector<BuffInfo> &buffInfos);

  void printChannel(const char *chanName, unsigned bitwidth);

  void printSource(const char *outName,
                   const char *instance,
                   double metric[4]);

  void printFU(const char *procName,
               const char *instName,
               StringVec &argList,
               UIntVec &argBWList,
               UIntVec &resBWList,
               UIntVec &outBWList,
               const char *calc,
               StringVec &outSendStr,
               IntVec &outResSuffixs,
               StringVec &normalizedOutList,
               StringVec &outList,
               Vector<BuffInfo> &buffInfos,
               double fuMetric[4]);

  void printSplit(const char *procName,
                  const char *splitName,
                  const char *guardStr,
                  const char *inputStr,
                  unsigned guardBW,
                  unsigned outBW,
                  CharPtrVec &outNameVec,
                  const char *instance,
                  int numOut,
                  double metric[4]);

  void printMerge(const char *procName,
                  const char *outName,
                  const char *guardStr,
                  unsigned guardBW,
                  unsigned inBW,
                  CharPtrVec &inNameVec,
                  const char *instance,
                  int numIn,
                  double metric[4]);

  void printArbiter(const char *procName,
                    const char *instance,
                    const char *outName,
                    const char *coutName,
                    unsigned outBW,
                    unsigned coutBW,
                    int numIn,
                    CharPtrVec &inNameVec,
                    double *metric);

  void printProcHeader(Process *p);

  void printProcEnding();

  void createChpBlock(Process *p);

  void prepareQueryExprForOpt(const char *cexpr_name,
                              int cexpr_type,
                              const char *lexpr_name,
                              int lexpr_type,
                              const char *rexpr_name,
                              int rexpr_type,
                              const char *expr_name,
                              int expr_type,
                              int body_expr_type,
                              unsigned bw,
                              Map<const char *, Expr *> &exprMap,
                              StringMap<unsigned> &hiddenBW,
                              Map<Expr *, Expr *> &hiddenExprs) {
    Expr *cExpr = getExprFromName(cexpr_name, exprMap, false, cexpr_type);
    Expr *lExpr = getExprFromName(lexpr_name, exprMap, false, lexpr_type);
    Expr *rExpr = getExprFromName(rexpr_name, exprMap, false, rexpr_type);
    Expr *rhs = getExprFromName(expr_name, exprMap, false, E_VAR);
    Expr *expr = new Expr;
    expr->type = expr_type;
    expr->u.e.l = cExpr;
    Expr *body_expr = new Expr;
    body_expr->type = body_expr_type;
    body_expr->u.e.l = lExpr;
    body_expr->u.e.r = rExpr;
    expr->u.e.r = body_expr;
    hiddenBW.insert({expr_name, bw});
    hiddenExprs.insert({rhs, expr});
    if (debug_verbose) {
      printf("rhs: ");
      print_expr(stdout, rhs);
      printf(", resExpr: ");
      print_expr(stdout, expr);
      printf(".\n");
    }
  }

  void prepareBinExprForOpt(const char *lexpr_name,
                            int lexpr_type,
                            const char *rexpr_name,
                            int rexpr_type,
                            const char *expr_name,
                            int expr_type,
                            unsigned bw,
                            Map<const char *, Expr *> &exprMap,
                            StringMap<unsigned> &hiddenBW,
                            Map<Expr *, Expr *> &hiddenExprs) {
    Expr *lExpr = getExprFromName(lexpr_name, exprMap, false, lexpr_type);
    Expr *rExpr = getExprFromName(rexpr_name, exprMap, false, rexpr_type);
    Expr *rhs = getExprFromName(expr_name, exprMap, false, E_VAR);
    Expr *expr = new Expr;
    expr->type = expr_type;
    expr->u.e.l = lExpr;
    expr->u.e.r = rExpr;
    hiddenBW.insert({expr_name, bw});
    hiddenExprs.insert({rhs, expr});
    if (debug_verbose) {
      printf("rhs: ");
      print_expr(stdout, rhs);
      printf(", resExpr: ");
      print_expr(stdout, expr);
      printf(".\n");
    }
  }

  void prepareUniExprForOpt(const char *lexpr_name,
                            int lexpr_type,
                            const char *expr_name,
                            int expr_type,
                            unsigned bw,
                            Map<const char *, Expr *> &exprMap,
                            StringMap<unsigned> &hiddenBW,
                            Map<Expr *, Expr *> &hiddenExprs) {
    Expr *lExpr = getExprFromName(lexpr_name, exprMap, false, lexpr_type);
    Expr *rhs = getExprFromName(expr_name, exprMap, false, E_VAR);
    Expr *expr = new Expr;
    expr->type = expr_type;
    expr->u.e.l = lExpr;
    hiddenBW.insert({expr_name, bw});
    hiddenExprs.insert({rhs, expr});
    if (debug_verbose) {
      printf("rhs: ");
      print_expr(stdout, rhs);
      printf(", resExpr: ");
      print_expr(stdout, expr);
      printf(".\n");
    }
  }

  void handleUniExpr(const char *op,
                            const char *exprName,
                            const int result_suffix,
                            char* calc,
                            unsigned result_bw,
                            UIntVec &resBWList) {
    char *curCal = new char[128 + strlen(exprName)];
    sprintf(curCal, "      res%d := %s %s;\n", result_suffix, op, exprName);
    if (debug_verbose) {
      printf("%s\n", curCal);
    }
    strcat(calc, curCal);
    if (result_bw == 0) {
      printf("for %s, result_bw is 0!\n", exprName);
      exit(-1);
    }
    resBWList.push_back(result_bw);
  }

 private:
  ChpCircuitGenerator *circuitGenerator;
  ChpLibGenerator *libGenerator;
};

#endif //DFLOWMAP__CHPBACKEND_H_

#ifndef DFLOWMAP_CHPGENERATOR_H
#define DFLOWMAP_CHPGENERATOR_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <act/act.h>
#include <act/passes.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <act/lang.h>
#include <act/types.h>
#include <act/expr.h>
#include <algorithm>
#include <act/act.h>
#include "ChpProcGenerator.h"
#include "Metrics.h"
#include "common.h"

#if LOGIC_OPTIMIZER
#include <act/expropt.h>
#endif

class ChpGenerator : public ActPass {
public:
  ChpGenerator(Act *a, const char *name, Metrics *metrics);

  void handleProcess(FILE *resFp, FILE *libFp, FILE *confFp, Process *p);

  int searchStringVec(StringVec &strVec, const char *str);

  const char *removeDot(const char *src);

  const char *getActIdOrCopyName(Scope *sc, ActId *actId);

  void
  printSink(FILE *resFp, FILE *libFp, FILE *confFp, const char *name, unsigned bitwidth);

  void printInt(FILE *resFp, FILE *libFp, FILE *confFp, const char *out,
                const char *normalizedOut, unsigned long val, unsigned outWidth);

  void collectBitwidthInfo(Process *p);

  void printBitwidthInfo();

  unsigned getActIdBW(ActId *actId, Process *p);

  unsigned getBitwidth(act_connection *actConnection);

  void getCurProc(const char *str, char *val);

  const char *
  EMIT_QUERY(Scope *sc, Expr *expr, const char *sym, const char *op, int type,
             const char *metricSym,
             char *procName, char *calc, char *def, StringVec &argList, StringVec
             &oriArgList, UIntVec &argBWList,
             UIntVec &resBWList, int &result_suffix, unsigned result_bw,
             char *calcStr,
             IntVec &boolRes, Map<char *, Expr *> &exprMap, StringMap<unsigned> &inBW,
             StringMap<unsigned> &hiddenBW, Map<Expr *, Expr *> &hiddenExprs);

  const char *
  EMIT_BIN(Scope *sc, Expr *expr, const char *sym, const char *op, int type,
           const char *metricSym,
           char *procName, char *calc, char *def, StringVec &argList, StringVec
           &oriArgList, UIntVec &argBWList, UIntVec &resBWList, int &result_suffix,
           unsigned result_bw, char *calcStr, IntVec &boolRes,
           Map<char *, Expr *> &exprMap, StringMap<unsigned> &inBW,
           StringMap<unsigned> &hiddenBW, Map<Expr *, Expr *> &hiddenExprs);

  const char *
  EMIT_UNI(Scope *sc, Expr *expr, const char *sym, const char *op, int type,
           const char *metricSym,
           char *procName, char *calc, char *def, StringVec &argList,
           StringVec &oriArgList,
           UIntVec &argBWList,
           UIntVec &resBWList, int &result_suffix, unsigned result_bw,
           char *calcStr,
           IntVec &boolRes, Map<char *, Expr *> &exprMap, StringMap<unsigned> &inBW,
           StringMap<unsigned> &hiddenBW, Map<Expr *, Expr *> &hiddenExprs);

  const char *
  printExpr(Scope *sc, Expr *expr, char *procName, char *calc, char *def,
            StringVec &argList,
            StringVec &oriArgList, UIntVec &argBWList, UIntVec &resBWList,
            int &result_suffix, unsigned result_bw, bool &constant,
            char *calcStr, IntVec &boolRes, Map<char *, Expr *> &exprMap,
            StringMap<unsigned> &inBW,
            StringMap<unsigned> &hiddenBW, Map<Expr *, Expr *> &hiddenExprs);

  unsigned getCopyUses(ActId *actId, Scope *sc);

  void updateOpUses(ActId *actId, Scope *sc);

  void recordOpUses(ActId *actId, ActIdVec &actIdVec);

  void printOpUses();

  unsigned getOpUses(ActId *actId, Scope *sc);

  void getActConnectionName(act_connection *actConnection, char *buff, int sz);

  void getActIdName(Scope *sc, ActId *actId, char *buff, int sz);

  void collectUniOpUses(Scope *sc, Expr *expr, StringVec &recordedOps);

  void collectBinOpUses(Scope *sc, Expr *expr, StringVec &recordedOps);

  void recordUniOpUses(Scope *sc, Expr *expr, ActIdVec &actIdVec);

  void recordBinOpUses(Scope *sc, Expr *expr, ActIdVec &actIdVec);

  void collectExprUses(Scope *sc, Expr *expr, StringVec &recordedOps);

  void recordExprUses(Scope *sc, Expr *expr, ActIdVec &actIdVec);

  void collectDflowClusterUses(Scope *sc, list_t *dflow, ActIdVec &actIdVec);

  void collectOpUses(Process *p);

  void createCopyProcs(FILE *resFp, FILE *libFp, FILE *confFp);

  void
  printDFlowFunc(FILE *resFp, FILE *libFp, FILE *confFp, const char *procName,
                 StringVec &argList, UIntVec &argBWList,
                 UIntVec &resBWList, UIntVec &outWidthList, const char *def,
                 char *calc, int result_suffix, StringVec &outSendStr,
                 IntVec &outResSuffixs, StringVec &normalizedOutList,
                 StringVec &outList, StringVec &initStrs, IntVec &boolRes,
                 Map<char *, Expr *> &exprMap, StringMap<unsigned> &inBW,
                 StringMap<unsigned> &hiddenBW, Map<int, int> &outRecord,
                 Map<Expr *, Expr *> &hiddenExprs);

  void
  handleDFlowFunc(FILE *resFp, FILE *libFp, FILE *confFp, Process *p,
                  act_dataflow_element *d, char *procName, char *calc,
                  char *def, StringVec &argList, StringVec &oriArgList,
                  UIntVec &argBWList,
                  UIntVec &resBWList, int &result_suffix, StringVec &outSendStr,
                  IntVec &outResSuffixs,
                  StringVec &outList, StringVec &normalizedOutList,
                  UIntVec &outWidthList, StringVec &initStrs, IntVec &boolRes,
                  Map<char *, Expr *> &exprMap, StringMap<unsigned> &inBW,
                  StringMap<unsigned> &hiddenBW, Map<int, int> &outRecord,
                  Map<Expr *, Expr *> &hiddenExprs);

  void handleNormalDflowElement(FILE *resFp, FILE *libFp, FILE *confFp, Process *p,
                                act_dataflow_element *d, unsigned &sinkCnt);

  void print_dflow(FILE *fp, list_t *dflow);

  void handleDFlowCluster(FILE *resFp, FILE *libFp, FILE *confFp, Process *p, list_t
  *dflow);

  void printIntVec(IntVec &ULongVec);

  void printULongVec(ULongVec &longVec);

  bool isOpUsed(Scope *sc, ActId *actId);

private:
  /* op, its bitwidth */
  Map<act_connection *, unsigned> bitwidthMap;
/* operator, # of times it is used (if it is used for more than once, then we create COPY for it) */
  Map<act_connection *, unsigned> opUses;
/* copy operator, # of times it has already been used */
  Map<act_connection *, unsigned> copyUses;
//  unsigned sinkCnt = 0;
  Metrics *metrics;
  ChpProcGenerator processGenerator{};

  void genExprFromStr(const char *str, Expr *expr, int exprType);

  void genExprFromInt(unsigned long val, Expr *expr);

  Expr *getExprFromName(char *name, Map<char *, Expr *> &exprMap,
                        bool exitOnMissing, int exprType);

};

extern int debug_verbose;

#endif //DFLOWMAP_CHPGENERATOR_H

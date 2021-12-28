#ifndef DFLOWMAP_CHPPROCGENERATOR_H
#define DFLOWMAP_CHPPROCGENERATOR_H

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
#include "ChpLibGenerator.h"
#include "Metrics.h"
#include "common.h"
#include "Helper.h"

#if LOGIC_OPTIMIZER
#include <act/expropt.h>
#endif

class ChpProcGenerator {
 public:

  ChpProcGenerator(Metrics *metrics, FILE *resFp, FILE *libFp, FILE *confFp);

  void handleProcess(Process *p);

  int searchStringVec(StringVec &strVec, const char *str);

  const char *removeDot(const char *src);

  const char *getActIdOrCopyName(Scope *sc, ActId *actId);

  void printSink(const char *name, unsigned bitwidth);

  void printInt(const char *out,
                const char *normalizedOut,
                unsigned long val,
                unsigned outWidth);

  void collectBitwidthInfo(Process *p);

  void printBitwidthInfo();

  unsigned getActIdBW(ActId *actId, Process *p);

  unsigned getBitwidth(act_connection *actConnection);

  void getCurProc(const char *str, char *val, bool isConstant);

  unsigned getExprBW(int type, unsigned lBW, unsigned rBW = 0);

  const char *EMIT_QUERY(Scope *sc,
                         Expr *expr,
                         const char *sym,
                         const char *op,
                         int type,
                         const char *metricSym,
                         char *procName,
                         char *calc,
                         char *def,
                         StringVec &argList,
                         StringVec
                         &oriArgList,
                         UIntVec &argBWList,
                         UIntVec &resBWList,
                         int &result_suffix,
                         unsigned &result_bw,
                         char *calcStr,
                         IntVec &boolRes,
                         Map<const char *, Expr *> &exprMap,
                         StringMap<unsigned>
                         &inBW,
                         StringMap<unsigned> &hiddenBW,
                         IntVec &queryResSuffixs,
                         IntVec &queryResSuffixs2,
                         Map<Expr *, Expr *> &hiddenExprs);

  const char *EMIT_BIN(Scope *sc,
                       Expr *expr,
                       const char *sym,
                       const char *op,
                       int type,
                       const char *metricSym,
                       char *procName,
                       char *calc,
                       char *def,
                       StringVec &argList,
                       StringVec
                       &oriArgList,
                       UIntVec &argBWList,
                       UIntVec &resBWList,
                       int &result_suffix,
                       unsigned &result_bw,
                       char *calcStr,
                       IntVec &boolRes,
                       Map<const char *, Expr *> &exprMap,
                       StringMap<unsigned> &inBW,
                       StringMap<unsigned> &hiddenBW,
                       IntVec &queryResSuffixs,
                       IntVec &queryResSuffixs2,
                       Map<Expr *, Expr *> &hiddenExprs);

  const char *EMIT_UNI(Scope *sc,
                       Expr *expr,
                       const char *sym,
                       const char *op,
                       int type,
                       const char *metricSym,
                       char *procName,
                       char *calc,
                       char *def,
                       StringVec &argList,
                       StringVec &oriArgList,
                       UIntVec &argBWList,
                       UIntVec &resBWList,
                       int &result_suffix,
                       unsigned &result_bw,
                       char *calcStr,
                       IntVec &boolRes,
                       Map<const char *, Expr *> &exprMap,
                       StringMap<unsigned> &inBW,
                       StringMap<unsigned> &hiddenBW,
                       IntVec &queryResSuffixs,
                       IntVec &queryResSuffixs2,
                       Map<Expr *, Expr *> &hiddenExprs);

  const char *printExpr(Scope *sc,
                        Expr *expr,
                        char *procName,
                        char *calc,
                        char *def,
                        StringVec &argList,
                        StringVec &oriArgList,
                        UIntVec &argBWList,
                        UIntVec &resBWList,
                        int &result_suffix,
                        unsigned &result_bw,
                        bool &constant,
                        char *calcStr,
                        IntVec &boolRes,
                        Map<const char *, Expr *> &exprMap,
                        StringMap<unsigned> &inBW,
                        StringMap<unsigned> &hiddenBW,
                        IntVec &queryResSuffixs,
                        IntVec &queryResSuffixs2,
                        Map<Expr *, Expr *> &hiddenExprs);

  unsigned getCopyUses(ActId *actId, Scope *sc);

  void updateOpUses(ActId *actId, Scope *sc);

  void updateOpUses(act_connection *actConnection);

  void recordOpUses(Scope *sc, ActId *actId, ActConnectVec &actConnectVec);

  void printOpUses();

  unsigned getOpUses(ActId *actId, Scope *sc);

  void getActConnectionName(act_connection *actConnection, char *buff, int sz);

  void getActIdName(Scope *sc, ActId *actId, char *buff, int sz);

  void collectUniOpUses(Scope *sc, Expr *expr, StringVec &recordedOps);

  void collectBinOpUses(Scope *sc, Expr *expr, StringVec &recordedOps);

  void recordUniOpUses(Scope *sc, Expr *expr, ActConnectVec &actConnectVec);

  void recordBinOpUses(Scope *sc, Expr *expr, ActConnectVec &actConnectVec);

  void collectExprUses(Scope *sc, Expr *expr, StringVec &recordedOps);

  void recordExprUses(Scope *sc, Expr *expr, ActConnectVec &actConnectVec);

  void collectDflowClusterUses(Scope *sc,
                               list_t *dflow,
                               ActConnectVec &actConnectVec);

  void collectOpUses(Process *p);

  void createCopyProcs();

  void printDFlowFunc(const char *procName,
                      StringVec &argList,
                      UIntVec &argBWList,
                      UIntVec &resBWList,
                      UIntVec &outWidthList,
                      const char *def,
                      char *calc,
                      int result_suffix,
                      StringVec &outSendStr,
                      IntVec &outResSuffixs,
                      StringVec &normalizedOutList,
                      StringVec &outList,
                      Map<unsigned, unsigned long> &initMap,
                      Map<unsigned, unsigned long> &buffMap,
                      IntVec &boolRes,
                      Map<const char *, Expr *> &exprMap,
                      StringMap<unsigned> &inBW,
                      StringMap<unsigned> &hiddenBW,
                      IntVec &queryResSuffixs,
                      IntVec &queryResSuffixs2,
                      Map<int, int> &outRecord,
                      Map<Expr *, Expr *> &hiddenExprs,
                      UIntVec &buffBWs);

  void handleDFlowFunc(Process *p,
                       act_dataflow_element *d,
                       char *procName,
                       char *calc,
                       char *def,
                       StringVec &argList,
                       StringVec &oriArgList,
                       UIntVec &argBWList,
                       UIntVec &resBWList,
                       int &result_suffix,
                       StringVec &outSendStr,
                       IntVec &outResSuffixs,
                       StringVec &outList,
                       StringVec &normalizedOutList,
                       UIntVec &outWidthList,
                       Map<unsigned, unsigned long> &initMap,
                       Map<unsigned, unsigned long> &buffMap,
                       IntVec &boolRes,
                       Map<const char *, Expr *> &exprMap,
                       StringMap<unsigned> &inBW,
                       StringMap<unsigned> &hiddenBW,
                       IntVec &queryResSuffixs,
                       IntVec &queryResSuffixs2,
                       Map<int, int> &outRecord,
                       UIntVec &buffBWs,
                       Map<Expr *, Expr *> &hiddenExprs);

  void handleNormalDflowElement(Process *p,
                                act_dataflow_element *d,
                                unsigned &sinkCnt);

  void print_dflow(FILE *fp, list_t *dflow);

  void handleDFlowCluster(Process *p, list_t *dflow);

  void printIntVec(IntVec &ULongVec);

  void printULongVec(ULongVec &longVec);

  bool isOpUsed(Scope *sc, ActId *actId);

  void genMemConfiguration(const char *procName);

 private:
  /* op, its bitwidth */
  Map<act_connection *, unsigned> bitwidthMap;
  /* operator, # of times it is used (if it is used for more than once, then we create COPY for it) */
  Map<act_connection *, unsigned> opUses;
  /* copy operator, # of times it has already been used */
  Map<act_connection *, unsigned> copyUses;
  Metrics *metrics;
  FILE *resFp;
  FILE *libFp;
  FILE *confFp;

  ChpLibGenerator libGenerator{};

  void genExprFromStr(const char *str, Expr *expr, int exprType);

  void genExprFromInt(unsigned long val, Expr *expr);

  Expr *getExprFromName(const char *name,
                        Map<const char *, Expr *> &exprMap,
                        bool exitOnMissing,
                        int exprType);

  void checkACTN(const char *channel, bool &actnCp, bool &actnDp);

  void updateACTN(double area, double leakPower, bool actnCp, bool actnDp);

  void updateStatistics(const double *metric,
                        const char *instance,
                        bool actnCp,
                        bool actnDp);

  void createINIT(Map<unsigned, unsigned long> &initMap,
                  UIntVec &outWidthList,
                  StringVec &outList);

  void createBuff(Map<unsigned, unsigned long> &initMap,
                  Map<unsigned, unsigned long> &buffMap,
                  UIntVec &outWidthList,
                  StringVec &outList);
};

#endif //DFLOWMAP_CHPPROCGENERATOR_H

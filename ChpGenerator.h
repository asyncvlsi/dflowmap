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
#include <math.h>
#include "ChpProcGenerator.h"
#include "Metrics.h"
#include "common.h"

#if LOGIC_OPTIMIZER
#include <act/expropt.h>
#endif

class ChpGenerator : public ActPass {
 public:
  void updateprocCount(const char *proc,
                       Map<const char *, unsigned> &procCount);

  ChpGenerator(Act *a, const char *name, Metrics *metrics);

  void handleProcess(FILE *resFp, FILE *libFp, FILE *confFp, Process *p,
                     Map<const char *, unsigned> &procCount);

  int searchStringVec(StringVec &strVec, const char *str);

  const char *removeDot(const char *src);

  const char *getActIdOrCopyName(Scope *sc, ActId *actId);

  void printSink(FILE *resFp, FILE *libFp, FILE *confFp, const char *name,
                 unsigned bitwidth);

  void printInt(FILE *resFp,
                FILE *libFp,
                FILE *confFp,
                const char *out,
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

  void createCopyProcs(FILE *resFp, FILE *libFp, FILE *confFp);

  double *getCopyMetric(unsigned N, unsigned bitwidth);

  unsigned getEquivalentBW(unsigned oriBW);

  double *getMSMetric(const char *procName, unsigned guardBW, unsigned inBW);

  double *getArbiterMetric(unsigned numInputs,
                           unsigned inBW,
                           unsigned coutBW);

  void printDFlowFunc(FILE *resFp,
                      FILE *libFp,
                      FILE *confFp,
                      const char *procName,
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

  void handleDFlowFunc(FILE *resFp,
                       FILE *libFp,
                       FILE *confFp,
                       Process *p,
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
                       Map<Expr *, Expr *> &hiddenExprs,
                       UIntVec &buffBWs,
                       Map<const char *, unsigned> &procCount);

  void handleNormalDflowElement(FILE *resFp,
                                FILE *libFp,
                                FILE *confFp,
                                Process *p,
                                act_dataflow_element *d,
                                unsigned &sinkCnt,
                                Map<const char *, unsigned> &procCount);

  void print_dflow(FILE *fp, list_t *dflow);

  void handleDFlowCluster(FILE *resFp,
                          FILE *libFp,
                          FILE *confFp,
                          Process *p,
                          list_t *dflow,
                          Map<const char *, unsigned> &procCount);

  void printIntVec(IntVec &ULongVec);

  void printULongVec(ULongVec &longVec);

  bool isOpUsed(Scope *sc, ActId *actId);

  void genMemConfiguration(FILE *confFp, const char *procName);

 private:
  /* op, its bitwidth */
  Map<act_connection *, unsigned> bitwidthMap;
  /* operator, # of times it is used (if it is used for more than once, then we create COPY for it) */
  Map<act_connection *, unsigned> opUses;
  /* copy operator, # of times it has already been used */
  Map<act_connection *, unsigned> copyUses;
  Metrics *metrics;
  ChpProcGenerator processGenerator{};

  void genExprFromStr(const char *str, Expr *expr, int exprType);

  void genExprFromInt(unsigned long val, Expr *expr);

  Expr *getExprFromName(const char *name,
                        Map<const char *, Expr *> &exprMap,
                        bool exitOnMissing,
                        int exprType);

  static bool isActnCp(const char *instance);

  static bool isActnDp(const char *instance);

  void checkACTN(const char *channel, bool &actnCp, bool &actnDp);

  void updateACTN(double area, double leakPower, bool actnCp, bool actnDp);

  void updateStatistics(const double *metric,
                        const char *instance,
                        bool actnCp,
                        bool actnDp);

  void createINIT(FILE *resFp,
                  FILE *libFp,
                  FILE *confFp,
                  Map<unsigned, unsigned long> &initMap,
                  UIntVec &outWidthList,
                  StringVec &outList);

  void createBuff(FILE *resFp, FILE *libFp, FILE *confFp,
                  Map<unsigned, unsigned long> &initMap,
                  Map<unsigned, unsigned long> &buffMap,
                  UIntVec &outWidthList, StringVec &outList);
};

#endif //DFLOWMAP_CHPGENERATOR_H

#ifndef DFLOWMAP_CHPPROCGENERATOR_H
#define DFLOWMAP_CHPPROCGENERATOR_H

#include <cstdio>
#include <cstring>
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
#include "ChpBackend.h"
#include "Metrics.h"
#include "common.h"
#include "Helper.h"

#if LOGIC_OPTIMIZER
#include <act/expropt.h>
#endif

class ProcGenerator {
 public:
  ProcGenerator(Metrics *metrics, ChpBackend *chpBackend);

  void handleProcess(Process *p);

  const char *getActIdOrCopyName(ActId *actId);

  void collectBitwidthInfo();

  void printBitwidthInfo();

  unsigned getActIdBW(ActId *actId);

  unsigned getBitwidth(act_connection *actConnection);

  void getCurProc(const char *str, char *val, bool isConstant);

  unsigned getExprBW(int type, unsigned lBW, unsigned rBW = 0);

  const char *EMIT_QUERY(Expr *expr,
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

  const char *EMIT_BIN(Expr *expr,
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

  const char *EMIT_UNI(Expr *expr,
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

  const char *printExpr(Expr *expr,
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

  unsigned getCopyUses(ActId *actId);

  void updateOpUses(ActId *actId);

  void updateOpUses(act_connection *actConnection);

  void recordOpUses(ActId *actId, ActConnectVec &actConnectVec);

  void printOpUses();

  unsigned getOpUses(ActId *actId);

  void getActConnectionName(act_connection *actConnection, char *buff, int sz);

  void getActIdName(ActId *actId, char *buff, int sz);

  void collectUniOpUses(Expr *expr, StringVec &recordedOps);

  void collectBinOpUses(Expr *expr, StringVec &recordedOps);

  void recordUniOpUses(Expr *expr, ActConnectVec &actConnectVec);

  void recordBinOpUses(Expr *expr, ActConnectVec &actConnectVec);

  void collectExprUses(Expr *expr, StringVec &recordedOps);

  void recordExprUses(Expr *expr, ActConnectVec &actConnectVec);

  void collectDflowClusterUses(list_t *dflow, ActConnectVec &actConnectVec);

  void collectOpUses();

  void createCopyProcs();

  void printDFlowFunc(const char *procName,
                      StringVec &argList,
                      UIntVec &argBWList,
                      UIntVec &resBWList,
                      UIntVec &outBWList,
                      const char *def,
                      char *calc,
                      int result_suffix,
                      StringVec &outSendStr,
                      IntVec &outResSuffixs,
                      StringVec &normalizedOutList,
                      StringVec &outList,
                      Map<unsigned, unsigned long> &initMap,
                      Vector<BuffInfo> &buffInfos,
                      IntVec &boolRes,
                      Map<const char *, Expr *> &exprMap,
                      StringMap<unsigned> &inBW,
                      StringMap<unsigned> &hiddenBW,
                      IntVec &queryResSuffixs,
                      IntVec &queryResSuffixs2,
                      Map<int, int> &outRecord,
                      Map<Expr *, Expr *> &hiddenExprs,
                      UIntVec &buffBWs);

  void handleDFlowFunc(act_dataflow_element *d,
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
                       Vector<BuffInfo> &buffInfos,
                       IntVec &boolRes,
                       Map<const char *, Expr *> &exprMap,
                       StringMap<unsigned> &inBW,
                       StringMap<unsigned> &hiddenBW,
                       IntVec &queryResSuffixs,
                       IntVec &queryResSuffixs2,
                       Map<int, int> &outRecord,
                       UIntVec &buffBWs,
                       Map<Expr *, Expr *> &hiddenExprs);

  void handleNormalDflowElement(act_dataflow_element *d, unsigned &sinkCnt);

  void print_dflow(FILE *fp, list_t *dflow);

  void handleDFlowCluster(list_t *dflow);

  bool isOpUsed(ActId *actId);

 private:
  /* op, its bitwidth */
  Map<act_connection *, unsigned> bitwidthMap;
  /* operator, # of times it is used (if it is used for more than once, then we create COPY for it) */
  Map<act_connection *, unsigned> opUses;
  /* copy operator, # of times it has already been used */
  Map<act_connection *, unsigned> copyUses;
  Metrics *metrics;
  ChpBackend *chpBackend;
  Process *p;
  Scope *sc;

  void checkACTN(const char *channel, bool &actnCp, bool &actnDp);

  void createSink(const char *name, unsigned bitwidth);

  void createSource(const char *outName,
                    unsigned long val,
                    unsigned bitwidth);
};

#endif //DFLOWMAP_CHPPROCGENERATOR_H

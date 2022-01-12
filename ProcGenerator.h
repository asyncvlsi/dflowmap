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
#include "DflowGenerator.h"

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

  const char *EMIT_QUERY(DflowGenerator *dflowGenerator,
                         Expr *expr,
                         const char *sym,
                         char *procName,
                         int &result_suffix,
                         unsigned &result_bw);

  const char *EMIT_BIN(DflowGenerator *dflowGenerator,
                       Expr *expr,
                       const char *sym,
                       const char *op,
                       int type,
                       char *procName,
                       int &result_suffix,
                       unsigned &result_bw);

  const char *EMIT_UNI(DflowGenerator *dflowGenerator,
                       Expr *expr,
                       const char *sym,
                       const char *op,
                       char *procName,
                       int &result_suffix,
                       unsigned &result_bw);

  const char *printExpr(DflowGenerator *dflowGenerator,
                        Expr *expr,
                        char *procName,
                        int &result_suffix,
                        unsigned &result_bw);

  unsigned getCopyUses(ActId *actId);

  void updateOpUses(ActId *actId);

  void updateOpUses(act_connection *actConnection);

  void recordOpUses(ActId *actId, ActConnectVec &actConnectVec);

  void printOpUses();

  unsigned getOpUses(ActId *actId);

  void collectUniOpUses(Expr *expr, StringVec &recordedOps);

  void collectBinOpUses(Expr *expr, StringVec &recordedOps);

  void recordUniOpUses(Expr *expr, ActConnectVec &actConnectVec);

  void recordBinOpUses(Expr *expr, ActConnectVec &actConnectVec);

  void collectExprUses(Expr *expr, StringVec &recordedOps);

  void recordExprUses(Expr *expr, ActConnectVec &actConnectVec);

  void collectDflowClusterUses(list_t *dflow, ActConnectVec &actConnectVec);

  void collectOpUses();

  void createCopyProcs();

  void printDFlowFunc(DflowGenerator *dflowGenerator,
                      const char *procName,
                      UIntVec &outBWList,
                      StringVec &outList,
                      Map<unsigned int, unsigned int> &outRecord,
                      Vector<BuffInfo> &buffInfos);

  void handleDFlowFunc(DflowGenerator *dflowGenerator,
                       act_dataflow_element *d,
                       char *procName,
                       int &result_suffix,
                       StringVec &outList,
                       UIntVec &outWidthList,
                       Map<unsigned int, unsigned int> &outRecord,
                       Vector<BuffInfo> &buffInfos);

  void handleNormalDflowElement(act_dataflow_element *d, unsigned &sinkCnt);

  void handleDFlowCluster(list_t *dflow);

  bool isOpUsed(ActId *actId);

  void handleBuff(Expr *bufExpr,
                  Expr *initExpr,
                  char *procName,
                  const char *outName,
                  unsigned outID,
                  unsigned outBW,
                  Vector<BuffInfo> &buffInfos);

  void handlePort(const Expr *expr,
                  char *procName,
                  int &result_suffix,
                  unsigned result_bw,
                  const char *exprName,
                  DflowGenerator *dflowGenerator);

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

  void createSink(const char *name, unsigned bitwidth);

  void createSource(const char *outName,
                    unsigned long val,
                    unsigned bitwidth);
};

#endif //DFLOWMAP_CHPPROCGENERATOR_H

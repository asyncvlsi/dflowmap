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
//#include "lib.cc"

class ChpGenerator : public ActPass {
public:
  ChpGenerator(Act *a, const char *name, Metrics *metrics);

  void handleProcess(FILE *resFp, FILE *libFp, FILE *confFp, Process *p);

  int searchStringVec(StringVec &strVec, const char *str);

  const char *removeDot(const char *src);

  const char *getActIdName(ActId *actId);

  void printSink(FILE *resFp, FILE *libFp, FILE *confFp, const char *name, int bitwidth);

  void printInt(FILE *resFp, FILE *libFp, FILE *confFp, const char *out,
                const char *normalizedOut, unsigned val,
                int outWidth);

  void collectBitwidthInfo(Process *p);

  void printBitwidthInfo();

  int getActIdBW(ActId *actId, Process *p);

  int getBitwidth(const char *varName);

  void getCurProc(const char *str, char *val);

  const char *
  EMIT_QUERY(Expr *expr, const char *sym, const char *op, int type, const char *metricSym,
             char *procName, char *calc, char *def, StringVec &argList, StringVec
             &oriArgList, IntVec &argBWList,
             IntVec &resBWList, int &result_suffix, int result_bw, char *calcStr,
             IntVec &boolRes);

  const char *
  EMIT_BIN(Expr *expr, const char *sym, const char *op, int type, const char *metricSym,
           char *procName, char *calc, char *def, StringVec &argList, StringVec
           &oriArgList, IntVec &argBWList, IntVec &resBWList, int &result_suffix,
           int result_bw, char *calcStr, IntVec &boolRes);

  const char *
  EMIT_UNI(Expr *expr, const char *sym, const char *op, int type, const char *metricSym,
           char *procName, char *calc, char *def, StringVec &argList,
           StringVec &oriArgList,
           IntVec &argBWList,
           IntVec &resBWList, int &result_suffix, int result_bw, char *calcStr,
           IntVec &boolRes);

  const char *
  printExpr(Expr *expr, char *procName, char *calc, char *def, StringVec &argList,
            StringVec &oriArgList, IntVec &argBWList,
            IntVec &resBWList, int &result_suffix, int result_bw, bool &constant,
            char *calcStr, IntVec &boolRes);

  unsigned getCopyUses(const char *op);

  void updateOpUses(const char *op);

  void recordOpUses(const char *op, CharPtrVec &charPtrVec);

  void printOpUses();

  unsigned getOpUses(const char *op);

  void collectUniOpUses(Expr *expr);

  void collectBinOpUses(Expr *expr);

  void recordUniOpUses(Expr *expr, CharPtrVec &charPtrVec);

  void recordBinOpUses(Expr *expr, CharPtrVec &charPtrVec);

  void collectExprUses(Expr *expr);

  void recordExprUses(Expr *expr, CharPtrVec &charPtrVec);

  void collectDflowClusterUses(list_t *dflow, CharPtrVec &charPtrVec);

  void collectOpUses(Process *p);

  void createCopyProcs(FILE *resFp, FILE *libFp, FILE *confFp);

  void
  printDFlowFunc(FILE *resFp, FILE *libFp, FILE *confFp, const char *procName,
                 StringVec &argList, IntVec &argBWList,
                 IntVec &resBWList, IntVec &outWidthList, const char *def,
                 char *calc, int result_suffix, StringVec &outSendStr,
                 IntVec &outResSuffixs, StringVec &normalizedOutList,
                 StringVec &outList, StringVec &initStrs, IntVec &boolRes);

  void
  handleDFlowFunc(FILE *resFp, FILE *libFp, FILE *confFp, Process *p,
                  act_dataflow_element *d, char *procName, char
                  *calc,
                  char *def, StringVec &argList, StringVec &oriArgList,
                  IntVec &argBWList,
                  IntVec &resBWList, int &result_suffix, StringVec &outSendStr,
                  IntVec &outResSuffixs,
                  StringVec &outList, StringVec &normalizedOutList,
                  IntVec &outWidthList, StringVec &initStrs, IntVec &boolRes);

  void handleNormalDflowElement(FILE *resFp, FILE *libFp, FILE *confFp, Process *p,
                                act_dataflow_element *d);

  void print_dflow(FILE *fp, list_t *dflow);

  void handleDFlowCluster(FILE *resFp, FILE *libFp, FILE *confFp, Process *p, list_t
  *dflow);

  void printIntVec(IntVec &intVec);

private:
  /* op, its bitwidth */
  Map<const char *, int> bitwidthMap;
/* operator, # of times it is used (if it is used for more than once, then we create COPY for it) */
  Map<const char *, unsigned> opUses;
/* copy operator, # of times it has already been used */
  Map<const char *, unsigned> copyUses;
  unsigned sinkCnt = 0;
  Metrics *metrics;
  ChpProcGenerator processGenerator;
};

#endif //DFLOWMAP_CHPGENERATOR_H

#include <act/lang.h>
#include "ChpGenerator.h"
#include "helper.cc"

void ChpGenerator::printIntVec(IntVec &intVec) {
  for (auto &val : intVec) {
    printf("%d ", val);
  }
  printf("\n");
}


void ChpGenerator::printULongVec(ULongVec &longVec) {
  for (auto &val : longVec) {
    printf("%lu ", val);
  }
  printf("\n");
}

int ChpGenerator::searchStringVec(StringVec &strVec, const char *str) {
  auto it = std::find(strVec.begin(), strVec.end(), str);
  if (it != strVec.end()) {
    return (it - strVec.begin());
  } else {
    return -1;
  }
}

const char *ChpGenerator::removeDot(const char *src) {
  int len = strlen(src);
  char *result = new char[len + 1];
  int cnt = 0;
  for (int i = 0; i < len; i++) {
    if (src[i] != '.') {
      result[cnt] = src[i];
      cnt++;
    }
  }
  result[cnt] = '\0';
  return result;
}

const char *ChpGenerator::getActIdOrCopyName(Scope *sc, ActId *actId) {
  char *str = new char[10240];
  if (actId) {
    char *actName = new char[10240];
    getActIdName(sc, actId, actName, 10240);
    unsigned outUses = getOpUses(actId, sc);
    if (outUses) {
      unsigned copyUse = getCopyUses(actId, sc);
      if (debug_verbose) {
        printf("for %s, outUses: %d, copyUse: %d\n", actName, outUses, copyUse);
      }
      if (copyUse <= outUses) {
        const char *normalizedName = removeDot(actName);
        sprintf(str, "%scopy.out[%u]", normalizedName, copyUse);
      } else {
        sprintf(str, "%s", actName);
      }
    } else {
      sprintf(str, "%s", actName);
    }
  } else {
    sprintf(str, "*");
  }
  return str;
}

void
ChpGenerator::printSink(FILE *resFp, FILE *libFp, FILE *confFp,
                        const char *name,
                        unsigned bitwidth) {
  if (name == nullptr) {
    fatal_error("sink name is NULL!\n");
  }
  const char *normalizedName = removeDot(name);
  fprintf(resFp, "sink<%u> %s_sink(%s);\n", bitwidth, normalizedName, name);
  char *instance = new char[1500];
  sprintf(instance, "sink<%u>", bitwidth);
  char *unitInstance = new char[1500];
  sprintf(unitInstance, "sink_1_");
  long *metric = metrics->getOpMetric(unitInstance);
  processGenerator.createSink(libFp, confFp, instance, metric);
  if (metric != nullptr) {
    metrics->updateStatistics(instance, metric[3], metric[0]);

  }
}

void
ChpGenerator::printInt(FILE *resFp, FILE *libFp, FILE *confFp, const char *out,
                       const char *normalizedOut,
                       unsigned long val, unsigned outWidth) {
  fprintf(resFp, "source<%lu,%u> %s_inst(%s);\n", val, outWidth, normalizedOut,
          out);
  char *instance = new char[1500];
  sprintf(instance, "source<%lu,%u>", val, outWidth);
  char *opName = new char[8];
  sprintf(opName, "source1");
  long *metric = metrics->getOpMetric(opName);
  processGenerator.createSource(libFp, confFp, instance, metric);
  if (metric != nullptr) {
    metrics->updateStatistics(instance, metric[3], metric[0]);
  }
}

void ChpGenerator::collectBitwidthInfo(Process *p) {
  ActInstiter inst(p->CurScope());
  for (inst = inst.begin(); inst != inst.end(); inst++) {
    ValueIdx *vx = *inst;
    act_connection *c;
    const char *varName = vx->getName();
    auto tmp = new ActId(vx->getName());
    c = tmp->Canonical(p->CurScope());
    delete tmp;
    int bitwidth = TypeFactory::bitWidth(vx->t);
    if (bitwidth <= 0) {
      printf("%s has negative bw %d!\n", varName, bitwidth);
    } else {
      if (debug_verbose) {
        printf("Update bitwidthMap for (%s, %d).\n", varName, bitwidth);
      }
      bitwidthMap.insert(std::make_pair(c, bitwidth));
    }
  }
}

void ChpGenerator::printBitwidthInfo() {
  printf("bitwidth info:\n");
  for (auto &bitwidthMapIt : bitwidthMap) {
    char *connectName = new char[10240];
    getActConnectionName(bitwidthMapIt.first, connectName, 10240);
    printf("(%s, %u) ", connectName, bitwidthMapIt.second);
  }
  printf("\n");
}

unsigned ChpGenerator::getActIdBW(ActId *actId, Process *p) {
  act_connection *c = actId->Canonical(p->CurScope());
  unsigned bw = getBitwidth(c);
  if (debug_verbose) {
    printf("Fetch BW for actID ");
    actId->Print(stdout);
    printf(": %u\n", bw);
  }
  return bw;
}

unsigned ChpGenerator::getBitwidth(act_connection *actConnection) {
  auto bitwidthMapIt = bitwidthMap.find(actConnection);
  if (bitwidthMapIt != bitwidthMap.end()) {
    return bitwidthMapIt->second;
  }
  char *varName = new char[10240];
  getActConnectionName(actConnection, varName, 10240);
  printf("We could not find bitwidth info for %s\n", varName);
  printBitwidthInfo();
  exit(-1);
}

void ChpGenerator::getCurProc(const char *str, char *val, bool isConstant) {
  val[0] = '\0';
  if (isConstant) {
    char curProc[100];
    sprintf(curProc, "c%s", str);
    strcpy(val, curProc);
  }
}

unsigned ChpGenerator::getExprBW(int type, unsigned lBW, unsigned rBW) {
  unsigned maxBW = lBW;
  if (rBW > lBW) {
    maxBW = rBW;
  }
  switch (type) {
    case E_INT: {
      fatal_error("We shoudld not try to get expr bw for E_INT!\n");
    }
    case E_VAR: {
      fatal_error("We shoudld not try to get expr bw for E_VAR!\n");
    }
    case E_AND:
    case E_OR:
    case E_XOR: {
      return maxBW;
    }
    case E_NE:
    case E_EQ:
    case E_GE:
    case E_LE:
    case E_GT:
    case E_LT: {
      return 1;
    }
    case E_PLUS: {
      return maxBW + 1;
    }
    case E_MINUS: {
      return maxBW;
    }
    case E_MULT: {
      return lBW + rBW;
    }
    case E_MOD:
    case E_LSR:
    case E_ASR:
    case E_DIV: {
      return maxBW;
    }
    case E_LSL: {
      return lBW + rBW;
    }
    case E_NOT:
    case E_UMINUS:
    case E_COMPLEMENT:
    case E_QUERY: {
      return maxBW;
    }
    default: {
      fatal_error("Try to get expr bw for unknown type %d\n", type);
      return -1;
    }
  }
}

const char *
ChpGenerator::EMIT_QUERY(Scope *sc, Expr *expr, const char *sym, const char *op,
                         int type,
                         const char *metricSym,
                         char *procName, char *calc, char *def,
                         StringVec &argList,
                         StringVec &oriArgList, UIntVec &argBWList,
                         UIntVec &resBWList, int &result_suffix,
                         unsigned &result_bw,
                         char *calcStr,
                         IntVec &boolResSuffixs,
                         Map<const char *, Expr *> &exprMap,
                         StringMap<unsigned> &inBW,
                         StringMap<unsigned> &hiddenBW, IntVec &queryResSuffixs,
                         IntVec &queryResSuffixs2,
                         Map<Expr *, Expr *> &hiddenExprs) {
  Expr *cExpr = expr->u.e.l;
  Expr *lExpr = expr->u.e.r->u.e.l;
  Expr *rExpr = expr->u.e.r->u.e.r;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  int oriResSuffix = result_suffix;
  bool cConst = false;
  char *cCalcStr = new char[1500];
  unsigned cBW = 0;
  const char *cStr = printExpr(sc, cExpr, procName, calc, def, argList,
                               oriArgList,
                               argBWList,
                               resBWList, result_suffix, cBW, cConst, cCalcStr,
                               boolResSuffixs, exprMap, inBW, hiddenBW,
                               queryResSuffixs,
                               queryResSuffixs2, hiddenExprs);
  if (cBW != 1) {
    print_expr(stdout, expr);
    printf(", cBW is %u\n!\n", cBW);
    exit(-1);
  }
  int cResSuffix = result_suffix;
  boolResSuffixs.push_back(result_suffix);
  bool lConst = false;
  char *lCalcStr = new char[1500];
  unsigned lResBW = 0;
  const char *lStr = printExpr(sc, lExpr, procName, calc, def, argList,
                               oriArgList,
                               argBWList,
                               resBWList,
                               result_suffix, lResBW, lConst, lCalcStr,
                               boolResSuffixs, exprMap, inBW, hiddenBW,
                               queryResSuffixs,
                               queryResSuffixs2, hiddenExprs);
  int lResSuffix = result_suffix;
  bool rConst = false;
  char *rCalcStr = new char[1500];
  unsigned rResBW = 0;
  const char *rStr = printExpr(sc, rExpr, procName, calc, def, argList,
                               oriArgList,
                               argBWList,
                               resBWList,
                               result_suffix, rResBW, rConst, rCalcStr,
                               boolResSuffixs, exprMap, inBW, hiddenBW,
                               queryResSuffixs,
                               queryResSuffixs2, hiddenExprs);
  int rResSuffix = result_suffix;
  char *newExpr = new char[100];
  result_suffix++;
  sprintf(newExpr, "res%d", result_suffix);
  char *curCal = new char[300];
  sprintf(curCal, "      res%d := %s ? %s : %s;\n", result_suffix, cStr, lStr,
          rStr);
  strcat(calc, curCal);
  int cPrefix = std::atoi(cStr + 3);
  int lPrefix = std::atoi(lStr + 3);
  int rPrefix = std::atoi(rStr + 3);
  if (String(cStr).find("res") == 0) {
    queryResSuffixs2.push_back(cPrefix);
  }
  if ((String(lStr).find("res") == 0) &&
      ((String(rStr).find("x") == 0)
       || (((String(rStr) <= "9")) && (String(rStr) >= "0"))
      )
      ) {
    queryResSuffixs.push_back(cPrefix);
  }
  if (String(lStr).find("x") == 0) {
    queryResSuffixs.push_back(rPrefix);
  }
  if (String(rStr).find("x") == 0) {
    queryResSuffixs.push_back(lPrefix);
  }
  if ((lResBW > 1) && (String(rStr).find("res") == 0)) {
    queryResSuffixs.push_back(rPrefix);
  }
  if ((rResBW > 1) && (String(lStr).find("res") == 0)) {
    queryResSuffixs.push_back(lPrefix);
  }

  if (lExpr->type == E_INT) {
    queryResSuffixs.push_back(rPrefix);
  } else if (rExpr->type == E_INT) {
    queryResSuffixs.push_back(lPrefix);
  }

  if ((lResBW == 0) && (rResBW == 0)) {
    print_expr(stdout, expr);
    printf(", both lResBW and rResBW are 0!\n");
    exit(-1);
  }
  if (result_bw == 0) {
    result_bw = getExprBW(type, lResBW, rResBW);
    if (result_bw == 0) {
      print_expr(stdout, expr);
      printf("result_bw is 0!\n");
      exit(-1);
    }
  }
  resBWList.push_back(result_bw);
  if (DEBUG_CLUSTER) {
    printf("query res%d has bw %u\n", result_suffix, result_bw);
    printf("      res%d := %s ? %s : %s;\n", result_suffix, cStr, lStr, rStr);
  }
  char *lVal = new char[100];
  getCurProc(lStr, lVal, lConst);
  char *rVal = new char[100];
  getCurProc(rStr, rVal, rConst);
  char *subProcName = new char[1500];
  sprintf(subProcName, "_%s%s%s", lVal, sym, rVal);
  strcat(procName, subProcName);
  if (debug_verbose) {
    printf(
        "\n\n\n\n\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
    printf("tri expr: ");
    print_expr(stdout, expr);
    printf("\ndflowmap generates calc: %s\n", calc);
    printf("arg list: ");
    for (auto &arg : argList) {
      printf("%s ", arg.c_str());
    }
    printf("\n");
    printf("arg bw list: ");
    for (auto &bw : argBWList) {
      printf("%u ", bw);
    }
    printf("\n");
    printf("res bw list: ");
    for (auto &bw2:resBWList) {
      printf("%u ", bw2);
    }
    printf("\n");
  }
  sprintf(calcStr, "%s", newExpr);
  /* create Expr */
  if (debug_verbose) {
    printf("[PERF] handle query expression for ");
    print_expr(stdout, expr);
  }
  bool resC = (oriResSuffix == cResSuffix);
  bool resL = (oriResSuffix == lResSuffix);
  bool resR = (oriResSuffix == rResSuffix);
  char *newCExprName = new char[1000];
  newCExprName[0] = '\0';
//  if (!resC) {
//    sprintf(newCExprName, "res%d", cResSuffix);
//  } else {
  sprintf(newCExprName, "%s", cStr);
//  }
  char *newLExprName = new char[1000];
  newLExprName[0] = '\0';
//  if (!resL) {
//    sprintf(newLExprName, "res%d", lResSuffix);
//  } else {
  sprintf(newLExprName, "%s", lStr);
//  }
  char *newRExprName = new char[1000];
  newRExprName[0] = '\0';
//  if (!resR) {
//    sprintf(newRExprName, "res%d", rResSuffix);
//  } else {
  sprintf(newRExprName, "%s", rStr);
//  }
  int cType = (cExpr->type == E_INT) ? E_INT : E_VAR;
  int lType = (lExpr->type == E_INT) ? E_INT : E_VAR;
  int rType = (rExpr->type == E_INT) ? E_INT : E_VAR;
  Expr *newCExpr = getExprFromName(newCExprName, exprMap, false, cType);
  Expr *newLExpr = getExprFromName(newLExprName, exprMap, false, lType);
  Expr *newRExpr = getExprFromName(newRExprName, exprMap, false, rType);
  Expr *resRHS = getExprFromName(newExpr, exprMap, false, E_VAR);
  Expr *resExpr = new Expr;
  resExpr->type = expr->type;
  resExpr->u.e.l = newCExpr;
  Expr *resRExpr = new Expr;
  resRExpr->type = expr->u.e.r->type;
  resRExpr->u.e.l = newLExpr;
  resRExpr->u.e.r = newRExpr;
  resExpr->u.e.r = resRExpr;
//  resExpr->u.e.r->u.e.l = newLExpr;
//  resExpr->u.e.r->u.e.r = newRExpr;
  hiddenBW.insert(GenPair(newExpr, result_bw));
  hiddenExprs.insert(GenPair(resRHS, resExpr));
  if (debug_verbose) {
    printf("resRHS: ");
    print_expr(stdout, resRHS);
    printf(", resExpr: ");
    print_expr(stdout, resExpr);
    printf(".\n");
  }
  return newExpr;
}

const char *
ChpGenerator::EMIT_BIN(Scope *sc, Expr *expr, const char *sym, const char *op,
                       int type,
                       const char *metricSym,
                       char *procName, char *calc, char *def,
                       StringVec &argList,
                       StringVec &oriArgList, UIntVec &argBWList,
                       UIntVec &resBWList,
                       int &result_suffix,
                       unsigned &result_bw, char *calcStr,
                       IntVec &boolResSuffixs,
                       Map<const char *, Expr *> &exprMap,
                       StringMap<unsigned> &inBW,
                       StringMap<unsigned> &hiddenBW, IntVec &queryResSuffixs,
                       IntVec &queryResSuffixs2,
                       Map<Expr *, Expr *> &hiddenExprs) {
  Expr *lExpr = expr->u.e.l;
  Expr *rExpr = expr->u.e.r;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  int oriResSuffix = result_suffix;
  bool lConst = false;
  char *lCalcStr = new char[1500];
  unsigned lResBW = 0;
  const char *lStr = printExpr(sc, lExpr, procName, calc, def, argList,
                               oriArgList,
                               argBWList,
                               resBWList,
                               result_suffix, lResBW, lConst, lCalcStr,
                               boolResSuffixs, exprMap, inBW, hiddenBW,
                               queryResSuffixs,
                               queryResSuffixs2, hiddenExprs);
  int lResSuffix = result_suffix;
  bool rConst = false;
  char *rCalcStr = new char[1500];
  unsigned rResBW = 0;
  const char *rStr = printExpr(sc, rExpr, procName, calc, def, argList,
                               oriArgList,
                               argBWList,
                               resBWList,
                               result_suffix, rResBW, rConst, rCalcStr,
                               boolResSuffixs, exprMap, inBW, hiddenBW,
                               queryResSuffixs,
                               queryResSuffixs2, hiddenExprs);

  int rResSuffix = result_suffix;
  if (lConst && rConst) {
    print_expr(stdout, expr);
    printf(" has both const operands!\n");
    printf("lExpr: ");
    print_expr(stdout, lExpr);
    printf(", rExpr: ");
    print_expr(stdout, rExpr);
    printf("\n");
    exit(-1);
  }
  char *newExpr = new char[100];
  result_suffix++;
  sprintf(newExpr, "res%d", result_suffix);
  char *curCal = new char[300];
  sprintf(curCal, "      res%d := %s %s %s;\n", result_suffix, lStr, op, rStr);
  strcat(calc, curCal);
  if ((String(op) == "=") && (String(rStr) == "0")) {
    int cPrefix = std::atoi(lStr + 3);
    queryResSuffixs.push_back(cPrefix);
  }
  if (String(lStr).find("res") == 0) {
    int lPrefix = std::atoi(lStr + 3);
    queryResSuffixs.push_back(lPrefix);
  }
  if (String(rStr).find("res") == 0) {
    int rPrefix = std::atoi(rStr + 3);
    queryResSuffixs.push_back(rPrefix);
  }

  if (lConst) {
    int rPrefix = std::atoi(rStr + 3);
    queryResSuffixs.push_back(rPrefix);
  } else if (rConst) {
    int lPrefix = std::atoi(lStr + 3);
    queryResSuffixs.push_back(lPrefix);
  }

  if ((lResBW == 0) && (rResBW == 0)) {
    print_expr(stdout, expr);
    printf(", both lResBW and rResBW are 0!\n");
    exit(-1);
  }
  if (result_bw == 0) {
    result_bw = getExprBW(type, lResBW, rResBW);
    if (result_bw == 0) {
      print_expr(stdout, expr);
      printf("result_bw is 0!\n");
      exit(-1);
    }
  }
  resBWList.push_back(result_bw);
  if (DEBUG_CLUSTER) {
    printf("bin res%d has bw %u\n", result_suffix, result_bw);
    printf("      res%d := %s %s %s;\n", result_suffix, lStr, op, rStr);
  }
  /* create Expr */
  if (debug_verbose) {
    printf("[PERF] handle bin expression for ");
    print_expr(stdout, expr);
    printf("***************\nres%d := %s %s %s;\n", result_suffix, lStr, op,
           rStr);
    print_expr(stdout, expr);
    printf("\n");
  }
  bool resL = (oriResSuffix == lResSuffix);
  bool resR = (oriResSuffix == rResSuffix);
  char *newLExprName = new char[1000];
  newLExprName[0] = '\0';
//  if (!resL) {
//    sprintf(newLExprName, "res%d", lResSuffix);
//  } else {
  sprintf(newLExprName, "%s", lStr);
//  }
  char *newRExprName = new char[1000];
  newRExprName[0] = '\0';
//  if (!resR) {
//    sprintf(newRExprName, "res%d", rResSuffix);
//  } else {
  sprintf(newRExprName, "%s", rStr);
//  }
  int lType = (lExpr->type == E_INT) ? E_INT : E_VAR;
  int rType = (rExpr->type == E_INT) ? E_INT : E_VAR;
  Expr *newLExpr = getExprFromName(newLExprName, exprMap, false, lType);
  Expr *newRExpr = getExprFromName(newRExprName, exprMap, false, rType);
  Expr *resRHS = getExprFromName(newExpr, exprMap, false, E_VAR);
  Expr *resExpr = new Expr;
  resExpr->type = expr->type;
  resExpr->u.e.l = newLExpr;
  resExpr->u.e.r = newRExpr;
  hiddenBW.insert(GenPair(newExpr, result_bw));
  hiddenExprs.insert(GenPair(resRHS, resExpr));
  if (debug_verbose) {
    printf("resRHS: ");
    print_expr(stdout, resRHS);
    printf(", resExpr: ");
    print_expr(stdout, resExpr);
    printf(".\n");
  }
  char *lVal = new char[100];
  getCurProc(lStr, lVal, lConst);
  char *rVal = new char[100];
  getCurProc(rStr, rVal, rConst);
  char *subProcName = new char[1500];
  if (!strcmp(lStr, rStr)) {
    sprintf(subProcName, "_%s%s2%s", lVal, sym, rVal);
  } else {
    sprintf(subProcName, "_%s%s%s", lVal, sym, rVal);
  }
  strcat(procName, subProcName);
  if (debug_verbose) {
    printf("binary expr: ");
    print_expr(stdout, expr);
    printf("\ndflowmap generates calc: %s\n", calc);
    printf("procName: %s\n", procName);
    printf("arg list: ");
    for (auto &arg : argList) {
      printf("%s ", arg.c_str());
    }
    printf("\n");
    printf("arg bw list: ");
    for (auto &bw : argBWList) {
      printf("%u ", bw);
    }
    printf("\n");
    printf("res bw list: ");
    for (auto &bw2:resBWList) {
      printf("%u ", bw2);
    }
    printf("\n");
  }
  sprintf(calcStr, "%s", newExpr);
  return newExpr;
}

Expr *ChpGenerator::getExprFromName(const char *name,
                                    Map<const char *, Expr *> &exprMap,
                                    bool exitOnMissing, int exprType) {
  for (auto &exprMapIt : exprMap) {
    if (strcmp(name, exprMapIt.first) == 0) {
      return exprMapIt.second;
    }
  }
  if (exitOnMissing) {
    printf("We could not find the expr for %s!\n", name);
    exit(-1);
  }
  Expr *newExpr = new Expr;
  if (exprType == E_INT) {
    genExprFromInt(std::stoul(std::string(name)), newExpr);
  } else {
    genExprFromStr(name, newExpr, exprType);
  }
  exprMap.insert(GenPair(name, newExpr));
  return newExpr;
}

const char *
ChpGenerator::EMIT_UNI(Scope *sc, Expr *expr, const char *sym, const char *op,
                       int type,
                       const char *metricSym,
                       char *procName, char *calc, char *def,
                       StringVec &argList,
                       StringVec &oriArgList,
                       UIntVec &argBWList,
                       UIntVec &resBWList, int &result_suffix,
                       unsigned &result_bw,
                       char *calcStr,
                       IntVec &boolResSuffixs,
                       Map<const char *, Expr *> &exprMap,
                       StringMap<unsigned> &inBW,
                       StringMap<unsigned> &hiddenBW, IntVec &queryResSuffixs,
                       IntVec &queryResSuffixs2,
                       Map<Expr *, Expr *> &hiddenExprs) {
  /* collect bitwidth info */
  Expr *lExpr = expr->u.e.l;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  int oriResSuffix = result_suffix;
  bool lConst;
  char *lCalcStr = new char[1500];
  unsigned lResBW = 0;
  const char *lStr = printExpr(sc, lExpr, procName, calc, def, argList,
                               oriArgList,
                               argBWList,
                               resBWList,
                               result_suffix, lResBW, lConst, lCalcStr,
                               boolResSuffixs, exprMap, inBW, hiddenBW,
                               queryResSuffixs,
                               queryResSuffixs2, hiddenExprs);
  int lResSuffix = result_suffix;
  char *val = new char[100];
  getCurProc(lStr, val, lConst);
  sprintf(procName, "%s_%s%s", procName, sym, val);
  char *newExpr = new char[100];
  result_suffix++;
  sprintf(newExpr, "res%d", result_suffix);
  char *curCal = new char[300];
  sprintf(curCal, "      res%d := %s %s;\n", result_suffix, op, lStr);
  if (result_bw == 0) {
    result_bw = getExprBW(type, lResBW);
    if (result_bw == 0) {
      print_expr(stdout, expr);
      printf("result_bw is 0!\n");
      exit(-1);
    }
  }
  resBWList.push_back(result_bw);
  if (DEBUG_CLUSTER) {
    printf("uni res%d has bw %u\n", result_suffix, result_bw);
    printf("      res%d := %s %s;\n", result_suffix, op, lStr);
  }
  strcat(calc, curCal);
  if (debug_verbose) {
    printf("unary expr: ");
    print_expr(stdout, expr);
    printf("\ndflowmap generates calc: %s\n", calc);
  }
  sprintf(calcStr, "%s", newExpr);
  /* create Expr */
  if (debug_verbose) {
    printf("[PERF] handle uni expression for ");
    print_expr(stdout, expr);
  }
  bool resL = (oriResSuffix == lResSuffix);
  char *newLExprName = new char[1000];
  newLExprName[0] = '\0';
//  if (!resL) {
//    sprintf(newLExprName, "res%d", lResSuffix);
//  } else {
  sprintf(newLExprName, "%s", lStr);
//  }
  int lType = (lExpr->type == E_INT) ? E_INT : E_VAR;
  Expr *newLExpr = getExprFromName(newLExprName, exprMap, false, lType);
  Expr *resRHS = getExprFromName(newExpr, exprMap, false, E_VAR);
  Expr *resExpr = new Expr;
  resExpr->type = expr->type;
  resExpr->u.e.l = newLExpr;
  hiddenBW.insert(GenPair(newExpr, result_bw));
  hiddenExprs.insert(GenPair(resRHS, resExpr));
  if (debug_verbose) {
    printf("resRHS: ");
    print_expr(stdout, resRHS);
    printf(", resExpr: ");
    print_expr(stdout, resExpr);
    printf(".\n");
  }
  return newExpr;
}

const char *
ChpGenerator::printExpr(Scope *sc, Expr *expr, char *procName, char *calc,
                        char *def,
                        StringVec &argList,
                        StringVec &oriArgList, UIntVec &argBWList,
                        UIntVec &resBWList, int &result_suffix,
                        unsigned &result_bw,
                        bool &constant, char *calcStr, IntVec &boolResSuffixs,
                        Map<const char *, Expr *> &exprMap,
                        StringMap<unsigned> &inBW,
                        StringMap<unsigned> &hiddenBW, IntVec &queryResSuffixs,
                        IntVec &queryResSuffixs2,
                        Map<Expr *, Expr *> &hiddenExprs) {
  int type = expr->type;
  switch (type) {
    case E_INT: {
      if (procName[0] == '\0') {
        fatal_error("we should NOT process Source here!\n");
      }
      unsigned long val = expr->u.v;
      const char *valStr = strdup(std::to_string(val).c_str());
      sprintf(calcStr, "%s", valStr);
      constant = true;
      if (result_bw == 0) {
        if (val == 0) {
          result_bw = 1;
        } else {
          result_bw = (unsigned) (log2(val)) + 1;
        }
      }
      return valStr;
    }
    case E_VAR: {
      int numArgs = argList.size();
      auto actId = (ActId *) expr->u.e.l;
      act_connection *actConnection = actId->Canonical(sc);
      unsigned argBW = getBitwidth(actConnection);
      char *oriVarName = new char[10240];
      getActIdName(sc, actId, oriVarName, 10240);
      char *curArg = new char[10240];
      int idx = searchStringVec(oriArgList, oriVarName);
      if (idx == -1) {
        oriArgList.push_back(oriVarName);
        const char *mappedVarName = getActIdOrCopyName(sc, actId);
        argList.push_back(mappedVarName);
        if (debug_verbose) {
          printf("oriVarName: %s, mappedVarName: %s\n", oriVarName,
                 mappedVarName);
        }
        sprintf(calcStr, "%s_%d", oriVarName, numArgs);
        sprintf(curArg, "x%d", numArgs);
        argBWList.push_back(argBW);
      } else {
        sprintf(calcStr, "%s_%d", oriVarName, idx);
        sprintf(curArg, "x%d", idx);
      }
      inBW.insert(GenPair(curArg, argBW));
      if (result_bw == 0) {
        result_bw = argBW;
      }
//      if (procName[0] == '\0') {
//        resBWList.push_back(result_bw);
//        if (DEBUG_CLUSTER) {
//          printf("var %s has bw %u\n", oriVarName, result_bw);
//        }
//      }
      getExprFromName(curArg, exprMap, false, E_VAR);
      return curArg;
    }
    case E_AND: {
      return EMIT_BIN(sc, expr, "and", "&", type, "and", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_OR: {
      return EMIT_BIN(sc, expr, "or", "|", type, "and", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_NOT: {
      return EMIT_UNI(sc, expr, "not", "~", type, "and", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_PLUS: {
      return EMIT_BIN(sc, expr, "add", "+", type, "add", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_MINUS: {
      return EMIT_BIN(sc, expr, "minus", "-", type, "add", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_MULT: {
      return EMIT_BIN(sc, expr, "mul", "*", type, "mul", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_DIV: {
      return EMIT_BIN(sc, expr, "div", "/", type, "div", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_MOD: {
      return EMIT_BIN(sc, expr, "mod", "%", type, "rem", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_LSL: {
      return EMIT_BIN(sc, expr, "lsl", "<<", type, "lshift", procName, calc,
                      def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_LSR: {
      return EMIT_BIN(sc, expr, "lsr", ">>", type, "lshift", procName, calc,
                      def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_ASR: {
      return EMIT_BIN(sc, expr, "asr", ">>>", type, "lshift", procName, calc,
                      def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_UMINUS: {
      return EMIT_UNI(sc, expr, "neg", "-", type, "and", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_XOR: {
      return EMIT_BIN(sc, expr, "xor", "^", type, "and", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_LT: {
      return EMIT_BIN(sc, expr, "lt", "<", type, "icmp", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_GT: {
      return EMIT_BIN(sc, expr, "gt", ">", type, "icmp", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_LE: {
      return EMIT_BIN(sc, expr, "le", "<=", type, "icmp", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_GE: {
      return EMIT_BIN(sc, expr, "ge", ">=", type, "icmp", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_EQ: {
      return EMIT_BIN(sc, expr, "eq", "=", type, "icmp", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2,
                      hiddenExprs);
    }
    case E_NE: {
      return EMIT_BIN(sc, expr, "ne", "!=", type, "icmp", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_COMPLEMENT: {
      return EMIT_UNI(sc, expr, "compl", "~", type, "and", procName, calc, def,
                      argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolResSuffixs,
                      exprMap, inBW,
                      hiddenBW, queryResSuffixs, queryResSuffixs2, hiddenExprs);
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      Expr *rExpr = expr->u.e.r;
      if (rExpr) {
        result_bw = rExpr->u.v;
      } else {
        result_bw = 1;
      }
      return printExpr(sc, lExpr, procName, calc, def, argList, oriArgList,
                       argBWList,
                       resBWList,
                       result_suffix, result_bw, constant, calcStr,
                       boolResSuffixs,
                       exprMap,
                       inBW, hiddenBW, queryResSuffixs, queryResSuffixs2,
                       hiddenExprs);
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      result_bw = 1;
      return printExpr(sc, lExpr, procName, calc, def, argList, oriArgList,
                       argBWList, resBWList, result_suffix, result_bw,
                       constant, calcStr, boolResSuffixs, exprMap, inBW,
                       hiddenBW, queryResSuffixs,
                       queryResSuffixs2,
                       hiddenExprs);
    }
    case E_QUERY: {
      return EMIT_QUERY(sc, expr, "q", "?", type, "q", procName, calc, def,
                        argList, oriArgList, argBWList, resBWList,
                        result_suffix,
                        result_bw, calcStr, boolResSuffixs, exprMap, inBW,
                        hiddenBW,
                        queryResSuffixs, queryResSuffixs2,
                        hiddenExprs);
    }
    default: {
      print_expr(stdout, expr);
      printf("\n");
      fatal_error(
          "when printing expression, encounter unknown expression type %d\n",
          type);
      break;
    }
  }
  fatal_error("Shouldn't be here");
  return "-should-not-be-here-";
}

void
ChpGenerator::getActConnectionName(act_connection *actConnection, char *buff,
                                   int sz) {
  if (actConnection == nullptr) {
    fatal_error("Try to get the name of NULL act connection!\n");
  }
  ActId *uid = actConnection->toid();
  uid->sPrint(buff, sz);
  delete uid;
}

void ChpGenerator::getActIdName(Scope *sc, ActId *actId, char *buff, int sz) {
  ActId *uid = actId->Canonical(sc)->toid();
  uid->sPrint(buff, sz);
  delete uid;
}

unsigned ChpGenerator::getCopyUses(ActId *actId, Scope *sc) {
  act_connection *actConnection = actId->Canonical(sc);
  auto copyUsesIt = copyUses.find(actConnection);
  if (copyUsesIt == copyUses.end()) {
    char buf[10240];
    getActConnectionName(actConnection, buf, 10240);
    printf("We don't know how many times %s is used as COPY!\n", buf);
    exit(-1);
  }
  unsigned uses = copyUsesIt->second;
  copyUsesIt->second++;
  return uses;
}

void ChpGenerator::updateOpUses(act_connection *actConnection) {
  auto opUsesIt = opUses.find(actConnection);
  if (opUsesIt == opUses.end()) {
    opUses.insert(std::make_pair(actConnection, 0));
    copyUses.insert(std::make_pair(actConnection, 0));
  } else {
    opUsesIt->second++;
  }
}

void ChpGenerator::updateOpUses(ActId *actId, Scope *sc) {
  act_connection *actConnection = actId->Canonical(sc);
  updateOpUses(actConnection);
}

void ChpGenerator::recordOpUses(Scope *sc, ActId *actId,
                                ActConnectVec &actConnectVec) {
  act_connection *actConnection = actId->Canonical(sc);
  if (std::find(actConnectVec.begin(), actConnectVec.end(), actConnection) ==
      actConnectVec.end()) {
    actConnectVec.push_back(actConnection);
  }
}

void ChpGenerator::printOpUses() {
  printf("OP USES:\n");
  for (auto &opUsesIt : opUses) {
    char *opName = new char[10240];
    getActConnectionName(opUsesIt.first, opName, 10240);
    printf("(%s, %u) ", opName, opUsesIt.second);
  }
  printf("\n");
}

bool ChpGenerator::isOpUsed(Scope *sc, ActId *actId) {
  act_connection *actConnection = actId->Canonical(sc);
//  char *connectionName = new char[10240];
//  getActConnectionName(actConnection, connectionName, 10240);
//  printf("unique act connection name: %s\n", connectionName);
  return opUses.find(actConnection) != opUses.end();
}

unsigned ChpGenerator::getOpUses(ActId *actId, Scope *sc) {
  act_connection *actConnection = actId->Canonical(sc);
  auto opUsesIt = opUses.find(actConnection);
  if (opUsesIt != opUses.end()) {
    return opUsesIt->second;
  }
  char *buf = new char[10240];
  getActConnectionName(actConnection, buf, 10240);
  printf("We don't know how many times %s is used!\n", buf);
  printOpUses();
  exit(-1);
}

void
ChpGenerator::collectUniOpUses(Scope *sc, Expr *expr, StringVec &recordedOps) {
  Expr *lExpr = expr->u.e.l;
  collectExprUses(sc, lExpr, recordedOps);
}

void
ChpGenerator::collectBinOpUses(Scope *sc, Expr *expr, StringVec &recordedOps) {
  Expr *lExpr = expr->u.e.l;
  collectExprUses(sc, lExpr, recordedOps);
  Expr *rExpr = expr->u.e.r;
  collectExprUses(sc, rExpr, recordedOps);
}

void ChpGenerator::recordUniOpUses(Scope *sc, Expr *expr,
                                   ActConnectVec &actConnectVec) {
  Expr *lExpr = expr->u.e.l;
  recordExprUses(sc, lExpr, actConnectVec);
}

void ChpGenerator::recordBinOpUses(Scope *sc, Expr *expr,
                                   ActConnectVec &actConnectVec) {
  Expr *lExpr = expr->u.e.l;
  recordExprUses(sc, lExpr, actConnectVec);
  Expr *rExpr = expr->u.e.r;
  recordExprUses(sc, rExpr, actConnectVec);
}

void
ChpGenerator::collectExprUses(Scope *sc, Expr *expr, StringVec &recordedOps) {
  int type = expr->type;
  switch (type) {
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
      collectBinOpUses(sc, expr, recordedOps);
      break;
    }
    case E_NOT:
    case E_UMINUS:
    case E_COMPLEMENT: {
      collectUniOpUses(sc, expr, recordedOps);
      break;
    }
    case E_INT: {
      break;
    }
    case E_VAR: {
      auto actId = (ActId *) expr->u.e.l;
      char *varName = new char[10240];
      getActIdName(sc, actId, varName, 10240);
      if (searchStringVec(recordedOps, varName) == -1) {
        updateOpUses(actId, sc);
        recordedOps.push_back(varName);
      }
      break;
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      collectExprUses(sc, lExpr, recordedOps);
      break;
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      collectExprUses(sc, lExpr, recordedOps);
      break;
    }
    case E_QUERY: {
      Expr *cExpr = expr->u.e.l;
      Expr *lExpr = expr->u.e.r->u.e.l;
      Expr *rExpr = expr->u.e.r->u.e.r;
      collectExprUses(sc, cExpr, recordedOps);
      collectExprUses(sc, lExpr, recordedOps);
      collectExprUses(sc, rExpr, recordedOps);
      break;
    }
    default: {
      print_expr(stdout, expr);
      printf("\nUnknown expression type %d when collecting expr use\n", type);
      exit(-1);
    }
  }
}

void ChpGenerator::recordExprUses(Scope *sc, Expr *expr,
                                  ActConnectVec &actConnectVec) {
  int type = expr->type;
  switch (type) {
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
      recordBinOpUses(sc, expr, actConnectVec);
      break;
    }
    case E_NOT:
    case E_UMINUS:
    case E_COMPLEMENT: {
      recordUniOpUses(sc, expr, actConnectVec);
      break;
    }
    case E_INT: {
      break;
    }
    case E_VAR: {
      auto actId = (ActId *) expr->u.e.l;
      recordOpUses(sc, actId, actConnectVec);
      break;
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      recordExprUses(sc, lExpr, actConnectVec);
      break;
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      recordExprUses(sc, lExpr, actConnectVec);
      break;
    }
    case E_QUERY: {
      Expr *cExpr = expr->u.e.l;
      Expr *lExpr = expr->u.e.r->u.e.l;
      Expr *rExpr = expr->u.e.r->u.e.r;
      recordExprUses(sc, cExpr, actConnectVec);
      recordExprUses(sc, lExpr, actConnectVec);
      recordExprUses(sc, rExpr, actConnectVec);
      break;
    }
    default: {
      print_expr(stdout, expr);
      printf("\nUnknown expression type %d when recording expr use\n", type);
      exit(-1);
    }
  }
}

void ChpGenerator::collectDflowClusterUses(Scope *sc, list_t *dflow,
                                           ActConnectVec &actConnectVec) {
  listitem_t *li;
  for (li = list_first (dflow); li; li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
      case ACT_DFLOW_FUNC: {
        Expr *expr = d->u.func.lhs;
        recordExprUses(sc, expr, actConnectVec);
        break;
      }
      case ACT_DFLOW_SPLIT: {
        ActId *input = d->u.splitmerge.single;
        recordOpUses(sc, input, actConnectVec);
        ActId *guard = d->u.splitmerge.guard;
        recordOpUses(sc, guard, actConnectVec);
        break;
      }
      case ACT_DFLOW_MERGE: {
        ActId *guard = d->u.splitmerge.guard;
        recordOpUses(sc, guard, actConnectVec);
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs < 2) {
          dflow_print(stdout, d);
          fatal_error("\nMerge has less than TWO inputs!\n");
        }
        ActId **inputs = d->u.splitmerge.multi;
        for (int i = 0; i < numInputs; i++) {
          ActId *in = inputs[i];
          recordOpUses(sc, in, actConnectVec);
        }
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
      case ACT_DFLOW_CLUSTER: {
        fatal_error("Do not support nested dflow_cluster!\n");
        break;
      }
      case ACT_DFLOW_SINK: {
        fatal_error("dflow cluster should not connect to SINK!\n");
      }
      default: {
        fatal_error("Unknown dataflow type %d\n", d->t);
        break;
      }
    }
  }
}

void ChpGenerator::collectOpUses(Process *p) {
  Scope *sc = p->CurScope();
  listitem_t *li;
  for (li = list_first (p->getlang()->getdflow()->dflow);
       li;
       li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
      case ACT_DFLOW_SINK: {
        ActId *input = d->u.sink.chan;
        updateOpUses(input, sc);
        break;
      }
      case ACT_DFLOW_FUNC: {
        Expr *expr = d->u.func.lhs;
        StringVec recordedOps;
        collectExprUses(sc, expr, recordedOps);
        break;
      }
      case ACT_DFLOW_SPLIT: {
        ActId *input = d->u.splitmerge.single;
        updateOpUses(input, sc);
        ActId *guard = d->u.splitmerge.guard;
        updateOpUses(guard, sc);
        break;
      }
      case ACT_DFLOW_MERGE: {
        ActId *guard = d->u.splitmerge.guard;
        updateOpUses(guard, sc);
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs < 2) {
          dflow_print(stdout, d);
          fatal_error("\nMerge has less than TWO inputs!\n");
        }
        ActId **inputs = d->u.splitmerge.multi;
        for (int i = 0; i < numInputs; i++) {
          ActId *in = inputs[i];
          updateOpUses(in, sc);
        }
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
      case ACT_DFLOW_CLUSTER: {
        ActConnectVec actConnectVec;
        collectDflowClusterUses(sc, d->u.dflow_cluster, actConnectVec);
        for (auto &actConnect : actConnectVec) {
          updateOpUses(actConnect);
        }
        break;
      }
      default: {
        fatal_error("Unknown dataflow type %d\n", d->t);
        break;
      }
    }
  }
}

void ChpGenerator::createCopyProcs(FILE *resFp, FILE *libFp, FILE *confFp) {
  fprintf(resFp, "/* copy processes */\n");
  for (auto &opUsesIt : opUses) {
    unsigned uses = opUsesIt.second;
    if (uses) {
      unsigned N = uses + 1;
      act_connection *actConnection = opUsesIt.first;
      unsigned bitwidth = getBitwidth(actConnection);
      char *opName = new char[10240];
      getActConnectionName(actConnection, opName, 10240);
      const char *normalizedName = removeDot(opName);
      fprintf(resFp, "copy<%u,%u> %scopy(%s);\n", bitwidth, N, normalizedName,
              opName);
      printf("[copy] %scopy\n", normalizedName);
      char *instance = new char[1500];
      sprintf(instance, "copy<%u,%u>", bitwidth, N);
      long *metric = metrics->getOpMetric(instance);
      if (!metric) {
        if (LOGIC_OPTIMIZER) {
          char *equivInstance = new char[1500];
          int equivN = ceil_log2(N) - 1;
          if (equivN < 1) {
            equivN = 1;
          }
          if (DEBUG_OPTIMIZER) {
            printf(
                "We are handling copy_%u_%u, and we are using mapping it to %d "
                "copy_%u_2\n", bitwidth, N, equivN, bitwidth);
          }
          sprintf(equivInstance, "copy<%u,2>", bitwidth);
          long *equivMetric = metrics->getOpMetric(equivInstance);
          if (!equivMetric) {
            fatal_error("Missing metrics for copy %s\n", equivInstance);
          }
          if (equivN == 1) {
            metric = equivMetric;
          } else {
            metric = new long[4];
            metric[0] = equivN * equivMetric[0];
            metric[1] = equivN * equivMetric[1];
            metric[2] = equivN * equivMetric[2];
            metric[3] = equivN * equivMetric[3];
          }
          char *normalizedOp = new char[10240];
          normalizedOp[0] = '\0';
          metrics->getNormalizedOpName(instance, normalizedOp);
          metrics->updateMetrics(normalizedOp, metric);
          metrics->writeMetricsFile(normalizedOp, metric);
        }
      }
      processGenerator.createCopy(libFp, confFp, instance, metric);
      metrics->updateCopyStatistics(bitwidth, N);
      if (metric != nullptr) {
        bool actnCp = false;
        bool actnDp = false;
//        checkACTN(normalizedName, actnCp, actnDp);
//        if (actnCp) {
//          printf("[actnCp]: %scopy\n", normalizedName);
//        } else if (actnDp) {
//          printf("[actnDp]: %scopy\n", normalizedName);
//        }
        updateStatistics(metric, instance, actnCp, actnDp);
      }
    }
  }
  fprintf(resFp, "\n");
}

void
ChpGenerator::updateStatistics(const long *metric, const char *instance,
                               bool actnCp, bool actnDp) {
  long area = metric[3];
  long leakPower = metric[0];
  metrics->updateStatistics(instance, area, leakPower);
  updateACTN(area, leakPower, actnCp, actnDp);
}

void ChpGenerator::genExprFromInt(unsigned long val, Expr *expr) {
  expr->type = E_INT;
  expr->u.v = val;
}

void ChpGenerator::genExprFromStr(const char *str, Expr *expr, int exprType) {
  auto newLActId = new ActId(str);
  expr->type = exprType;
  expr->u.e.l = (Expr *) (newLActId);
}

void ChpGenerator::createINIT(FILE *resFp, FILE *libFp, FILE *confFp,
                              Map<unsigned, unsigned long> &initMap,
                              UIntVec &outWidthList, StringVec &outList) {
  for (auto &initMapIt : initMap) {
    unsigned outID = initMapIt.first;
    const char *oriOut = outList[outID].c_str();
    char *newOut = new char[10240];
    sprintf(newOut, "%s_new", oriOut);
    unsigned outBW = outWidthList[outID];
    fprintf(resFp, "chan(int<%u>) %s;\n", outBW, newOut);
    unsigned long initVal = initMapIt.second;
    char *initInstance = new char[100];
    sprintf(initInstance, "init%u", outBW);
    long *initMetric = metrics->getOpMetric(initInstance);
    if (!initMetric && LOGIC_OPTIMIZER) {
      printf("We could not find metrics for %s!\n", initInstance);
      exit(-1);
    }
    processGenerator.createInit(libFp, confFp, initInstance, initMetric);
    if (initMetric != nullptr) {
      bool actnCp = false;
      bool actnDp = false;
//      checkACTN(oriOut, actnCp, actnDp);
//      if (actnCp) {
//        printf("[actnCp]: %s_init\n", oriOut);
//      } else if (actnDp) {
//        printf("[actnDp]: %s_init\n", oriOut);
//      }
      updateStatistics(initMetric, initInstance, actnCp, actnDp);
    }
    fprintf(resFp, "init<%lu,%u> %s_init(%s, %s);\n", initVal, outBW,
            oriOut, newOut, oriOut);
    printf("[init] %s_init\n", oriOut);
  }
}

void ChpGenerator::createBuff(FILE *resFp, FILE *libFp, FILE *confFp,
                              Map<unsigned, unsigned long> &initMap,
                              Map<unsigned, unsigned long> &buffMap,
                              UIntVec &outWidthList, StringVec &outList) {
  for (auto &buffMapIt : buffMap) {
    unsigned outID = buffMapIt.first;
    if (initMap.find(outID) != initMap.end()) {
      continue;
    }
    const char *oriOut = outList[outID].c_str();
    unsigned outBW = outWidthList[outID];
//    unsigned long numBuffs = buffMapIt.second;
    unsigned long numBuffs = 10;
    for (int bufCnt = 0; bufCnt < numBuffs; bufCnt++) {
      char *curBuff = new char[10240];
      sprintf(curBuff, "%s_buf%d", oriOut, bufCnt);
      fprintf(resFp, "chan(int<%u>) %s;\n", outBW, curBuff);
    }
    long *buffMetric = new long[4];
    buffMetric[0] = 0.55;
    buffMetric[1] = 0.007;
    buffMetric[2] = 59;
    buffMetric[3] = 2.12;
    char *buffInstance = new char[1024];
    sprintf(buffInstance, "onebuf<%u>", outBW);
    processGenerator.createBuff(libFp, confFp, buffInstance, buffMetric);
    for (int bufCnt = 0; bufCnt < numBuffs; bufCnt++) {
      char *curBuff = new char[10240];
      sprintf(curBuff, "%s_buf%d", oriOut, bufCnt);
      char *nxtBuff = new char[10240];
      if (bufCnt == (numBuffs - 1)) {
        sprintf(nxtBuff, "%s", oriOut);
      } else {
        sprintf(nxtBuff, "%s_buf%d", oriOut, (bufCnt + 1));
      }
      fprintf(resFp, "onebuf<%u> %s_inst(%s, %s);\n", outBW, curBuff,
              curBuff, nxtBuff);
    }
  }
}

void
ChpGenerator::printDFlowFunc(FILE *resFp, FILE *libFp, FILE *confFp,
                             const char *procName, StringVec &argList,
                             UIntVec &argBWList, UIntVec &resBWList,
                             UIntVec &outWidthList, const char *def, char *calc,
                             int result_suffix, StringVec &outSendStr,
                             IntVec &outResSuffixs,
                             StringVec &normalizedOutList, StringVec &outList,
                             Map<unsigned, unsigned long> &initMap,
                             Map<unsigned, unsigned long> &buffMap,
                             IntVec &boolResSuffixs,
                             Map<const char *, Expr *> &exprMap,
                             StringMap<unsigned> &inBW,
                             StringMap<unsigned> &hiddenBW,
                             IntVec &queryResSuffixs,
                             IntVec &queryResSuffixs2,
                             Map<int, int> &outRecord,
                             Map<Expr *, Expr *> &hiddenExprs,
                             UIntVec &buffBWs) {
  calc[strlen(calc) - 2] = ';';
  if (DEBUG_CLUSTER) {
    printf("PRINT DFLOW FUNCTION\n");
    printf("size: %d\n", strlen(procName));
    printf("procName: %s\n", procName);
    printf("arg list:\n");
    for (auto &arg : argList) {
      printf("%s ", arg.c_str());
    }
    printf("\n");
    printf("arg bw list:\n");
    for (auto &bw : argBWList) {
      printf("%u ", bw);
    }
    printf("\n");
    printf("res bw list:\n");
    for (auto &resBW : resBWList) {
      printf("%u ", resBW);
    }
    printf("\n");
    printf("outWidthList:\n");
    for (auto &outWidth : outWidthList) {
      printf("%u ", outWidth);
    }
    printf("\n");
    printf("def: %s\n", def);
    printf("calc: %s\n", calc);
    printf("result_suffix: %d\n", result_suffix);
    printf("outSendStr:\n");
    for (auto &outStr : outSendStr) {
      printf("%s\n", outStr.c_str());
    }
    printf("normalizedOutList:\n");
    for (auto &out : normalizedOutList) {
      printf("%s ", out.c_str());
    }
    printf("\n");
    printf("outList:\n");
    for (auto &out : outList) {
      printf("%s ", out.c_str());
    }
    printf("\n");
    printf("initMap:\n");
    for (auto &initMapIt : initMap) {
      printf("(%u, %lu) ", initMapIt.first, initMapIt.second);
    }
    printf("\n");
    printf("boolResSuffixs: ");
    printIntVec(boolResSuffixs);
  }
  char *instance = new char[MAX_INSTANCE_LEN];
  char *oriInstance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<", procName);
  sprintf(oriInstance, "%s<", procName);
  int numArgs = argList.size();
  int i = 0;
  for (; i < numArgs; i++) {
    char *subInstance = new char[100];
    if (i == (numArgs - 1)) {
      sprintf(subInstance, "%u>", argBWList[i]);
    } else {
      sprintf(subInstance, "%u,", argBWList[i]);
    }
    strcat(instance, subInstance);
    strcat(oriInstance, subInstance);
  }
//  int numOuts = outWidthList.size();
//  for (i = 0; i < numOuts; i++) {
//    char *subInstance = new char[100];
//    if (i == (numOuts - 1)) {
//      sprintf(subInstance, "%u>", outWidthList[i]);
//    } else {
//      sprintf(subInstance, "%u,", outWidthList[i]);
//    }
//    strcat(oriInstance, subInstance);
//  }

  createINIT(resFp, libFp, confFp, initMap, outWidthList, outList);
  createBuff(resFp, libFp, confFp, initMap, buffMap, outWidthList, outList);

  fprintf(resFp, "%s ", instance);
  if (debug_verbose) {
    printf("[fu]: ");
  }
  for (auto &normalizedOut : normalizedOutList) {
    fprintf(resFp, "%s_", normalizedOut.c_str());
    if (debug_verbose) {
      printf("%s_", normalizedOut.c_str());
    }
  }
  fprintf(resFp, "inst(");
  if (debug_verbose) {
    printf("inst\n");
  }
  for (auto &arg : argList) {
    fprintf(resFp, "%s, ", arg.c_str());
  }
  int numOuts = outList.size();
  if (numOuts < 1) {
    fatal_error("No output is found!\n");
  }
  for (i = 0; i < numOuts; i++) {
    char *actualOut = new char[10240];
    const char *oriOut = outList[i].c_str();
    if (initMap.find(i) != initMap.end()) {
      sprintf(actualOut, "%s_new", oriOut);
    } else if (buffMap.find(i) != buffMap.end()) {
      sprintf(actualOut, "%s_buf0", oriOut);
    } else {
      sprintf(actualOut, "%s", oriOut);
    }
    fprintf(resFp, "%s", actualOut);
    if (i == (numOuts - 1)) {
      fprintf(resFp, ");\n");
    } else {
      fprintf(resFp, ", ");
    }
  }
  /* create chp library */
  if (strlen(instance) < 5) {
    fatal_error("Invalid instance name %s\n", instance);
  }
  char *outSend = new char[10240];
  sprintf(outSend, "      ");
  for (i = 0; i < numOuts - 1; i++) {
    char *subSend = new char[1500];
    sprintf(subSend, "%s, ", outSendStr[i].c_str());
    strcat(outSend, subSend);
  }
  char *subSend = new char[1500];
  sprintf(subSend, "%s;\n", outSendStr[i].c_str());
  strcat(outSend, subSend);
  char *log = new char[1500];
  sprintf(log, "      log(\"send (\", ");
  for (auto &outResSuffix : outResSuffixs) {
    char *subLog = new char[100];
    sprintf(subLog, "res%u, \",\", ", outResSuffix);
    strcat(log, subLog);
  }
  char *subLog = new char[100];
  sprintf(subLog, "\")\")");
  strcat(log, subLog);
  strcat(outSend, log);
  int numInitElems = initMap.size();
  if (DEBUG_CLUSTER) {
    printf("numInitElems: %d\n", numInitElems);
  }
  /* Get the perf metric */
  char *opName = oriInstance + 5;
  if (debug_verbose) {
    printf("start to search metric for %s\n", opName);
  }
  long *metric = metrics->getOpMetric(opName);
  if (!metric) {
#if LOGIC_OPTIMIZER
    if (debug_verbose) {
      printf("Will run logic optimizer for %s\n", opName);
    }
    /* Prepare in_expr_list */
    list_t *in_expr_list = list_new();
    iHashtable *in_expr_map = ihash_new(0);
    iHashtable *in_width_map = ihash_new(0);
    unsigned totalInBW = 0;
    unsigned lowBWInPorts = 0;
    unsigned highBWInPorts = 0;
    for (auto &inBWIt : inBW) {
      String inName = inBWIt.first;
      unsigned bw = inBWIt.second;
      totalInBW += bw;
      if (bw >= 32) {
        highBWInPorts++;
      } else {
        lowBWInPorts++;
      }
      char *inChar = new char[10240];
      sprintf(inChar, "%s", inName.c_str());
      if (DEBUG_OPTIMIZER) {
        printf("inChar: %s\n", inChar);
      }
      Expr *inExpr = getExprFromName(inChar, exprMap, true, -1);
      list_append(in_expr_list, inExpr);
      ihash_bucket_t *b_expr, *b_width;
      b_expr = ihash_add(in_expr_map, (long) inExpr);
      b_expr->v = inChar;
      b_width = ihash_add(in_width_map, (long) inExpr);
      b_width->i = (int) bw;
    }
    /* Prepare hidden_expr_list */
    list_t *hidden_expr_list = list_new();
    list_t *hidden_expr_name_list = list_new();
//    iHashtable *out_expr_map = ihash_new(0);
    iHashtable *out_width_map = ihash_new(0);
    for (auto &hiddenBWIt : hiddenBW) {
      String hiddenName = hiddenBWIt.first;
      unsigned bw = hiddenBWIt.second;
      char *hiddenChar = new char[1024];
      sprintf(hiddenChar, "%s", hiddenName.c_str());
      if (DEBUG_OPTIMIZER) {
        printf("hiddenChar: %s\n", hiddenChar);
      }
      Expr *hiddenRHS = getExprFromName(hiddenChar, exprMap, true, -1);
      ihash_bucket_t *b_expr2, *b_width2;
      b_expr2 = ihash_add(in_expr_map, (long) hiddenRHS);
      b_expr2->v = hiddenChar;
      b_width2 = ihash_add(in_width_map, (long) hiddenRHS);
      b_width2->i = (int) bw;
      Expr *hiddenExpr = hiddenExprs.find(hiddenRHS)->second;
      list_append(hidden_expr_list, hiddenExpr);
      list_append(hidden_expr_name_list, hiddenChar);
      ihash_bucket_t *b_width = ihash_lookup(out_width_map, (long) hiddenExpr);
      if (!b_width) {
        b_width = ihash_add(out_width_map, (long) hiddenExpr);
        b_width->i = (int) bw;
      }
    }
    /* Prepare out_expr_list */
    list_t *out_expr_list = list_new();
    list_t *out_expr_name_list = list_new();
    IntVec processedResIDs;
//    int numOuts = outRecord.size();
    for (int ii = 0; ii < numOuts; ii++) {
      int resID = outRecord.find(ii)->second;
      char *resChar = new char[1024];
      sprintf(resChar, "res%d", resID);
      if (DEBUG_OPTIMIZER) {
        printf("resChar: %s\n", resChar);
      }
      Expr *resExpr = getExprFromName(resChar, exprMap, true, -1);
      list_append(out_expr_list, resExpr);

//      b_expr = ihash_add(out_expr_map, (long) resExpr);
      char *outChar = new char[1024];
      sprintf(outChar, "out%d", ii);
//      b_expr->v = outChar;
      list_append(out_expr_name_list, outChar);
      if (std::find(processedResIDs.begin(), processedResIDs.end(), resID)
          != processedResIDs.end()) {
        continue;
      }
      ihash_bucket_t *b_width;
      b_width = ihash_add(out_width_map, (long) resExpr);
      unsigned bw = outWidthList[ii];
      b_width->i = (int) bw;
      processedResIDs.push_back(resID);
    }
    auto optimizer = new ExternalExprOpt(genus, bd, false);
    if (DEBUG_OPTIMIZER) {
      listitem_t *li;
      printf("in_expr_bundle:\n");
      for (li = list_first (in_expr_list); li; li = list_next (li)) {
        long key = (long) list_value(li);
        char *val = (char *) ihash_lookup(in_expr_map, key)->v;
        int bw = ihash_lookup(in_width_map, key)->i;
        printf("key: %ld, val: %s, bw: %d\n", key, val, bw);
        Expr *e = (Expr *) list_value (li);
        print_expr(stdout, e);
        printf("\n");
      }
      printf("\nout_expr_bundle:\n");
      for (li = list_first (out_expr_list); li; li = list_next (li)) {
        long key = (long) list_value(li);
        int bw = ihash_lookup(out_width_map, key)->i;
        printf("key: %ld, bw: %d\n", key, bw);
        Expr *e = (Expr *) list_value (li);
        print_expr(stdout, e);
        printf("\n");
      }
      for (li = list_first (out_expr_name_list); li; li = list_next (li)) {
        char *outName = (char *) list_value(li);
        printf("outName: %s\n", outName);
      }
      printf("\nhidden expr:\n");
      for (li = list_first (hidden_expr_list); li; li = list_next (li)) {
        Expr *e = (Expr *) list_value (li);
        print_expr(stdout, e);
        printf("\n");
        long key = (long) list_value(li);
        int bw = ihash_lookup(out_width_map, key)->i;
        printf("key: %ld, bw: %d\n", key, bw);
      }
      printf("\n");
    }
    char *normalizedOp = new char[MAX_INSTANCE_LEN];
    normalizedOp[0] = '\0';
    metrics->getNormalizedOpName(opName, normalizedOp);
    char *optimizerProcName = new char[1000];
    sprintf(optimizerProcName, "op");
    printf("Run logic optimizer for %s\n", optimizerProcName);
    ExprBlockInfo *info = optimizer->run_external_opt(optimizerProcName, in_expr_list,
                                                      in_expr_map,
                                                      in_width_map,
                                                      out_expr_list, out_expr_name_list,
                                                      out_width_map,
                                                      hidden_expr_list,
                                                      hidden_expr_name_list);
    printf(
        "Generated block %s: Area: %e m2, Dyn Power: %e W, Leak Power: %e W, delay: %e "
        "s\n",
        optimizerProcName, info->area, info->power_typ_dynamic, info->power_typ_static,
        info->delay_typ);
    long leakpower = (long) (info->power_typ_static * 1e9);  // Leakage power (nW)
    long energy = (long) (info->power_typ_dynamic * info->delay_typ * 1e15);  // 1e-15J
    long delay = (long) (info->delay_typ * 1e12); // Delay (ps)
    long area = (long) (info->area * 1e12);  // AREA (um^2)
    /* adjust perf number by adding latch, etc. */
    long *latchMetric = metrics->getOpMetric("latch1");
    if (latchMetric == nullptr) {
      fatal_error("We could not find metric for latch1!\n");
    }
    area = (long) (area + totalInBW * latchMetric[3] + lowBWInPorts * 1.43
                   + highBWInPorts * 2.86 + delay / 500 * 1.43);
    leakpower = (long) (leakpower + totalInBW * latchMetric[0] + lowBWInPorts * 0.15
                        + highBWInPorts * 5.36 + delay / 500 * 1.38);
    energy = (long) (energy + totalInBW * latchMetric[1] + lowBWInPorts * 4.516
                     + highBWInPorts * 20.19 + delay / 500 * 28.544);
    long *twoToOneMetric = metrics->getOpMetric("twoToOne");
    if (twoToOneMetric == nullptr) {
      fatal_error("We could not find metric for 2-in-1-out!\n");
    }
    delay = delay + twoToOneMetric[2] + latchMetric[2];
    /* adjust perf number in case there are BUFFs (i.e., "register" in Verilog) */
    if (DEBUG_OPTIMIZER) {
      printf("buffBWs: ");
      for (auto &buffBW : buffBWs) {
        printf("%u ", buffBW);
      }
      printf("\n");
    }
    unsigned numBuffs = buffBWs.size();
    if (numBuffs > numOuts) {
      fatal_error("%s has more buffs (%u) than outputs(%u)!\n", normalizedOp, numBuffs,
                  numOuts);
    }
    for (auto &buffBW : buffBWs) {
      char *regName = new char[10];
      sprintf(regName, "reg%u", buffBW);
      long *regMetric = metrics->getOpMetric(regName);
      if (regMetric == nullptr) {
        fatal_error("We could not find metric for %s!\n", regName);
      }
      leakpower += regMetric[0];
      energy += regMetric[1];
      delay += regMetric[2];
      area += regMetric[3];
    }
    /* get the final metric */
    metric = new long[4];
    metric[0] = leakpower;
    metric[1] = energy;
    metric[2] = delay;
    metric[3] = area;
    metrics->updateMetrics(normalizedOp, metric);
    metrics->writeMetricsFile(normalizedOp, metric);
#endif
  }

  processGenerator.createFULib(libFp, confFp, procName, calc, def, outSend,
                               numArgs,
                               numOuts, instance, metric, resBWList,
                               outWidthList, queryResSuffixs, queryResSuffixs2);
  if (metric != nullptr) {
    metrics->updateStatistics(opName, metric[3], metric[0]);
  }
}

void
ChpGenerator::handleDFlowFunc(FILE *resFp, FILE *libFp, FILE *confFp,
                              Process *p,
                              act_dataflow_element *d,
                              char *procName, char *calc,
                              char *def, StringVec &argList,
                              StringVec &oriArgList,
                              UIntVec &argBWList,
                              UIntVec &resBWList, int &result_suffix,
                              StringVec &outSendStr,
                              IntVec &outResSuffixs,
                              StringVec &outList, StringVec &normalizedOutList,
                              UIntVec &outWidthList,
                              Map<unsigned, unsigned long> &initMap,
                              Map<unsigned, unsigned long> &buffMap,
                              IntVec &boolResSuffixs,
                              Map<const char *, Expr *> &exprMap,
                              StringMap<unsigned> &inBW,
                              StringMap<unsigned> &hiddenBW,
                              IntVec &queryResSuffixs,
                              IntVec &queryResSuffixs2,
                              Map<int, int> &outRecord,
                              Map<Expr *, Expr *> &hiddenExprs, UIntVec
                              &buffBWs,
                              Map<const char *, unsigned> &procCount) {
  if (d->t != ACT_DFLOW_FUNC) {
    dflow_print(stdout, d);
    printf("This is not dflow_func!\n");
    exit(-1);
  }
  Scope *sc = p->CurScope();
  /* handle left hand side */
  Expr *expr = d->u.func.lhs;
  int type = expr->type;
  /* handle right hand side */
  ActId *rhs = d->u.func.rhs;
  char out[10240];
  getActIdName(sc, rhs, out, 10240);
  const char *normalizedOut = removeDot(out);
  unsigned outWidth = getActIdBW(rhs, p);
  if (debug_verbose) {
    printf("%%%%%%%%%%%%%%%%%%%%%%\nHandle expr ");
    print_expr(stdout, expr);
    printf("\n%%%%%%%%%%%%%%%%%%%%%%\n");
  }
  Expr *initExpr = d->u.func.init;
  Expr *nbufs = d->u.func.nbufs;
  if (initExpr) {
    if (initExpr->type != E_INT) {
      print_expr(stdout, initExpr);
      printf("The init value is not E_INT type!\n");
      exit(-1);
    }
  }
  if (nbufs) {
    printf("[nbuf] ");
    rhs->Print(stdout);
    printf(", nBuff: ");
    print_expr(stdout, nbufs);
    printf(", isInit: %d", (initExpr != nullptr));
    printf("\n");
  }
  if (type == E_INT) {
    unsigned long val = expr->u.v;
    printInt(resFp, libFp, confFp, out, normalizedOut, val, outWidth);
    if (initExpr) {
      print_expr(stdout, expr);
      printf(" has const lOp, but its rOp has init token!\n");
      exit(-1);
    }
  } else {
    bool constant = false;
    char *calcStr = new char[1500];
    calcStr[0] = '\0';
    unsigned result_bw = outWidth;
    if (debug_verbose) {
      printf("Start to print expression ");
      print_expr(stdout, expr);
      printf(", its type: %d\n", expr->type);
    }
    const char *exprStr = printExpr(sc, expr, procName, calc, def, argList,
                                    oriArgList,
                                    argBWList, resBWList,
                                    result_suffix, result_bw, constant, calcStr,
                                    boolResSuffixs, exprMap,
                                    inBW, hiddenBW, queryResSuffixs,
                                    queryResSuffixs2, hiddenExprs);
    if (constant) {
      print_expr(stdout, expr);
      printf("=> we should not process constant lhs here!\n");
      exit(-1);
    }
    /* check if the expression only has E_VAR. Note that it could be built-in int/bool, e.g., int(varName, bw). In
     * this case, it still only has E_VAR expression. */
    bool onlyVarExpr = false;
    if (type == E_VAR) {
      onlyVarExpr = true;
    } else if ((type == E_BUILTIN_INT) || (type == E_BUILTIN_BOOL)) {
      onlyVarExpr = (expr->u.e.l->type == E_VAR);
    }
    if (onlyVarExpr) {
      if (debug_verbose) {
        printf("The expression ");
        print_expr(stdout, expr);
        printf(" is E_VAR!\n");
      }
      char *subProc = new char[6];
      if (strlen(procName)) {
        sprintf(subProc, "_port");
      } else {
        sprintf(subProc, "func_port");
      }
      strcat(procName, subProc);
      result_suffix++;
      resBWList.push_back(result_bw);
      char *subCalc = new char[1500];
      sprintf(subCalc, "      res%d := %s;\n", result_suffix, exprStr);
      strcat(calc, subCalc);
      char *resName = new char[5];
      sprintf(resName, "res%d", result_suffix);
      Expr *resRHS = getExprFromName(resName, exprMap, false, E_VAR);
      Expr *xExpr = getExprFromName(exprStr, exprMap, false, E_VAR);
      hiddenBW.insert(GenPair(resName, result_bw));
      hiddenExprs.insert(GenPair(resRHS, xExpr));
    }

    if (initExpr) {
      unsigned long initVal = initExpr->u.v;
      char *subProcName = new char[1500];
      sprintf(subProcName, "_init%lu", initVal);
      strcat(procName, subProcName);
      buffBWs.push_back(result_bw);
    }
    if (DEBUG_CLUSTER) {
      printf(
          "___________________________________\n\n\n\n\n\nFor dataflow element: ");
      dflow_print(stdout, d);
      printf("\n___________________________________________\n");
      printf("procName: %s\n", procName);
      printf("arg list:\n");
      for (auto &arg : argList) {
        printf("%s ", arg.c_str());
      }
      printf("\n");
      printf("oriArgList:\n");
      for (auto &oriArg : oriArgList) {
        printf("%s ", oriArg.c_str());
      }
      printf("\n");
      printf("arg bw list:\n");
      for (auto &bw : argBWList) {
        printf("%u ", bw);
      }
      printf("\n");
      printf("res bw list:\n");
      for (auto &resBW : resBWList) {
        printf("%u ", resBW);
      }
      printf("\n");
      printf("out bw: %d\n", outWidth);
      printf("def: %s\n", def);
      printf("calc: %s\n", calc);
      printf("result_suffix: %d\n", result_suffix);
      printf("normalizedOut: %s, out: %s\n", normalizedOut, out);
      printf("init expr: ");
      print_expr(stdout, initExpr);
      printf("\n");
    }
    outList.push_back(out);
    normalizedOutList.push_back(normalizedOut);
    outWidthList.push_back(outWidth);
    char *outStr = new char[10240];
    outStr[0] = '\0';
    int numOuts = outList.size();
    char *outName = new char[10240];
    int outID = numOuts - 1;
    sprintf(outName, "out%d", outID);
    sprintf(outStr, "out%d!res%d", outID, result_suffix);
    queryResSuffixs.push_back(result_suffix);
    outRecord.insert(GenPair(outID, result_suffix));
    if (initExpr) {
      unsigned long initVal = initExpr->u.v;
      initMap.insert(GenPair((numOuts - 1), initVal));
    }
    if (nbufs) {
      unsigned long numBuff = nbufs->u.v;
      buffMap.insert(GenPair((numOuts - 1), numBuff));
    }
    if (DEBUG_CLUSTER) {
      printf("@@@@@@@@@@@@@@@@ generate %s\n", outStr);
    }
    outSendStr.push_back(outStr);
    outResSuffixs.push_back(result_suffix);
    char outWidthStr[1024];
    sprintf(outWidthStr, "_%d", outWidth);
    strcat(procName, outWidthStr);
    if (!nbufs && !initExpr) {
      /* if this process contains BUFF, then it is stateful and can NOT be
       * shared! */
      updateprocCount(procName, procCount);
    }
  }
}

void
ChpGenerator::updateprocCount(const char *proc, Map<const char *, unsigned>
&procCount) {
  printf("update proc count for %s.\n", proc);
  for (auto &procCountIt : procCount) {
    if (!strcmp(proc, procCountIt.first)) {
      procCountIt.second++;
      return;
    }
  }
  procCount.insert(GenPair(proc, 1));
}

bool ChpGenerator::isActnCp(const char *instance) {
  return std::string(instance).find(Constant::ACTN_CP_PREFIX) == 0;
}

bool ChpGenerator::isActnDp(const char *instance) {
  return std::string(instance).find(Constant::ACTN_DP_PREFIX) == 0;
}

void ChpGenerator::checkACTN(const char *channel, bool &actnCp, bool &actnDp) {
  if (isActnCp(channel)) {
    actnCp = true;
  }
  if (isActnDp(channel)) {
    actnDp = true;
  }
  if (actnCp && actnDp) {
    printf("%s is both actnCp and actnDp!\n", channel);
    exit(-1);
  }
}

void
ChpGenerator::updateACTN(long area, long leakPower, bool actnCp, bool actnDp) {
  if (actnCp) {
    metrics->updateACTNCpMetrics(area, leakPower);
  } else if (actnDp) {
    metrics->updateACTNDpMetrics(area, leakPower);
  }
}

void
ChpGenerator::handleNormalDflowElement(FILE *resFp, FILE *libFp, FILE *confFp,
                                       Process *p, act_dataflow_element *d,
                                       unsigned &sinkCnt,
                                       Map<const char *, unsigned> &procCount) {
  Scope *sc = p->CurScope();
  switch (d->t) {
    case ACT_DFLOW_FUNC: {
      char *procName = new char[MAX_PROC_NAME_LEN];
      procName[0] = '\0';
//      sprintf(procName, "func_");
      char *calc = new char[MAX_CALC_LEN];
      sprintf(calc, "\n");
      IntVec boolResSuffixs;
      char *def = new char[10240];
      def[0] = '\0';
      sprintf(def, "\n");
      StringVec argList;
      StringVec oriArgList;
      UIntVec argBWList;
      UIntVec resBWList;
      int result_suffix = -1;
      StringVec outSendStr;
      IntVec outResSuffixs;
      StringVec outList;
      StringVec normalizedOutList;
      UIntVec outWidthList;
      Map<unsigned, unsigned long> initMap;
      Map<unsigned, unsigned long> buffMap;
      Map<const char *, Expr *> exprMap;
      StringMap<unsigned> inBW;
      StringMap<unsigned> hiddenBW;
      IntVec queryResSuffixs;
      IntVec queryResSuffixs2;
      Map<int, int> outRecord;
      Map<Expr *, Expr *> hiddenExprs;
      UIntVec buffBWs;
      handleDFlowFunc(resFp, libFp, confFp, p, d, procName, calc, def, argList,
                      oriArgList, argBWList, resBWList, result_suffix,
                      outSendStr,
                      outResSuffixs, outList, normalizedOutList, outWidthList,
                      initMap,
                      buffMap, boolResSuffixs, exprMap, inBW, hiddenBW,
                      queryResSuffixs, queryResSuffixs2, outRecord,
                      hiddenExprs,
                      buffBWs, procCount);
      if (DEBUG_CLUSTER) {
        printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        printf("Process normal dflow:\n");
        dflow_print(stdout, d);
        printf("\n");
      }
      if (strlen(calc) > 1) {
        printDFlowFunc(resFp, libFp, confFp, procName, argList, argBWList,
                       resBWList,
                       outWidthList, def, calc,
                       result_suffix, outSendStr, outResSuffixs,
                       normalizedOutList,
                       outList, initMap, buffMap, boolResSuffixs, exprMap, inBW,
                       hiddenBW,
                       queryResSuffixs,
                       queryResSuffixs2,
                       outRecord, hiddenExprs, buffBWs);
      }
      break;
    }
    case ACT_DFLOW_SPLIT: {
      ActId *input = d->u.splitmerge.single;
      unsigned bitwidth = getActIdBW(input, p);
      ActId **outputs = d->u.splitmerge.multi;
      int numOutputs = d->u.splitmerge.nmulti;
      char *procName = new char[MAX_PROC_NAME_LEN];
      sprintf(procName, "%s_%d", Constant::SPLIT_PREFIX, numOutputs);
      ActId *guard = d->u.splitmerge.guard;
      unsigned guardBW = getActIdBW(guard, p);
      char *splitName = new char[2000];
      char *inputName = new char[10240];
      getActIdName(sc, input, inputName, 10240);
      const char *normalizedInput = removeDot(inputName);
      sprintf(splitName, "%s", normalizedInput);
      char *guardName = new char[10240];
      getActIdName(sc, guard, guardName, 10240);
      const char *normalizedGuard = removeDot(guardName);
      strcat(splitName, normalizedGuard);
      CharPtrVec sinkVec;
      bool actnCp = false;
      bool actnDp = false;
      for (int i = 0; i < numOutputs; i++) {
        ActId *out = outputs[i];
        if (!out) {
          strcat(splitName, "sink_");
          char *sinkName = new char[2100];
          sprintf(sinkName, "sink%d", sinkCnt);
          sinkCnt++;
          sinkVec.push_back(sinkName);
        } else {
          char *outName = new char[10240];
          getActIdName(sc, out, outName, 10240);
          const char *normalizedOut = removeDot(outName);
          strcat(splitName, normalizedOut);
          checkACTN(normalizedOut, actnCp, actnDp);
        }
      }
      for (auto &sink : sinkVec) {
        fprintf(resFp, "chan(int<%d>) %s;\n", bitwidth, sink);
        printSink(resFp, libFp, confFp, sink, bitwidth);
      }
      fprintf(resFp, "%s<%d,%d> %s(", procName, guardBW, bitwidth, splitName);
      if (actnCp) {
        printf("[actnCp]: %s\n", splitName);
      } else if (actnDp) {
        printf("[actnDp]: %s\n", splitName);
      }
      if (debug_verbose) {
        printf("[split]: %s\n", splitName);
      }
      const char *guardStr = getActIdOrCopyName(sc, guard);
      const char *inputStr = getActIdOrCopyName(sc, input);
      fprintf(resFp, "%s, %s", guardStr, inputStr);
      for (int i = 0; i < numOutputs; i++) {
        ActId *out = outputs[i];
        if (!out) {
          const char *sinkName = sinkVec.back();
          sinkVec.pop_back();
          fprintf(resFp, ", %s", sinkName);
        } else {
          char *outName = new char[10240];
          getActIdName(sc, out, outName, 10240);
          fprintf(resFp, ", %s", outName);
        }
      }
      fprintf(resFp, ");\n");
      char *instance = new char[MAX_INSTANCE_LEN];
      sprintf(instance, "%s<%d,%d>", procName, guardBW, bitwidth);
      long *metric = metrics->getOpMetric(instance);
      processGenerator.createSplit(libFp, confFp, procName, instance, metric,
                                   numOutputs);
      if (metric != nullptr) {
        updateStatistics(metric, instance, actnCp, actnDp);
        long area = metric[3];
        long leakPower = metric[0];
        metrics->updateSplitMetrics(area, leakPower);
      } else if (LOGIC_OPTIMIZER) {
        printf("We could not find metrics for the SPLIT %s!\n", instance);
        exit(-1);
      }
      break;
    }
    case ACT_DFLOW_MERGE: {
      ActId *output = d->u.splitmerge.single;
      char *outputName = new char[10240];
      getActIdName(sc, output, outputName, 10240);
      const char *normalizedOutput = removeDot(outputName);
      bool actnCp = false;
      bool actnDp = false;
      checkACTN(normalizedOutput, actnCp, actnDp);
      unsigned inBW = getActIdBW(output, p);
      ActId *guard = d->u.splitmerge.guard;
      unsigned guardBW = getActIdBW(guard, p);
      int numInputs = d->u.splitmerge.nmulti;
      char *procName = new char[MAX_PROC_NAME_LEN];
      sprintf(procName, "%s_%d", Constant::MERGE_PREFIX, numInputs);
      fprintf(resFp, "%s<%d,%d> %s_inst(", procName, guardBW, inBW,
              normalizedOutput);
      if (debug_verbose) {
        printf("[merge]: %s_inst\n", normalizedOutput);
        if (actnCp) {
          printf("[actnCp]: %s_inst\n", normalizedOutput);
        } else if (actnDp) {
          printf("[actnDp]: %s_inst\n", normalizedOutput);
        }
      }
      const char *guardStr = getActIdOrCopyName(sc, guard);
      fprintf(resFp, "%s, ", guardStr);

      ActId **inputs = d->u.splitmerge.multi;
      for (int i = 0; i < numInputs; i++) {
        ActId *in = inputs[i];
        const char *inStr = getActIdOrCopyName(sc, in);
        fprintf(resFp, "%s, ", inStr);
      }
      fprintf(resFp, "%s);\n", outputName);

      char *instance = new char[MAX_INSTANCE_LEN];
      sprintf(instance, "%s<%d,%d>", procName, guardBW, inBW);
      long *metric = metrics->getOpMetric(instance);
      processGenerator.createMerge(libFp, confFp, procName, instance, metric,
                                   numInputs);
      if (metric != nullptr) {
        updateStatistics(metric, instance, actnCp, actnDp);
        long area = metric[3];
        long leakPower = metric[0];
        metrics->updateMergeMetrics(area, leakPower);
      } else if (LOGIC_OPTIMIZER) {
        printf("We could not find metrics for the MERGE %s!\n", instance);
        exit(-1);
      }
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
    case ACT_DFLOW_CLUSTER: {
      dflow_print(stdout, d);
      fatal_error("We should not process dflow_clsuter here!");
    }
    case ACT_DFLOW_SINK: {
      ActId *input = d->u.sink.chan;
      char *inputName = new char[10240];
      getActIdName(sc, input, inputName, 10240);
      unsigned bw = getBitwidth(input->Canonical(sc));
      printSink(resFp, libFp, confFp, inputName, bw);
      if (debug_verbose) {
        printf("%s is not used anywhere!\n", inputName);
      }
      break;
    }
    default: {
      fatal_error("Unknown dataflow type %d\n", d->t);
    }
  }
}

void ChpGenerator::print_dflow(FILE *fp, list_t *dflow) {
  listitem_t *li;
  act_dataflow_element *e;

  for (li = list_first (dflow); li; li = list_next (li)) {
    e = (act_dataflow_element *) list_value (li);
    dflow_print(fp, e);
    if (list_next (li)) {
      fprintf(fp, ";");
    }
    fprintf(fp, "\n");
  }
}

void
ChpGenerator::handleDFlowCluster(FILE *resFp, FILE *libFp, FILE *confFp,
                                 Process *p,
                                 list_t *dflow,
                                 Map<const char *, unsigned> &procCount) {
  listitem_t *li;
  char *procName = new char[MAX_CLUSTER_PROC_NAME_LEN];
  procName[0] = '\0';
//  sprintf(procName, "func_");
  char *calc = new char[MAX_CALC_LEN];
  calc[0] = '\0';
  sprintf(calc, "\n");
  IntVec boolResSuffixs;
  char *def = new char[10240];
  def[0] = '\0';
  sprintf(def, "\n");
  StringVec argList;
  StringVec oriArgList;
  UIntVec argBWList;
  UIntVec resBWList;
  int result_suffix = -1;
  StringVec outSendStr;
  IntVec outResSuffixs;
  StringVec outList;
  StringVec normalizedOutList;
  UIntVec outWidthList;
  Map<unsigned, unsigned long> initMap;
  Map<unsigned, unsigned long> buffMap;
  Map<const char *, Expr *> exprMap;
  StringMap<unsigned> inBW;
  StringMap<unsigned> hiddenBW;
  IntVec queryResSuffixs;
  IntVec queryResSuffixs2;
  Map<int, int> outRecord;
  Map<Expr *, Expr *> hiddenExprs;
  unsigned elementCnt = 0;
  UIntVec buffBWs;
  for (li = list_first (dflow); li; li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    if (DEBUG_CLUSTER) {
      printf("Start to process dflow_cluster element ");
      dflow_print(stdout, d);
      printf(", current proc name: %s\n", procName);
    }
    if (d->t == ACT_DFLOW_FUNC) {
      handleDFlowFunc(resFp, libFp, confFp, p, d, procName, calc, def, argList,
                      oriArgList, argBWList, resBWList, result_suffix,
                      outSendStr,
                      outResSuffixs, outList, normalizedOutList, outWidthList,
                      initMap,
                      buffMap, boolResSuffixs, exprMap, inBW, hiddenBW,
                      queryResSuffixs, queryResSuffixs2, outRecord,
                      hiddenExprs,
                      buffBWs, procCount);
      char *subProc = new char[1024];
      sprintf(subProc, "_p%d", elementCnt);
      elementCnt++;
      strcat(procName, subProc);
    } else {
      dflow_print(stdout, d);
      fatal_error("This dflow statement should not appear in dflow-cluster!\n");
    }
    if (DEBUG_CLUSTER) {
      printf("After processing dflow_cluster element ");
      dflow_print(stdout, d);
      printf(", the proc name: %s\n", procName);
    }
  }
  if (DEBUG_CLUSTER) {
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("Process cluster dflow:\n");
    print_dflow(stdout, dflow);
    printf("\n");
  }
  if (strlen(calc) > 1) {
    printDFlowFunc(resFp, libFp, confFp, procName, argList, argBWList,
                   resBWList,
                   outWidthList, def,
                   calc,
                   result_suffix, outSendStr, outResSuffixs, normalizedOutList,
                   outList,
                   initMap, buffMap, boolResSuffixs, exprMap, inBW, hiddenBW,
                   queryResSuffixs,
                   queryResSuffixs2,
                   outRecord,
                   hiddenExprs, buffBWs);
  }
}

ChpGenerator::ChpGenerator(Act *a, const char *name, Metrics *metrics)
    : ActPass(a, name) {
  processGenerator.initialize();
  this->metrics = metrics;
}

void ChpGenerator::genMemConfiguration(FILE *confFp, const char *procName) {
  processGenerator.genMemConfiguration(confFp, procName);
}

void
ChpGenerator::handleProcess(FILE *resFp, FILE *libFp, FILE *confFp, Process *p,
                            Map<const char *, unsigned> &procCount) {
  const char *pName = p->getName();
  printf("processing %s\n", pName);
  if (p->getlang()->getchp()) {
    p->Print(libFp);
    return;
  }
  if (!p->getlang()->getdflow()) {
    fatal_error("Process `%s': no dataflow body", p->getName());
  }
  p->PrintHeader(resFp, "defproc");
  fprintf(resFp, "\n{");
  p->CurScope()->Print(resFp);
  bitwidthMap.clear();
  opUses.clear();
  copyUses.clear();
  collectBitwidthInfo(p);
  collectOpUses(p);
  createCopyProcs(resFp, libFp, confFp);
  listitem_t *li;
  unsigned sinkCnt = 0;
  for (li = list_first (p->getlang()->getdflow()->dflow); li; li = list_next (
      li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    if (d->t == ACT_DFLOW_CLUSTER) {
      list_t *dflow_cluster = d->u.dflow_cluster;
      handleDFlowCluster(resFp, libFp, confFp, p, dflow_cluster, procCount);
    } else {
      handleNormalDflowElement(resFp, libFp, confFp, p, d, sinkCnt, procCount);
    }
  }
  fprintf(resFp, "}\n\n");
}

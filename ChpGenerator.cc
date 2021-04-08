#include "ChpGenerator.h"

void ChpGenerator::printIntVec(IntVec &intVec) {
  for (auto &val : intVec) {
    printf("%d ", val);
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

const char *ChpGenerator::getActIdName(ActId *actId) {
  char *str = new char[100];
  if (actId) {
    const char *actName = actId->getName();
    unsigned outUses = getOpUses(actName);
    if (outUses) {
      unsigned copyUse = getCopyUses(actName);
      if (copyUse <= outUses) {
        sprintf(str, "%scopy.out[%u]", actId->getName(), copyUse);
      } else {
        sprintf(str, "%s", actId->getName());
      }
    } else {
      sprintf(str, "%s", actId->getName());
    }
  } else {
    sprintf(str, "*");
  }
  return str;
}

void
ChpGenerator::printSink(FILE *resFp, FILE *libFp, FILE *confFp, const char *name,
                        int bitwidth) {
  if (name == nullptr) {
    fatal_error("sink name is NULL!\n");
  }
  fprintf(resFp, "sink<%d> %s_sink(%s);\n", bitwidth, name, name);
  char *instance = new char[1500];
  sprintf(instance, "sink<%d>", bitwidth);
  int *metric = metrics->getOpMetric(instance);
  processGenerator.createSink(libFp, confFp, instance, metric);
  if (metric != nullptr) {
    metrics->updateAreaStatistics(instance, metric[3]);
  }
}

void ChpGenerator::printInt(FILE *resFp, FILE *libFp, FILE *confFp, const char *out,
                            const char *normalizedOut,
                            unsigned val, int outWidth) {
  fprintf(resFp, "source<%u,%d> %s_inst(%s);\n", val, outWidth, normalizedOut, out);
  char *instance = new char[1500];
  sprintf(instance, "source<%u,%d>", val, outWidth);
  char *opName = new char[1500];
  sprintf(opName, "source%d", outWidth);
  int *metric = metrics->getOpMetric(opName);
  processGenerator.createSource(libFp, confFp, instance, metric);
  if (metric != nullptr) {
    metrics->updateAreaStatistics(instance, metric[3]);
  }
}

void ChpGenerator::collectBitwidthInfo(Process *p) {
  ActInstiter inst(p->CurScope());
  for (inst = inst.begin(); inst != inst.end(); inst++) {
    ValueIdx *vx = *inst;
    const char *varName = vx->getName();
    int bitwidth = TypeFactory::bitWidth(vx->t);
    bitwidthMap.insert(std::make_pair(varName, bitwidth));
  }
}

void ChpGenerator::printBitwidthInfo() {
  printf("bitwidth info:\n");
  for (auto bitwidthMapIt = bitwidthMap.begin(); bitwidthMapIt != bitwidthMap.end();
       bitwidthMapIt++) {
    printf("(%s, %d) ", bitwidthMapIt->first, bitwidthMapIt->second);
  }
  printf("\n");
}

int ChpGenerator::getActIdBW(ActId *actId, Process *p) {
  Array **aref = nullptr;
  InstType *instType = p->CurScope()->FullLookup(actId, aref);
  return TypeFactory::bitWidth(instType);
}

int ChpGenerator::getBitwidth(const char *varName) {
  for (auto &bitwidthMapIt : bitwidthMap) {
    const char *record = bitwidthMapIt.first;
    if (strcmp(record, varName) == 0) {
      return bitwidthMapIt.second;
    }
  }
  printf("We could not find bitwidth info for %s\n", varName);
  printBitwidthInfo();
  exit(-1);
}

void ChpGenerator::getCurProc(const char *str, char *val) {
  char curProc[100];
  if (strstr(str, "res")) {
    sprintf(curProc, "r%s", str + 3);
  } else if (strstr(str, "x")) {
    sprintf(curProc, "%s", str + 1);
  } else {
    sprintf(curProc, "c%s", str);
  }
  strcpy(val, curProc);
}

const char *
ChpGenerator::EMIT_QUERY(Expr *expr, const char *sym, const char *op, int type,
                         const char *metricSym,
                         char *procName, char *calc, char *def, StringVec &argList,
                         StringVec
                         &oriArgList, IntVec &argBWList,
                         IntVec &resBWList, int &result_suffix, int result_bw,
                         char *calcStr,
                         IntVec &boolRes) {
  Expr *cExpr = expr->u.e.l;
  Expr *lExpr = expr->u.e.r->u.e.l;
  Expr *rExpr = expr->u.e.r->u.e.r;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }

  bool cConst = false;
  char *cCalcStr = new char[1500];
  const char *cStr = printExpr(cExpr, procName, calc, def, argList, oriArgList, argBWList,
                               resBWList, result_suffix, result_bw, cConst, cCalcStr,
                               boolRes);
  boolRes.push_back(result_suffix);
  bool lConst = false;
  char *lCalcStr = new char[1500];
  const char *lStr = printExpr(lExpr, procName, calc, def, argList, oriArgList, argBWList,
                               resBWList,
                               result_suffix, result_bw, lConst, lCalcStr, boolRes);
  bool rConst = false;
  char *rCalcStr = new char[1500];
  const char *rStr = printExpr(rExpr, procName, calc, def, argList, oriArgList, argBWList,
                               resBWList,
                               result_suffix, result_bw, rConst, rCalcStr, boolRes);

  char *newExpr = new char[100];
  result_suffix++;
  sprintf(newExpr, "res%d", result_suffix);
  char *curCal = new char[300];
  sprintf(curCal, "      res%d := %s ? %s : %s;\n", result_suffix, cStr, lStr, rStr);
  strcat(calc, curCal);
  resBWList.push_back(result_bw);
//  printf("      res%d := %s ? %s : %s;\n", result_suffix, cStr, lStr, rStr);
//  printf("  wire [%d:0] res%d;\n", (result_bw-1), result_suffix);

  char *lVal = new char[100];
  getCurProc(lStr, lVal);
  char *rVal = new char[100];
  getCurProc(rStr, rVal);

  char *subProcName = new char[1500];
  sprintf(subProcName, "_%s%s%s", lVal, sym, rVal);
  strcat(procName, subProcName);
  if (DEBUG_VERBOSE) {
    printf("\n\n\n\n\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
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
      printf("%d ", bw);
    }
    printf("\n");
    printf("res bw list: ");
    for (auto &bw2:resBWList) {
      printf("%d ", bw2);
    }
    printf("\n");
  }
  sprintf(calcStr, "%s", newExpr);
  return newExpr;
}

const char *
ChpGenerator::EMIT_BIN(Expr *expr, const char *sym, const char *op, int type,
                       const char *metricSym,
                       char *procName, char *calc, char *def, StringVec &argList,
                       StringVec
                       &oriArgList, IntVec &argBWList, IntVec &resBWList,
                       int &result_suffix,
                       int result_bw, char *calcStr, IntVec &boolRes) {
  Expr *lExpr = expr->u.e.l;
  Expr *rExpr = expr->u.e.r;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  bool lConst = false;
  char *lCalcStr = new char[1500];
  const char *lStr = printExpr(lExpr, procName, calc, def, argList, oriArgList, argBWList,
                               resBWList,
                               result_suffix, result_bw, lConst, lCalcStr, boolRes);
  bool rConst = false;
  char *rCalcStr = new char[1500];
  const char *rStr = printExpr(rExpr, procName, calc, def, argList, oriArgList, argBWList,
                               resBWList,
                               result_suffix, result_bw, rConst, rCalcStr, boolRes);
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
  resBWList.push_back(result_bw);
//  printf("      res%d := %s %s %s;\n", result_suffix, lStr, op, rStr);
//  printf("  wire [%d:0] res%d;\n", (result_bw-1), result_suffix);

  char *lVal = new char[100];
  getCurProc(lStr, lVal);
  char *rVal = new char[100];
  getCurProc(rStr, rVal);

  char *subProcName = new char[1500];
  sprintf(subProcName, "_%s%s%s", lVal, sym, rVal);
  strcat(procName, subProcName);
  if (DEBUG_VERBOSE) {
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
      printf("%d ", bw);
    }
    printf("\n");
    printf("res bw list: ");
    for (auto &bw2:resBWList) {
      printf("%d ", bw2);
    }
    printf("\n");
  }
  sprintf(calcStr, "%s", newExpr);
  return newExpr;
}

const char *
ChpGenerator::EMIT_UNI(Expr *expr, const char *sym, const char *op, int type,
                       const char *metricSym,
                       char *procName, char *calc, char *def, StringVec &argList,
                       StringVec &oriArgList,
                       IntVec &argBWList,
                       IntVec &resBWList, int &result_suffix, int result_bw,
                       char *calcStr,
                       IntVec &boolRes) {
  /* collect bitwidth info */
  Expr *lExpr = expr->u.e.l;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  bool lConst;
  char *lCalcStr = new char[1500];
  const char *lStr = printExpr(lExpr, procName, calc, def, argList, oriArgList, argBWList,
                               resBWList,
                               result_suffix, result_bw, lConst, lCalcStr, boolRes);
//  if (lConst) {
//    print_expr(stdout, expr);
//    printf(" has const operands!\n");
//    exit(-1);
//  }
  char *val = new char[100];
  getCurProc(lStr, val);
  sprintf(procName, "%s_%s%s", procName, sym, val);
  char *newExpr = new char[100];
  result_suffix++;
  sprintf(newExpr, "res%d", result_suffix);
  char *curCal = new char[300];
  sprintf(curCal, "      res%d := %s %s;\n", result_suffix, op, lStr);
  resBWList.push_back(result_bw);
//  printf("      res%d := %s %s;\n", result_suffix, op, lStr);
//  printf("  wire [%d:0] res%d;\n", (result_bw-1), result_suffix);
  strcat(calc, curCal);
  if (DEBUG_VERBOSE) {
    printf("unary expr: ");
    print_expr(stdout, expr);
    printf("\ndflowmap generates calc: %s\n", calc);
  }
  sprintf(calcStr, "%s", newExpr);
  return newExpr;
}

const char *
ChpGenerator::printExpr(Expr *expr, char *procName, char *calc, char *def,
                        StringVec &argList,
                        StringVec &oriArgList, IntVec &argBWList,
                        IntVec &resBWList, int &result_suffix, int result_bw,
                        bool &constant,
                        char *calcStr, IntVec &boolRes) {
  int type = expr->type;
  switch (type) {
    case E_INT: {
      if (procName[0] == '\0') {
        fatal_error("we should NOT process Source here!\n");
      }
      unsigned int val = expr->u.v;
      const char *valStr = strdup(std::to_string(val).c_str());
      sprintf(calcStr, "%s", valStr);
      constant = true;
      return valStr;
    }
    case E_VAR: {
      int numArgs = argList.size();
      auto actId = (ActId *) expr->u.e.l;
      const char *oriVarName = actId->getName();
      char *curArg = new char[1000];
      const char *mappedVarName = getActIdName(actId);
      int idx = searchStringVec(oriArgList, oriVarName);
      if (idx == -1) {
        oriArgList.push_back(oriVarName);
        argList.push_back(mappedVarName);
        sprintf(calcStr, "%s_%d", oriVarName, numArgs);
        sprintf(curArg, "x%d", numArgs);
      } else {
        sprintf(calcStr, "%s_%d", oriVarName, idx);
        sprintf(curArg, "x%d", idx);
      }
      int argBW = getBitwidth(oriVarName);
      argBWList.push_back(argBW);
      if (procName[0] == '\0') {
        resBWList.push_back(result_bw);
      }
      return curArg;
    }
    case E_AND: {
      return EMIT_BIN(expr, "and", "&", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_OR: {
      return EMIT_BIN(expr, "or", "|", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_NOT: {
      return EMIT_UNI(expr, "not", "~", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_PLUS: {
      return EMIT_BIN(expr, "add", "+", type, "add", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_MINUS: {
      return EMIT_BIN(expr, "minus", "-", type, "add", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_MULT: {
      return EMIT_BIN(expr, "mul", "*", type, "mul", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_DIV: {
      return EMIT_BIN(expr, "div", "/", type, "div", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_MOD: {
      return EMIT_BIN(expr, "mod", "%", type, "rem", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_LSL: {
      return EMIT_BIN(expr, "lsl", "<<", type, "lshift", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_LSR: {
      return EMIT_BIN(expr, "lsr", ">>", type, "lshift", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_ASR: {
      return EMIT_BIN(expr, "asr", ">>>", type, "lshift", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_UMINUS: {
      return EMIT_UNI(expr, "neg", "-", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_XOR: {
      return EMIT_BIN(expr, "xor", "^", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_LT: {
      return EMIT_BIN(expr, "lt", "<", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_GT: {
      return EMIT_BIN(expr, "gt", ">", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_LE: {
      return EMIT_BIN(expr, "le", "<=", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_GE: {
      return EMIT_BIN(expr, "ge", ">=", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_EQ: {
      return EMIT_BIN(expr, "eq", "=", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, 1, calcStr, boolRes);
    }
    case E_NE: {
      return EMIT_BIN(expr, "ne", "!=", type, "icmp", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_COMPLEMENT: {
      return EMIT_UNI(expr, "compl", "~", type, "and", procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList,
                      result_suffix, result_bw, calcStr, boolRes);
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      Expr *rExpr = expr->u.e.r;
      unsigned int bw;
      if (rExpr) {
        bw = rExpr->u.v;
      } else {
        bw = 1;  //TODO: double check
      }
      return printExpr(lExpr, procName, calc, def, argList, oriArgList, argBWList,
                       resBWList,
                       result_suffix, bw, constant, calcStr, boolRes);
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      int bw = 1;
      return printExpr(lExpr, procName, calc, def, argList, oriArgList,
                       argBWList, resBWList, result_suffix, bw,
                       constant, calcStr, boolRes);
    }
    case E_QUERY: {
      return EMIT_QUERY(expr, "query", "?", type, "query", procName, calc, def,
                        argList, oriArgList, argBWList, resBWList, result_suffix,
                        result_bw, calcStr, boolRes);
    }
    default: {
      print_expr(stdout, expr);
      printf("\n");
      fatal_error("when printing expression, encounter unknown expression type %d\n",
                  type);
      break;
    }
  }
  fatal_error("Shouldn't be here");
  return "-should-not-be-here-";
}

unsigned ChpGenerator::getCopyUses(const char *op) {
  auto copyUsesIt = copyUses.find(op);
  if (copyUsesIt == copyUses.end()) {
    printf("We don't know how many times %s is used as COPY!\n", op);
    exit(-1);
  }
  unsigned uses = copyUsesIt->second;
  copyUsesIt->second++;
  return uses;
}

void ChpGenerator::updateOpUses(const char *op) {
  auto opUsesIt = opUses.find(op);
  if (opUsesIt == opUses.end()) {
    opUses.insert(std::make_pair(op, 0));
    copyUses.insert(std::make_pair(op, 0));
  } else {
    opUsesIt->second++;
  }
}

void ChpGenerator::recordOpUses(const char *op, CharPtrVec &charPtrVec) {
  if (std::find(charPtrVec.begin(), charPtrVec.end(), op) == charPtrVec.end()) {
    charPtrVec.push_back(op);
  }
}

void ChpGenerator::printOpUses() {
  printf("OP USES:\n");
  for (auto &opUsesIt : opUses) {
    printf("(%s, %u) ", opUsesIt.first, opUsesIt.second);
  }
  printf("\n");
}

unsigned ChpGenerator::getOpUses(const char *op) {
  auto opUsesIt = opUses.find(op);
  if (opUsesIt == opUses.end()) {
    printf("We don't know how many times %s is used!\n", op);
    printOpUses();
    exit(-1);
  }
  return opUsesIt->second;
}

void ChpGenerator::collectUniOpUses(Expr *expr) {
  Expr *lExpr = expr->u.e.l;
  collectExprUses(lExpr);
}

void ChpGenerator::collectBinOpUses(Expr *expr) {
  Expr *lExpr = expr->u.e.l;
  collectExprUses(lExpr);
  Expr *rExpr = expr->u.e.r;
  collectExprUses(rExpr);
}

void ChpGenerator::recordUniOpUses(Expr *expr, CharPtrVec &charPtrVec) {
  Expr *lExpr = expr->u.e.l;
  recordExprUses(lExpr, charPtrVec);
}

void ChpGenerator::recordBinOpUses(Expr *expr, CharPtrVec &charPtrVec) {
  Expr *lExpr = expr->u.e.l;
  recordExprUses(lExpr, charPtrVec);
  Expr *rExpr = expr->u.e.r;
  recordExprUses(rExpr, charPtrVec);
}

void ChpGenerator::collectExprUses(Expr *expr) {
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
      collectBinOpUses(expr);
      break;
    }
    case E_NOT:
    case E_UMINUS:
    case E_COMPLEMENT: {
      collectUniOpUses(expr);
      break;
    }
    case E_INT: {
      break;
    }
    case E_VAR: {
      auto actId = (ActId *) expr->u.e.l;
      updateOpUses(actId->getName());
      break;
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      collectExprUses(lExpr);
      break;
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      collectExprUses(lExpr);
      break;
    }
    case E_QUERY: {
      Expr *cExpr = expr->u.e.l;
      Expr *lExpr = expr->u.e.r->u.e.l;
      Expr *rExpr = expr->u.e.r->u.e.r;
      collectExprUses(cExpr);
      collectExprUses(lExpr);
      collectExprUses(rExpr);
      break;
    }
    default: {
      print_expr(stdout, expr);
      printf("\nUnknown expression type %d when collecting expr use\n", type);
      exit(-1);
    }
  }
}

void ChpGenerator::recordExprUses(Expr *expr, CharPtrVec &charPtrVec) {
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
      recordBinOpUses(expr, charPtrVec);
      break;
    }
    case E_NOT:
    case E_UMINUS:
    case E_COMPLEMENT: {
      recordUniOpUses(expr, charPtrVec);
      break;
    }
    case E_INT: {
      break;
    }
    case E_VAR: {
      auto actId = (ActId *) expr->u.e.l;
      recordOpUses(actId->getName(), charPtrVec);
      break;
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      recordExprUses(lExpr, charPtrVec);
      break;
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      recordExprUses(lExpr, charPtrVec);
      break;
    }
    case E_QUERY: {
      Expr *cExpr = expr->u.e.l;
      Expr *lExpr = expr->u.e.r->u.e.l;
      Expr *rExpr = expr->u.e.r->u.e.r;
      recordExprUses(cExpr, charPtrVec);
      recordExprUses(lExpr, charPtrVec);
      recordExprUses(rExpr, charPtrVec);
      break;
    }
    default: {
      print_expr(stdout, expr);
      printf("\nUnknown expression type %d when recording expr use\n", type);
      exit(-1);
    }
  }
}

void ChpGenerator::collectDflowClusterUses(list_t *dflow, CharPtrVec &charPtrVec) {
  listitem_t *li;
  for (li = list_first (dflow); li; li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
      case ACT_DFLOW_FUNC: {
        Expr *expr = d->u.func.lhs;
        recordExprUses(expr, charPtrVec);
        break;
      }
      case ACT_DFLOW_SPLIT: {
        ActId *input = d->u.splitmerge.single;
        recordOpUses(input->getName(), charPtrVec);
        ActId *guard = d->u.splitmerge.guard;
        recordOpUses(guard->getName(), charPtrVec);
        break;
      }
      case ACT_DFLOW_MERGE: {
        ActId *guard = d->u.splitmerge.guard;
        recordOpUses(guard->getName(), charPtrVec);
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs < 2) {
          dflow_print(stdout, d);
          fatal_error("\nMerge has less than TWO inputs!\n");
        }
        ActId **inputs = d->u.splitmerge.multi;
        for (int i = 0; i < numInputs; i++) {
          ActId *in = inputs[i];
          recordOpUses(in->getName(), charPtrVec);
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
      default: {
        fatal_error("Unknown dataflow type %d\n", d->t);
        break;
      }
    }
  }
}

void ChpGenerator::collectOpUses(Process *p) {
  listitem_t *li;
  for (li = list_first (p->getlang()->getdflow()->dflow);
       li;
       li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
      case ACT_DFLOW_FUNC: {
        Expr *expr = d->u.func.lhs;
        collectExprUses(expr);
        break;
      }
      case ACT_DFLOW_SPLIT: {
        ActId *input = d->u.splitmerge.single;
        updateOpUses(input->getName());
        ActId *guard = d->u.splitmerge.guard;
        updateOpUses(guard->getName());
        break;
      }
      case ACT_DFLOW_MERGE: {
        ActId *guard = d->u.splitmerge.guard;
        updateOpUses(guard->getName());
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs < 2) {
          dflow_print(stdout, d);
          fatal_error("\nMerge has less than TWO inputs!\n");
        }
        ActId **inputs = d->u.splitmerge.multi;
        for (int i = 0; i < numInputs; i++) {
          ActId *in = inputs[i];
          updateOpUses(in->getName());
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
        CharPtrVec charPtrVec;
        collectDflowClusterUses(d->u.dflow_cluster, charPtrVec);
        for (auto &charPtr : charPtrVec) {
          updateOpUses(charPtr);
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
      int N = uses + 1;
      const char *opName = opUsesIt.first;
      int bitwidth = getBitwidth(opName);
      fprintf(resFp, "copy<%d,%u> %scopy(%s);\n", bitwidth, N, opName, opName);
      char *instance = new char[1500];
      sprintf(instance, "copy<%d,%u>", bitwidth, N);
      int *metric = metrics->getOpMetric(instance);
      processGenerator.createCopy(libFp, confFp, instance, metric);
      metrics->updateCopyStatistics(bitwidth, N);
      if (metric != nullptr) {
        metrics->updateAreaStatistics(instance, metric[3]);
      }
    }
  }
  fprintf(resFp, "\n");
}

void
ChpGenerator::printDFlowFunc(FILE *resFp, FILE *libFp, FILE *confFp,
                             const char *procName, StringVec &argList,
                             IntVec &argBWList, IntVec &resBWList,
                             IntVec &outWidthList, const char *def, char *calc,
                             int result_suffix, StringVec &outSendStr,
                             IntVec &outResSuffixs,
                             StringVec &normalizedOutList, StringVec &outList,
                             StringVec &initStrs, IntVec &boolRes) {
  calc[strlen(calc) - 2] = ';';
  if (DEBUG_CLUSTER) {
    printf("PRINT DFLOW FUNCTION\n");
    printf("procName: %s\n", procName);
    printf("arg list:\n");
    for (auto &arg : argList) {
      printf("%s ", arg.c_str());
    }
    printf("\n");
    printf("arg bw list:\n");
    for (auto &bw : argBWList) {
      printf("%d ", bw);
    }
    printf("\n");
    printf("res bw list:\n");
    for (auto &resBW : resBWList) {
      printf("%d ", resBW);
    }
    printf("\n");
    printf("outWidthList:\n");
    for (auto &outBW : outWidthList) {
      printf("%d ", outBW);
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
    printf("initStrs:\n");
    for (auto &initStr : initStrs) {
      printf("%s ", initStr.c_str());
    }
    printf("\n");
    printf("boolRes: ");
    printIntVec(boolRes);
  }

  char *instance = new char[10240];
  sprintf(instance, "%s<", procName);
  int numArgs = argList.size();
  int i = 0;
  for (; i < numArgs; i++) {
    char *subInstance = new char[100];
    sprintf(subInstance, "%d,", argBWList[i]);
    strcat(instance, subInstance);
  }
  for (auto &outWidth : outWidthList) {
    char *subInstance = new char[100];
    sprintf(subInstance, "%d,", outWidth);
    strcat(instance, subInstance);
  }
  int numRes = resBWList.size();
  for (i = 0; i < numRes - 1; i++) {
    char *subInstance = new char[100];
    sprintf(subInstance, "%d,", resBWList[i]);
    strcat(instance, subInstance);
  }
  char *subInstance = new char[100];
  sprintf(subInstance, "%d>", resBWList[i]);
  strcat(instance, subInstance);

  fprintf(resFp, "%s ", instance);
  for (auto &normalizedOut : normalizedOutList) {
    fprintf(resFp, "%s_", normalizedOut.c_str());
  }
  fprintf(resFp, "inst(");
  for (auto &arg : argList) {
    fprintf(resFp, "%s, ", arg.c_str());
  }
  int numOuts = outList.size();
  if (numOuts < 1) {
    fatal_error("No output is found!\n");
  }
  for (i = 0; i < numOuts - 1; i++) {
    fprintf(resFp, "%s, ", outList[i].c_str());
  }
  fprintf(resFp, "%s);\n", outList[i].c_str());
  /* create chp library */
  if (strlen(instance) < 5) {
    fatal_error("Invalid instance name %s\n", instance);
  }
  char *opName = instance + 5;
  int *metric = metrics->getOpMetric(opName);
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
    sprintf(subLog, "res%d, \",\", ", outResSuffix);
    strcat(log, subLog);
  }
  char *subLog = new char[100];
  sprintf(subLog, "\")\")");
  strcat(log, subLog);
  strcat(outSend, log);
  char *initSend = nullptr;
  int numInitStrs = initStrs.size();
  if (DEBUG_CLUSTER) {
    printf("numInitStrs: %d\n", numInitStrs);
  }
  if (numInitStrs > 0) {
    initSend = new char[10240];
    sprintf(initSend, "    ");
    for (i = 0; i < numInitStrs - 1; i++) {
      char *subInitSend = new char[1500];
      sprintf(subInitSend, "%s, ", initStrs[i].c_str());
      strcat(initSend, subInitSend);
    }
    char *subInitSend = new char[1500];
    sprintf(subInitSend, "%s;\n", initStrs[i].c_str());
    strcat(initSend, subInitSend);
  }
  processGenerator.createFULib(libFp, confFp, procName, calc, def, outSend, initSend,
                               numArgs, numOuts,
                               numRes, instance,
                               metric, boolRes);
  if (metric != nullptr) {
    metrics->updateAreaStatistics(procName, metric[3]);
  }
}

void
ChpGenerator::handleDFlowFunc(FILE *resFp, FILE *libFp, FILE *confFp, Process *p,
                              act_dataflow_element *d,
                              char *procName, char *calc,
                              char *def, StringVec &argList, StringVec &oriArgList,
                              IntVec &argBWList,
                              IntVec &resBWList, int &result_suffix,
                              StringVec &outSendStr,
                              IntVec &outResSuffixs,
                              StringVec &outList, StringVec &normalizedOutList,
                              IntVec &outWidthList, StringVec &initStrs,
                              IntVec &boolRes) {
  if (d->t != ACT_DFLOW_FUNC) {
    dflow_print(stdout, d);
    printf("This is not dflow_func!\n");
    exit(-1);
  }
  /* handle left hand side */
  Expr *expr = d->u.func.lhs;
  int type = expr->type;
  /* handle right hand side */
  ActId *rhs = d->u.func.rhs;
  char out[10240];
  out[0] = '\0';
  rhs->sPrint(out, 10240, NULL, 0);
  const char *normalizedOut = removeDot(out);
  int outWidth = getActIdBW(rhs, p);
  if (DEBUG_VERBOSE) {
    printf("%%%%%%%%%%%%%%%%%%%%%\nHandle expr ");
    print_expr(stdout, expr);
    printf("\n%%%%%%%%%%%%%%%%%%%%%\n");
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
  if (type == E_INT) {
    unsigned int val = expr->u.v;
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
    printExpr(expr, procName, calc, def, argList, oriArgList, argBWList, resBWList,
              result_suffix, outWidth, constant, calcStr, boolRes);
    if (constant) {
      print_expr(stdout, expr);
      printf("=> we should not process constant lhs here!\n");
      exit(-1);
    }
    int numArgs = argList.size();
    if (procName[0] == '\0') {
      sprintf(procName, "func_port");
      if ((numArgs != 1) || (result_suffix != -1) || (calc[0] != '\n')) {
        printf("We are processing expression ");
        print_expr(stdout, expr);
        printf(", the procName is empty, "
               "but the argList or result_suffix or calc is abnormal!\n");
        exit(-1);
      }
      result_suffix++;
      char *subCalc = new char[1500];
      sprintf(subCalc, "res0 := x0;\n");
      strcat(calc, subCalc);
    }
    if (initExpr) {
      int initVal = initExpr->u.v;
      char *subProcName = new char[1500];
      sprintf(subProcName, "_init%d", initVal);
      strcat(procName, subProcName);
    }
    if (DEBUG_CLUSTER) {
      printf("___________________________________\n\n\n\n\n\nFor dataflow element: ");
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
        printf("%d ", bw);
      }
      printf("\n");
      printf("res bw list:\n");
      for (auto &resBW : resBWList) {
        printf("%d ", resBW);
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
    sprintf(outStr, "out%d!res%d", (numOuts - 1), result_suffix);
    char *ase = new char[1500];
    if (initExpr) {
      int initVal = initExpr->u.v;
      sprintf(ase, "out%d!%d", (numOuts - 1), initVal);
      initStrs.push_back(ase);
    }
    if (DEBUG_CLUSTER) {
      printf("@@@@@@@@@@@@@@@@ generate %s\n", outStr);
    }
    outSendStr.push_back(outStr);
    outResSuffixs.push_back(result_suffix);
  }
}

void ChpGenerator::handleNormalDflowElement(FILE *resFp, FILE *libFp, FILE *confFp,
                                            Process *p, act_dataflow_element *d) {
  switch (d->t) {
    case ACT_DFLOW_FUNC: {
      char *procName = new char[10240];
      procName[0] = '\0';
      char *calc = new char[20240];
      calc[0] = '\0';
      sprintf(calc, "\n");
      IntVec boolRes;
      char *def = new char[10240];
      def[0] = '\0';
      sprintf(def, "\n");
      StringVec argList;
      StringVec oriArgList;
      IntVec argBWList;
      IntVec resBWList;
      int result_suffix = -1;
      StringVec outSendStr;
      IntVec outResSuffixs;
      StringVec outList;
      StringVec normalizedOutList;
      IntVec outWidthList;
      StringVec initStrs;
      handleDFlowFunc(resFp, libFp, confFp, p, d, procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList, result_suffix, outSendStr, outResSuffixs, outList,
                      normalizedOutList, outWidthList, initStrs, boolRes);
      if (DEBUG_CLUSTER) {
        printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        printf("Process normal dflow:\n");
        dflow_print(stdout, d);
        printf("\n");
      }
      if (strlen(procName)) {
        printDFlowFunc(resFp, libFp, confFp, procName, argList, argBWList, resBWList,
                       outWidthList, def, calc,
                       result_suffix, outSendStr, outResSuffixs, normalizedOutList,
                       outList, initStrs, boolRes);
      }
      break;
    }
    case ACT_DFLOW_SPLIT: {
      ActId *input = d->u.splitmerge.single;
      const char *inputName = input->getName();
      int inputSize = strlen(inputName);
      int bitwidth = getActIdBW(input, p);
      ActId **outputs = d->u.splitmerge.multi;
      int numOutputs = d->u.splitmerge.nmulti;
      char *procName = new char[10240];
      sprintf(procName, "control_split_%d", numOutputs);
      char *splitName = new char[2000];
      ActId *guard = d->u.splitmerge.guard;
      int guardBW = getActIdBW(guard, p);
      const char *guardName = guard->getName();
      sprintf(splitName, "%s", guardName);
      for (int i = 0; i < numOutputs; i++) {
        ActId *out = outputs[i];
        if (!out) {
          strcat(splitName, "sink_");
        } else {
          strcat(splitName, out->getName());
        }
      }
      fprintf(resFp, "%s<%d,%d> %s(", procName, guardBW, bitwidth, splitName);
      const char *guardStr = getActIdName(guard);
      const char *inputStr = getActIdName(input);
      fprintf(resFp, "%s, %s", guardStr, inputStr);
      StringVec sinkVec;
      for (int i = 0; i < numOutputs; i++) {
        ActId *out = outputs[i];
        if (!out) {
          char *sinkName = new char[2100];
          sprintf(sinkName, "sink%d", sinkCnt);
          sinkCnt++;
          fprintf(resFp, ", %s", sinkName);
          sinkVec.push_back(sinkName);
        } else {
          fprintf(resFp, ", %s", out->getName());
        }
      }
      fprintf(resFp, ");\n");
      for (auto &sink : sinkVec) {
        printSink(resFp, libFp, confFp, sink.c_str(), bitwidth);
      }
      char *instance = new char[1500];
      sprintf(instance, "%s<%d,%d>", procName, guardBW, bitwidth);
      int *metric = metrics->getOpMetric(instance);
      processGenerator.createSplit(libFp, confFp, procName, instance, metric,
                                   numOutputs);
      if (metric != nullptr) {
        metrics->updateAreaStatistics(instance, metric[3]);
      }
      break;
    }
    case ACT_DFLOW_MERGE: {
      ActId *output = d->u.splitmerge.single;
      const char *outputName = output->getName();
      int inBW = getActIdBW(output, p);
      ActId *guard = d->u.splitmerge.guard;
      int guardBW = getActIdBW(guard, p);
      int numInputs = d->u.splitmerge.nmulti;
      char *procName = new char[10240];
      sprintf(procName, "control_merge_%d", numInputs);

      fprintf(resFp, "%s<%d,%d> %s_inst(", procName, guardBW, inBW, outputName);
      const char *guardStr = getActIdName(guard);
      fprintf(resFp, "%s, ", guardStr);

      ActId **inputs = d->u.splitmerge.multi;
      for (int i = 0; i < numInputs; i++) {
        ActId *in = inputs[i];
        const char *inStr = getActIdName(in);
        fprintf(resFp, "%s, ", inStr);
      }
      fprintf(resFp, "%s);\n", outputName);

      char *instance = new char[1500];
      sprintf(instance, "%s<%d,%d>", procName, guardBW, inBW);
      int *metric = metrics->getOpMetric(instance);
      processGenerator.createMerge(libFp, confFp, procName, instance, metric, numInputs);
      if (metric != nullptr) {
        metrics->updateAreaStatistics(instance, metric[3]);
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
    default: {
      fatal_error("Unknown dataflow type %d\n", d->t);
      break;
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
ChpGenerator::handleDFlowCluster(FILE *resFp, FILE *libFp, FILE *confFp, Process *p,
                                 list_t *dflow) {
  listitem_t *li;
  char *procName = new char[10240];
  procName[0] = '\0';
  char *calc = new char[20240];
  calc[0] = '\0';
  sprintf(calc, "\n");
  IntVec boolRes;
  char *def = new char[10240];
  def[0] = '\0';
  sprintf(def, "\n");
  StringVec argList;
  StringVec oriArgList;
  IntVec argBWList;
  IntVec resBWList;
  int result_suffix = -1;
  StringVec outSendStr;
  IntVec outResSuffixs;
  StringVec outList;
  StringVec normalizedOutList;
  IntVec outWidthList;
  StringVec initStrs;
  for (li = list_first (dflow); li; li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    if (d->t == ACT_DFLOW_FUNC) {
      handleDFlowFunc(resFp, libFp, confFp, p, d, procName, calc, def, argList,
                      oriArgList,
                      argBWList,
                      resBWList, result_suffix, outSendStr, outResSuffixs, outList,
                      normalizedOutList, outWidthList, initStrs, boolRes);
    } else {
      dflow_print(stdout, d);
      fatal_error("This dflow statement should not appear in dflow-cluster!\n");
    }
  }
  if (DEBUG_CLUSTER) {
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("Process cluster dflow:\n");
    print_dflow(stdout, dflow);
    printf("\n");
  }
  if (strlen(procName)) {
    printDFlowFunc(resFp, libFp, confFp, procName, argList, argBWList, resBWList,
                   outWidthList, def,
                   calc,
                   result_suffix, outSendStr, outResSuffixs, normalizedOutList, outList,
                   initStrs, boolRes);
  }
}

ChpGenerator::ChpGenerator(Act *a, const char *name, Metrics *metrics)
    : ActPass(a, name) {
  processGenerator.initialize();
  this->metrics = metrics;
}

void
ChpGenerator::handleProcess(FILE *resFp, FILE *libFp, FILE *confFp, Process *p) {
  const char *pName = p->getName();
  printf("processing %s\n", pName);
  bool mainProc = (strcmp(pName, "main<>") == 0);
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
  for (li = list_first (p->getlang()->getdflow()->dflow);
       li;
       li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    if (d->t == ACT_DFLOW_CLUSTER) {
      list_t *dflow_cluster = d->u.dflow_cluster;
      if (DEBUG_CLUSTER) {
        print_dflow(stdout, dflow_cluster);
      }
      handleDFlowCluster(resFp, libFp, confFp, p, dflow_cluster);
    } else {
      handleNormalDflowElement(resFp, libFp, confFp, p, d);
    }
  }
  if (mainProc) {
    const char *outPort = "main_out";
    int outWidth = getBitwidth(outPort);
    printSink(resFp, libFp, confFp, outPort, outWidth);
  }
  fprintf(resFp, "}\n\n");
}

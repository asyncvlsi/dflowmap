#include <act/lang.h>
#include "ProcGenerator.h"

const char *ProcGenerator::getActIdOrCopyName(ActId *actId) {
  char *str = new char[10240];
  if (actId) {
    char *actName = new char[10240];
    getActIdName(sc, actId, actName, 10240);
    unsigned outUses = getOpUses(actId);
    if (outUses) {
      unsigned copyUse = getCopyUses(actId);
      if (debug_verbose) {
        printf("for %s, outUses: %d, copyUse: %d\n", actName, outUses, copyUse);
      }
      if (copyUse <= outUses) {
        const char *normalizedName = getNormActIdName(actName);
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

void ProcGenerator::collectBitwidthInfo() {
  ActInstiter inst(p->CurScope());
  for (inst = inst.begin(); inst != inst.end(); inst++) {
    ValueIdx *vx = *inst;
    act_connection *c;
    const char *varName = vx->getName();
    auto tmp = ActId::parseId(vx->getName());
    c = tmp->Canonical(p->CurScope());
    delete tmp;
    int bitwidth = TypeFactory::bitWidth(vx->t);
    if (bitwidth <= 0) {
      if (debug_verbose) {
        printf("%s has negative bw %d!\n", varName, bitwidth);
      }
    } else {
      if (debug_verbose) {
        printf("Update bitwidthMap for (%s, %d).\n", varName, bitwidth);
      }
      bitwidthMap.insert({c, bitwidth});
    }
  }
}

void ProcGenerator::printBitwidthInfo() {
  printf("bitwidth info:\n");
  for (auto &bitwidthMapIt : bitwidthMap) {
    char *connectName = new char[10240];
    getActConnectionName(bitwidthMapIt.first, connectName, 10240);
    printf("(%s, %u) ", connectName, bitwidthMapIt.second);
  }
  printf("\n");
}

unsigned ProcGenerator::getActIdBW(ActId *actId) {
  act_connection *c = actId->Canonical(p->CurScope());
  unsigned bw = getBitwidth(c);
  if (debug_verbose) {
    printf("Fetch BW for actID ");
    actId->Print(stdout);
    printf(": %u\n", bw);
  }
  return bw;
}

unsigned ProcGenerator::getBitwidth(act_connection *actConnection) {
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

const char *ProcGenerator::EMIT_QUERY(DflowGenerator *dflowGenerator,
                                      Expr *expr,
                                      const char *sym,
                                      char *procName,
                                      int &result_suffix,
                                      unsigned &result_bw) {
  Expr *cExpr = expr->u.e.l;
  Expr *lExpr = expr->u.e.r->u.e.l;
  Expr *rExpr = expr->u.e.r->u.e.r;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  unsigned cBW = 1;
  const char *cStr = printExpr(dflowGenerator,
                               cExpr,
                               procName,
                               result_suffix,
                               cBW);
  const char *lStr = printExpr(dflowGenerator,
                               lExpr,
                               procName,
                               result_suffix,
                               result_bw);
  const char *rStr = printExpr(dflowGenerator,
                               rExpr,
                               procName,
                               result_suffix,
                               result_bw);
  /* generate subProc name */
  char *cVal = new char[100];
  getCurProc(cStr, cVal);
  char *lVal = new char[100];
  getCurProc(lStr, lVal);
  char *rVal = new char[100];
  getCurProc(rStr, rVal);
  if (!strcmp(lStr, rStr)) {
    printf("This query expr has the same true/false branch!\n");
    print_expr(stdout, expr);
    printf("!\n");
    exit(-1);
  }
  char *subProcName = new char[1500];
  sprintf(subProcName, "_%s%s%s%s", lVal, sym, cVal, rVal);
  strcat(procName, subProcName);

  char *finalExprName = new char[100];
  result_suffix++;
  sprintf(finalExprName, "res%d", result_suffix);

  dflowGenerator->printQueryExpr(cStr, lStr, rStr, result_suffix, result_bw);

//  char *curCal = new char[300];
//  sprintf(curCal, "      res%d := bool(%s) ? %s : %s;\n",
//          result_suffix, cStr, lStr, rStr);
//  strcat(calc, curCal);
//  if (result_bw == 0) {
//    print_expr(stdout, expr);
//    printf(": result_bw is 0!\n");
//    exit(-1);
//  }
//  resBWList.push_back(result_bw);


//  if (debug_verbose) {
//    printf("[PERF] handle query expression for ");
//    print_expr(stdout, expr);
//    printf("\ndflowmap generates calc: %s\n", calc);
//    printf("arg list: ");
//    for (auto &arg : argList) {
//      printf("%s ", arg.c_str());
//    }
//    printf("\n");
//    printf("arg bw list: ");
//    for (auto &bw : argBWList) {
//      printf("%u ", bw);
//    }
//    printf("\n");
//    printf("res bw list: ");
//    for (auto &bw2:resBWList) {
//      printf("%u ", bw2);
//    }
//    printf("\n");
//    printf("query res%d has bw %u\n", result_suffix, result_bw);
//    printf("      res%d := %s ? %s : %s;\n", result_suffix, cStr, lStr, rStr);
//  }
  /* create Expr */
  int cType = (cExpr->type == E_INT) ? E_INT : E_VAR;
  int lType = (lExpr->type == E_INT) ? E_INT : E_VAR;
  int rType = (rExpr->type == E_INT) ? E_INT : E_VAR;
  int exprType = expr->type;
  int bodyExprType = expr->u.e.r->type;
  dflowGenerator->prepareQueryExprForOpt(cStr,
                                         cType,
                                         lStr,
                                         lType,
                                         rStr,
                                         rType,
                                         finalExprName,
                                         exprType,
                                         bodyExprType,
                                         result_bw);
//  chpBackend->prepareQueryExprForOpt(cStr,
//                                     cType,
//                                     lStr,
//                                     lType,
//                                     rStr,
//                                     rType,
//                                     finalExprName,
//                                     exprType,
//                                     bodyExprType,
//                                     result_bw,
//                                     exprMap,
//                                     hiddenBW,
//                                     hiddenExprs);
  return finalExprName;
}

const char *ProcGenerator::EMIT_BIN(DflowGenerator *dflowGenerator,
                                    Expr *expr,
                                    const char *sym,
                                    const char *op,
                                    int type,
                                    char *procName,
                                    int &result_suffix,
                                    unsigned &result_bw) {
  if (debug_verbose) {
    printf("Handle bin expr ");
    print_expr(stdout, expr);
    printf("\n");
  }
  Expr *lExpr = expr->u.e.l;
  Expr *rExpr = expr->u.e.r;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  const char *lStr = printExpr(dflowGenerator,
                               lExpr,
                               procName,
                               result_suffix,
                               result_bw);
  const char *rStr = printExpr(dflowGenerator,
                               rExpr,
                               procName,
                               result_suffix,
                               result_bw);
  /* generate subProc name */
  char *lVal = new char[100];
  getCurProc(lStr, lVal);
  char *rVal = new char[100];
  getCurProc(rStr, rVal);
  char *subProcName = new char[1500];
  if (!strcmp(lStr, rStr)) {
    sprintf(subProcName, "_%s%s2%s", lVal, sym, rVal);
  } else {
    sprintf(subProcName, "_%s%s%s", lVal, sym, rVal);
  }
  strcat(procName, subProcName);

  char *finalExprName = new char[100];
  result_suffix++;
  sprintf(finalExprName, "res%d", result_suffix);

  dflowGenerator->printBinExpr(op, lStr, rStr, type, result_suffix, result_bw);


//  if (result_bw == 0) {
//    print_expr(stdout, expr);
//    printf(": result_bw is 0!\n");
//    exit(-1);
//  }
//  resBWList.push_back(result_bw);
//  char *curCal = new char[300];
//  bool binType = isBinType(type);
//  if (binType) {
//    sprintf(curCal, "      res%d := int(%s %s %s);\n",
//            result_suffix, lStr, op, rStr);
//  } else {
//    sprintf(curCal, "      res%d := %s %s %s;\n",
//            result_suffix, lStr, op, rStr);
//  }
//  strcat(calc, curCal);

//  if (debug_verbose) {
//    printf("[PERF] handle bin expression for ");
//    print_expr(stdout, expr);
//    printf("***************\nres%d := %s %s %s;\n", result_suffix, lStr, op,
//           rStr);
//    print_expr(stdout, expr);
//    printf("\n");
//    printf("bin res%d has bw %u\n", result_suffix, result_bw);
//    printf("      res%d := %s %s %s;\n", result_suffix, lStr, op, rStr);
//    printf("binary expr: ");
//    print_expr(stdout, expr);
//    printf("\ndflowmap generates calc: %s\n", calc);
//    printf("procName: %s\n", procName);
//    printf("arg list: ");
//    for (auto &arg : argList) {
//      printf("%s ", arg.c_str());
//    }
//    printf("\n");
//    printf("arg bw list: ");
//    for (auto &bw : argBWList) {
//      printf("%u ", bw);
//    }
//    printf("\n");
//    printf("res bw list: ");
//    for (auto &bw2:resBWList) {
//      printf("%u ", bw2);
//    }
//    printf("\n");
//  }

  int lType = (lExpr->type == E_INT) ? E_INT : E_VAR;
  int rType = (rExpr->type == E_INT) ? E_INT : E_VAR;
  int exprType = expr->type;
  dflowGenerator->prepareBinExprForOpt(lStr,
                                       lType,
                                       rStr,
                                       rType,
                                       finalExprName,
                                       exprType,
                                       result_bw);
//  chpBackend->prepareBinExprForOpt(lStr,
//                                   lType,
//                                   rStr,
//                                   rType,
//                                   finalExprName,
//                                   exprType,
//                                   result_bw,
//                                   exprMap,
//                                   hiddenBW,
//                                   hiddenExprs);
  return finalExprName;
}

const char *ProcGenerator::EMIT_UNI(DflowGenerator *dflowGenerator,
                                    Expr *expr,
                                    const char *sym,
                                    const char *op,
                                    char *procName,
                                    int &result_suffix,
                                    unsigned &result_bw) {
  /* collect bitwidth info */
  Expr *lExpr = expr->u.e.l;
  if (procName[0] == '\0') {
    sprintf(procName, "func");
  }
  const char *lExprName = printExpr(dflowGenerator,
                                    lExpr,
                                    procName,
                                    result_suffix,
                                    result_bw);
  /* generate subProc name */
  char *val = new char[100];
  getCurProc(lExprName, val);
  sprintf(procName, "%s_%s%s", procName, sym, val);

  char *finalExprName = new char[100];
  result_suffix++;
  sprintf(finalExprName, "res%d", result_suffix);
  dflowGenerator->printUniExpr(op, lExprName, result_suffix, result_bw);
//  chpBackend->handleUniExpr(op,
//                            lExprName,
//                            result_suffix,
//                            calc,
//                            result_bw,
//                            resBWList);
  /* create Expr */
//  if (debug_verbose) {
//    printf("[PERF] handle uni expression for ");
//    print_expr(stdout, expr);
//    printf("uni res%d has bw %u\n", result_suffix, result_bw);
//    printf("      res%d := %s %s;\n", result_suffix, op, lExprName);
//    printf("unary expr: ");
//    print_expr(stdout, expr);
//    printf("\ndflowmap generates calc: %s\n", calc);
//  }

  /* prepare for the logic optimizer */
  int lType = (lExpr->type == E_INT) ? E_INT : E_VAR;
  int exprType = expr->type;
  dflowGenerator->prepareUniExprForOpt(lExprName,
                                       lType,
                                       finalExprName,
                                       exprType,
                                       result_bw);
//  chpBackend->prepareUniExprForOpt(lExprName,
//                                   lType,
//                                   finalExprName,
//                                   exprType,
//                                   result_bw,
//                                   exprMap,
//                                   hiddenBW,
//                                   hiddenExprs);
  return finalExprName;
}

const char *ProcGenerator::printExpr(DflowGenerator *dflowGenerator,
                                     Expr *expr,
                                     char *procName,
                                     int &result_suffix,
                                     unsigned &result_bw) {
  int type = expr->type;
  switch (type) {
    case E_INT: {
      if (procName[0] == '\0') {
        printf("we should NOT process Source here!\n");
        exit(-1);
      }
      unsigned long val = expr->u.v;
      const char *valStr = strdup(std::to_string(val).c_str());
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
      auto actId = (ActId *) expr->u.e.l;
      act_connection *actConnection = actId->Canonical(sc);
      unsigned argBW = getBitwidth(actConnection);
      char *oriVarName = new char[10240];
      getActIdName(sc, actId, oriVarName, 10240);
      const char *mappedVarName = getActIdOrCopyName(actId);
      if (result_bw == 0) {
        result_bw = argBW;
      }
      if (debug_verbose) {
        printf("oriVarName: %s, mappedVarName: %s, res_bw: %u\n",
               oriVarName, mappedVarName, result_bw);
      }
      return dflowGenerator->handleEVar(oriVarName, mappedVarName, argBW);

//      char *curArg = new char[10240];
//      int idx = searchStringVec(oriArgList, oriVarName);
//      if (idx == -1) {
//        int numArgs = argList.size();
//        oriArgList.push_back(oriVarName);
//        argList.push_back(mappedVarName);
//
//        sprintf(curArg, "x%d", numArgs);
//        argBWList.push_back(argBW);
//      } else {
//        sprintf(curArg, "x%d", idx);
//      }
//      inBW.insert({curArg, argBW});
//
//      return curArg;
    }
    case E_AND: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "and",
                      "&",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_OR: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "or",
                      "|",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_NOT: {
      return EMIT_UNI(dflowGenerator,
                      expr,
                      "not",
                      "~",
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_PLUS: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "add",
                      "+",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_MINUS: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "minus",
                      "-",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_MULT: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "mul",
                      "*",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_DIV: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "div",
                      "/",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_MOD: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "mod",
                      "%",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_LSL: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "lsl",
                      "<<",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_LSR: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "lsr",
                      ">>",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_ASR: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "asr",
                      ">>>",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_UMINUS: {
      return EMIT_UNI(dflowGenerator,
                      expr,
                      "neg",
                      "-",
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_XOR: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "xor",
                      "^",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_LT: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "lt",
                      "<",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_GT: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "gt",
                      ">",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_LE: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "le",
                      "<=",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_GE: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "ge",
                      ">=",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_EQ: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "eq",
                      "=",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_NE: {
      return EMIT_BIN(dflowGenerator,
                      expr,
                      "ne",
                      "!=",
                      type,
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_COMPLEMENT: {
      return EMIT_UNI(dflowGenerator,
                      expr,
                      "compl",
                      "~",
                      procName,
                      result_suffix,
                      result_bw);
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      if (result_bw == 0) {
        Expr *rExpr = expr->u.e.r;
        if (rExpr) {
          result_bw = rExpr->u.v;
        } else {
          result_bw = 1;
        }
      }
      if (debug_verbose) {
        printf("It is E_BUILTIN_INT! The real expression is ");
        print_expr(stdout, lExpr);
        printf(", result_bw: %u\n", result_bw);
      }
      return printExpr(dflowGenerator,
                       lExpr,
                       procName,
                       result_suffix,
                       result_bw);
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      result_bw = 1;
      return printExpr(dflowGenerator,
                       lExpr,
                       procName,
                       result_suffix,
                       result_bw);
    }
    case E_QUERY: {
      return EMIT_QUERY(dflowGenerator,
                        expr,
                        "q",
                        procName,
                        result_suffix,
                        result_bw);
    }
    default: {
      print_expr(stdout, expr);
      printf("\n");
      printf(
          "when printing expression, encounter unknown expression type %d\n",
          type);
      exit(-1);
    }
  }
  printf("Shouldn't be here");
  exit(-1);
}

unsigned ProcGenerator::getCopyUses(ActId *actId) {
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

void ProcGenerator::updateOpUses(act_connection *actConnection) {
  auto opUsesIt = opUses.find(actConnection);
  if (opUsesIt == opUses.end()) {
    opUses.insert({actConnection, 0});
    copyUses.insert({actConnection, 0});
  } else {
    opUsesIt->second++;
  }
}

void ProcGenerator::updateOpUses(ActId *actId) {
  act_connection *actConnection = actId->Canonical(sc);
  updateOpUses(actConnection);
}

void ProcGenerator::recordOpUses(ActId *actId,
                                 ActConnectVec &actConnectVec) {
  act_connection *actConnection = actId->Canonical(sc);
  if (std::find(actConnectVec.begin(), actConnectVec.end(), actConnection) ==
      actConnectVec.end()) {
    actConnectVec.push_back(actConnection);
  }
}

void ProcGenerator::printOpUses() {
  printf("OP USES:\n");
  for (auto &opUsesIt : opUses) {
    char *opName = new char[10240];
    getActConnectionName(opUsesIt.first, opName, 10240);
    printf("(%s, %u) ", opName, opUsesIt.second);
  }
  printf("\n");
}

bool ProcGenerator::isOpUsed(ActId *actId) {
  act_connection *actConnection = actId->Canonical(sc);
  return opUses.find(actConnection) != opUses.end();
}

unsigned ProcGenerator::getOpUses(ActId *actId) {
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

void ProcGenerator::collectUniOpUses(Expr *expr, StringVec &recordedOps) {
  Expr *lExpr = expr->u.e.l;
  collectExprUses(lExpr, recordedOps);
}

void ProcGenerator::collectBinOpUses(Expr *expr, StringVec &recordedOps) {
  Expr *lExpr = expr->u.e.l;
  collectExprUses(lExpr, recordedOps);
  Expr *rExpr = expr->u.e.r;
  collectExprUses(rExpr, recordedOps);
}

void ProcGenerator::recordUniOpUses(Expr *expr,
                                    ActConnectVec &actConnectVec) {
  Expr *lExpr = expr->u.e.l;
  recordExprUses(lExpr, actConnectVec);
}

void ProcGenerator::recordBinOpUses(Expr *expr,
                                    ActConnectVec &actConnectVec) {
  Expr *lExpr = expr->u.e.l;
  recordExprUses(lExpr, actConnectVec);
  Expr *rExpr = expr->u.e.r;
  recordExprUses(rExpr, actConnectVec);
}

void ProcGenerator::collectExprUses(Expr *expr, StringVec &recordedOps) {
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
      collectBinOpUses(expr, recordedOps);
      break;
    }
    case E_NOT:
    case E_UMINUS:
    case E_COMPLEMENT: {
      collectUniOpUses(expr, recordedOps);
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
        updateOpUses(actId);
        recordedOps.push_back(varName);
      }
      break;
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      collectExprUses(lExpr, recordedOps);
      break;
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      collectExprUses(lExpr, recordedOps);
      break;
    }
    case E_QUERY: {
      Expr *cExpr = expr->u.e.l;
      Expr *lExpr = expr->u.e.r->u.e.l;
      Expr *rExpr = expr->u.e.r->u.e.r;
      collectExprUses(cExpr, recordedOps);
      collectExprUses(lExpr, recordedOps);
      collectExprUses(rExpr, recordedOps);
      break;
    }
    default: {
      print_expr(stdout, expr);
      printf("\nUnknown expression type %d when collecting expr use\n", type);
      exit(-1);
    }
  }
}

void ProcGenerator::recordExprUses(Expr *expr,
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
      recordBinOpUses(expr, actConnectVec);
      break;
    }
    case E_NOT:
    case E_UMINUS:
    case E_COMPLEMENT: {
      recordUniOpUses(expr, actConnectVec);
      break;
    }
    case E_INT: {
      break;
    }
    case E_VAR: {
      auto actId = (ActId *) expr->u.e.l;
      recordOpUses(actId, actConnectVec);
      break;
    }
    case E_BUILTIN_INT: {
      Expr *lExpr = expr->u.e.l;
      recordExprUses(lExpr, actConnectVec);
      break;
    }
    case E_BUILTIN_BOOL: {
      Expr *lExpr = expr->u.e.l;
      recordExprUses(lExpr, actConnectVec);
      break;
    }
    case E_QUERY: {
      Expr *cExpr = expr->u.e.l;
      Expr *lExpr = expr->u.e.r->u.e.l;
      Expr *rExpr = expr->u.e.r->u.e.r;
      recordExprUses(cExpr, actConnectVec);
      recordExprUses(lExpr, actConnectVec);
      recordExprUses(rExpr, actConnectVec);
      break;
    }
    default: {
      print_expr(stdout, expr);
      printf("\nUnknown expression type %d when recording expr use\n", type);
      exit(-1);
    }
  }
}

void ProcGenerator::collectDflowClusterUses(list_t *dflow,
                                            ActConnectVec &actConnectVec) {
  listitem_t *li;
  for (li = list_first (dflow); li; li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
      case ACT_DFLOW_FUNC: {
        Expr *expr = d->u.func.lhs;
        recordExprUses(expr, actConnectVec);
        break;
      }
      case ACT_DFLOW_SPLIT: {
        ActId *input = d->u.splitmerge.single;
        recordOpUses(input, actConnectVec);
        ActId *guard = d->u.splitmerge.guard;
        recordOpUses(guard, actConnectVec);
        break;
      }
      case ACT_DFLOW_MERGE: {
        ActId *guard = d->u.splitmerge.guard;
        recordOpUses(guard, actConnectVec);
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs < 2) {
          dflow_print(stdout, d);
          printf("\nMerge has less than TWO inputs!\n");
          exit(-1);
        }
        ActId **inputs = d->u.splitmerge.multi;
        for (int i = 0; i < numInputs; i++) {
          ActId *in = inputs[i];
          recordOpUses(in, actConnectVec);
        }
        break;
      }
      case ACT_DFLOW_MIXER: {
        printf("We don't support MIXER for now!\n");
        exit(-1);
      }
      case ACT_DFLOW_ARBITER: {
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs < 2) {
          dflow_print(stdout, d);
          printf("\nArbiter has less than TWO inputs!\n");
          exit(-1);
        }
        ActId **inputs = d->u.splitmerge.multi;
        for (int i = 0; i < numInputs; i++) {
          ActId *in = inputs[i];
          recordOpUses(in, actConnectVec);
        }
        break;
      }
      case ACT_DFLOW_CLUSTER: {
        printf("Do not support nested dflow_cluster!\n");
        exit(-1);
      }
      case ACT_DFLOW_SINK: {
        printf("dflow cluster should not connect to SINK!\n");
        exit(-1);
      }
      default: {
        printf("Unknown dataflow type %d\n", d->t);
        exit(-1);
      }
    }
  }
}

void ProcGenerator::collectOpUses() {
  listitem_t *li;
  for (li = list_first (p->getlang()->getdflow()->dflow);
       li;
       li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
      case ACT_DFLOW_SINK: {
        ActId *input = d->u.sink.chan;
        updateOpUses(input);
        break;
      }
      case ACT_DFLOW_FUNC: {
        Expr *expr = d->u.func.lhs;
        StringVec recordedOps;
        collectExprUses(expr, recordedOps);
        break;
      }
      case ACT_DFLOW_SPLIT: {
        ActId *input = d->u.splitmerge.single;
        updateOpUses(input);
        ActId *guard = d->u.splitmerge.guard;
        updateOpUses(guard);
        break;
      }
      case ACT_DFLOW_MERGE: {
        ActId *guard = d->u.splitmerge.guard;
        updateOpUses(guard);
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs < 2) {
          dflow_print(stdout, d);
          printf("\nMerge has less than TWO inputs!\n");
          exit(-1);
        }
        ActId **inputs = d->u.splitmerge.multi;
        for (int i = 0; i < numInputs; i++) {
          ActId *in = inputs[i];
          updateOpUses(in);
        }
        break;
      }
      case ACT_DFLOW_MIXER: {
        printf("We don't support MIXER for now!\n");
        exit(-1);
        break;
      }
      case ACT_DFLOW_ARBITER: {
        int numInputs = d->u.splitmerge.nmulti;
        if (numInputs < 2) {
          dflow_print(stdout, d);
          printf("\nArbiter has less than TWO inputs!\n");
          exit(-1);
        }
        ActId **inputs = d->u.splitmerge.multi;
        for (int i = 0; i < numInputs; i++) {
          ActId *in = inputs[i];
          updateOpUses(in);
        }
        break;
      }
      case ACT_DFLOW_CLUSTER: {
        ActConnectVec actConnectVec;
        collectDflowClusterUses(d->u.dflow_cluster, actConnectVec);
        for (auto &actConnect : actConnectVec) {
          updateOpUses(actConnect);
        }
        break;
      }
      default: {
        printf("Unknown dataflow type %d\n", d->t);
        exit(-1);
      }
    }
  }
}

void ProcGenerator::createCopyProcs() {
  for (auto &opUsesIt : opUses) {
    unsigned uses = opUsesIt.second;
    if (uses) {
      unsigned numOut = uses + 1;
      act_connection *actConnection = opUsesIt.first;
      unsigned bitwidth = getBitwidth(actConnection);
      char *inName = new char[10240];
      getActConnectionName(actConnection, inName, 10240);
      double *metric = metrics->getOrGenCopyMetric(bitwidth, numOut);
      chpBackend->createCopyProcs(inName, bitwidth, numOut, metric);
    }
  }
}

void ProcGenerator::createSink(const char *name, unsigned bitwidth) {
  double *metric = metrics->getSinkMetric(bitwidth);
  chpBackend->printSink(name, bitwidth, metric);
}

void ProcGenerator::createSource(const char *outName,
                                 unsigned long val,
                                 unsigned bitwidth) {
  char *instance = new char[1500];
  sprintf(instance, "source<%lu,%u>", val, bitwidth);
  double *metric = metrics->getSourceMetric(instance, bitwidth);
  chpBackend->printSource(outName, instance, metric);
}

void ProcGenerator::printDFlowFunc(DflowGenerator *dflowGenerator,
                                   const char *procName,
                                   UIntVec &outBWList,
                                   StringVec &outSendStr,
                                   IntVec &outResSuffixs,
                                   StringVec &normalizedOutList,
                                   StringVec &outList,
                                   Vector<BuffInfo> &buffInfos,
                                   Map<int, int> &outRecord,
                                   UIntVec &buffBWs) {
//  if (debug_verbose) {
//    printf("PRINT DFLOW FUNCTION\n");
//    printf("size: %d\n", strlen(procName));
//    printf("procName: %s\n", procName);
//    printf("arg list:\n");
//    for (auto &arg : argList) {
//      printf("%s ", arg.c_str());
//    }
//    printf("\n");
//    printf("arg bw list:\n");
//    for (auto &bw : argBWList) {
//      printf("%u ", bw);
//    }
//    printf("\n");
//    printf("res bw list:\n");
//    for (auto &resBW : resBWList) {
//      printf("%u ", resBW);
//    }
//    printf("\n");
//    printf("outWidthList:\n");
//    for (auto &outWidth : outBWList) {
//      printf("%u ", outWidth);
//    }
//    printf("\n");
//    printf("calc: %s\n", calc);
//    printf("result_suffix: %d\n", result_suffix);
//    printf("outSendStr:\n");
//    for (auto &outStr : outSendStr) {
//      printf("%s\n", outStr.c_str());
//    }
//    printf("normalizedOutList:\n");
//    for (auto &out : normalizedOutList) {
//      printf("%s ", out.c_str());
//    }
//    printf("\n");
//    printf("outList:\n");
//    for (auto &out : outList) {
//      printf("%s ", out.c_str());
//    }
//    printf("\n");
//    printf("buffInfos: ");
//    for (auto &buffInfo : buffInfos) {
//      printf("(%u, %lu, %lu, %d) ", buffInfo.outputID, buffInfo.nBuff,
//             buffInfo.initVal, buffInfo.hasInitVal);
//    }
//    printf("\n");
//  }
//  calc[strlen(calc) - 2] = ';';

  const char *calc = dflowGenerator->getCalc();
  StringVec &argList = dflowGenerator->getArgList();
  UIntVec &argBWList = dflowGenerator->getArgBWList();
  UIntVec &resBWList = dflowGenerator->getResBWList();
  Map<const char *, Expr *> &exprMap = dflowGenerator->getExprMap();
  StringMap<unsigned> &inBW = dflowGenerator->getInBW();
  StringMap<unsigned> &hiddenBW = dflowGenerator->getHiddenBWs();
  Map<Expr *, Expr *> &hiddenExprs = dflowGenerator->getHiddenExprs();

  char *instance = new char[MAX_INSTANCE_LEN];
  sprintf(instance, "%s<", procName);
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
  }
  double *fuMetric = metrics->getOrGenFUMetric(instance,
                                               inBW,
                                               hiddenBW,
                                               exprMap,
                                               hiddenExprs,
                                               outRecord,
                                               outBWList);
  chpBackend->printFU(procName,
                      instance,
                      argList,
                      argBWList,
                      resBWList,
                      outBWList,
                      calc,
                      outSendStr,
                      outResSuffixs,
                      normalizedOutList,
                      outList,
                      buffInfos,
                      fuMetric);
  chpBackend->printBuff(buffInfos);
}

void ProcGenerator::handleDFlowFunc(DflowGenerator *dflowGenerator,
                                    act_dataflow_element *d,
                                    char *procName,
                                    int &result_suffix,
                                    StringVec &outSendStr,
                                    IntVec &outResSuffixs,
                                    StringVec &outList,
                                    StringVec &normalizedOutList,
                                    UIntVec &outWidthList,
                                    Vector<BuffInfo> &buffInfos,
                                    Map<int, int> &outRecord,
                                    UIntVec &buffBWs) {
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
  getActIdName(sc, rhs, out, 10240);
  const char *normalizedOut = getNormActIdName(out);
  unsigned outWidth = getActIdBW(rhs);
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
  if (debug_verbose) {
    if (nbufs) {
      printf("[nbuf] ");
      rhs->Print(stdout);
      printf(", nBuff: ");
      print_expr(stdout, nbufs);
      printf(", isInit: %d", (initExpr != nullptr));
      printf("\n");
    }
  }
  if (type == E_INT) {
    unsigned long val = expr->u.v;
    createSource(out, val, outWidth);
    if (initExpr) {
      print_expr(stdout, expr);
      printf(" has const lOp, but its rOp has init token!\n");
      exit(-1);
    }
  } else {
    unsigned result_bw = outWidth;
    if (debug_verbose) {
      printf("Start to print expression ");
      print_expr(stdout, expr);
      printf(", its type: %d\n", expr->type);
    }
    const char *exprStr = printExpr(dflowGenerator,
                                    expr,
                                    procName,
                                    result_suffix,
                                    result_bw);
    /* check if the expression only has E_VAR. Note that it could be built-in int/bool, e.g., int(varName, bw). In
     * this case, it still only has E_VAR expression. */
    Expr *actualExpr = expr;
    while ((type == E_BUILTIN_INT) || (type == E_BUILTIN_BOOL)) {
      actualExpr = actualExpr->u.e.l;
      type = actualExpr->type;
    }
    bool onlyVarExpr = (type == E_VAR);
    if (onlyVarExpr) {
      if (debug_verbose) {
        printf("The expression ");
        print_expr(stdout, expr);
        printf(" is E_VAR!\n");
      }
      char *subProc = new char[10];
      if (strlen(procName)) {
        sprintf(subProc, "_port");
      } else {
        sprintf(subProc, "func_port");
      }
      strcat(procName, subProc);
      result_suffix++;
      dflowGenerator->printPort(exprStr, result_suffix, result_bw);


//      resBWList.push_back(result_bw);
//      char *subCalc = new char[1500];
//      sprintf(subCalc, "      res%d := %s;\n", result_suffix, exprStr);
//      strcat(calc, subCalc);

      char *resName = new char[5];
      sprintf(resName, "res%d", result_suffix);
      dflowGenerator->preparePortForOpt(resName, exprStr, result_bw);

//      Expr *resRHS = getExprFromName(resName, exprMap, false, E_VAR);
//      Expr *xExpr = getExprFromName(exprStr, exprMap, false, E_VAR);
//      hiddenBW.insert({resName, result_bw});
//      hiddenExprs.insert({resRHS, xExpr});
    }

    if (initExpr) {
      unsigned long initVal = initExpr->u.v;
      char *subProcName = new char[1500];
      sprintf(subProcName, "_init%lu", initVal);
      strcat(procName, subProcName);
      buffBWs.push_back(result_bw);
    }
//    if (debug_verbose) {
//      printf(
//          "___________________________________\n\n\n\n\n\nFor dataflow element: ");
//      dflow_print(stdout, d);
//      printf("\n___________________________________________\n");
//      printf("procName: %s\n", procName);
//      printf("arg list:\n");
//      for (auto &arg : argList) {
//        printf("%s ", arg.c_str());
//      }
//      printf("\n");
//      printf("oriArgList:\n");
//      for (auto &oriArg : oriArgList) {
//        printf("%s ", oriArg.c_str());
//      }
//      printf("\n");
//      printf("arg bw list:\n");
//      for (auto &bw : argBWList) {
//        printf("%u ", bw);
//      }
//      printf("\n");
//      printf("res bw list:\n");
//      for (auto &resBW : resBWList) {
//        printf("%u ", resBW);
//      }
//      printf("\n");
//      printf("out bw: %d\n", outWidth);
//      printf("calc: %s\n", calc);
//      printf("result_suffix: %d\n", result_suffix);
//      printf("normalizedOut: %s, out: %s\n", normalizedOut, out);
//      printf("init expr: ");
//      print_expr(stdout, initExpr);
//      printf("\n");
//    }
    outList.push_back(out);
    normalizedOutList.push_back(normalizedOut);
    outWidthList.push_back(outWidth);
    char *outStr = new char[10240];
    outStr[0] = '\0';
    unsigned numOuts = outList.size();
    char *outName = new char[10240];
    unsigned outID = numOuts - 1;
    sprintf(outName, "out%u", outID);
    sprintf(outStr, "out%u!res%d", outID, result_suffix);
    outRecord.insert({outID, result_suffix});
    if (nbufs) {
      unsigned long numBuff = nbufs->u.v;
      unsigned long initVal = -1;
      bool hasInitVal = false;
      if (initExpr) {
        initVal = initExpr->u.v;
        hasInitVal = true;
      }
      double *buffMetric = metrics->getBuffMetric(numBuff, outWidth);
      BuffInfo buff_info;
      buff_info.outputID = outID;
      buff_info.bw = outWidth;
      buff_info.nBuff = numBuff;
      buff_info.initVal = initVal;
      buff_info.finalOutput = new char[1 + strlen(out)];
      sprintf(buff_info.finalOutput, "%s", out);
      buff_info.hasInitVal = hasInitVal;
      buff_info.metric = buffMetric;
      buffInfos.push_back(buff_info);
    }
    if (debug_verbose) {
      printf("@@@@@@@@@@@@@@@@ generate %s\n", outStr);
    }
    outSendStr.push_back(outStr);
    outResSuffixs.push_back(result_suffix);
    char outWidthStr[1024];
    sprintf(outWidthStr, "_%d", outWidth);
    strcat(procName, outWidthStr);
  }
}

void ProcGenerator::handleNormalDflowElement(act_dataflow_element *d,
                                             unsigned &sinkCnt) {
  switch (d->t) {
    case ACT_DFLOW_FUNC: {
      if (debug_verbose) {
        printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        printf("Process normal dflow:\n");
        dflow_print(stdout, d);
        printf("\n");
      }
      char *procName = new char[MAX_PROC_NAME_LEN];
      procName[0] = '\0';
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
      Vector<BuffInfo> buffInfos;
      Map<const char *, Expr *> exprMap;
      StringMap<unsigned> inBW;
      StringMap<unsigned> hiddenBW;
      Map<int, int> outRecord;
      Map<Expr *, Expr *> hiddenExprs;
      UIntVec buffBWs;
      auto dflowGenerator = new DflowGenerator(argList,
                                               oriArgList,
                                               argBWList,
                                               resBWList,
                                               exprMap,
                                               inBW,
                                               hiddenBW,
                                               hiddenExprs);
      handleDFlowFunc(dflowGenerator,
                      d,
                      procName,
                      result_suffix,
                      outSendStr,
                      outResSuffixs,
                      outList,
                      normalizedOutList,
                      outWidthList,
                      buffInfos,
                      outRecord,
                      buffBWs);
      if (strlen(calc) > 1) {
        printDFlowFunc(dflowGenerator,
                       procName,
                       outWidthList,
                       outSendStr,
                       outResSuffixs,
                       normalizedOutList,
                       outList,
                       buffInfos,
                       outRecord,
                       buffBWs);
      }
      break;
    }
    case ACT_DFLOW_SPLIT: {
      ActId *input = d->u.splitmerge.single;
      unsigned outBW = getActIdBW(input);
      ActId **outputs = d->u.splitmerge.multi;
      int numOutputs = d->u.splitmerge.nmulti;
      char *procName = new char[MAX_PROC_NAME_LEN];
      sprintf(procName, "%s_%d", Constant::SPLIT_PREFIX, numOutputs);
      ActId *guard = d->u.splitmerge.guard;
      unsigned guardBW = getActIdBW(guard);
      char *splitName = new char[2000];
      char *inputName = new char[10240];
      getActIdName(sc, input, inputName, 10240);
      const char *normalizedInput = getNormActIdName(inputName);
      sprintf(splitName, "%s", normalizedInput);
      char *guardName = new char[10240];
      getActIdName(sc, guard, guardName, 10240);
      const char *normalizedGuard = getNormActIdName(guardName);
      strcat(splitName, normalizedGuard);
      CharPtrVec sinkVec;
      bool actnCp = false;
      bool actnDp = false;
      CharPtrVec outNameVec;
      for (int i = 0; i < numOutputs; i++) {
        ActId *out = outputs[i];
        if (!out) {
          strcat(splitName, "sink_");
          char *sinkName = new char[2100];
          sprintf(sinkName, "sink%d", sinkCnt);
          sinkCnt++;
          sinkVec.push_back(sinkName);
          outNameVec.push_back(sinkName);
        } else {
          char *outName = new char[10240];
          getActIdName(sc, out, outName, 10240);
          const char *normalizedOut = getNormActIdName(outName);
          strcat(splitName, normalizedOut);
          checkACTN(normalizedOut, actnCp, actnDp);
          outNameVec.push_back(outName);
        }
      }
      for (auto &sink : sinkVec) {
        chpBackend->printChannel(sink, outBW);
        createSink(sink, outBW);
      }

      if (actnCp) {
        printf("[actnCp]: %s\n", splitName);
      } else if (actnDp) {
        printf("[actnDp]: %s\n", splitName);
      }
      printf("[split]: %s\n", splitName);

      const char *guardStr = getActIdOrCopyName(guard);
      const char *inputStr = getActIdOrCopyName(input);
      char *instance = new char[MAX_INSTANCE_LEN];
      sprintf(instance, "%s<%d,%d>", procName, guardBW, outBW);

      double *metric = metrics->getMSMetric(instance,
                                            procName,
                                            guardBW,
                                            outBW,
                                            actnCp,
                                            actnDp);
      chpBackend->printSplit(procName,
                             splitName,
                             guardStr,
                             inputStr,
                             guardBW,
                             outBW,
                             outNameVec,
                             instance,
                             numOutputs,
                             metric);
      break;
    }
    case ACT_DFLOW_MERGE: {
      ActId *output = d->u.splitmerge.single;
      char *outputName = new char[10240];
      getActIdName(sc, output, outputName, 10240);
      const char *normalizedOutput = getNormActIdName(outputName);
      bool actnCp = false;
      bool actnDp = false;
      checkACTN(normalizedOutput, actnCp, actnDp);
      printf("[merge]: %s_inst\n", normalizedOutput);
      if (actnCp) {
        printf("[actnCp]: %s_inst\n", normalizedOutput);
      } else if (actnDp) {
        printf("[actnDp]: %s_inst\n", normalizedOutput);
      }
      unsigned inBW = getActIdBW(output);
      ActId *guard = d->u.splitmerge.guard;
      unsigned guardBW = getActIdBW(guard);
      int numInputs = d->u.splitmerge.nmulti;
      char *procName = new char[MAX_PROC_NAME_LEN];
      sprintf(procName, "%s_%d", Constant::MERGE_PREFIX, numInputs);
      const char *guardStr = getActIdOrCopyName(guard);
      ActId **inputs = d->u.splitmerge.multi;
      CharPtrVec inNameVec;
      for (int i = 0; i < numInputs; i++) {
        ActId *in = inputs[i];
        const char *inStr = getActIdOrCopyName(in);
        inNameVec.push_back(inStr);
      }
      char *instance = new char[MAX_INSTANCE_LEN];
      sprintf(instance, "%s<%d,%d>", procName, guardBW, inBW);
      double *metric = metrics->getMSMetric(instance,
                                            procName,
                                            guardBW,
                                            inBW,
                                            actnCp,
                                            actnDp);
      chpBackend->printMerge(procName,
                             outputName,
                             guardStr,
                             guardBW,
                             inBW,
                             inNameVec,
                             instance,
                             numInputs,
                             metric);
      break;
    }
    case ACT_DFLOW_MIXER: {
      printf("We don't support MIXER for now!\n");
      exit(-1);
      break;
    }
    case ACT_DFLOW_ARBITER: {
      ActId *output = d->u.splitmerge.single;
      char *outputName = new char[10240];
      getActIdName(sc, output, outputName, 10240);
      const char *normalizedOutput = getNormActIdName(outputName);
      bool actnCp = false;
      bool actnDp = false;
      checkACTN(normalizedOutput, actnCp, actnDp);
      printf("[arbiter]: %s_inst\n", normalizedOutput);
      if (actnCp) {
        printf("[actnCp]: %s_inst\n", normalizedOutput);
      } else if (actnDp) {
        printf("[actnDp]: %s_inst\n", normalizedOutput);
      }

      unsigned outBW = getActIdBW(output);
      int numInputs = d->u.splitmerge.nmulti;
      int coutBW = ceil(log2(numInputs));
      char *procName = new char[MAX_PROC_NAME_LEN];
      sprintf(procName, "%s_%d", Constant::ARBITER_PREFIX, numInputs);

      ActId **inputs = d->u.splitmerge.multi;
      CharPtrVec inNameVec;
      for (int i = 0; i < numInputs; i++) {
        ActId *in = inputs[i];
        const char *inStr = getActIdOrCopyName(in);
        inNameVec.push_back(inStr);
      }
      ActId *cout = d->u.splitmerge.nondetctrl;
      char *coutName = new char[10240];
      getActIdName(sc, cout, coutName, 10240);

      char *instance = new char[MAX_INSTANCE_LEN];
      sprintf(instance, "%s<%d,%d>", procName, outBW, coutBW);

      double *metric = metrics->getArbiterMetric(instance,
                                                 numInputs,
                                                 outBW,
                                                 coutBW,
                                                 actnCp,
                                                 actnDp);
      chpBackend->printArbiter(procName,
                               instance,
                               outputName,
                               coutName,
                               outBW,
                               coutBW,
                               numInputs,
                               inNameVec,
                               metric);
      break;
    }
    case ACT_DFLOW_CLUSTER: {
      dflow_print(stdout, d);
      printf("We should not process dflow_clsuter here!");
      exit(-1);
    }
    case ACT_DFLOW_SINK: {
      ActId *input = d->u.sink.chan;
      char *inputName = new char[10240];
      getActIdName(sc, input, inputName, 10240);
      unsigned bw = getBitwidth(input->Canonical(sc));
      createSink(inputName, bw);
      if (debug_verbose) {
        printf("%s is not used anywhere!\n", inputName);
      }
      break;
    }
    default: {
      printf("Unknown dataflow type %d\n", d->t);
      exit(-1);
    }
  }
}

void ProcGenerator::handleDFlowCluster(list_t *dflow) {
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
  Vector<BuffInfo> buffInfos;
  Map<const char *, Expr *> exprMap;
  StringMap<unsigned> inBW;
  StringMap<unsigned> hiddenBW;
  Map<int, int> outRecord;
  Map<Expr *, Expr *> hiddenExprs;
  unsigned elementCnt = 0;
  UIntVec buffBWs;
  auto dflowGenerator = new DflowGenerator(argList,
                                           oriArgList,
                                           argBWList,
                                           resBWList,
                                           exprMap,
                                           inBW,
                                           hiddenBW,
                                           hiddenExprs);
  for (li = list_first (dflow); li; li = list_next (li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    if (debug_verbose) {
      printf("Start to process dflow_cluster element ");
      dflow_print(stdout, d);
      printf(", current proc name: %s\n", procName);
    }
    if (d->t == ACT_DFLOW_FUNC) {
      handleDFlowFunc(dflowGenerator,
                      d,
                      procName,
                      result_suffix,
                      outSendStr,
                      outResSuffixs,
                      outList,
                      normalizedOutList,
                      outWidthList,
                      buffInfos,
                      outRecord,
                      buffBWs);
      char *subProc = new char[1024];
      sprintf(subProc, "_p%d", elementCnt);
      elementCnt++;
      strcat(procName, subProc);
    } else {
      dflow_print(stdout, d);
      printf("This dflow statement should not appear in dflow-cluster!\n");
      exit(-1);
    }
    if (debug_verbose) {
      printf("After processing dflow_cluster element ");
      dflow_print(stdout, d);
      printf(", the proc name: %s\n", procName);
    }
  }
  if (debug_verbose) {
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("Process cluster dflow:\n");
    print_dflow(stdout, dflow);
    printf("\n");
  }
  if (strlen(calc) > 1) {
    printDFlowFunc(dflowGenerator,
                   procName,
                   outWidthList,
                   outSendStr,
                   outResSuffixs,
                   normalizedOutList,
                   outList,
                   buffInfos,
                   outRecord,
                   buffBWs);
  }
}

ProcGenerator::ProcGenerator(Metrics *metrics,
                             ChpBackend *chpBackend) {
  this->metrics = metrics;
  this->chpBackend = chpBackend;
}

void ProcGenerator::handleProcess(Process *proc) {
  this->p = proc;
  this->sc = p->CurScope();
  const char *pName = p->getName();
  if (debug_verbose) {
    printf("processing %s\n", pName);
  }
  if (p->getlang()->getchp()) {
    chpBackend->createChpBlock(p);
    return;
  }
  if (!p->getlang()->getdflow()) {
    printf("Process `%s': no dataflow body", p->getName());
    exit(-1);
  }

  chpBackend->printProcHeader(p);
  bitwidthMap.clear();
  opUses.clear();
  copyUses.clear();
  collectBitwidthInfo();
  collectOpUses();
  createCopyProcs();
  listitem_t *li;
  unsigned sinkCnt = 0;
  for (li = list_first (p->getlang()->getdflow()->dflow); li; li = list_next (
      li)) {
    auto *d = (act_dataflow_element *) list_value (li);
    if (d->t == ACT_DFLOW_CLUSTER) {
      list_t *dflow_cluster = d->u.dflow_cluster;
      handleDFlowCluster(dflow_cluster);
    } else {
      handleNormalDflowElement(d, sinkCnt);
    }
  }
  chpBackend->printProcEnding();
}

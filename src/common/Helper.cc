/*
 * This file is part of the ACT library
 *
 * Copyright (c) 2021 Rui Li
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "Helper.h"
#include <act/iter.h>
#include <common/int.h>
#include <cctype>
#include <string>
#include <vector>

/* the names ACT prints the type by: its full name, with and without the
   leading "::" (a trailing "<>" is matched by _replace_type), and if
   base_too the name within its namespace */
static std::vector<std::string> _type_names(InstType *it, bool base_too) {
  auto u = dynamic_cast<UserDef *>(it->BaseType());
  Assert (u, "Not a user-defined type?");
  char *f = u->getFullName();
  std::string full(f);
  free(f);
  if (full.size() > 2 && full.compare(full.size() - 2, 2, "<>") == 0) {
    full.resize(full.size() - 2);
  }
  std::vector<std::string> v;
  v.push_back(full);
  if (full.compare(0, 2, "::") == 0) v.push_back(full.substr(2));
  else v.push_back("::" + full);
  if (base_too) {
    char b[4096];
    it->sPrint(b, sizeof b);
    std::string s(b);
    if (s.size() > 2 && s.compare(s.size() - 2, 2, "<>") == 0) {
      s.resize(s.size() - 2);
    }
    v.push_back(s);
  }
  return v;
}

/* every occurrence of from in s that is a whole type name (a trailing "<>"
   included) becomes to */
static void _replace_type(std::string &s, const std::string &from,
                          const std::string &to) {
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != std::string::npos) {
    size_t end = pos + from.size();
    bool left = pos == 0 || isspace((unsigned char) s[pos - 1])
                || strchr("(;,{", s[pos - 1]);
    if (left && s.compare(end, 2, "<>") == 0) end += 2;
    bool right = end >= s.size() || isspace((unsigned char) s[end])
                 || strchr("[;,)", s[end]);
    if (left && right) {
      s.replace(pos, end - pos, to);
      pos += to.size();
    } else {
      pos = end;
    }
  }
}

std::string modelText(Process *p, const std::string &text) {
  std::string s = text;
  ActNamespace *stdNS = ActNamespace::Global()->findNS(Constant::STD_NAMESPACE);
  char b[4096];

  /* the definition's own name */
  size_t k = s.find("defproc ");
  if (k != std::string::npos) {
    size_t st = k + 8, e;
    int depth = 0;
    for (e = st; e < s.size(); e++) {
      char c = s[e];
      if (c == '<') depth++;
      else if (c == '>') depth--;
      else if (depth == 0 && (c == '(' || isspace((unsigned char) c))) break;
    }
    ActNamespace::Act()->msnprintfproc(b, sizeof b, p);
    s.replace(st, e - st, b);
  }

  ActInstiter iter(p->CurScope());
  for (iter = iter.begin(); iter != iter.end(); iter++) {
    ValueIdx *vx = *iter;
    if (TypeFactory::isProcessType(vx->t)) {
      /* an instance of a user process */
      auto q = dynamic_cast<Process *>(vx->t->BaseType());
      if (!q || q->getns() == stdNS) continue;
      ActNamespace::Act()->msnprintfproc(b, sizeof b, q);
      for (const std::string &n : _type_names(vx->t, false)) {
        _replace_type(s, n, b);
      }
    } else if (TypeFactory::isChanType(vx->t)) {
      /* a channel carrying a struct */
      InstType *dt = TypeFactory::getChanDataType(vx->t);
      if (!dt || !TypeFactory::isStructure(dt)) continue;
      std::string to = "int<" + std::to_string(TypeFactory::totBitWidth(dt)) + ">";
      for (const std::string &n : _type_names(dt, true)) {
        for (const char *c : {"chan(", "chan?(", "chan!("}) {
          std::string from = std::string(c) + n + ")";
          std::string into = std::string(c) + to + ")";
          for (size_t pos = s.find(from); pos != std::string::npos;
               pos = s.find(from, pos + into.size())) {
            s.replace(pos, from.size(), into);
          }
        }
      }
    }
  }
  return s;
}

std::string modelInstances(Process *p, const std::string &text) {
  ActNamespace *stdNS = ActNamespace::Global()->findNS(Constant::STD_NAMESPACE);
  size_t c = text.find("/* connections */");
  std::string inst = text.substr(0, c);
  std::string decls;
  char b[4096];
  ActInstiter iter(p->CurScope());
  for (iter = iter.begin(); iter != iter.end(); iter++) {
    ValueIdx *vx = *iter;
    bool proc = TypeFactory::isProcessType(vx->t);
    bool chan = TypeFactory::isChanType(vx->t);
    if (!proc && !chan) continue;
    std::string n = vx->getName();
    if (p->isPort(n.c_str())) continue;
    if (inst.find(" " + n + ";") != std::string::npos
        || inst.find(" " + n + "[") != std::string::npos) {
      continue;
    }
    if (chan) {
      /* a channel, at its packed width, as modelText writes them */
      InstType *dt = TypeFactory::getChanDataType(vx->t);
      if (dt && TypeFactory::isBoolType(dt)) {
        decls += "chan(bool)";
      } else {
        decls += "chan(int<" + std::to_string(TypeFactory::totBitWidth(vx->t)) + ">)";
      }
    } else {
      auto q = dynamic_cast<Process *>(vx->t->BaseType());
      if (!q) continue;
      if (q->getns() == stdNS) {
        char *f = q->getFullName();
        decls += f;
        free(f);
      } else {
        ActNamespace::Act()->msnprintfproc(b, sizeof b, q);
        decls += b;
      }
    }
    decls += " " + n;
    if (vx->t->arrayInfo()) {
      vx->t->arrayInfo()->sPrint(b, sizeof b);
      decls += b;
    }
    decls += ";\n";
  }
  if (decls.empty()) return text;
  std::string s = text;
  s.insert(c == std::string::npos ? s.size() : c, decls + "\n");
  return s;
}

void normalizeName(char *src, char toDel, char newChar) {
  char *pos = strchr(src, toDel);
  while (pos) {
    *pos = newChar;
    pos = strchr(pos + 1, toDel);
  }
}

const char *getNormInstanceName(const char *src) {
  char *result = new char[1 + strlen(src)];
  sprintf(result, "%s", src);
  normalizeName(result, '<', '_');
  normalizeName(result, '>', '_');
  normalizeName(result, ',', '_');
  return result;
}

char *_getNormActIdName(const char *src) {
  char *result = new char[1 + strlen(src)];
  sprintf(result, "%s", src);
  normalizeName(result, '.', '_');
  normalizeName(result, '[', '_');
  normalizeName(result, ']', '_');
  return result;
}

const char *getNormActIdName(const char *src) {
  return _getNormActIdName (src);
}

void printActConnectionName (FILE *fp, act_connection *c)
{
  char buf[10240];
  ActId *id = c->toid();
  id->sPrint (buf, 10240);
  delete id;
  normalizeName (buf, '.', '_');
  normalizeName (buf, '[', '_');
  normalizeName (buf, ']', '_');
  fprintf (fp, "%s", buf);
}

void printIntVec(IntVec &intVec) {
  for (auto &val: intVec) {
    printf("%d ", val);
  }
  printf("\n");
}

void printULongVec(ULongVec &longVec) {
  for (auto &val: longVec) {
    printf("%lu ", val);
  }
  printf("\n");
}

int searchStringVec(StringVec &strVec, const char *str) {
  auto it = std::find(strVec.begin(), strVec.end(), str);
  if (it != strVec.end()) {
    return (it - strVec.begin());
  } else {
    return -1;
  }
}

Expr *genExprFromStr(const char *str, int exprType) {
  Expr *expr = new Expr();
  auto newLActId = ActId::parseId(str);
  expr->type = exprType;
  expr->u.e.l = (Expr *) (newLActId);
  return expr;
}

Expr *genExprFromInt(unsigned long val) {
  Expr *expr = new Expr();
  expr->type = E_INT;
  expr->u.ival.v = val;
  expr->u.ival.v_extra = nullptr;
  return expr;
}

/*
 * a constant by the name printExpr gives it, int(V,W): V past 64 bits (a
 * BigInt printed in hex, int(0x...,W)) as an E_INT with its BigInt in
 * v_extra, as ACT keeps one -- std::stoul threw on it
 */
Expr *genExprFromIntName(const char *name) {
  std::string s(name + 4);
  size_t comma = s.find(',');
  std::string v = s.substr(0, comma);
  bool hex = v.compare(0, 2, "0x") == 0 || v.compare(0, 2, "0X") == 0;
  if (!hex || v.size() <= 2 + 16) {
    return genExprFromInt(std::stoul(v, NULL, 0));
  }
  int w = comma == std::string::npos ? 4 * (int) (v.size() - 2)
                                     : std::stoi(s.substr(comma + 1));
  auto b = new BigInt(w, 0, 0);
  *b = v;
  b->setWidth(w);
  Expr *expr = genExprFromInt(b->getVal(0));
  expr->u.ival.v_extra = b;
  return expr;
}

Expr *getExprFromName(const char *name,
                      Map<const char *, Expr *> &exprMap,
                      bool exitOnMissing,
                      int exprType) {
  for (auto &exprMapIt: exprMap) {
    if (strcmp(name, exprMapIt.first) == 0) {
      return exprMapIt.second;
    }
  }
  if (exitOnMissing) {
    printf("We could not find the expr for %s!\n", name);
    exit(-1);
  }
  Expr *newExpr = nullptr;
  if (exprType == E_INT) {
    if (strncmp ("int(", name, 4) == 0) {
      newExpr = genExprFromIntName(name);
    }
    else {
      newExpr = genExprFromInt(std::stoul(std::string(name), NULL, 0));
    }
  } else {
    if (strncmp ("int(", name, 4) == 0) {
      newExpr = genExprFromIntName(name);
    }
    else {
      newExpr = genExprFromStr(name, exprType);
    }
  }
  exprMap.insert({name, newExpr});
  return newExpr;
}

bool isBinType(int exprType) {
  return (exprType == E_LT) || (exprType == E_GT) || (exprType == E_LE)
      || (exprType == E_GE) || (exprType == E_EQ) || (exprType == E_NE);
}

void getActIdName(Scope *sc, ActId *actId, char *buff, int sz) {
  ActId *uid = actId->Canonical(sc)->toid();
  uid->sPrint(buff, sz);
  delete uid;
}

void getCurProc(const char *str, char *val) {
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

void getActConnectionName(act_connection *actConnection,
                          char *buff,
                          int sz) {
  if (actConnection == nullptr) {
    printf("Try to get the name of NULL act connection!\n");
    exit(-1);
  }
  ActId *uid = actConnection->toid();
  uid->sPrint(buff, sz);
  delete uid;
}

void print_dflow(FILE *fp, list_t *dflow) {
  listitem_t *li;
  act_dataflow_element *e;

  for (li = list_first (dflow); li; li = list_next (li)) {
    e = (act_dataflow_element *) list_value (li);
    dflow_print(fp, e, 1);
    if (list_next (li)) {
      fprintf(fp, ";");
    }
    fprintf(fp, "\n");
  }
}

void removeDirectoryIfExist(const char *dir) {
  if (std::filesystem::is_directory(dir) && std::filesystem::exists(dir)) {
    std::filesystem::remove_all(dir);
  }
}

void createDirectoryIfNotExist(const char *dir) {
  if (!std::filesystem::is_directory(dir)
      || !std::filesystem::exists(dir)) {
    std::filesystem::create_directory(dir);
  }
}

void createFileIfNotExist(const char *file, std::ios_base::openmode mode) {
  if (!std::filesystem::is_regular_file(file)
      || !std::filesystem::exists(std::filesystem::path(file))) {
    std::ofstream file_stream;
    file_stream.open(file, mode);
  }
}

void createMetricsFileIfNotExist(const char *file, std::ios_base::openmode mode) {
  if (!std::filesystem::is_regular_file(file)
      || !std::filesystem::exists(std::filesystem::path(file))) {
    std::ofstream file_stream;
    file_stream.open(file, mode);
    file_stream << getenv ("ACT_TECH") << "\n";
  }
}


void copyFileToTargetDir(const char *srcFile,
                         const char *targetDir,
                         const char *errMsg) {
  std::filesystem::path sourceFile = srcFile;
  std::filesystem::path targetParent = targetDir;
  auto target = targetParent / sourceFile.filename();
  try {
    std::filesystem::copy_file(srcFile,
                               target,
                               std::filesystem::copy_options::overwrite_existing);
  } catch (std::exception &e) {
    printf("%s. Reason is: %s\n", errMsg, e.what());
    exit(-1);
  }
}

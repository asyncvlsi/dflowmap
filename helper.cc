#include "Helper.h"

bool isActnCp(const char *instance) {
  return std::string(instance).find(Constant::ACTN_CP_PREFIX) == 0;
}

bool isActnDp(const char *instance) {
  return std::string(instance).find(Constant::ACTN_DP_PREFIX) == 0;
}

void normalizeName(char *src, char toDel, char newChar) {
  char *pos = strchr(src, toDel);
  while (pos) {
    *pos = newChar;
    pos = strchr(pos + 1, toDel);
  }
}

void getNormalizedOpName(const char *op, char *normalizedOp) {
  strcat(normalizedOp, op);
  normalizeName(normalizedOp, '<', '_');
  normalizeName(normalizedOp, '>', '_');
  normalizeName(normalizedOp, ',', '_');
}

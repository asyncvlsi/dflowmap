#ifndef DFLOWMAP__HELPER_H_
#define DFLOWMAP__HELPER_H_
#include <cstring>
#include "Constant.h"

bool isActnCp(const char *instance);

bool isActnDp(const char *instance);

void normalizeName(char *src, char toDel, char newChar);

void getNormalizedOpName(const char *op, char *normalizedOp);

#endif //DFLOWMAP__HELPER_H_

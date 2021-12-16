#ifndef DFLOWMAP_CONSTANT_H
#define DFLOWMAP_CONSTANT_H

#include <string>

class Constant {
public:
  static constexpr const char* MERGE_PREFIX = "control_merge";
  static constexpr const char* ARBITER_PREFIX = "control_arbiter";
  static constexpr const char* SPLIT_PREFIX = "control_split";
  static constexpr const char* ACTN_CP_PREFIX = "actn_cp_";
  static constexpr const char* ACTN_DP_PREFIX = "actn_dp_";
};

#endif //DFLOWMAP_CONSTANT_H

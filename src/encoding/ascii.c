#include "parse.h"

static size_t alnum(const char *value) {
  if (
    (*value >= 'a' && *value <= 'z') ||
    (*value >= 'A' && *value <= 'Z') ||
    (*value >= '0' && *value <= '9')
  ) return 1;

  return 0;
}

encoding_t ascii = {
  .name = "ASCII",
  .alnum = alnum
};

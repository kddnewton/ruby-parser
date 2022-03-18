#include "parse.h"

#define UNUSED __attribute__((unused))

static void array(UNUSED token_t *opening, UNUSED token_t *closing, size_t size) {
  printf("ARRAY=%zu\n", size);
}

static void assign(token_t *operator) {
  switch (operator->type) {
    case TOKEN_AMPERSAND_EQUAL: printf("BITWISE_AND_ASSIGN\n"); break;
    case TOKEN_CARET_EQUAL: printf("BITWISE_XOR_ASSIGN\n"); break;
    case TOKEN_DOUBLE_AMPERSAND_EQUAL: printf("LOGICAL_AND_ASSIGN\n"); break;
    case TOKEN_DOUBLE_PIPE_EQUAL: printf("LOGICAL_OR_ASSIGN\n"); break;
    case TOKEN_DOUBLE_STAR_EQUAL: printf("EXPONENT_ASSIGN\n"); break;
    case TOKEN_EQUAL: printf("ASSIGN\n"); break;
    case TOKEN_MINUS_EQUAL: printf("SUBTRACT_ASSIGN\n"); break;
    case TOKEN_PERCENT_EQUAL: printf("MODULO_ASSIGN\n"); break;
    case TOKEN_PIPE_EQUAL: printf("BITWISE_OR_ASSIGN\n"); break;
    case TOKEN_PLUS_EQUAL: printf("ADD_ASSIGN\n"); break;
    case TOKEN_SHIFT_LEFT_EQUAL: printf("SHIFT_LEFT_ASSIGN\n"); break;
    case TOKEN_SHIFT_RIGHT_EQUAL: printf("SHIFT_RIGHT_ASSIGN\n"); break;
    case TOKEN_SLASH_EQUAL: printf("DIVIDE_ASSIGN\n"); break;
    case TOKEN_STAR_EQUAL: printf("MULTIPLY_ASSIGN\n"); break;
    default: printf("???\n"); break;
  }
}

static void begin(UNUSED token_t *opening, UNUSED token_t *closing) {
  printf("BEGIN\n");
}

static void binary(token_t *operator) {
  switch (operator->type) {
    case TOKEN_AMPERSAND: printf("BITWISE_AND\n"); break;
    case TOKEN_AND: printf("COMPOSITION_AND\n"); break;
    case TOKEN_BANG_EQUAL: printf("BANG_EQUAL\n"); break;
    case TOKEN_BANG_TILDE: printf("BANG_TILDE\n"); break;
    case TOKEN_CARET: printf("BITWISE_XOR\n"); break;
    case TOKEN_COMPARE: printf("COMPARE\n"); break;
    case TOKEN_DOUBLE_AMPERSAND: printf("LOGICAL_AND\n"); break;
    case TOKEN_DOUBLE_DOT: printf("RANGE_INCLUSIVE\n"); break;
    case TOKEN_DOUBLE_EQUAL: printf("DOUBLE_EQUAL\n"); break;
    case TOKEN_DOUBLE_PIPE: printf("LOGICAL_OR\n"); break;
    case TOKEN_DOUBLE_STAR: printf("EXPONENT\n"); break;
    case TOKEN_EQUAL_TILDE: printf("EQUAL_TILDE\n"); break;
    case TOKEN_GREATER_EQUAL: printf("GREATER_EQUAL\n"); break;
    case TOKEN_GREATER: printf("GREATER\n"); break;
    case TOKEN_IF: printf("IF_MODIFIER\n"); break;
    case TOKEN_LESS_EQUAL: printf("LESS_EQUAL\n"); break;
    case TOKEN_LESS: printf("LESS\n"); break;
    case TOKEN_MINUS: printf("SUBTRACT\n"); break;
    case TOKEN_OR: printf("COMPOSITION_OR\n"); break;
    case TOKEN_PERCENT: printf("MODULO\n"); break;
    case TOKEN_PIPE: printf("BITWISE_OR\n"); break;
    case TOKEN_PLUS: printf("ADD\n"); break;
    case TOKEN_RESCUE: printf("RESCUE_MODIFIER\n"); break;
    case TOKEN_SHIFT_LEFT: printf("SHIFT_LEFT\n"); break;
    case TOKEN_SHIFT_RIGHT: printf("SHIFT_RIGHT\n"); break;
    case TOKEN_SLASH: printf("DIVIDE\n"); break;
    case TOKEN_STAR: printf("MULTIPLY\n"); break;
    case TOKEN_TRIPLE_DOT: printf("RANGE_EXCLUSIVE\n"); break;
    case TOKEN_TRIPLE_EQUAL: printf("TRIPLE_EQUAL\n"); break;
    case TOKEN_UNLESS: printf("UNLESS_MODIFIER\n"); break;
    case TOKEN_UNTIL: printf("UNTIL_MODIFIER\n"); break;
    case TOKEN_WHILE: printf("WHILE_MODIFIER\n"); break;
    default: printf("???\n"); break;
  }
}

static void defined(UNUSED token_t *keyword) {
  printf("DEFINED\n");
}

static void group(UNUSED token_t *opening, UNUSED token_t *closing) {
  printf("GROUP\n");
}

static void index_call(UNUSED token_t *opening, UNUSED token_t *closing) {
  printf("INDEX_CALL\n");
}

static void index_expr(UNUSED token_t *opening, UNUSED token_t *closing) {
  printf("INDEX\n");
}

static void literal(token_t *value) {
  switch (value->type) {
    case TOKEN_FALSE: printf("FALSE\n"); return;
    case TOKEN_NIL: printf("NIL\n"); return;
    case TOKEN_SELF: printf("SELF\n"); return;
    case TOKEN_TRUE: printf("TRUE\n"); return;

    case TOKEN_BACK_REFERENCE: printf("BACK_REFERENCE"); break;
    case TOKEN_GLOBAL_VARIABLE: printf("GLOBAL_VARIABLE"); break;
    case TOKEN_IDENTIFIER: printf("VCALL"); break;
    case TOKEN_INTEGER: printf("INTEGER"); break;
    case TOKEN_METHOD_IDENTIFIER: printf("FCALL"); break;
    case TOKEN_NTH_REFERENCE: printf("NTH_REFERENCE"); break;

    default: printf("???"); break;
  }

  printf("=%.*s\n", (int) (value->end - value->start), value->start);
}

static void not(UNUSED token_t *keyword) {
  printf("NOT\n");
}

static void ternary(void) {
  printf("TERNARY\n");
}

static void unary(token_t *operator) {
  switch (operator->type) {
    case TOKEN_MINUS: printf("UMINUS\n"); break;
    case TOKEN_BANG: printf("UBANG\n"); break;
    case TOKEN_TILDE: printf("UTILDE\n"); break;
    case TOKEN_PLUS: printf("UPLUS\n"); break;
    case TOKEN_TRIPLE_DOT: printf("BEGINLESS_RANGE_EXCLUSIVE\n"); break;
    case TOKEN_DOUBLE_DOT: printf("BEGINLESS_RANGE_INCLUSIVE\n"); break;
    default: printf("???\n"); break;
  }
}

static void while_block(UNUSED token_t *keyword) {
  printf("WHILE\n");
}

static void until_block(UNUSED token_t *keyword) {
  printf("UNTIL\n");
}

#undef UNUSED

visitor_t printer = {
  .array = array,
  .assign = assign,
  .begin = begin,
  .binary = binary,
  .defined = defined,
  .group = group,
  .index_call = index_call,
  .index_expr = index_expr,
  .literal = literal,
  .not = not,
  .ternary = ternary,
  .unary = unary,
  .until_block = until_block,
  .while_block = while_block
};

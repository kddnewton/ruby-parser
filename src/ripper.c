#include "parse.h"

// This is used for tests to align the output of the tokenization with the
// output that Ripper.lex gives.
const char * ripper_event(token_type_t type) {
  switch (type) {
      case TOKEN_FALSE:
      case TOKEN_NIL:
      case TOKEN_SELF:
      case TOKEN_TRUE:
        return "kw";
      case TOKEN_COLON:
      case TOKEN_COMPARE:
      case TOKEN_DOUBLE_EQUAL:
      case TOKEN_DOUBLE_STAR:
      case TOKEN_DOUBLE_STAR_EQUAL:
      case TOKEN_EQUAL:
      case TOKEN_EQUAL_TILDE:
      case TOKEN_GREATER:
      case TOKEN_GREATER_EQUAL:
      case TOKEN_LESS:
      case TOKEN_LESS_EQUAL:
      case TOKEN_MINUS:
      case TOKEN_MINUS_EQUAL:
      case TOKEN_PERCENT:
      case TOKEN_PERCENT_EQUAL:
      case TOKEN_PLUS:
      case TOKEN_PLUS_EQUAL:
      case TOKEN_QUESTION_MARK:
      case TOKEN_SHIFT_LEFT:
      case TOKEN_SHIFT_LEFT_EQUAL:
      case TOKEN_SHIFT_RIGHT:
      case TOKEN_SHIFT_RIGHT_EQUAL:
      case TOKEN_SLASH:
      case TOKEN_SLASH_EQUAL:
      case TOKEN_STAR:
      case TOKEN_STAR_EQUAL:
      case TOKEN_TILDE:
      case TOKEN_TRIPLE_EQUAL:
        return "op";
      case TOKEN_BACK_REFERENCE: return "backref";
      case TOKEN_COMMA: return "comma";
      case TOKEN_GLOBAL_VARIABLE: return "gvar";
      case TOKEN_INTEGER: return "int";
      case TOKEN_LEFT_BRACKET: return "lbracket";
      case TOKEN_LEFT_PARENTHESIS: return "lparen";
      case TOKEN_NTH_REFERENCE: return "backref";
      case TOKEN_RIGHT_BRACKET: return "rbracket";
      case TOKEN_RIGHT_PARENTHESIS: return "rparen";
      case TOKEN_SEMICOLON: return "semicolon";
      default:
        return "???";
    }
}

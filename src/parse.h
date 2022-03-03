#ifndef PARSE_H
#define PARSE_H

#include <sys/stat.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  const char *name;
  size_t (*alnum)(const char *);
} encoding_t;

encoding_t ascii;

typedef enum {
  TOKEN_EOF = 0,
  TOKEN_AMPERSAND_EQUAL,        // &=
  TOKEN_AMPERSAND,              // &
  TOKEN_AND,                    // and
  TOKEN_BACK_REFERENCE,
  TOKEN_BANG_EQUAL,             // !=
  TOKEN_BANG_TILDE,             // !~
  TOKEN_BANG,                   // !
  TOKEN_BEGIN,                  // begin
  TOKEN_CARET_EQUAL,            // ^=
  TOKEN_CARET,                  // ^
  TOKEN_COLON,                  // :
  TOKEN_COMMA,                  // ,
  TOKEN_COMPARE,                // <=>
  TOKEN_DEFINED,                // defined?
  TOKEN_DOUBLE_AMPERSAND_EQUAL, // &&=
  TOKEN_DOUBLE_AMPERSAND,       // &&
  TOKEN_DOUBLE_DOT,             // ..
  TOKEN_DOUBLE_EQUAL,           // ==
  TOKEN_DOUBLE_PIPE_EQUAL,      // ||=
  TOKEN_DOUBLE_PIPE,            // ||
  TOKEN_DOUBLE_STAR_EQUAL,      // **=
  TOKEN_DOUBLE_STAR,            // **
  TOKEN_END,                    // end
  TOKEN_ENSURE,                 // ensure
  TOKEN_EQUAL_TILDE,            // =~
  TOKEN_EQUAL,                  // =
  TOKEN_FALSE,                  // false
  TOKEN_GLOBAL_VARIABLE,
  TOKEN_GREATER_EQUAL,          // >=
  TOKEN_GREATER,                // >
  TOKEN_IDENTIFIER,
  TOKEN_IF,                     // if
  TOKEN_INTEGER,
  TOKEN_LEFT_BRACKET,           // [
  TOKEN_LEFT_PARENTHESIS,       // (
  TOKEN_LESS_EQUAL,             // <=
  TOKEN_LESS,                   // <
  TOKEN_METHOD_IDENTIFIER,
  TOKEN_MINUS_EQUAL,            // -=
  TOKEN_MINUS,                  // -
  TOKEN_NEWLINE,                // \n
  TOKEN_NIL,                    // nil
  TOKEN_NOT,                    // not
  TOKEN_NTH_REFERENCE,
  TOKEN_OR,                     // or
  TOKEN_PERCENT_EQUAL,          // %=
  TOKEN_PERCENT,                // %
  TOKEN_PIPE_EQUAL,             // |=
  TOKEN_PIPE,                   // |
  TOKEN_PLUS_EQUAL,             // +=
  TOKEN_PLUS,                   // +
  TOKEN_QUESTION_MARK,          // ?
  TOKEN_RESCUE,                 // rescue
  TOKEN_RIGHT_BRACKET,          // ]
  TOKEN_RIGHT_PARENTHESIS,      // )
  TOKEN_SELF,                   // self
  TOKEN_SEMICOLON,              // ;
  TOKEN_SHIFT_LEFT_EQUAL,       // <<=
  TOKEN_SHIFT_LEFT,             // <<
  TOKEN_SHIFT_RIGHT_EQUAL,      // >>=
  TOKEN_SHIFT_RIGHT,            // >>
  TOKEN_SLASH_EQUAL,            // /=
  TOKEN_SLASH,                  // /
  TOKEN_STAR_EQUAL,             // *=
  TOKEN_STAR,                   // *
  TOKEN_TILDE,                  // ~
  TOKEN_TRIPLE_DOT,             // ...
  TOKEN_TRIPLE_EQUAL,           // ===
  TOKEN_TRUE,                   // true
  TOKEN_UNLESS,                 // unless
  TOKEN_UNTIL,                  // until
  TOKEN_WHILE                   // while
} token_type_t;

// This struct represents a token in the Ruby source. We use it to track both
// type and location information.
typedef struct {
  token_type_t type;
  const char *start;
  const char *end;
} token_t;

typedef struct {
  void *(*array)(token_t *opening, token_t *closing, size_t size);
  void *(*assign)(token_t *operator, void *left, void *right);
  void *(*begin)(token_t *opening, token_t *closing, void *statements);
  void *(*binary)(token_t *operator, void *left, void *right);
  void *(*defined)(token_t *keyword, void *expression);
  void *(*group)(token_t *opening, token_t *closing, void *expression);
  void *(*index_call)(token_t *opening, token_t *closing);
  void *(*index_expr)(token_t *opening, token_t *closing, void *expression);
  void *(*literal)(token_t *value);
  void *(*not)(token_t *keyword, void *expression);
  void *(*ternary)(void *predicate, void *truthy, void *falsey);
  void *(*unary)(token_t *operator, void *value);
  void *(*until_block)(token_t *keyword, void *predicate, void *statements);
  void *(*while_block)(token_t *keyword, void *predicate, void *statements);
} visitor_t;

visitor_t printer;

void tokenize(off_t, const char *);
void parse(off_t, const char *, visitor_t *);

#endif

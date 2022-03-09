#include "parse.h"

typedef enum {
  CONTEXT_MAIN,
  CONTEXT_ARRAY, // ]
  CONTEXT_BEGIN, // ensure end
  CONTEXT_ENSURE, // end
  CONTEXT_LOOP, // end
} context_type_t;

typedef struct context {
  context_type_t type;
  struct context *parent;
} context_t;

// This struct represents the overall parser. It contains a reference to the
// source file, as well as pointers that indicate where in the source it's
// currently parsing. Finally, it also contains the most recent and current
// token that it's considering.
typedef struct {
  const char *start;    // the pointer to the start of the source
  const char *end;      // the pointer to the end of the source
  token_t previous;     // the last token we considered
  token_t current;      // the current token we're considering
  int lineno;           // the current line number we're looking at
  visitor_t *visitor;   // the visitor used to visit each node as it is built
  encoding_t *encoding; // the current encoding being used for parsing
  context_t *context;   // the linked list of contexts for this parser
} parser_t;

// Returns the character at the given offset from the current character. If one
// can't be read because there aren't enough, then it returns \0.
static inline char peek(parser_t *parser, size_t offset) {
  const char *pointer = parser->current.end + offset;
  return pointer < parser->end ? *pointer : '\0';
}

// If the character to be read matches the given value, then returns true and
// advanced the current pointer.
static inline bool match(parser_t *parser, char value) {
  if (*parser->current.end == value) {
    parser->current.end++;
    return true;
  }
  return false;
}

static bool isdigit(const char value) {
  return '0' <= value && value <= '9';
}

static size_t identchar(parser_t *parser) {
  if (*(parser->current.end) == '_') {
    return 1;
  }

  return parser->encoding->alnum((parser->current.end));
}

static size_t lex_identifier(parser_t *parser) {
  for (size_t width = identchar(parser); width != 0; width = identchar(parser)) {
    parser->current.end += width;
  }

  return parser->current.end - parser->current.start;
}

static token_type_t lex_global_variable(parser_t *parser) {
  switch (*parser->current.end++) {
    case '_': // $_: last read line string
      if (identchar(parser)) break;
      return TOKEN_GLOBAL_VARIABLE;
    case '~': // $~: match-data
    case '*': // $*: argv
    case '$': // $$: pid
    case '?': // $?: last status
    case '!': // $!: error string
    case '@': // $@: error position
    case '/': // $/: input record separator
    case '\\': // $\: output record separator
    case ';': // $;: field separator
    case ',': // $,: output field separator
    case '.': // $.: last read line number
    case '=': // $=: ignorecase
    case ':': // $:: load path
    case '<': // $<: reading filename
    case '>': // $>: default output handle
      return TOKEN_GLOBAL_VARIABLE;
    case '-':
      if (identchar(parser)) {
        parser->current.end++;
      }
      return TOKEN_GLOBAL_VARIABLE;
    case '&':	// $&: last match
    case '`': // $`: string before last match
    case '\'': // $': string after last match
    case '+': // $+: string matches last paren
      return TOKEN_BACK_REFERENCE;
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      while (isdigit(*parser->current.end)) {
        parser->current.end++;
      }
      return TOKEN_NTH_REFERENCE;
  }

  lex_identifier(parser);
  return TOKEN_GLOBAL_VARIABLE;
}

// Parses forward until it hits the end of the current numeric token that the
// parser is looking at.
static token_type_t lex_numeric(parser_t *parser) {
  parser->current.type = TOKEN_INTEGER;

  while (isdigit(*parser->current.end)) {
    parser->current.end++;
  }

  return TOKEN_INTEGER;
}

// Moves the source pointer one token forward and returns the type of token that
// was just seen.
static token_type_t lex_token_type(parser_t *parser) {
  // Assign the entire current struct to the previous struct to maintain all of
  // that information for when it's needed.
  parser->previous = parser->current;

  for (;;) {
    parser->current.start = parser->current.end;

    switch (*parser->current.end++) {
      case '\0': // NUL or end of script
      case '\004': // ^D
      case '\032': // ^Z
        return TOKEN_EOF;

      case ' ':
      case '\t':
      case '\f':
      case '\r':
      case '\v': {
        size_t offset = 0;
        char current;

        // Skip past as many spaces as we can so we don't have to jump around
        // too much.
        while (
          (current = peek(parser, offset++)) &&
            (current == ' ' || current == '\t' || current == '\f' ||
             current == '\r' || current == '\v')
        );

        parser->current.end += offset - 1;
        break;
      }
      case '\n': {
        do {
          parser->lineno++; 
        } while (match(parser, '\n'));

        return TOKEN_NEWLINE;
      }

      case ',': return TOKEN_COMMA;
      case ';': return TOKEN_SEMICOLON;
      case ':': return TOKEN_COLON;
      case '?': return TOKEN_QUESTION_MARK;
      case '(': return TOKEN_LEFT_PARENTHESIS;
      case ')': return TOKEN_RIGHT_PARENTHESIS;
      case '[': return TOKEN_LEFT_BRACKET;
      case ']': return TOKEN_RIGHT_BRACKET;
      case '~': return TOKEN_TILDE;

      // = =~ == ===
      case '=':
        if (match(parser, '~')) return TOKEN_EQUAL_TILDE;
        if (match(parser, '=')) return match(parser, '=') ? TOKEN_TRIPLE_EQUAL : TOKEN_DOUBLE_EQUAL;
        return TOKEN_EQUAL;

      // < << <<= <= <=>
      case '<':
        if (match(parser, '<')) return match(parser, '=') ? TOKEN_SHIFT_LEFT_EQUAL : TOKEN_SHIFT_LEFT;
        if (match(parser, '=')) return match(parser, '>') ? TOKEN_COMPARE : TOKEN_LESS_EQUAL;
        return TOKEN_LESS;

      // > >> >>= >=
      case '>':
        if (match(parser, '>')) return match(parser, '=') ? TOKEN_SHIFT_RIGHT_EQUAL : TOKEN_SHIFT_RIGHT;
        return match(parser, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER;

      // + +=
      case '+': return match(parser, '=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS;

      // - -=
      case '-': return match(parser, '=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS;

      // * ** **= *=
      case '*':
        if (match(parser, '*')) return match(parser, '=') ? TOKEN_DOUBLE_STAR_EQUAL : TOKEN_DOUBLE_STAR;
        return match(parser, '=') ? TOKEN_STAR_EQUAL : TOKEN_STAR;

      // / /=
      case '/': return match(parser, '=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH;

      // % %=
      case '%': return match(parser, '=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT;

      // & && &&= &=
      case '&':
        if (match(parser, '&')) return match(parser, '=') ? TOKEN_DOUBLE_AMPERSAND_EQUAL : TOKEN_DOUBLE_AMPERSAND;
        return match(parser, '=') ? TOKEN_AMPERSAND_EQUAL : TOKEN_AMPERSAND;

      // | || ||= |=
      case '|':
        if (match(parser, '|')) return match(parser, '=') ? TOKEN_DOUBLE_PIPE_EQUAL : TOKEN_DOUBLE_PIPE;
        return match(parser, '=') ? TOKEN_PIPE_EQUAL : TOKEN_PIPE;

      // ^ ^=
      case '^': return match(parser, '=') ? TOKEN_CARET_EQUAL : TOKEN_CARET;

      // .. ...
      case '.':
        if (!match(parser, '.')) return TOKEN_EOF; // this is temporary
        return match(parser, '.') ? TOKEN_TRIPLE_DOT : TOKEN_DOUBLE_DOT;

      // ! !~ !=
      case '!':
        if (match(parser, '~')) return TOKEN_BANG_TILDE;
        return match(parser, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG;

      case '$': // this is temporary
        return lex_global_variable(parser);

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        return lex_numeric(parser);

      default: {
        size_t width = lex_identifier(parser);
        if (width == 0) return TOKEN_EOF;

        #define KEYWORD(value, size, token) \
          if (width == size && strncmp(parser->current.start, value, size) == 0) return token;

        if (peek(parser, 1) != '=' && (match(parser, '!') || match(parser, '?'))) {
          width++;
          KEYWORD("defined?", 8, TOKEN_DEFINED)
          return TOKEN_METHOD_IDENTIFIER;
        }

        KEYWORD("and", 3, TOKEN_AND)
        KEYWORD("begin", 5, TOKEN_BEGIN)
        KEYWORD("end", 3, TOKEN_END)
        KEYWORD("ensure", 6, TOKEN_ENSURE)
        KEYWORD("false", 5, TOKEN_FALSE)
        KEYWORD("if", 2, TOKEN_IF)
        KEYWORD("nil", 3, TOKEN_NIL)
        KEYWORD("not", 3, TOKEN_NOT)
        KEYWORD("or", 2, TOKEN_OR)
        KEYWORD("rescue", 6, TOKEN_RESCUE)
        KEYWORD("self", 4, TOKEN_SELF)
        KEYWORD("true", 4, TOKEN_TRUE)
        KEYWORD("unless", 6, TOKEN_UNLESS)
        KEYWORD("until", 5, TOKEN_UNTIL)
        KEYWORD("while", 5, TOKEN_WHILE)
        return TOKEN_IDENTIFIER;

        #undef KEYWORD
      }
    }
  }
}

// Get the next token type and set its value on the current pointer.
static inline void lex_token(parser_t *parser) {
  parser->current.type = lex_token_type(parser);
}

typedef enum {
  PRECEDENCE_NONE,
  PRECEDENCE_LITERAL,         // integer global_variable true false nil self
  // braces
  PRECEDENCE_MODIFIER,        // if unless while until
  PRECEDENCE_COMPOSITION,     // or and
  PRECEDENCE_NOT,             // not
  PRECEDENCE_DEFINED,         // defined?
  PRECEDENCE_ASSIGNMENT,      // = += -= *= /= %= &= |= ^= &&= ||= <<= >>= **=
  PRECEDENCE_MODIFIER_RESCUE, // rescue
  PRECEDENCE_TERNARY,         // ?:
  PRECEDENCE_RANGE,           // .. ...
  PRECEDENCE_LOGICAL_OR,      // ||
  PRECEDENCE_LOGICAL_AND,     // &&
  PRECEDENCE_EQUALITY,        // <=> == === != =~ !~
  PRECEDENCE_COMPARISON,      // > >= < <=
  PRECEDENCE_BITWISE_OR,      // | ^
  PRECEDENCE_BITWISE_AND,     // &
  PRECEDENCE_SHIFT,           // << >>
  PRECEDENCE_TERM,            // + -
  PRECEDENCE_FACTOR,          // * / %
  // unary minus
  PRECEDENCE_EXPONENT,        // **
  PRECEDENCE_UNARY,           // ! ~ +
  PRECEDENCE_INDEX,           // [] []=
} precedence_t;

typedef void (parse_function_t)(parser_t *);

typedef struct {
  parse_function_t *prefix;
  parse_function_t *infix;
  precedence_t left_bind;
  precedence_t right_bind;
} parse_rule_t;

parse_rule_t parse_rules[];

static bool accept(parser_t *parser, token_type_t type) {
  if (parser->current.type == type) {
    lex_token(parser);
    return true;
  }
  return false;
}

static bool accept_any(parser_t *parser, size_t count, ...) {
  va_list types;
  va_start(types, count);

  for (size_t index = 0; index < count; index++) {
    if (parser->current.type == va_arg(types, token_type_t)) {
      lex_token(parser);
      va_end(types);
      return true;
    }
  }

  va_end(types);
  return false;
}

static void consume(parser_t *parser, const char *message, token_type_t type) {
  if (!accept(parser, type)) {
    fprintf(stderr, "%s\n", message);
  }
}

static void consume_any(parser_t *parser, const char *message, size_t count, ...) {
  va_list types;
  va_start(types, count);

  for (size_t index = 0; index < count; index++) {
    if (parser->current.type == va_arg(types, token_type_t)) {
      lex_token(parser);
      va_end(types);
      return;
    }
  }

  fprintf(stderr, "%s", message);
  va_end(types);
}

static void parse_precedence(parser_t *parser, precedence_t precedence) {
  // If this is the end of the file, then return immediately.
  if (parser->current.type == TOKEN_EOF) {
    return;
  }

  // If this is the end of the context, then return immediately.
  switch (parser->context->type) {
    case CONTEXT_MAIN:
      break;
    case CONTEXT_ARRAY:
      if (parser->current.type == TOKEN_RIGHT_BRACKET) return;
      break;
    case CONTEXT_BEGIN:
      if (parser->current.type == TOKEN_ENSURE) return;
    case CONTEXT_ENSURE:
    case CONTEXT_LOOP:
      if (parser->current.type == TOKEN_END) return;
  }

  lex_token(parser);

  parse_function_t *prefix = parse_rules[parser->previous.type].prefix;
  if (prefix == NULL) {
    return;
  }

  prefix(parser);

  while (precedence <= parse_rules[parser->current.type].left_bind) {
    lex_token(parser);

    parse_function_t *infix = parse_rules[parser->previous.type].infix;
    if (infix == NULL) {
      return;
    }

    infix(parser);
  }
}

static void parse_expression(parser_t *parser) {
  parse_precedence(parser, PRECEDENCE_NONE + 1);
}

static size_t parse_list(parser_t *parser, context_type_t context_type) {
  context_t context = { .type = context_type, .parent = parser->context };
  parser->context = &context;

  bool parsing;
  size_t size = 0;

  for (parsing = true; parsing; size++) {
    parse_expression(parser);

    switch (context_type) {
      case CONTEXT_ARRAY:
        if (accept(parser, TOKEN_COMMA)) {
          // array element
        } else {
          parsing = false;
        }
        break;
      case CONTEXT_BEGIN:
      case CONTEXT_ENSURE:
      case CONTEXT_MAIN:
      case CONTEXT_LOOP:
        if (accept_any(parser, 2, TOKEN_NEWLINE, TOKEN_SEMICOLON)) {
          // statement
        } else {
          parsing = false;
        }
    }
  }

  parser->context = context.parent;
  return size;
}

// Parses an array literal.
//
//     [1, 2, 3]
//
static void parse_array(parser_t *parser) {
  token_t opening = parser->previous;
  size_t size;

  if (accept(parser, TOKEN_RIGHT_BRACKET)) {
    size = 0;
  } else {
    size = parse_list(parser, CONTEXT_ARRAY);
    consume(parser, "Expected ']' after the array elements.", TOKEN_RIGHT_BRACKET);
  }

  token_t closing = parser->previous;
  parser->visitor->array(&opening, &closing, size);
}

// Parses an assignment expression.
//
//     foo = 1
//
static void parse_assign(parser_t *parser) {
  token_t operator = parser->previous;
  parse_precedence(parser, parse_rules[operator.type].right_bind);
  parser->visitor->assign(&operator, NULL, NULL);
}

// Parses a begin expression (with an optional ensure clause).
//
//    begin
//    end
//
//    begin
//    ensure
//    end
//
static void parse_begin(parser_t *parser) {
  token_t opening = parser->previous;

  accept_any(parser, 2, TOKEN_NEWLINE, TOKEN_SEMICOLON);
  parse_list(parser, CONTEXT_BEGIN);

  if (accept(parser, TOKEN_ENSURE)) {
    accept_any(parser, 2, TOKEN_NEWLINE, TOKEN_SEMICOLON);
    parse_list(parser, CONTEXT_ENSURE);
  }

  consume(parser, "Expected 'end' after the begin block.", TOKEN_END);
  token_t closing = parser->previous;
  parser->visitor->begin(&opening, &closing, NULL);
}

// Parses a binary expression.
//
//     1 + 2
//
static void parse_binary(parser_t *parser) {
  token_t operator = parser->previous;
  parse_precedence(parser, parse_rules[operator.type].right_bind);
  parser->visitor->binary(&operator, NULL, NULL);
}

// Parses a defined? expression.
//
//     defined? foo
//
static void parse_defined(parser_t *parser) {
  token_t keyword = parser->previous;

  if (accept(parser, TOKEN_LEFT_PARENTHESIS)) {
    parse_expression(parser);
    consume(parser, "Expected ')' after expression.", TOKEN_RIGHT_PARENTHESIS);
  } else {
    parse_expression(parser);
  }

  parser->visitor->defined(&keyword, NULL);
}

// Parses a grouped expression.
//
//     (1 + 2)
//
static void parse_grouping(parser_t *parser) {
  token_t opening = parser->previous;

  parse_expression(parser);
  consume(parser, "Expected ')' after expression.", TOKEN_RIGHT_PARENTHESIS);

  token_t closing = parser->previous;
  parser->visitor->group(&opening, &closing, NULL);
}

// Parses an index expression (with or without an inner expression).
//
//     foo[]
//     foo[1]
//
static void parse_index(parser_t *parser) {
  token_t opening = parser->previous;

  if (accept(parser, TOKEN_RIGHT_BRACKET)) {
    token_t closing = parser->previous;
    parser->visitor->index_call(&opening, &closing);
  } else {
    parse_precedence(parser, PRECEDENCE_MODIFIER_RESCUE + 1);
    consume(parser, "Expected ']' after expression.", TOKEN_RIGHT_BRACKET);

    token_t closing = parser->previous;
    parser->visitor->index_expr(&opening, &closing, NULL);
  }
}

// Parses a literal value.
//
//     $1
//     false
//     $foo
//     foo
//     1
//     nil
//     _1
//     self
//     true
//
static void parse_literal(parser_t *parser) {
  parser->visitor->literal(&parser->previous);
}

// Parses a while or until loop.
//
//     while foo
//     end
//
static void parse_loop(parser_t *parser) {
  token_t token = parser->previous;

  parse_expression(parser);
  consume_any(parser, "Expected separator after predicate.", 2, TOKEN_NEWLINE, TOKEN_SEMICOLON);
  parse_list(parser, CONTEXT_LOOP);

  if (token.type == TOKEN_WHILE) {
    parser->visitor->while_block(&token, NULL, NULL);
  } else {
    parser->visitor->until_block(&token, NULL, NULL);
  }
}

// Parses a not expression.
//
//     not foo
//
static void parse_not(parser_t *parser) {
  token_t keyword = parser->previous;

  if (accept(parser, TOKEN_LEFT_PARENTHESIS)) {
    parse_expression(parser);
    consume(parser, "Expected ')' after expression.", TOKEN_RIGHT_PARENTHESIS);
  } else {
    parse_expression(parser);
  }

  parser->visitor->not(&keyword, NULL);
}

// Parses a ternary expression.
//
//     foo ? bar : baz
//
static void parse_ternary(parser_t *parser) {
  precedence_t right_bind = parse_rules[parser->previous.type].right_bind;

  parse_precedence(parser, right_bind);
  consume(parser, "Expected ':' after expression.", TOKEN_COLON);

  parse_precedence(parser, right_bind);
  parser->visitor->ternary(NULL, NULL, NULL);
}

// Parses a unary expression.
//
//     -foo
//
static void parse_unary(parser_t *parser) {
  token_t operator = parser->previous;
  parse_precedence(parser, PRECEDENCE_UNARY);
  parser->visitor->unary(&operator, NULL);
}

// These macros define associativity by defining binding power for the left and right side of the token
#define LEFT(precedence) precedence, precedence + 1
#define RIGHT(precedence) precedence, precedence
#define NONE RIGHT(PRECEDENCE_NONE)

parse_rule_t parse_rules[] = {
  [TOKEN_AMPERSAND_EQUAL]        = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_AMPERSAND]              = { NULL,           parse_binary,  LEFT(PRECEDENCE_BITWISE_AND) },
  [TOKEN_AND]                    = { NULL,           parse_binary,  LEFT(PRECEDENCE_COMPOSITION) },
  [TOKEN_BACK_REFERENCE]         = { parse_literal,  NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_BANG_EQUAL]             = { NULL,           parse_binary,  LEFT(PRECEDENCE_EQUALITY) },
  [TOKEN_BANG_TILDE]             = { NULL,           parse_binary,  LEFT(PRECEDENCE_EQUALITY) },
  [TOKEN_BANG]                   = { parse_unary,    NULL,          LEFT(PRECEDENCE_UNARY) },
  [TOKEN_BEGIN]                  = { parse_begin,    NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_CARET_EQUAL]            = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_CARET]                  = { NULL,           parse_binary,  LEFT(PRECEDENCE_BITWISE_OR) },
  [TOKEN_COLON]                  = { NULL,           NULL,          NONE },
  [TOKEN_COMMA]                  = { NULL,           NULL,          NONE },
  [TOKEN_COMPARE]                = { NULL,           parse_binary,  LEFT(PRECEDENCE_EQUALITY) },
  [TOKEN_DEFINED]                = { parse_defined,  NULL,          LEFT(PRECEDENCE_DEFINED) },
  [TOKEN_DOUBLE_AMPERSAND_EQUAL] = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_DOUBLE_AMPERSAND]       = { NULL,           parse_binary,  LEFT(PRECEDENCE_LOGICAL_AND) },
  [TOKEN_DOUBLE_DOT]             = { parse_unary,    parse_binary,  LEFT(PRECEDENCE_RANGE) },
  [TOKEN_DOUBLE_EQUAL]           = { NULL,           parse_binary,  LEFT(PRECEDENCE_EQUALITY) },
  [TOKEN_DOUBLE_PIPE_EQUAL]      = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_DOUBLE_PIPE]            = { NULL,           parse_binary,  LEFT(PRECEDENCE_LOGICAL_OR) },
  [TOKEN_DOUBLE_STAR_EQUAL]      = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_DOUBLE_STAR]            = { NULL,           parse_binary,  RIGHT(PRECEDENCE_EXPONENT) },
  [TOKEN_END]                    = { NULL,           NULL,          NONE },
  [TOKEN_EQUAL_TILDE]            = { NULL,           parse_binary,  LEFT(PRECEDENCE_EQUALITY) },
  [TOKEN_EQUAL]                  = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_FALSE]                  = { parse_literal,  NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_GLOBAL_VARIABLE]        = { parse_literal,  NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_GREATER_EQUAL]          = { NULL,           parse_binary,  LEFT(PRECEDENCE_COMPARISON) },
  [TOKEN_GREATER]                = { NULL,           parse_binary,  LEFT(PRECEDENCE_COMPARISON) },
  [TOKEN_IDENTIFIER]             = { parse_literal,  NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_IF]                     = { NULL,           parse_binary,  LEFT(PRECEDENCE_MODIFIER) },
  [TOKEN_INTEGER]                = { parse_literal,  NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_LEFT_BRACKET]           = { parse_array,    parse_index,   LEFT(PRECEDENCE_INDEX) },
  [TOKEN_LEFT_PARENTHESIS]       = { parse_grouping, NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_LESS_EQUAL]             = { NULL,           parse_binary,  LEFT(PRECEDENCE_COMPARISON) },
  [TOKEN_LESS]                   = { NULL,           parse_binary,  LEFT(PRECEDENCE_COMPARISON) },
  [TOKEN_METHOD_IDENTIFIER]      = { parse_literal,  NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_MINUS_EQUAL]            = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_MINUS]                  = { parse_unary,    parse_binary,  LEFT(PRECEDENCE_TERM) },
  [TOKEN_NEWLINE]                = { NULL,           NULL,          NONE },
  [TOKEN_NIL]                    = { parse_literal,  NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_NOT]                    = { parse_not,      NULL,          LEFT(PRECEDENCE_NOT) },
  [TOKEN_NTH_REFERENCE]          = { parse_literal,  NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_OR]                     = { NULL,           parse_binary,  LEFT(PRECEDENCE_COMPOSITION) },
  [TOKEN_PERCENT_EQUAL]          = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_PERCENT]                = { NULL,           parse_binary,  LEFT(PRECEDENCE_FACTOR) },
  [TOKEN_PIPE_EQUAL]             = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_PIPE]                   = { NULL,           parse_binary,  LEFT(PRECEDENCE_BITWISE_OR) },
  [TOKEN_PLUS_EQUAL]             = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_PLUS]                   = { parse_unary,    parse_binary,  LEFT(PRECEDENCE_TERM) },
  [TOKEN_QUESTION_MARK]          = { NULL,           parse_ternary, RIGHT(PRECEDENCE_TERNARY) },
  [TOKEN_RESCUE]                 = { NULL,           parse_binary,  LEFT(PRECEDENCE_MODIFIER_RESCUE) },
  [TOKEN_RIGHT_BRACKET]          = { NULL,           NULL,          NONE },
  [TOKEN_RIGHT_PARENTHESIS]      = { NULL,           NULL,          NONE },
  [TOKEN_SELF]                   = { parse_literal,  NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_SEMICOLON]              = { NULL,           NULL,          NONE },
  [TOKEN_SHIFT_LEFT_EQUAL]       = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_SHIFT_LEFT]             = { NULL,           parse_binary,  LEFT(PRECEDENCE_SHIFT) },
  [TOKEN_SHIFT_RIGHT_EQUAL]      = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_SHIFT_RIGHT]            = { NULL,           parse_binary,  LEFT(PRECEDENCE_SHIFT) },
  [TOKEN_SLASH_EQUAL]            = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_SLASH]                  = { NULL,           parse_binary,  LEFT(PRECEDENCE_FACTOR) },
  [TOKEN_STAR_EQUAL]             = { NULL,           parse_assign,  RIGHT(PRECEDENCE_ASSIGNMENT) },
  [TOKEN_STAR]                   = { NULL,           parse_binary,  LEFT(PRECEDENCE_FACTOR) },
  [TOKEN_TILDE]                  = { parse_unary,    NULL,          LEFT(PRECEDENCE_UNARY) },
  [TOKEN_TRIPLE_DOT]             = { parse_unary,    parse_binary,  LEFT(PRECEDENCE_RANGE) },
  [TOKEN_TRIPLE_EQUAL]           = { NULL,           parse_binary,  LEFT(PRECEDENCE_EQUALITY) },
  [TOKEN_TRUE]                   = { parse_literal,  NULL,          RIGHT(PRECEDENCE_LITERAL) },
  [TOKEN_UNLESS]                 = { NULL,           parse_binary,  LEFT(PRECEDENCE_MODIFIER) },
  [TOKEN_UNTIL]                  = { parse_loop,     parse_binary,  LEFT(PRECEDENCE_MODIFIER) },
  [TOKEN_WHILE]                  = { parse_loop,     parse_binary,  LEFT(PRECEDENCE_MODIFIER) }
};

#undef LEFT
#undef RIGHT
#undef NONE

const char * ripper_event(token_type_t type);

// Loop through every token that the parser produces and output a small
// descriptive message describing it.
void tokenize(off_t size, const char *source) {
  parser_t parser = {
    .start = source,
    .end = source + size,
    .current = { .start = source, .end = source },
    .lineno = 1,
    .encoding = &ascii
  };

  for (lex_token(&parser); parser.current.type != TOKEN_EOF; lex_token(&parser)) {
    printf(
      "%ld-%ld %s %.*s\n",
      parser.current.start - parser.start,
      parser.current.end - parser.start,
      ripper_event(parser.current.type),
      (int) (parser.current.end - parser.current.start),
      parser.current.start
    );
  }
}

// Go through the entire parse process and visit each node in the tree from the
// bottom to the top.
void parse(off_t size, const char *source, visitor_t *visitor) {
  parser_t parser = {
    .start = source,
    .end = source + size,
    .current = { .start = source, .end = source },
    .lineno = 1,
    .visitor = visitor,
    .encoding = &ascii,
    .context = NULL
  };
  
  parse_start = source;

  lex_token(&parser);
  parse_list(&parser, CONTEXT_MAIN);
}

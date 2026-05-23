#ifndef TOKENIZER_H
#define TOKENIZER_H 1

#include <stdbool.h>

#include "general.h"

typedef enum
{
    TOK_ERROR,

    TOK_EOF,
    TOK_LABEL,
    TOK_OPCODE,
    TOK_EXT_OPCODE,
    TOK_TARGET,
    TOK_NUMBER,
    TOK_FLOAT,
    TOK_COMMA,
    TOK_DOT,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_CHAR_LITERAL,
    TOK_STRING,

    TOK_BRACKET_LEFT,
    TOK_BRACKET_RIGHT,
    TOK_SQUARE_BRACKET_LEFT,
    TOK_SQUARE_BRACKET_RIGHT,
    TOK_CURLY_BRACKET_LEFT,
    TOK_CURLY_BRACKET_RIGHT,

    TOK_PLUS,
    TOK_MINUS,
    TOK_ASTERISK,
    TOK_FORWARDSLASH,

    TOK_EXCLAMATION_MARK,
    TOK_AT,
    TOK_HASHTAG,

    TOK_DOLLARSIGN,
    TOK_PERCENT,
    TOK_CARET,
    TOK_AMPERSAND,
    TOK_QUESTION_MARK,
    TOK_TILDA,

    TOK_LESSER,
    TOK_GREATER,
    TOK_LESSER_OR_EQUAL,
    TOK_GREATER_OR_EQUAL,

    TOK_EQUAL,
    TOK_NOT_EQUAL,
    
    TOK_PLUS_EQUAL,
    TOK_MINUS_EQUAL,
    TOK_ASTERISK_EQUAL,
    TOK_FORWARDSLASH_EQUAL,

    TOK_COMMENT
} token_type;

typedef struct
{
    i64 value;      // numeric value for numbers
    char text[256]; // copy of the lexeme
    token_type type;
    u8 text_length;
} token;

const char *token_str(token_type t);

token token_next(const char **src, i32 *lines_ret);
void token_debug_all(const char *src);

#endif
/* lang/token.h — Luna scripting language token definitions */
#ifndef LUNA_TOKEN_H
#define LUNA_TOKEN_H
#include "../include/types.h"

typedef enum {
    /* Literals */
    TOK_INT, TOK_FLOAT, TOK_STRING, TOK_TRUE, TOK_FALSE, TOK_NULL,
    /* Identifiers & keywords */
    TOK_IDENT,
    TOK_LET, TOK_FN, TOK_RETURN, TOK_IF, TOK_ELSE, TOK_ELIF,
    TOK_WHILE, TOK_FOR, TOK_IN, TOK_BREAK, TOK_CONTINUE,
    TOK_IMPORT, TOK_CLASS, TOK_SELF, TOK_NEW, TOK_DELETE,
    TOK_AND, TOK_OR, TOK_NOT,
    /* Operators */
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_EQ, TOK_NEQ, TOK_LT, TOK_LE, TOK_GT, TOK_GE,
    TOK_ASSIGN, TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN,
    TOK_STAR_ASSIGN, TOK_SLASH_ASSIGN,
    TOK_AMPERSAND, TOK_PIPE, TOK_CARET, TOK_TILDE,
    TOK_LSHIFT, TOK_RSHIFT,
    /* Delimiters */
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACKET, TOK_RBRACKET,
    TOK_LBRACE, TOK_RBRACE,
    TOK_COMMA, TOK_DOT, TOK_COLON, TOK_SEMICOLON,
    TOK_ARROW,      /* -> */
    TOK_FAT_ARROW,  /* => */
    /* Structural */
    TOK_NEWLINE,
    TOK_INDENT,
    TOK_DEDENT,
    TOK_EOF,
    /* Error */
    TOK_ERROR
} TokenType;

#define TOKEN_MAX_LEN 256

typedef struct {
    TokenType type;
    int       line;
    int       col;
    union {
        int64_t  as_int;
        double   as_float;
        char     as_str[TOKEN_MAX_LEN];
    };
} Token;

/* Keyword table for fast lookup */
typedef struct { const char* word; TokenType type; } Keyword;
static const Keyword KEYWORDS[] = {
    {"let",      TOK_LET},    {"fn",       TOK_FN},
    {"return",   TOK_RETURN}, {"if",       TOK_IF},
    {"else",     TOK_ELSE},   {"elif",     TOK_ELIF},
    {"while",    TOK_WHILE},  {"for",      TOK_FOR},
    {"in",       TOK_IN},     {"break",    TOK_BREAK},
    {"continue", TOK_CONTINUE},{"import",  TOK_IMPORT},
    {"class",    TOK_CLASS},  {"self",     TOK_SELF},
    {"new",      TOK_NEW},    {"and",      TOK_AND},
    {"or",       TOK_OR},     {"not",      TOK_NOT},
    {"true",     TOK_TRUE},   {"false",    TOK_FALSE},
    {"null",     TOK_NULL},   {NULL,       TOK_ERROR}
};

#endif /* LUNA_TOKEN_H */

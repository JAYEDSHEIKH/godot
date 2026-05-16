#pragma once
#include "core/string/ustring.h"

enum class NexTokenKind {
    LIT_INT, LIT_FLOAT, LIT_STRING, LIT_BOOL, LIT_CHAR,

    KW_FN, KW_RET, KW_VAR, KW_CONST,
    KW_IF, KW_ELIF, KW_ELSE,
    KW_FOR, KW_WHILE, KW_LOOP, KW_BREAK, KW_CONTINUE,
    KW_IN, KW_BY,
    KW_MATCH,
    KW_STRUCT, KW_IMPL, KW_ENUM,
    KW_USE, KW_PUB,
    KW_ASYNC, KW_AWAIT,
    KW_AND, KW_OR, KW_NOT,
    KW_SOME, KW_NONE, KW_OK, KW_ERR,
    KW_TRUE, KW_FALSE,
    KW_EXTENDS,
    KW_SIGNAL,
    KW_EMIT,
    KW_AUTOLOAD,
    KW_AS,
    KW_IS,
    KW_INLINE,
    KW_PARALLEL,

    SYM_WALRUS,
    SYM_COLON,
    SYM_SEMICOLON,
    SYM_COMMA,
    SYM_DOT,
    SYM_DOTDOT,
    SYM_DOTDOTEQ,
    SYM_ARROW,
    SYM_FAT_ARROW,
    SYM_ASSIGN,
    SYM_LPAREN, SYM_RPAREN,
    SYM_LBRACE, SYM_RBRACE,
    SYM_LBRACKET, SYM_RBRACKET,
    SYM_QUESTION,
    SYM_AT,
    SYM_DOLLAR,
    SYM_HASH,
    SYM_LT_ANGLE,
    SYM_GT_ANGLE,

    OP_PLUS, OP_MINUS, OP_STAR, OP_SLASH, OP_PERCENT,
    OP_STARSTAR,
    OP_EQ, OP_NEQ,
    OP_LT, OP_GT, OP_LTE, OP_GTE,
    OP_PLUS_EQ, OP_MINUS_EQ, OP_STAR_EQ, OP_SLASH_EQ, OP_PERCENT_EQ,

    IDENT,
    EOF_TOKEN,
    ERROR,
};

struct NexToken {
    NexTokenKind kind;
    String       value;
    int          line;
    int          col;
};

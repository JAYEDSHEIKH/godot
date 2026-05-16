#include "nex_tokenizer.h"

void NexTokenizer::set_source(const String &src) {
    source = src;
    pos    = 0;
    line   = 1;
    col    = 1;
}

char NexTokenizer::peek(int offset) const {
    int i = pos + offset;
    if (i >= source.length()) return '\0';
    return (char)source[i];
}

char NexTokenizer::advance() {
    char c = (char)source[pos++];
    if (c == '\n') { line++; col = 1; } else { col++; }
    return c;
}

NexToken NexTokenizer::make_tok(NexTokenKind kind, const String &value) const {
    NexToken t;
    t.kind  = kind;
    t.value = value;
    t.line  = line;
    t.col   = col;
    return t;
}

void NexTokenizer::skip_whitespace_and_comments() {
    while (pos < source.length()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            while (pos < source.length() && peek() != '\n') advance();
        } else if (c == '/' && peek(1) == '*') {
            advance(); advance();
            while (pos < source.length()) {
                if (peek() == '*' && peek(1) == '/') { advance(); advance(); break; }
                advance();
            }
        } else {
            break;
        }
    }
}

NexToken NexTokenizer::read_string() {
    advance(); // consume opening "
    String result;
    while (pos < source.length() && peek() != '"') {
        if (peek() == '\\') {
            advance();
            char esc = advance();
            switch (esc) {
                case 'n':  result += "\n"; break;
                case 't':  result += "\t"; break;
                case '"':  result += "\""; break;
                case '\\': result += "\\"; break;
                default:   result += String::chr(esc); break;
            }
        } else {
            result += String::chr(advance());
        }
    }
    if (pos < source.length()) advance(); // consume closing "
    return make_tok(NexTokenKind::LIT_STRING, result);
}

NexToken NexTokenizer::read_char_literal() {
    advance(); // consume '
    String result;
    if (peek() == '\\') {
        advance();
        char esc = advance();
        switch (esc) {
            case 'n': result = "\n"; break;
            case 't': result = "\t"; break;
            default:  result = String::chr(esc); break;
        }
    } else {
        result = String::chr(advance());
    }
    if (peek() == '\'') advance();
    return make_tok(NexTokenKind::LIT_CHAR, result);
}

NexToken NexTokenizer::read_number() {
    String result;
    bool is_float = false;
    while (pos < source.length() && (isdigit(peek()) || peek() == '_')) {
        char c = advance();
        if (c != '_') result += String::chr(c);
    }
    if (peek() == '.' && isdigit(peek(1))) {
        is_float = true;
        result += String::chr(advance()); // .
        while (pos < source.length() && isdigit(peek())) {
            result += String::chr(advance());
        }
    }
    return make_tok(is_float ? NexTokenKind::LIT_FLOAT : NexTokenKind::LIT_INT, result);
}

NexToken NexTokenizer::read_ident_or_kw() {
    String result;
    while (pos < source.length() && (isalnum(peek()) || peek() == '_')) {
        result += String::chr(advance());
    }

    static const struct { const char *kw; NexTokenKind kind; } keywords[] = {
        {"fn",       NexTokenKind::KW_FN},
        {"ret",      NexTokenKind::KW_RET},
        {"var",      NexTokenKind::KW_VAR},
        {"const",    NexTokenKind::KW_CONST},
        {"if",       NexTokenKind::KW_IF},
        {"elif",     NexTokenKind::KW_ELIF},
        {"else",     NexTokenKind::KW_ELSE},
        {"for",      NexTokenKind::KW_FOR},
        {"while",    NexTokenKind::KW_WHILE},
        {"loop",     NexTokenKind::KW_LOOP},
        {"break",    NexTokenKind::KW_BREAK},
        {"continue", NexTokenKind::KW_CONTINUE},
        {"in",       NexTokenKind::KW_IN},
        {"by",       NexTokenKind::KW_BY},
        {"match",    NexTokenKind::KW_MATCH},
        {"struct",   NexTokenKind::KW_STRUCT},
        {"impl",     NexTokenKind::KW_IMPL},
        {"enum",     NexTokenKind::KW_ENUM},
        {"use",      NexTokenKind::KW_USE},
        {"pub",      NexTokenKind::KW_PUB},
        {"async",    NexTokenKind::KW_ASYNC},
        {"await",    NexTokenKind::KW_AWAIT},
        {"and",      NexTokenKind::KW_AND},
        {"or",       NexTokenKind::KW_OR},
        {"not",      NexTokenKind::KW_NOT},
        {"some",     NexTokenKind::KW_SOME},
        {"none",     NexTokenKind::KW_NONE},
        {"ok",       NexTokenKind::KW_OK},
        {"err",      NexTokenKind::KW_ERR},
        {"true",     NexTokenKind::KW_TRUE},
        {"false",    NexTokenKind::KW_FALSE},
        {"extends",  NexTokenKind::KW_EXTENDS},
        {"signal",   NexTokenKind::KW_SIGNAL},
        {"emit",     NexTokenKind::KW_EMIT},
        {"autoload", NexTokenKind::KW_AUTOLOAD},
        {"as",       NexTokenKind::KW_AS},
        {"is",       NexTokenKind::KW_IS},
        {"parallel", NexTokenKind::KW_PARALLEL},
        {nullptr, NexTokenKind::ERROR}
    };

    for (int i = 0; keywords[i].kw != nullptr; i++) {
        if (result == keywords[i].kw) {
            return make_tok(keywords[i].kind, result);
        }
    }

    return make_tok(NexTokenKind::IDENT, result);
}

NexToken NexTokenizer::read_symbol() {
    char c = advance();
    switch (c) {
        case ';': return make_tok(NexTokenKind::SYM_SEMICOLON, ";");
        case ',': return make_tok(NexTokenKind::SYM_COMMA, ",");
        case '(': return make_tok(NexTokenKind::SYM_LPAREN, "(");
        case ')': return make_tok(NexTokenKind::SYM_RPAREN, ")");
        case '{': return make_tok(NexTokenKind::SYM_LBRACE, "{");
        case '}': return make_tok(NexTokenKind::SYM_RBRACE, "}");
        case '[': return make_tok(NexTokenKind::SYM_LBRACKET, "[");
        case ']': return make_tok(NexTokenKind::SYM_RBRACKET, "]");
        case '?': return make_tok(NexTokenKind::SYM_QUESTION, "?");
        case '@': return make_tok(NexTokenKind::SYM_AT, "@");
        case '$': return make_tok(NexTokenKind::SYM_DOLLAR, "$");
        case '#': return make_tok(NexTokenKind::SYM_HASH, "#");
        case '.': {
            if (peek() == '.' && peek(1) == '=') {
                advance(); advance();
                return make_tok(NexTokenKind::SYM_DOTDOTEQ, "..=");
            } else if (peek() == '.') {
                advance();
                return make_tok(NexTokenKind::SYM_DOTDOT, "..");
            }
            return make_tok(NexTokenKind::SYM_DOT, ".");
        }
        case ':': {
            if (peek() == '=') { advance(); return make_tok(NexTokenKind::SYM_WALRUS, ":="); }
            return make_tok(NexTokenKind::SYM_COLON, ":");
        }
        case '=': {
            if (peek() == '=') { advance(); return make_tok(NexTokenKind::OP_EQ, "=="); }
            return make_tok(NexTokenKind::SYM_ASSIGN, "=");
        }
        case '!': {
            if (peek() == '=') { advance(); return make_tok(NexTokenKind::OP_NEQ, "!="); }
            return make_tok(NexTokenKind::ERROR, "!");
        }
        case '<': {
            if (peek() == '=') { advance(); return make_tok(NexTokenKind::OP_LTE, "<="); }
            return make_tok(NexTokenKind::OP_LT, "<");
        }
        case '>': {
            if (peek() == '=') { advance(); return make_tok(NexTokenKind::OP_GTE, ">="); }
            return make_tok(NexTokenKind::OP_GT, ">");
        }
        case '+': {
            if (peek() == '=') { advance(); return make_tok(NexTokenKind::OP_PLUS_EQ, "+="); }
            return make_tok(NexTokenKind::OP_PLUS, "+");
        }
        case '-': {
            if (peek() == '>') { advance(); return make_tok(NexTokenKind::SYM_ARROW, "->"); }
            if (peek() == '=') { advance(); return make_tok(NexTokenKind::OP_MINUS_EQ, "-="); }
            return make_tok(NexTokenKind::OP_MINUS, "-");
        }
        case '*': {
            if (peek() == '*') { advance(); return make_tok(NexTokenKind::OP_STARSTAR, "**"); }
            if (peek() == '=') { advance(); return make_tok(NexTokenKind::OP_STAR_EQ, "*="); }
            return make_tok(NexTokenKind::OP_STAR, "*");
        }
        case '/': {
            if (peek() == '=') { advance(); return make_tok(NexTokenKind::OP_SLASH_EQ, "/="); }
            return make_tok(NexTokenKind::OP_SLASH, "/");
        }
        case '%': {
            if (peek() == '=') { advance(); return make_tok(NexTokenKind::OP_PERCENT_EQ, "%="); }
            return make_tok(NexTokenKind::OP_PERCENT, "%");
        }
        default:
            return make_tok(NexTokenKind::ERROR, String::chr(c));
    }
}

Vector<NexToken> NexTokenizer::tokenize() {
    Vector<NexToken> tokens;
    while (true) {
        skip_whitespace_and_comments();
        if (pos >= source.length()) {
            tokens.push_back(make_tok(NexTokenKind::EOF_TOKEN, ""));
            break;
        }
        char c = peek();
        NexToken tok;
        if (c == '"') {
            tok = read_string();
        } else if (c == '\'') {
            tok = read_char_literal();
        } else if (isdigit(c)) {
            tok = read_number();
        } else if (isalpha(c) || c == '_') {
            tok = read_ident_or_kw();
        } else {
            tok = read_symbol();
        }
        tokens.push_back(tok);
    }
    return tokens;
}

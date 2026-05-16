#pragma once
#include "nex_token.h"
#include "core/templates/vector.h"

class NexTokenizer {
    String  source;
    int     pos  = 0;
    int     line = 1;
    int     col  = 1;

public:
    void             set_source(const String &src);
    Vector<NexToken> tokenize();

private:
    char     peek(int offset = 0) const;
    char     advance();
    void     skip_whitespace_and_comments();
    NexToken read_string();
    NexToken read_char_literal();
    NexToken read_number();
    NexToken read_ident_or_kw();
    NexToken read_symbol();
    NexToken make_tok(NexTokenKind kind, const String &value = "") const;
};

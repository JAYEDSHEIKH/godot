#pragma once
#include "nex_ast.h"
#include "nex_token.h"
#include "core/templates/list.h"

class NexParser {
    Vector<NexToken> tokens;
    int              pos = 0;

public:
    NexNode *parse(const Vector<NexToken> &p_tokens);
    List<String> get_errors() const { return errors; }

private:
    List<String> errors;

    NexToken &peek(int offset = 0);
    NexToken  consume();
    NexToken  expect(NexTokenKind kind, const String &msg);
    bool      check(NexTokenKind kind) const;
    bool      match_tok(NexTokenKind kind);
    bool      at_end() const;

    NexNode *parse_program();
    NexNode *parse_top_level();
    NexNode *parse_extends();
    NexNode *parse_use();
    NexNode *parse_fn(bool is_async = false, bool is_pub = false);
    NexNode *parse_struct();
    NexNode *parse_impl();
    NexNode *parse_enum();
    NexNode *parse_signal();
    NexNode *parse_annotation();
    NexNode *parse_const_decl();

    NexNode *parse_block();
    NexNode *parse_stmt();
    NexNode *parse_var_decl(bool force_mutable = false);
    NexNode *parse_if();
    NexNode *parse_for();
    NexNode *parse_while();
    NexNode *parse_loop();
    NexNode *parse_match();
    NexNode *parse_match_arm();
    NexNode *parse_emit();
    NexNode *parse_return();
    NexNode *parse_autoload();

    NexNode *parse_expr();
    NexNode *parse_inline_if();
    NexNode *parse_or_expr();
    NexNode *parse_and_expr();
    NexNode *parse_comparison();
    NexNode *parse_addition();
    NexNode *parse_multiplication();
    NexNode *parse_power();
    NexNode *parse_unary();
    NexNode *parse_postfix();
    NexNode *parse_primary();

    NexType  parse_type_annotation();
    NexType  parse_type_expr();
    void     parse_fn_params(NexNode *fn_node);
};

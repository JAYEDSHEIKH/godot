#include "nex_parser.h"

NexToken dummy_eof;

NexToken &NexParser::peek(int offset) {
    int i = pos + offset;
    if (i >= tokens.size()) { dummy_eof.kind = NexTokenKind::EOF_TOKEN; return dummy_eof; }
    return tokens.write[i];
}

NexToken NexParser::consume() {
    if (pos >= tokens.size()) { NexToken t; t.kind = NexTokenKind::EOF_TOKEN; return t; }
    return tokens[pos++];
}

bool NexParser::at_end() const { return pos >= tokens.size() || tokens[pos].kind == NexTokenKind::EOF_TOKEN; }

bool NexParser::check(NexTokenKind kind) const {
    if (pos >= tokens.size()) return false;
    return tokens[pos].kind == kind;
}

bool NexParser::match_tok(NexTokenKind kind) {
    if (check(kind)) { consume(); return true; }
    return false;
}

NexToken NexParser::expect(NexTokenKind kind, const String &msg) {
    if (!check(kind)) {
        errors.push_back(vformat("Line %d: Expected %s", peek().line, msg));
        NexToken t; t.kind = NexTokenKind::ERROR; return t;
    }
    return consume();
}

NexNode *NexParser::parse(const Vector<NexToken> &p_tokens) {
    tokens = p_tokens;
    pos = 0;
    return parse_program();
}

NexNode *NexParser::parse_program() {
    NexNode *prog = new NexNode(NexNode::PROGRAM, 0);
    while (!at_end()) {
        NexNode *stmt = parse_top_level();
        if (stmt) prog->children.push_back(stmt);
    }
    return prog;
}

NexNode *NexParser::parse_top_level() {
    int ln = peek().line;
    NexNode *annotation = nullptr;
    if (check(NexTokenKind::SYM_AT)) {
        annotation = parse_annotation();
    }

    bool is_pub = false;
    if (check(NexTokenKind::KW_PUB)) { consume(); is_pub = true; }

    bool is_async = false;
    if (check(NexTokenKind::KW_ASYNC)) { consume(); is_async = true; }

    NexNode *node = nullptr;
    if (check(NexTokenKind::KW_EXTENDS)) {
        node = parse_extends();
    } else if (check(NexTokenKind::KW_USE)) {
        node = parse_use();
    } else if (check(NexTokenKind::KW_AUTOLOAD)) {
        node = parse_autoload();
    } else if (check(NexTokenKind::KW_FN)) {
        node = parse_fn(is_async, is_pub);
    } else if (check(NexTokenKind::KW_STRUCT)) {
        node = parse_struct();
    } else if (check(NexTokenKind::KW_IMPL)) {
        node = parse_impl();
    } else if (check(NexTokenKind::KW_ENUM)) {
        node = parse_enum();
    } else if (check(NexTokenKind::KW_SIGNAL)) {
        node = parse_signal();
    } else if (check(NexTokenKind::KW_CONST)) {
        node = parse_const_decl();
    } else if (check(NexTokenKind::KW_VAR)) {
        node = parse_var_decl(true);
    } else {
        node = parse_stmt();
    }

    if (annotation && node) {
        node->annotation_name = annotation->str_val;
        node->annotation_args = annotation->annotation_args;
        delete annotation;
    }
    return node;
}

NexNode *NexParser::parse_annotation() {
    int ln = peek().line;
    consume(); // @
    NexNode *ann = new NexNode(NexNode::ANNOTATION, ln);
    ann->str_val = expect(NexTokenKind::IDENT, "annotation name").value;
    if (check(NexTokenKind::IDENT)) {
        String extra = consume().value;
        ann->str_val += "_" + extra;
    }
    if (match_tok(NexTokenKind::SYM_LPAREN)) {
        while (!check(NexTokenKind::SYM_RPAREN) && !at_end()) {
            NexNode *arg = parse_expr();
            if (arg) {
                if (arg->kind == NexNode::INT_LITERAL)   ann->annotation_args.push_back(Variant(arg->int_val));
                else if (arg->kind == NexNode::FLOAT_LITERAL) ann->annotation_args.push_back(Variant(arg->float_val));
                else if (arg->kind == NexNode::STR_LITERAL)   ann->annotation_args.push_back(Variant(arg->str_val));
                delete arg;
            }
            if (!match_tok(NexTokenKind::SYM_COMMA)) break;
        }
        expect(NexTokenKind::SYM_RPAREN, "')'");
    }
    return ann;
}

NexNode *NexParser::parse_extends() {
    int ln = peek().line;
    consume(); // extends
    NexNode *node = new NexNode(NexNode::EXTENDS_STMT, ln);
    node->str_val = expect(NexTokenKind::IDENT, "class name").value;
    while (check(NexTokenKind::SYM_DOT)) {
        consume(); node->str_val += "."; node->str_val += expect(NexTokenKind::IDENT, "class part").value;
    }
    match_tok(NexTokenKind::SYM_SEMICOLON);
    return node;
}

NexNode *NexParser::parse_use() {
    int ln = peek().line;
    consume(); // use
    NexNode *node = new NexNode(NexNode::USE_STMT, ln);
    node->str_val = expect(NexTokenKind::IDENT, "module name").value;
    while (check(NexTokenKind::SYM_DOT)) {
        consume(); node->str_val += "."; node->str_val += expect(NexTokenKind::IDENT, "module part").value;
    }
    match_tok(NexTokenKind::SYM_SEMICOLON);
    return node;
}

NexNode *NexParser::parse_autoload() {
    int ln = peek().line;
    consume(); // autoload
    NexNode *node = new NexNode(NexNode::AUTOLOAD_STMT, ln);
    node->str_val = expect(NexTokenKind::IDENT, "autoload name").value;
    expect(NexTokenKind::KW_FROM, "from");
    NexNode *path = parse_primary();
    if (path) { node->str_val += "|"; node->str_val += path->str_val; delete path; }
    match_tok(NexTokenKind::SYM_SEMICOLON);
    return node;
}

NexNode *NexParser::parse_signal() {
    int ln = peek().line;
    consume(); // signal
    NexNode *node = new NexNode(NexNode::SIGNAL_DEF, ln);
    node->str_val = expect(NexTokenKind::IDENT, "signal name").value;
    if (match_tok(NexTokenKind::SYM_LPAREN)) {
        while (!check(NexTokenKind::SYM_RPAREN) && !at_end()) {
            NexNode *param = new NexNode(NexNode::FN_PARAM, ln);
            param->str_val = expect(NexTokenKind::IDENT, "param name").value;
            if (match_tok(NexTokenKind::SYM_COLON)) {
                param->type_annotation = parse_type_annotation();
            }
            node->children.push_back(param);
            if (!match_tok(NexTokenKind::SYM_COMMA)) break;
        }
        expect(NexTokenKind::SYM_RPAREN, "')'");
    }
    match_tok(NexTokenKind::SYM_SEMICOLON);
    return node;
}

NexNode *NexParser::parse_fn(bool is_async, bool is_pub) {
    int ln = peek().line;
    consume(); // fn
    NexNode *node = new NexNode(NexNode::FN_DEF, ln);
    node->is_async = is_async;
    node->is_pub   = is_pub;
    node->str_val  = expect(NexTokenKind::IDENT, "function name").value;
    if (match_tok(NexTokenKind::SYM_LPAREN)) {
        parse_fn_params(node);
        expect(NexTokenKind::SYM_RPAREN, "')'");
    }
    if (match_tok(NexTokenKind::SYM_ARROW)) {
        node->type_annotation = parse_type_annotation();
    }
    if (check(NexTokenKind::SYM_LBRACE)) {
        node->children.push_back(parse_block());
    } else {
        match_tok(NexTokenKind::SYM_SEMICOLON);
    }
    return node;
}

void NexParser::parse_fn_params(NexNode *fn_node) {
    while (!check(NexTokenKind::SYM_RPAREN) && !at_end()) {
        bool param_mutable = false;
        if (check(NexTokenKind::KW_VAR)) { consume(); param_mutable = true; }
        int pln = peek().line;
        NexNode *param = new NexNode(NexNode::FN_PARAM, pln);
        param->str_val = expect(NexTokenKind::IDENT, "parameter name").value;
        param->is_mutable = param_mutable;
        if (match_tok(NexTokenKind::SYM_COLON)) {
            param->type_annotation = parse_type_annotation();
        }
        if (match_tok(NexTokenKind::SYM_ASSIGN)) {
            param->children.push_back(parse_expr());
        }
        fn_node->children.push_back(param);
        if (!match_tok(NexTokenKind::SYM_COMMA)) break;
    }
}

NexNode *NexParser::parse_struct() {
    int ln = peek().line;
    consume(); // struct
    NexNode *node = new NexNode(NexNode::STRUCT_DEF, ln);
    node->str_val = expect(NexTokenKind::IDENT, "struct name").value;
    expect(NexTokenKind::SYM_LBRACE, "'{'");
    while (!check(NexTokenKind::SYM_RBRACE) && !at_end()) {
        int fln = peek().line;
        NexNode *field = new NexNode(NexNode::STRUCT_FIELD, fln);
        field->str_val = expect(NexTokenKind::IDENT, "field name").value;
        expect(NexTokenKind::SYM_COLON, "':'");
        field->type_annotation = parse_type_annotation();
        node->children.push_back(field);
        match_tok(NexTokenKind::SYM_COMMA);
    }
    expect(NexTokenKind::SYM_RBRACE, "'}'");
    return node;
}

NexNode *NexParser::parse_impl() {
    int ln = peek().line;
    consume(); // impl
    NexNode *node = new NexNode(NexNode::IMPL_BLOCK, ln);
    node->str_val = expect(NexTokenKind::IDENT, "struct name").value;
    expect(NexTokenKind::SYM_LBRACE, "'{'");
    while (!check(NexTokenKind::SYM_RBRACE) && !at_end()) {
        NexNode *annotation = nullptr;
        if (check(NexTokenKind::SYM_AT)) annotation = parse_annotation();
        bool async = false;
        if (check(NexTokenKind::KW_ASYNC)) { consume(); async = true; }
        if (check(NexTokenKind::KW_FN)) {
            NexNode *fn = parse_fn(async);
            if (annotation) { fn->annotation_name = annotation->str_val; delete annotation; }
            node->children.push_back(fn);
        } else {
            break;
        }
    }
    expect(NexTokenKind::SYM_RBRACE, "'}'");
    return node;
}

NexNode *NexParser::parse_enum() {
    int ln = peek().line;
    consume(); // enum
    NexNode *node = new NexNode(NexNode::ENUM_DEF, ln);
    node->str_val = expect(NexTokenKind::IDENT, "enum name").value;
    expect(NexTokenKind::SYM_LBRACE, "'{'");
    int idx = 0;
    while (!check(NexTokenKind::SYM_RBRACE) && !at_end()) {
        int vln = peek().line;
        NexNode *variant = new NexNode(NexNode::ENUM_VARIANT, vln);
        variant->str_val = expect(NexTokenKind::IDENT, "variant name").value;
        variant->int_val = idx++;
        if (match_tok(NexTokenKind::SYM_LPAREN)) {
            while (!check(NexTokenKind::SYM_RPAREN) && !at_end()) {
                NexNode *f = new NexNode(NexNode::STRUCT_FIELD, vln);
                f->str_val = expect(NexTokenKind::IDENT, "field name").value;
                expect(NexTokenKind::SYM_COLON, "':'");
                f->type_annotation = parse_type_annotation();
                variant->children.push_back(f);
                if (!match_tok(NexTokenKind::SYM_COMMA)) break;
            }
            expect(NexTokenKind::SYM_RPAREN, "')'");
        }
        node->children.push_back(variant);
        match_tok(NexTokenKind::SYM_COMMA);
    }
    expect(NexTokenKind::SYM_RBRACE, "'}'");
    return node;
}

NexNode *NexParser::parse_const_decl() {
    int ln = peek().line;
    consume(); // const
    NexNode *node = new NexNode(NexNode::CONST_DEF, ln);
    node->str_val = expect(NexTokenKind::IDENT, "constant name").value;
    if (match_tok(NexTokenKind::SYM_COLON)) {
        node->type_annotation = parse_type_annotation();
    }
    expect(NexTokenKind::SYM_WALRUS, "':='");
    node->children.push_back(parse_expr());
    match_tok(NexTokenKind::SYM_SEMICOLON);
    return node;
}

NexNode *NexParser::parse_block() {
    int ln = peek().line;
    expect(NexTokenKind::SYM_LBRACE, "'{'");
    NexNode *block = new NexNode(NexNode::BLOCK, ln);
    while (!check(NexTokenKind::SYM_RBRACE) && !at_end()) {
        NexNode *stmt = parse_stmt();
        if (stmt) block->children.push_back(stmt);
    }
    expect(NexTokenKind::SYM_RBRACE, "'}'");
    return block;
}

NexNode *NexParser::parse_stmt() {
    int ln = peek().line;
    NexNode *annotation = nullptr;
    if (check(NexTokenKind::SYM_AT)) annotation = parse_annotation();

    bool is_async = false;
    if (check(NexTokenKind::KW_ASYNC)) { consume(); is_async = true; }

    NexNode *stmt = nullptr;

    if (check(NexTokenKind::KW_VAR)) {
        stmt = parse_var_decl(true);
    } else if (check(NexTokenKind::KW_CONST)) {
        stmt = parse_const_decl();
    } else if (check(NexTokenKind::KW_IF)) {
        stmt = parse_if();
    } else if (check(NexTokenKind::KW_FOR)) {
        stmt = parse_for();
    } else if (check(NexTokenKind::KW_WHILE)) {
        stmt = parse_while();
    } else if (check(NexTokenKind::KW_LOOP)) {
        stmt = parse_loop();
    } else if (check(NexTokenKind::KW_MATCH)) {
        stmt = parse_match();
    } else if (check(NexTokenKind::KW_RET)) {
        stmt = parse_return();
    } else if (check(NexTokenKind::KW_EMIT)) {
        stmt = parse_emit();
    } else if (check(NexTokenKind::KW_BREAK)) {
        consume();
        stmt = new NexNode(NexNode::BREAK_STMT, ln);
        match_tok(NexTokenKind::SYM_SEMICOLON);
    } else if (check(NexTokenKind::KW_CONTINUE)) {
        consume();
        stmt = new NexNode(NexNode::CONTINUE_STMT, ln);
        match_tok(NexTokenKind::SYM_SEMICOLON);
    } else if (check(NexTokenKind::KW_FN)) {
        stmt = parse_fn(is_async);
    } else if (check(NexTokenKind::KW_STRUCT)) {
        stmt = parse_struct();
    } else if (check(NexTokenKind::KW_SIGNAL)) {
        stmt = parse_signal();
    } else if (check(NexTokenKind::KW_ENUM)) {
        stmt = parse_enum();
    } else {
        NexNode *expr = parse_expr();
        if (!expr) { match_tok(NexTokenKind::SYM_SEMICOLON); return nullptr; }
        // Check for := (immutable decl), var was handled above
        if (check(NexTokenKind::SYM_WALRUS)) {
            consume();
            stmt = new NexNode(NexNode::VAR_DECL, ln);
            stmt->is_mutable = false;
            if (expr->kind == NexNode::IDENTIFIER) {
                stmt->str_val = expr->str_val;
            }
            if (check(NexTokenKind::SYM_COLON) && expr->kind == NexNode::IDENTIFIER) {
                // actually handle x: type := val
            }
            stmt->children.push_back(parse_expr());
            delete expr;
            match_tok(NexTokenKind::SYM_SEMICOLON);
        } else if (check(NexTokenKind::SYM_ASSIGN)) {
            consume();
            stmt = new NexNode(NexNode::ASSIGN_STMT, ln);
            stmt->children.push_back(expr);
            stmt->children.push_back(parse_expr());
            match_tok(NexTokenKind::SYM_SEMICOLON);
        } else if (check(NexTokenKind::OP_PLUS_EQ) || check(NexTokenKind::OP_MINUS_EQ) ||
                   check(NexTokenKind::OP_STAR_EQ)  || check(NexTokenKind::OP_SLASH_EQ) ||
                   check(NexTokenKind::OP_PERCENT_EQ)) {
            stmt = new NexNode(NexNode::ASSIGN_OP_STMT, ln);
            stmt->op = consume().value;
            stmt->children.push_back(expr);
            stmt->children.push_back(parse_expr());
            match_tok(NexTokenKind::SYM_SEMICOLON);
        } else {
            stmt = expr;
            match_tok(NexTokenKind::SYM_SEMICOLON);
        }
    }

    if (annotation && stmt) {
        stmt->annotation_name = annotation->str_val;
        stmt->annotation_args = annotation->annotation_args;
        delete annotation;
    }
    return stmt;
}

NexNode *NexParser::parse_var_decl(bool force_mutable) {
    int ln = peek().line;
    if (force_mutable) consume(); // var
    NexNode *node = new NexNode(NexNode::VAR_DECL, ln);
    node->is_mutable = force_mutable;
    node->str_val = expect(NexTokenKind::IDENT, "variable name").value;
    if (match_tok(NexTokenKind::SYM_COLON)) {
        node->type_annotation = parse_type_annotation();
    }
    if (match_tok(NexTokenKind::SYM_WALRUS) || match_tok(NexTokenKind::SYM_ASSIGN)) {
        node->children.push_back(parse_expr());
    }
    match_tok(NexTokenKind::SYM_SEMICOLON);
    return node;
}

NexNode *NexParser::parse_if() {
    int ln = peek().line;
    consume(); // if
    NexNode *node = new NexNode(NexNode::IF_STMT, ln);
    node->children.push_back(parse_expr());
    node->children.push_back(parse_block());
    while (check(NexTokenKind::KW_ELIF)) {
        int eln = peek().line;
        consume();
        NexNode *elif_cond = parse_expr();
        NexNode *elif_body = parse_block();
        NexNode *elif_node = new NexNode(NexNode::IF_STMT, eln);
        elif_node->children.push_back(elif_cond);
        elif_node->children.push_back(elif_body);
        node->children.push_back(elif_node);
    }
    if (match_tok(NexTokenKind::KW_ELSE)) {
        node->children.push_back(parse_block());
    }
    return node;
}

NexNode *NexParser::parse_for() {
    int ln = peek().line;
    consume(); // for
    NexNode *node = nullptr;
    String name1 = expect(NexTokenKind::IDENT, "loop variable").value;
    if (match_tok(NexTokenKind::SYM_COMMA)) {
        String name2 = expect(NexTokenKind::IDENT, "item variable").value;
        expect(NexTokenKind::KW_IN, "'in'");
        node = new NexNode(NexNode::FOR_EACH_STMT, ln);
        node->str_val = name1 + "," + name2;
        node->children.push_back(parse_expr());
        node->children.push_back(parse_block());
    } else {
        expect(NexTokenKind::KW_IN, "'in'");
        NexNode *range_or_expr = parse_expr();
        if (range_or_expr && range_or_expr->kind == NexNode::RANGE_EXPR) {
            node = new NexNode(NexNode::FOR_RANGE_STMT, ln);
            node->str_val = name1;
            node->children.push_back(range_or_expr);
            if (check(NexTokenKind::KW_BY)) {
                consume();
                node->children.push_back(parse_expr());
            }
            node->children.push_back(parse_block());
        } else {
            node = new NexNode(NexNode::FOR_EACH_STMT, ln);
            node->str_val = name1;
            node->children.push_back(range_or_expr);
            node->children.push_back(parse_block());
        }
    }
    return node;
}

NexNode *NexParser::parse_while() {
    int ln = peek().line;
    consume(); // while
    NexNode *node = new NexNode(NexNode::WHILE_STMT, ln);
    node->children.push_back(parse_expr());
    node->children.push_back(parse_block());
    return node;
}

NexNode *NexParser::parse_loop() {
    int ln = peek().line;
    consume(); // loop
    NexNode *node = new NexNode(NexNode::LOOP_STMT, ln);
    node->children.push_back(parse_block());
    return node;
}

NexNode *NexParser::parse_match() {
    int ln = peek().line;
    consume(); // match
    NexNode *node = new NexNode(NexNode::MATCH_STMT, ln);
    node->children.push_back(parse_expr());
    expect(NexTokenKind::SYM_LBRACE, "'{'");
    while (!check(NexTokenKind::SYM_RBRACE) && !at_end()) {
        node->children.push_back(parse_match_arm());
        match_tok(NexTokenKind::SYM_COMMA);
    }
    expect(NexTokenKind::SYM_RBRACE, "'}'");
    return node;
}

NexNode *NexParser::parse_match_arm() {
    int ln = peek().line;
    NexNode *arm = new NexNode(NexNode::MATCH_ARM, ln);
    // pattern (could be _, literal, range, enum variant)
    arm->children.push_back(parse_expr());
    expect(NexTokenKind::SYM_ARROW, "'->'");
    if (check(NexTokenKind::SYM_LBRACE)) {
        arm->children.push_back(parse_block());
    } else {
        NexNode *body = new NexNode(NexNode::BLOCK, ln);
        NexNode *expr = parse_expr();
        match_tok(NexTokenKind::SYM_SEMICOLON);
        body->children.push_back(expr);
        arm->children.push_back(body);
    }
    return arm;
}

NexNode *NexParser::parse_emit() {
    int ln = peek().line;
    consume(); // emit
    NexNode *node = new NexNode(NexNode::EMIT_STMT, ln);
    node->str_val = expect(NexTokenKind::IDENT, "signal name").value;
    if (match_tok(NexTokenKind::SYM_LPAREN)) {
        while (!check(NexTokenKind::SYM_RPAREN) && !at_end()) {
            node->children.push_back(parse_expr());
            if (!match_tok(NexTokenKind::SYM_COMMA)) break;
        }
        expect(NexTokenKind::SYM_RPAREN, "')'");
    }
    match_tok(NexTokenKind::SYM_SEMICOLON);
    return node;
}

NexNode *NexParser::parse_return() {
    int ln = peek().line;
    consume(); // ret
    NexNode *node = new NexNode(NexNode::RETURN_STMT, ln);
    if (!check(NexTokenKind::SYM_SEMICOLON) && !check(NexTokenKind::SYM_RBRACE) && !at_end()) {
        node->children.push_back(parse_expr());
    }
    match_tok(NexTokenKind::SYM_SEMICOLON);
    return node;
}

NexType NexParser::parse_type_annotation() {
    return parse_type_expr();
}

NexType NexParser::parse_type_expr() {
    if (check(NexTokenKind::SYM_LBRACKET)) {
        consume();
        NexType elem = parse_type_expr();
        expect(NexTokenKind::SYM_RBRACKET, "']'");
        return NexType::make_array(elem);
    }
    if (check(NexTokenKind::SYM_LPAREN)) {
        consume();
        NexType t;
        t.base = NexBaseType::TUPLE;
        while (!check(NexTokenKind::SYM_RPAREN) && !at_end()) {
            t.tuple_types.push_back(parse_type_expr());
            if (!match_tok(NexTokenKind::SYM_COMMA)) break;
        }
        expect(NexTokenKind::SYM_RPAREN, "')'");
        return t;
    }

    String name = "";
    if (check(NexTokenKind::IDENT)) name = consume().value;
    else if (check(NexTokenKind::KW_STR) || peek().value == "str") { consume(); return NexType::make_str(); }

    if (name == "int")      return NexType::make_int();
    if (name == "int8")   { NexType t; t.base = NexBaseType::INT8;    return t; }
    if (name == "int16")  { NexType t; t.base = NexBaseType::INT16;   return t; }
    if (name == "int32")  { NexType t; t.base = NexBaseType::INT32;   return t; }
    if (name == "int64")    return NexType::make_int();
    if (name == "float")    return NexType::make_float();
    if (name == "float32"){ NexType t; t.base = NexBaseType::FLOAT32; return t; }
    if (name == "float64")  return NexType::make_float();
    if (name == "bool")     return NexType::make_bool();
    if (name == "str")      return NexType::make_str();
    if (name == "char")     return NexType::make_char();
    if (name == "void")     return NexType::make_void();
    if (name == "Vector2")  return NexType::make_vec2();
    if (name == "Vector3")  return NexType::make_vec3();
    if (name == "Color")    return NexType::make_color();
    if (name == "Option") {
        expect(NexTokenKind::SYM_LT_ANGLE, "'<'");
        NexType inner = parse_type_expr();
        expect(NexTokenKind::SYM_GT_ANGLE, "'>'");
        return NexType::make_option(inner);
    }
    if (name == "Result") {
        expect(NexTokenKind::SYM_LT_ANGLE, "'<'");
        NexType ok = parse_type_expr();
        expect(NexTokenKind::SYM_COMMA, "','");
        NexType err = parse_type_expr();
        expect(NexTokenKind::SYM_GT_ANGLE, "'>'");
        return NexType::make_result(ok, err);
    }
    if (name == "fn") {
        NexType t;
        t.base = NexBaseType::FN;
        expect(NexTokenKind::SYM_LPAREN, "'('");
        while (!check(NexTokenKind::SYM_RPAREN) && !at_end()) {
            t.fn_params.push_back(parse_type_expr());
            if (!match_tok(NexTokenKind::SYM_COMMA)) break;
        }
        expect(NexTokenKind::SYM_RPAREN, "')'");
        if (match_tok(NexTokenKind::SYM_ARROW)) {
            t.fn_ret = new NexType(parse_type_expr());
        }
        return t;
    }
    if (name == "map") {
        expect(NexTokenKind::SYM_LBRACKET, "'['");
        NexType key = parse_type_expr();
        expect(NexTokenKind::SYM_COMMA, "','");
        NexType val = parse_type_expr();
        expect(NexTokenKind::SYM_RBRACKET, "']'");
        NexType t;
        t.base = NexBaseType::MAP;
        t.key_type   = new NexType(key);
        t.value_type = new NexType(val);
        return t;
    }
    if (!name.is_empty()) {
        // Assume it's a struct/enum/object type
        NexType t;
        t.base = NexBaseType::OBJECT;
        t.name = StringName(name);
        return t;
    }
    return NexType::make_variant();
}

NexNode *NexParser::parse_expr() {
    return parse_inline_if();
}

NexNode *NexParser::parse_inline_if() {
    NexNode *expr = parse_or_expr();
    if (!expr) return nullptr;
    if (check(NexTokenKind::KW_IF)) {
        int ln = peek().line;
        consume();
        NexNode *cond = parse_or_expr();
        NexNode *else_expr = nullptr;
        if (check(NexTokenKind::KW_ELSE)) { consume(); else_expr = parse_or_expr(); }
        NexNode *node = new NexNode(NexNode::INLINE_IF_EXPR, ln);
        node->children.push_back(cond);
        node->children.push_back(expr);
        if (else_expr) node->children.push_back(else_expr);
        return node;
    }
    return expr;
}

NexNode *NexParser::parse_or_expr() {
    NexNode *left = parse_and_expr();
    while (check(NexTokenKind::KW_OR)) {
        int ln = peek().line; consume();
        NexNode *right = parse_and_expr();
        NexNode *node = new NexNode(NexNode::BINARY_EXPR, ln);
        node->op = "or";
        node->children.push_back(left);
        node->children.push_back(right);
        left = node;
    }
    return left;
}

NexNode *NexParser::parse_and_expr() {
    NexNode *left = parse_comparison();
    while (check(NexTokenKind::KW_AND)) {
        int ln = peek().line; consume();
        NexNode *right = parse_comparison();
        NexNode *node = new NexNode(NexNode::BINARY_EXPR, ln);
        node->op = "and";
        node->children.push_back(left);
        node->children.push_back(right);
        left = node;
    }
    return left;
}

NexNode *NexParser::parse_comparison() {
    NexNode *left = parse_addition();
    while (check(NexTokenKind::OP_EQ) || check(NexTokenKind::OP_NEQ) ||
           check(NexTokenKind::OP_LT) || check(NexTokenKind::OP_GT)  ||
           check(NexTokenKind::OP_LTE) || check(NexTokenKind::OP_GTE)) {
        int ln = peek().line;
        String op = consume().value;
        NexNode *right = parse_addition();
        NexNode *node = new NexNode(NexNode::BINARY_EXPR, ln);
        node->op = op;
        node->children.push_back(left);
        node->children.push_back(right);
        left = node;
    }
    return left;
}

NexNode *NexParser::parse_addition() {
    NexNode *left = parse_multiplication();
    while (check(NexTokenKind::OP_PLUS) || check(NexTokenKind::OP_MINUS)) {
        int ln = peek().line;
        String op = consume().value;
        NexNode *right = parse_multiplication();
        NexNode *node = new NexNode(NexNode::BINARY_EXPR, ln);
        node->op = op;
        node->children.push_back(left);
        node->children.push_back(right);
        left = node;
    }
    return left;
}

NexNode *NexParser::parse_multiplication() {
    NexNode *left = parse_power();
    while (check(NexTokenKind::OP_STAR) || check(NexTokenKind::OP_SLASH) || check(NexTokenKind::OP_PERCENT)) {
        int ln = peek().line;
        String op = consume().value;
        NexNode *right = parse_power();
        NexNode *node = new NexNode(NexNode::BINARY_EXPR, ln);
        node->op = op;
        node->children.push_back(left);
        node->children.push_back(right);
        left = node;
    }
    return left;
}

NexNode *NexParser::parse_power() {
    NexNode *left = parse_unary();
    if (check(NexTokenKind::OP_STARSTAR)) {
        int ln = peek().line; consume();
        NexNode *right = parse_power();
        NexNode *node = new NexNode(NexNode::BINARY_EXPR, ln);
        node->op = "**";
        node->children.push_back(left);
        node->children.push_back(right);
        return node;
    }
    return left;
}

NexNode *NexParser::parse_unary() {
    if (check(NexTokenKind::KW_NOT) || check(NexTokenKind::OP_MINUS)) {
        int ln = peek().line;
        String op = consume().value;
        NexNode *operand = parse_unary();
        NexNode *node = new NexNode(NexNode::UNARY_EXPR, ln);
        node->op = op;
        node->children.push_back(operand);
        return node;
    }
    return parse_postfix();
}

NexNode *NexParser::parse_postfix() {
    NexNode *node = parse_primary();
    if (!node) return nullptr;
    while (true) {
        if (check(NexTokenKind::SYM_DOT)) {
            int ln = peek().line; consume();
            String field = expect(NexTokenKind::IDENT, "field name").value;
            if (check(NexTokenKind::SYM_LPAREN)) {
                consume();
                NexNode *call = new NexNode(NexNode::METHOD_CALL_EXPR, ln);
                call->str_val = field;
                call->children.push_back(node);
                while (!check(NexTokenKind::SYM_RPAREN) && !at_end()) {
                    call->children.push_back(parse_expr());
                    if (!match_tok(NexTokenKind::SYM_COMMA)) break;
                }
                expect(NexTokenKind::SYM_RPAREN, "')'");
                node = call;
            } else {
                NexNode *fa = new NexNode(NexNode::FIELD_EXPR, ln);
                fa->str_val = field;
                fa->children.push_back(node);
                node = fa;
            }
        } else if (check(NexTokenKind::SYM_LBRACKET)) {
            int ln = peek().line; consume();
            NexNode *idx = new NexNode(NexNode::INDEX_EXPR, ln);
            idx->children.push_back(node);
            idx->children.push_back(parse_expr());
            expect(NexTokenKind::SYM_RBRACKET, "']'");
            node = idx;
        } else if (check(NexTokenKind::SYM_QUESTION)) {
            int ln = peek().line; consume();
            NexNode *q = new NexNode(NexNode::QUESTION_EXPR, ln);
            q->children.push_back(node);
            node = q;
        } else if (check(NexTokenKind::KW_AS)) {
            int ln = peek().line; consume();
            NexNode *cast = new NexNode(NexNode::AS_CAST_EXPR, ln);
            cast->children.push_back(node);
            cast->type_annotation = parse_type_annotation();
            node = cast;
        } else if (check(NexTokenKind::KW_IS)) {
            int ln = peek().line; consume();
            NexNode *is_chk = new NexNode(NexNode::IS_CHECK_EXPR, ln);
            is_chk->children.push_back(node);
            is_chk->type_annotation = parse_type_annotation();
            node = is_chk;
        } else if (check(NexTokenKind::SYM_DOTDOT) || check(NexTokenKind::SYM_DOTDOTEQ)) {
            int ln = peek().line;
            bool inclusive = peek().kind == NexTokenKind::SYM_DOTDOTEQ;
            consume();
            NexNode *rng = new NexNode(NexNode::RANGE_EXPR, ln);
            rng->bool_val = inclusive;
            rng->children.push_back(node);
            rng->children.push_back(parse_addition());
            node = rng;
        } else {
            break;
        }
    }
    return node;
}

NexNode *NexParser::parse_primary() {
    int ln = peek().line;

    if (check(NexTokenKind::LIT_INT)) {
        NexToken t = consume();
        NexNode *n = new NexNode(NexNode::INT_LITERAL, ln);
        n->int_val = t.value.to_int();
        return n;
    }
    if (check(NexTokenKind::LIT_FLOAT)) {
        NexToken t = consume();
        NexNode *n = new NexNode(NexNode::FLOAT_LITERAL, ln);
        n->float_val = t.value.to_float();
        return n;
    }
    if (check(NexTokenKind::LIT_STRING)) {
        NexToken t = consume();
        NexNode *n = new NexNode(NexNode::STR_LITERAL, ln);
        n->str_val = t.value;
        return n;
    }
    if (check(NexTokenKind::LIT_CHAR)) {
        NexToken t = consume();
        NexNode *n = new NexNode(NexNode::CHAR_LITERAL, ln);
        n->str_val = t.value;
        return n;
    }
    if (check(NexTokenKind::KW_TRUE)) {
        consume();
        NexNode *n = new NexNode(NexNode::BOOL_LITERAL, ln);
        n->bool_val = true;
        return n;
    }
    if (check(NexTokenKind::KW_FALSE)) {
        consume();
        NexNode *n = new NexNode(NexNode::BOOL_LITERAL, ln);
        n->bool_val = false;
        return n;
    }
    if (check(NexTokenKind::KW_NONE)) {
        consume();
        return new NexNode(NexNode::OPTION_NONE_EXPR, ln);
    }
    if (check(NexTokenKind::KW_SOME)) {
        consume();
        expect(NexTokenKind::SYM_LPAREN, "'('");
        NexNode *n = new NexNode(NexNode::OPTION_SOME_EXPR, ln);
        n->children.push_back(parse_expr());
        expect(NexTokenKind::SYM_RPAREN, "')'");
        return n;
    }
    if (check(NexTokenKind::KW_OK)) {
        consume();
        expect(NexTokenKind::SYM_LPAREN, "'('");
        NexNode *n = new NexNode(NexNode::RESULT_OK_EXPR, ln);
        n->children.push_back(parse_expr());
        expect(NexTokenKind::SYM_RPAREN, "')'");
        return n;
    }
    if (check(NexTokenKind::KW_ERR)) {
        consume();
        expect(NexTokenKind::SYM_LPAREN, "'('");
        NexNode *n = new NexNode(NexNode::RESULT_ERR_EXPR, ln);
        n->children.push_back(parse_expr());
        expect(NexTokenKind::SYM_RPAREN, "')'");
        return n;
    }
    if (check(NexTokenKind::SYM_DOLLAR)) {
        consume();
        NexNode *n = new NexNode(NexNode::DOLLAR_PATH_EXPR, ln);
        n->str_val = expect(NexTokenKind::IDENT, "node path").value;
        while (check(NexTokenKind::SYM_SLASH) || (check(NexTokenKind::SYM_DOT) && !check(NexTokenKind::SYM_DOTDOT))) {
            consume();
            n->str_val += "/";
            n->str_val += expect(NexTokenKind::IDENT, "node name").value;
        }
        return n;
    }
    if (check(NexTokenKind::SYM_LPAREN)) {
        consume();
        NexNode *inner = parse_expr();
        if (check(NexTokenKind::SYM_COMMA)) {
            NexNode *tuple = new NexNode(NexNode::TUPLE_EXPR, ln);
            tuple->children.push_back(inner);
            while (match_tok(NexTokenKind::SYM_COMMA)) {
                tuple->children.push_back(parse_expr());
            }
            expect(NexTokenKind::SYM_RPAREN, "')'");
            return tuple;
        }
        expect(NexTokenKind::SYM_RPAREN, "')'");
        return inner;
    }
    if (check(NexTokenKind::SYM_LBRACKET)) {
        consume();
        NexNode *arr = new NexNode(NexNode::ARRAY_LITERAL, ln);
        while (!check(NexTokenKind::SYM_RBRACKET) && !at_end()) {
            arr->children.push_back(parse_expr());
            if (!match_tok(NexTokenKind::SYM_COMMA)) break;
        }
        expect(NexTokenKind::SYM_RBRACKET, "']'");
        return arr;
    }
    if (check(NexTokenKind::KW_FN)) {
        consume();
        NexNode *closure = new NexNode(NexNode::CLOSURE_EXPR, ln);
        if (match_tok(NexTokenKind::SYM_LPAREN)) {
            parse_fn_params(closure);
            expect(NexTokenKind::SYM_RPAREN, "')'");
        }
        if (match_tok(NexTokenKind::SYM_ARROW)) {
            closure->type_annotation = parse_type_annotation();
        }
        closure->children.push_back(parse_block());
        return closure;
    }
    if (check(NexTokenKind::KW_AWAIT)) {
        consume();
        NexNode *aw = new NexNode(NexNode::AWAIT_EXPR, ln);
        aw->children.push_back(parse_expr());
        return aw;
    }
    if (check(NexTokenKind::IDENT)) {
        String name = consume().value;
        if (check(NexTokenKind::SYM_LPAREN)) {
            consume();
            NexNode *call = new NexNode(NexNode::CALL_EXPR, ln);
            call->str_val = name;
            while (!check(NexTokenKind::SYM_RPAREN) && !at_end()) {
                call->children.push_back(parse_expr());
                if (!match_tok(NexTokenKind::SYM_COMMA)) break;
            }
            expect(NexTokenKind::SYM_RPAREN, "')'");
            return call;
        }
        if (check(NexTokenKind::SYM_LBRACE)) {
            // Struct literal
            consume();
            NexNode *sl = new NexNode(NexNode::STRUCT_LITERAL, ln);
            sl->str_val = name;
            while (!check(NexTokenKind::SYM_RBRACE) && !at_end()) {
                String fname = expect(NexTokenKind::IDENT, "field name").value;
                expect(NexTokenKind::SYM_COLON, "':'");
                NexNode *fval = parse_expr();
                NexNode *fnode = new NexNode(NexNode::FIELD_EXPR, ln);
                fnode->str_val = fname;
                fnode->children.push_back(fval);
                sl->children.push_back(fnode);
                match_tok(NexTokenKind::SYM_COMMA);
            }
            expect(NexTokenKind::SYM_RBRACE, "'}'");
            return sl;
        }
        NexNode *ident = new NexNode(NexNode::IDENTIFIER, ln);
        ident->str_val = name;
        return ident;
    }
    return nullptr;
}

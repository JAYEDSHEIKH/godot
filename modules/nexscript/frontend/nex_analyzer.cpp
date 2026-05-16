#include "nex_analyzer.h"

void NexAnalyzer::push_scope() {
    Scope *s = new Scope();
    s->parent = current_scope;
    current_scope = s;
}

void NexAnalyzer::pop_scope() {
    if (current_scope) {
        Scope *old = current_scope;
        current_scope = old->parent;
        delete old;
    }
}

void NexAnalyzer::declare(const StringName &name, const NexType &type, bool mutable_flag) {
    if (current_scope) {
        current_scope->bindings[name] = type;
        current_scope->mutability[name] = mutable_flag;
    }
}

const NexType *NexAnalyzer::lookup(const StringName &name) const {
    const Scope *s = current_scope;
    while (s) {
        if (s->bindings.has(name)) return &s->bindings[name];
        s = s->parent;
    }
    if (globals.has(name)) return &globals[name];
    return nullptr;
}

bool NexAnalyzer::is_mutable_binding(const StringName &name) const {
    const Scope *s = current_scope;
    while (s) {
        if (s->mutability.has(name)) return s->mutability[name];
        s = s->parent;
    }
    return false;
}

NexType NexAnalyzer::godot_type_for_name(const StringName &name) {
    if (name == "Node" || name == "Node2D" || name == "Node3D" || name == "CharacterBody2D" ||
        name == "RigidBody2D" || name == "Area2D" || name == "Sprite2D" || name == "Label" ||
        name == "Button" || name == "Control" || name == "AnimationPlayer" || name == "Camera2D") {
        return NexType::make_object(name);
    }
    return NexType::make_variant();
}

void NexAnalyzer::resolve_struct_offsets(StructInfo &info) {
    int offset = 0;
    for (FieldInfo &f : info.fields) {
        f.byte_offset = offset;
        offset += f.type.size_bytes();
    }
    info.total_size = offset;
}

void NexAnalyzer::collect_top_level(NexNode *root) {
    for (NexNode *child : root->children) {
        if (!child) continue;
        if (child->kind == NexNode::STRUCT_DEF) {
            StructInfo si;
            si.name = StringName(child->str_val);
            for (NexNode *f : child->children) {
                if (f->kind == NexNode::STRUCT_FIELD) {
                    FieldInfo fi;
                    fi.name = StringName(f->str_val);
                    fi.type = f->type_annotation;
                    si.field_index[fi.name] = si.fields.size();
                    si.fields.push_back(fi);
                }
            }
            resolve_struct_offsets(si);
            structs[si.name] = si;
        } else if (child->kind == NexNode::FN_DEF) {
            FnInfo fi;
            fi.name = StringName(child->str_val);
            fi.return_type = child->type_annotation;
            fi.is_async = child->is_async;
            for (NexNode *p : child->children) {
                if (p->kind == NexNode::FN_PARAM) fi.param_types.push_back(p->type_annotation);
                else break;
            }
            functions[fi.name] = fi;
        } else if (child->kind == NexNode::SIGNAL_DEF) {
            SignalInfo si;
            si.name = StringName(child->str_val);
            for (NexNode *p : child->children) {
                si.param_types.push_back(p->type_annotation);
            }
            signals[si.name] = si;
        } else if (child->kind == NexNode::ENUM_DEF) {
            NexType et;
            et.base = NexBaseType::ENUM;
            et.name = StringName(child->str_val);
            enums[et.name] = et;
        } else if (child->kind == NexNode::CONST_DEF) {
            if (!child->children.is_empty()) {
                NexNode *val = child->children[0];
                NexType ct;
                if (val->kind == NexNode::INT_LITERAL)   ct = NexType::make_int();
                else if (val->kind == NexNode::FLOAT_LITERAL) ct = NexType::make_float();
                else if (val->kind == NexNode::STR_LITERAL)   ct = NexType::make_str();
                else if (val->kind == NexNode::BOOL_LITERAL)  ct = NexType::make_bool();
                globals[StringName(child->str_val)] = ct;
            }
        }
    }
}

bool NexAnalyzer::analyze(NexNode *root) {
    push_scope();
    // Register Godot built-ins
    globals[StringName("print")]   = NexType::make_void();
    globals[StringName("Input")]   = NexType::make_object(StringName("Input"));
    globals[StringName("Vector2")] = NexType::make_vec2();
    globals[StringName("Vector3")] = NexType::make_vec3();
    globals[StringName("Color")]   = NexType::make_color();
    globals[StringName("math")]    = NexType::make_object(StringName("Math"));

    collect_top_level(root);
    analyze_node(root);
    pop_scope();
    return errors.is_empty();
}

void NexAnalyzer::analyze_node(NexNode *node) {
    if (!node) return;

    switch (node->kind) {
        case NexNode::PROGRAM:
            for (NexNode *child : node->children) analyze_node(child);
            break;

        case NexNode::EXTENDS_STMT:
            base_class = StringName(node->str_val);
            break;

        case NexNode::FN_DEF: {
            push_scope();
            current_fn_return = node->type_annotation;
            // Register params
            for (NexNode *p : node->children) {
                if (p->kind == NexNode::FN_PARAM) {
                    declare(StringName(p->str_val), p->type_annotation, p->is_mutable);
                } else {
                    analyze_node(p);
                    break;
                }
            }
            // Analyze body (last child is block for non-forward decls)
            for (NexNode *p : node->children) {
                if (p->kind == NexNode::BLOCK) analyze_node(p);
            }
            pop_scope();
        } break;

        case NexNode::BLOCK:
            push_scope();
            for (NexNode *child : node->children) analyze_node(child);
            pop_scope();
            break;

        case NexNode::VAR_DECL: {
            NexType decl_type = node->type_annotation;
            if (!node->children.is_empty()) {
                NexType init_type = analyze_expr(node->children[0]);
                if (decl_type.base == NexBaseType::UNKNOWN) decl_type = init_type;
                else check_type_match(decl_type, init_type, node->line);
            }
            declare(StringName(node->str_val), decl_type, node->is_mutable);
            node->resolved_type = decl_type;
        } break;

        case NexNode::CONST_DEF: {
            NexType ct;
            if (!node->children.is_empty()) ct = analyze_expr(node->children[0]);
            globals[StringName(node->str_val)] = ct;
            node->resolved_type = ct;
        } break;

        case NexNode::ASSIGN_STMT: {
            check_immutability(node);
            if (node->children.size() >= 2) {
                NexType rhs = analyze_expr(node->children[1]);
                node->resolved_type = rhs;
            }
        } break;

        case NexNode::ASSIGN_OP_STMT: {
            check_immutability(node);
            if (node->children.size() >= 2) {
                analyze_expr(node->children[0]);
                analyze_expr(node->children[1]);
            }
        } break;

        case NexNode::RETURN_STMT:
            if (!node->children.is_empty()) {
                NexType rt = analyze_expr(node->children[0]);
                check_type_match(current_fn_return, rt, node->line);
            }
            break;

        case NexNode::IF_STMT:
            if (!node->children.is_empty()) analyze_expr(node->children[0]);
            for (int i = 1; i < node->children.size(); i++) analyze_node(node->children[i]);
            break;

        case NexNode::FOR_RANGE_STMT: {
            push_scope();
            declare(StringName(node->str_val), NexType::make_int(), true);
            for (NexNode *c : node->children) analyze_node(c);
            pop_scope();
        } break;

        case NexNode::FOR_EACH_STMT: {
            push_scope();
            declare(StringName(node->str_val), NexType::make_variant(), true);
            for (NexNode *c : node->children) analyze_node(c);
            pop_scope();
        } break;

        case NexNode::WHILE_STMT:
        case NexNode::LOOP_STMT:
            for (NexNode *c : node->children) analyze_node(c);
            break;

        case NexNode::MATCH_STMT:
            if (!node->children.is_empty()) analyze_expr(node->children[0]);
            for (int i = 1; i < node->children.size(); i++) analyze_node(node->children[i]);
            break;

        case NexNode::MATCH_ARM:
            if (!node->children.is_empty()) analyze_expr(node->children[0]);
            if (node->children.size() > 1) analyze_node(node->children[1]);
            break;

        case NexNode::EMIT_STMT:
            for (NexNode *c : node->children) analyze_expr(c);
            break;

        case NexNode::IMPL_BLOCK:
            for (NexNode *c : node->children) {
                if (c->kind == NexNode::FN_DEF) {
                    push_scope();
                    // Register self
                    declare(StringName("self"), NexType::make_struct(StringName(node->str_val)), false);
                    current_fn_return = c->type_annotation;
                    for (NexNode *p : c->children) {
                        if (p->kind == NexNode::FN_PARAM) {
                            declare(StringName(p->str_val), p->type_annotation, p->is_mutable);
                        } else {
                            analyze_node(p);
                            break;
                        }
                    }
                    for (NexNode *p : c->children) {
                        if (p->kind == NexNode::BLOCK) analyze_node(p);
                    }
                    pop_scope();
                }
            }
            break;

        default:
            analyze_expr(node);
            break;
    }
}

void NexAnalyzer::check_immutability(NexNode *assign_node) {
    if (assign_node->children.is_empty()) return;
    NexNode *lhs = assign_node->children[0];
    if (lhs->kind == NexNode::IDENTIFIER) {
        StringName name = StringName(lhs->str_val);
        if (!is_mutable_binding(name)) {
            errors.push_back(vformat(
                "Line %d: Cannot assign to '%s' — it is immutable. Use 'var %s := ...' to declare it mutable.",
                assign_node->line, String(name), String(name)
            ));
        }
    }
}

void NexAnalyzer::check_type_match(const NexType &expected, const NexType &got, int line) {
    if (expected.base == NexBaseType::UNKNOWN || got.base == NexBaseType::UNKNOWN) return;
    if (expected.base == NexBaseType::VARIANT || got.base == NexBaseType::VARIANT) return;
    if (!(expected == got)) {
        // Allow int/float coercion in some cases
        if (expected.is_numeric() && got.is_numeric()) return;
        errors.push_back(vformat(
            "Line %d: Type mismatch — expected '%s', got '%s'",
            line, expected.to_string(), got.to_string()
        ));
    }
}

NexType NexAnalyzer::analyze_expr(NexNode *node) {
    if (!node) return NexType::make_variant();

    switch (node->kind) {
        case NexNode::INT_LITERAL:   node->resolved_type = NexType::make_int();   return node->resolved_type;
        case NexNode::FLOAT_LITERAL: node->resolved_type = NexType::make_float(); return node->resolved_type;
        case NexNode::STR_LITERAL:   node->resolved_type = NexType::make_str();   return node->resolved_type;
        case NexNode::BOOL_LITERAL:  node->resolved_type = NexType::make_bool();  return node->resolved_type;
        case NexNode::CHAR_LITERAL:  node->resolved_type = NexType::make_char();  return node->resolved_type;
        case NexNode::OPTION_NONE_EXPR: {
            NexType t = NexType::make_option(NexType::make_variant());
            node->resolved_type = t;
            return t;
        }
        case NexNode::OPTION_SOME_EXPR: {
            NexType inner = node->children.is_empty() ? NexType::make_variant() : analyze_expr(node->children[0]);
            NexType t = NexType::make_option(inner);
            node->resolved_type = t;
            return t;
        }
        case NexNode::RESULT_OK_EXPR: {
            NexType inner = node->children.is_empty() ? NexType::make_variant() : analyze_expr(node->children[0]);
            NexType t = NexType::make_result(inner, NexType::make_str());
            node->resolved_type = t;
            return t;
        }
        case NexNode::RESULT_ERR_EXPR: {
            NexType inner = node->children.is_empty() ? NexType::make_variant() : analyze_expr(node->children[0]);
            NexType t = NexType::make_result(NexType::make_variant(), inner);
            node->resolved_type = t;
            return t;
        }
        case NexNode::IDENTIFIER: {
            StringName name = StringName(node->str_val);
            const NexType *t = lookup(name);
            if (!t) {
                // Check if it's a struct/enum name
                if (structs.has(name)) {
                    node->resolved_type = NexType::make_struct(name);
                    return node->resolved_type;
                }
                if (enums.has(name)) {
                    node->resolved_type = enums[name];
                    return node->resolved_type;
                }
                node->resolved_type = NexType::make_variant();
            } else {
                node->resolved_type = *t;
            }
            return node->resolved_type;
        }
        case NexNode::BINARY_EXPR:
            return analyze_binary(node);
        case NexNode::UNARY_EXPR: {
            NexType operand = analyze_expr(node->children[0]);
            node->resolved_type = operand;
            return operand;
        }
        case NexNode::CALL_EXPR:
            return analyze_call(node);
        case NexNode::METHOD_CALL_EXPR:
            return analyze_method_call(node);
        case NexNode::FIELD_EXPR:
            return analyze_field_access(node);
        case NexNode::INDEX_EXPR: {
            NexType arr = node->children.is_empty() ? NexType::make_variant() : analyze_expr(node->children[0]);
            if (arr.base == NexBaseType::ARRAY && arr.elem_type) {
                node->resolved_type = *arr.elem_type;
            } else {
                node->resolved_type = NexType::make_variant();
            }
            return node->resolved_type;
        }
        case NexNode::ARRAY_LITERAL: {
            NexType elem = NexType::make_variant();
            for (NexNode *c : node->children) {
                elem = analyze_expr(c);
            }
            node->resolved_type = NexType::make_array(elem);
            return node->resolved_type;
        }
        case NexNode::STRUCT_LITERAL: {
            StringName name = StringName(node->str_val);
            node->resolved_type = NexType::make_struct(name);
            for (NexNode *f : node->children) {
                if (!f->children.is_empty()) analyze_expr(f->children[0]);
            }
            return node->resolved_type;
        }
        case NexNode::TUPLE_EXPR: {
            NexType t;
            t.base = NexBaseType::TUPLE;
            for (NexNode *c : node->children) t.tuple_types.push_back(analyze_expr(c));
            node->resolved_type = t;
            return t;
        }
        case NexNode::RANGE_EXPR: {
            for (NexNode *c : node->children) analyze_expr(c);
            node->resolved_type = NexType::make_variant();
            return node->resolved_type;
        }
        case NexNode::INLINE_IF_EXPR: {
            if (node->children.size() >= 2) {
                analyze_expr(node->children[0]);
                NexType t = analyze_expr(node->children[1]);
                if (node->children.size() > 2) analyze_expr(node->children[2]);
                node->resolved_type = t;
                return t;
            }
            return NexType::make_variant();
        }
        case NexNode::AWAIT_EXPR: {
            if (!node->children.is_empty()) return analyze_expr(node->children[0]);
            return NexType::make_variant();
        }
        case NexNode::QUESTION_EXPR: {
            if (!node->children.is_empty()) {
                NexType inner = analyze_expr(node->children[0]);
                if (inner.base == NexBaseType::RESULT && inner.result_ok) {
                    node->resolved_type = *inner.result_ok;
                    return node->resolved_type;
                }
            }
            return NexType::make_variant();
        }
        case NexNode::AS_CAST_EXPR: {
            node->resolved_type = node->type_annotation;
            if (!node->children.is_empty()) analyze_expr(node->children[0]);
            return node->resolved_type;
        }
        case NexNode::DOLLAR_PATH_EXPR:
            node->resolved_type = NexType::make_object(StringName("Node"));
            return node->resolved_type;
        case NexNode::CLOSURE_EXPR: {
            NexType fn;
            fn.base = NexBaseType::FN;
            fn.fn_ret = new NexType(node->type_annotation);
            node->resolved_type = fn;
            return fn;
        }
        default:
            analyze_node(node);
            return node->resolved_type;
    }
}

NexType NexAnalyzer::analyze_binary(NexNode *node) {
    if (node->children.size() < 2) return NexType::make_variant();
    NexType lhs = analyze_expr(node->children[0]);
    NexType rhs = analyze_expr(node->children[1]);
    const String &op = node->op;

    if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=" ||
        op == "and" || op == "or" || op == "not") {
        node->resolved_type = NexType::make_bool();
        return node->resolved_type;
    }
    if (op == "/") {
        node->resolved_type = NexType::make_float();
        return node->resolved_type;
    }
    if (op == "+") {
        if (lhs.base == NexBaseType::STR) { node->resolved_type = NexType::make_str(); return node->resolved_type; }
    }
    if (lhs.is_integer() && rhs.is_integer()) {
        node->resolved_type = NexType::make_int();
    } else if (lhs.is_numeric() || rhs.is_numeric()) {
        node->resolved_type = NexType::make_float();
    } else {
        node->resolved_type = lhs;
    }
    return node->resolved_type;
}

NexType NexAnalyzer::analyze_call(NexNode *node) {
    StringName name = StringName(node->str_val);
    for (NexNode *arg : node->children) analyze_expr(arg);

    if (functions.has(name)) {
        node->resolved_type = functions[name].return_type;
        return node->resolved_type;
    }
    // Godot builtins
    if (name == "print") { node->resolved_type = NexType::make_void(); return node->resolved_type; }
    if (name == "Vector2" || name == "vec2") { node->resolved_type = NexType::make_vec2(); return node->resolved_type; }
    if (name == "Vector3" || name == "vec3") { node->resolved_type = NexType::make_vec3(); return node->resolved_type; }
    if (name == "Color") { node->resolved_type = NexType::make_color(); return node->resolved_type; }

    node->resolved_type = NexType::make_variant();
    return node->resolved_type;
}

NexType NexAnalyzer::analyze_method_call(NexNode *node) {
    if (node->children.is_empty()) return NexType::make_variant();
    NexType obj_type = analyze_expr(node->children[0]);
    for (int i = 1; i < node->children.size(); i++) analyze_expr(node->children[i]);
    StringName method = StringName(node->str_val);

    // Array methods
    if (obj_type.base == NexBaseType::ARRAY) {
        if (method == "len" || method == "size") { node->resolved_type = NexType::make_int(); return node->resolved_type; }
        if (method == "push" || method == "pop") { node->resolved_type = NexType::make_void(); return node->resolved_type; }
        if (method == "sort") { node->resolved_type = NexType::make_void(); return node->resolved_type; }
        if (method == "contains") { node->resolved_type = NexType::make_bool(); return node->resolved_type; }
        if (method == "map" || method == "filter") { node->resolved_type = obj_type; return node->resolved_type; }
    }
    if (obj_type.base == NexBaseType::STR) {
        if (method == "len" || method == "length") { node->resolved_type = NexType::make_int(); return node->resolved_type; }
        if (method == "upper" || method == "lower" || method == "trim" || method == "replace") {
            node->resolved_type = NexType::make_str(); return node->resolved_type;
        }
        if (method == "contains" || method == "starts_with" || method == "ends_with") {
            node->resolved_type = NexType::make_bool(); return node->resolved_type;
        }
    }
    node->resolved_type = NexType::make_variant();
    return node->resolved_type;
}

NexType NexAnalyzer::analyze_field_access(NexNode *node) {
    if (node->children.is_empty()) return NexType::make_variant();
    NexType obj_type = analyze_expr(node->children[0]);
    StringName field_name = StringName(node->str_val);

    if (obj_type.base == NexBaseType::STRUCT) {
        if (structs.has(obj_type.name)) {
            const StructInfo &si = structs[obj_type.name];
            if (si.field_index.has(field_name)) {
                int idx = si.field_index[field_name];
                node->resolved_type = si.fields[idx].type;
                return node->resolved_type;
            }
        }
    }
    if (obj_type.base == NexBaseType::VECTOR2) {
        if (field_name == "x" || field_name == "y") { node->resolved_type = NexType::make_float(); return node->resolved_type; }
    }
    if (obj_type.base == NexBaseType::VECTOR3) {
        if (field_name == "x" || field_name == "y" || field_name == "z") { node->resolved_type = NexType::make_float(); return node->resolved_type; }
    }
    node->resolved_type = NexType::make_variant();
    return node->resolved_type;
}

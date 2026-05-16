#include "nex_ir_gen.h"

int NexIRGen::alloc_slot(const NexType &type) {
    int s = next_slot++;
    current_fn->slot_types.push_back(type);
    current_fn->slot_count = next_slot;
    return s;
}

int NexIRGen::new_label() {
    return next_label++;
}

void NexIRGen::patch_label(int label_placeholder_instr, int target_instr) {
    if (label_placeholder_instr >= 0 && label_placeholder_instr < current_fn->instructions.size()) {
        current_fn->instructions.write[label_placeholder_instr].label = target_instr;
    }
}

void NexIRGen::emit(NexIRInstr instr) {
    current_fn->instructions.push_back(instr);
}

uint32_t NexIRGen::intern_string(const String &s) {
    for (int i = 0; i < current_fn->string_pool.size(); i++) {
        if (current_fn->string_pool[i] == s) return (uint32_t)i;
    }
    current_fn->string_pool.push_back(s);
    return (uint32_t)(current_fn->string_pool.size() - 1);
}

NexOp NexIRGen::arith_op(const String &op, const NexType &type) {
    bool is_float = type.is_float();
    if (op == "+")  return is_float ? NexOp::FLOAT_ADD  : NexOp::INT_ADD;
    if (op == "-")  return is_float ? NexOp::FLOAT_SUB  : NexOp::INT_SUB;
    if (op == "*")  return is_float ? NexOp::FLOAT_MUL  : NexOp::INT_MUL;
    if (op == "/")  return is_float ? NexOp::FLOAT_DIV  : NexOp::FLOAT_DIV;
    if (op == "%")  return NexOp::INT_MOD;
    if (op == "**") return is_float ? NexOp::FLOAT_POW  : NexOp::INT_POW;
    return NexOp::NOP;
}

NexOp NexIRGen::cmp_op(const String &op, const NexType &type) {
    bool is_float = type.is_float();
    bool is_str   = type.base == NexBaseType::STR;
    if (op == "==") return is_str ? NexOp::STR_EQ  : (is_float ? NexOp::FLOAT_EQ  : NexOp::INT_EQ);
    if (op == "!=") return is_str ? NexOp::STR_NEQ : (is_float ? NexOp::FLOAT_NEQ : NexOp::INT_NEQ);
    if (op == "<")  return is_float ? NexOp::FLOAT_LT  : NexOp::INT_LT;
    if (op == ">")  return is_float ? NexOp::FLOAT_GT  : NexOp::INT_GT;
    if (op == "<=") return is_float ? NexOp::FLOAT_LTE : NexOp::INT_LTE;
    if (op == ">=") return is_float ? NexOp::FLOAT_GTE : NexOp::INT_GTE;
    return NexOp::NOP;
}

NexFunction *NexIRGen::generate_function(NexNode *fn_def_node) {
    NexFunction *fn = new NexFunction();
    fn->name        = StringName(fn_def_node->str_val);
    fn->return_type = fn_def_node->type_annotation;
    fn->is_async    = fn_def_node->is_async;
    fn->is_inline   = (fn_def_node->annotation_name == "inline");
    current_fn      = fn;
    next_slot       = 0;
    next_label      = 0;
    local_slots.clear();

    for (NexNode *child : fn_def_node->children) {
        if (child->kind == NexNode::FN_PARAM) {
            int slot = alloc_slot(child->type_annotation);
            local_slots[StringName(child->str_val)] = slot;
            fn->param_count++;
        } else if (child->kind == NexNode::BLOCK) {
            gen_block(child);
        }
    }

    if (fn->return_type.base == NexBaseType::VOID || fn->return_type.base == NexBaseType::UNKNOWN) {
        NexIRInstr ret;
        ret.op = NexOp::RETURN_VOID;
        emit(ret);
    }
    NexIRInstr end;
    end.op = NexOp::END;
    emit(end);
    return fn;
}

NexFunction *NexIRGen::generate_script_body(NexNode *program_node) {
    NexFunction *fn = new NexFunction();
    fn->name = StringName("__script_init__");
    current_fn = fn;
    next_slot = 0;
    next_label = 0;
    local_slots.clear();

    for (NexNode *child : program_node->children) {
        if (child && child->kind != NexNode::FN_DEF &&
            child->kind != NexNode::STRUCT_DEF &&
            child->kind != NexNode::IMPL_BLOCK  &&
            child->kind != NexNode::ENUM_DEF    &&
            child->kind != NexNode::EXTENDS_STMT) {
            gen_stmt(child);
        }
    }

    NexIRInstr end;
    end.op = NexOp::END;
    emit(end);
    return fn;
}

void NexIRGen::gen_block(NexNode *block_node) {
    if (!block_node) return;
    for (NexNode *child : block_node->children) {
        gen_stmt(child);
    }
}

void NexIRGen::gen_stmt(NexNode *node) {
    if (!node) return;
    switch (node->kind) {
        case NexNode::VAR_DECL:
        case NexNode::CONST_DEF: {
            int slot;
            if (node->children.is_empty()) {
                slot = alloc_slot(node->type_annotation);
                NexIRInstr load;
                load.op = NexOp::LOAD_NULL;
                load.dst = slot;
                emit(load);
            } else {
                slot = gen_expr(node->children[0]);
            }
            local_slots[StringName(node->str_val)] = slot;
        } break;
        case NexNode::ASSIGN_STMT: {
            if (node->children.size() < 2) break;
            NexNode *lhs = node->children[0];
            int rhs_slot = gen_expr(node->children[1]);
            if (lhs->kind == NexNode::IDENTIFIER) {
                StringName name = StringName(lhs->str_val);
                if (local_slots.has(name)) {
                    NexIRInstr store;
                    store.op = NexOp::STORE_LOCAL;
                    store.dst = local_slots[name];
                    store.src_a = rhs_slot;
                    emit(store);
                }
            } else if (lhs->kind == NexNode::FIELD_EXPR) {
                // struct field assignment
                gen_field_access(lhs, rhs_slot);
            }
        } break;
        case NexNode::ASSIGN_OP_STMT: {
            if (node->children.size() < 2) break;
            NexNode *lhs = node->children[0];
            int lhs_slot = gen_expr(lhs);
            int rhs_slot = gen_expr(node->children[1]);
            NexType lhs_type = lhs->resolved_type;
            String base_op = node->op.substr(0, node->op.length() - 1);
            NexOp arith = arith_op(base_op, lhs_type);
            int result_slot = alloc_slot(lhs_type);
            NexIRInstr op_instr;
            op_instr.op = arith;
            op_instr.dst = result_slot;
            op_instr.src_a = lhs_slot;
            op_instr.src_b = rhs_slot;
            emit(op_instr);
            if (lhs->kind == NexNode::IDENTIFIER) {
                StringName name = StringName(lhs->str_val);
                if (local_slots.has(name)) {
                    NexIRInstr store;
                    store.op = NexOp::STORE_LOCAL;
                    store.dst = local_slots[name];
                    store.src_a = result_slot;
                    emit(store);
                }
            }
        } break;
        case NexNode::RETURN_STMT: {
            if (node->children.is_empty()) {
                NexIRInstr ret; ret.op = NexOp::RETURN_VOID; emit(ret);
            } else {
                int slot = gen_expr(node->children[0]);
                NexIRInstr ret; ret.op = NexOp::RETURN; ret.src_a = slot; emit(ret);
            }
        } break;
        case NexNode::IF_STMT:
            gen_if(node);
            break;
        case NexNode::FOR_RANGE_STMT:
            gen_for_range(node);
            break;
        case NexNode::FOR_EACH_STMT:
            gen_for_each(node);
            break;
        case NexNode::WHILE_STMT:
            gen_while(node);
            break;
        case NexNode::LOOP_STMT:
            gen_loop(node);
            break;
        case NexNode::MATCH_STMT:
            gen_match(node);
            break;
        case NexNode::EMIT_STMT: {
            Vector<int> arg_slots;
            for (NexNode *c : node->children) arg_slots.push_back(gen_expr(c));
            NexIRInstr emit_instr;
            emit_instr.op = NexOp::EMIT_SIGNAL;
            emit_instr.call_name = StringName(node->str_val);
            emit(emit_instr);
        } break;
        case NexNode::BLOCK:
            gen_block(node);
            break;
        case NexNode::BREAK_STMT: {
            NexIRInstr jmp; jmp.op = NexOp::JUMP; jmp.label = -999; // patched later
            emit(jmp);
        } break;
        case NexNode::CONTINUE_STMT: {
            NexIRInstr jmp; jmp.op = NexOp::JUMP; jmp.label = -998;
            emit(jmp);
        } break;
        default:
            gen_expr(node);
            break;
    }
}

int NexIRGen::gen_expr(NexNode *node) {
    if (!node) return -1;
    NexType result_type = node->resolved_type;

    switch (node->kind) {
        case NexNode::INT_LITERAL: {
            int slot = alloc_slot(NexType::make_int());
            NexIRInstr instr; instr.op = NexOp::LOAD_INT_CONST;
            instr.dst = slot; instr.imm_int = node->int_val; emit(instr);
            return slot;
        }
        case NexNode::FLOAT_LITERAL: {
            int slot = alloc_slot(NexType::make_float());
            NexIRInstr instr; instr.op = NexOp::LOAD_FLOAT_CONST;
            instr.dst = slot; instr.imm_float = node->float_val; emit(instr);
            return slot;
        }
        case NexNode::BOOL_LITERAL: {
            int slot = alloc_slot(NexType::make_bool());
            NexIRInstr instr; instr.op = NexOp::LOAD_BOOL_CONST;
            instr.dst = slot; instr.imm_int = node->bool_val ? 1 : 0; emit(instr);
            return slot;
        }
        case NexNode::STR_LITERAL: {
            int slot = alloc_slot(NexType::make_str());
            NexIRInstr instr; instr.op = NexOp::LOAD_STR_CONST;
            instr.dst = slot; instr.imm_str = intern_string(node->str_val); emit(instr);
            return slot;
        }
        case NexNode::IDENTIFIER: {
            StringName name = StringName(node->str_val);
            if (local_slots.has(name)) {
                int src = local_slots[name];
                int dst = alloc_slot(result_type);
                NexIRInstr instr; instr.op = NexOp::LOAD_LOCAL;
                instr.dst = dst; instr.src_a = src; emit(instr);
                return dst;
            }
            // Return -1 for unknown (could be global/godot)
            return -1;
        }
        case NexNode::BINARY_EXPR: {
            if (node->children.size() < 2) return -1;
            int lhs = gen_expr(node->children[0]);
            int rhs = gen_expr(node->children[1]);
            NexType lhs_type = node->children[0]->resolved_type;
            const String &op = node->op;

            if (op == "and") {
                int dst = alloc_slot(NexType::make_bool());
                NexIRInstr instr; instr.op = NexOp::BOOL_AND;
                instr.dst = dst; instr.src_a = lhs; instr.src_b = rhs; emit(instr);
                return dst;
            }
            if (op == "or") {
                int dst = alloc_slot(NexType::make_bool());
                NexIRInstr instr; instr.op = NexOp::BOOL_OR;
                instr.dst = dst; instr.src_a = lhs; instr.src_b = rhs; emit(instr);
                return dst;
            }

            bool is_cmp = (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=");
            NexType result = is_cmp ? NexType::make_bool() : result_type;
            if (result.base == NexBaseType::UNKNOWN) result = lhs_type;
            int dst = alloc_slot(result);
            NexIRInstr instr;
            instr.op = is_cmp ? cmp_op(op, lhs_type) : arith_op(op, result);
            instr.dst = dst; instr.src_a = lhs; instr.src_b = rhs;
            emit(instr);
            return dst;
        }
        case NexNode::UNARY_EXPR: {
            if (node->children.is_empty()) return -1;
            int src = gen_expr(node->children[0]);
            int dst = alloc_slot(result_type);
            NexIRInstr instr;
            if (node->op == "not") instr.op = NexOp::BOOL_NOT;
            else if (node->op == "-") {
                if (result_type.is_float()) instr.op = NexOp::FLOAT_NEG;
                else instr.op = NexOp::INT_NEG;
            }
            instr.dst = dst; instr.src_a = src; emit(instr);
            return dst;
        }
        case NexNode::CALL_EXPR: {
            Vector<int> arg_slots;
            for (NexNode *c : node->children) arg_slots.push_back(gen_expr(c));
            int dst = alloc_slot(result_type);
            NexIRInstr instr;
            if (node->str_val == "print") {
                instr.op = NexOp::CALL_BUILTIN;
            } else {
                instr.op = NexOp::CALL_DIRECT;
            }
            instr.call_name = StringName(node->str_val);
            instr.dst = dst;
            if (!arg_slots.is_empty()) instr.src_a = arg_slots[0];
            if (arg_slots.size() > 1) instr.src_b = arg_slots[1];
            emit(instr);
            return dst;
        }
        case NexNode::METHOD_CALL_EXPR: {
            if (node->children.is_empty()) return -1;
            int obj_slot = gen_expr(node->children[0]);
            Vector<int> arg_slots;
            for (int i = 1; i < node->children.size(); i++) arg_slots.push_back(gen_expr(node->children[i]));
            int dst = alloc_slot(result_type);
            NexIRInstr instr; instr.op = NexOp::CALL_GODOT_METHOD;
            instr.call_name = StringName(node->str_val);
            instr.dst = dst; instr.src_a = obj_slot;
            if (!arg_slots.is_empty()) instr.src_b = arg_slots[0];
            emit(instr);
            return dst;
        }
        case NexNode::FIELD_EXPR: {
            if (node->children.is_empty()) return -1;
            int obj_slot = gen_expr(node->children[0]);
            NexType obj_type = node->children[0]->resolved_type;
            int dst = alloc_slot(result_type);
            if (obj_type.base == NexBaseType::STRUCT && structs.has(obj_type.name)) {
                const NexAnalyzer::StructInfo &si = structs[obj_type.name];
                StringName field_name = StringName(node->str_val);
                if (si.field_index.has(field_name)) {
                    int idx = si.field_index[field_name];
                    const NexAnalyzer::FieldInfo &fi = si.fields[idx];
                    NexIRInstr instr;
                    if (fi.type.is_integer()) instr.op = NexOp::LOAD_FIELD_INT;
                    else if (fi.type.is_float()) instr.op = NexOp::LOAD_FIELD_FLOAT;
                    else if (fi.type.base == NexBaseType::VECTOR2) instr.op = NexOp::LOAD_FIELD_VEC2;
                    else if (fi.type.base == NexBaseType::STR) instr.op = NexOp::LOAD_FIELD_STR;
                    else instr.op = NexOp::LOAD_FIELD_OBJ;
                    instr.dst = dst; instr.src_a = obj_slot;
                    instr.field_offset = fi.byte_offset;
                    emit(instr);
                    return dst;
                }
            }
            // Fallback: Godot method on object
            NexIRInstr instr; instr.op = NexOp::CALL_GODOT_METHOD;
            instr.call_name = StringName(node->str_val);
            instr.dst = dst; instr.src_a = obj_slot;
            emit(instr);
            return dst;
        }
        case NexNode::INDEX_EXPR: {
            if (node->children.size() < 2) return -1;
            int arr_slot = gen_expr(node->children[0]);
            int idx_slot = gen_expr(node->children[1]);
            NexType arr_type = node->children[0]->resolved_type;
            int dst = alloc_slot(result_type);
            NexIRInstr instr;
            if (arr_type.elem_type && arr_type.elem_type->is_integer()) instr.op = NexOp::ARRAY_GET_INT;
            else if (arr_type.elem_type && arr_type.elem_type->is_float()) instr.op = NexOp::ARRAY_GET_FLOAT;
            else instr.op = NexOp::LOAD_INDEX;
            instr.dst = dst; instr.src_a = arr_slot; instr.src_b = idx_slot;
            emit(instr);
            return dst;
        }
        case NexNode::ARRAY_LITERAL: {
            int dst = alloc_slot(result_type);
            NexIRInstr new_arr; new_arr.op = NexOp::ARRAY_NEW; new_arr.dst = dst; emit(new_arr);
            for (NexNode *c : node->children) {
                int elem = gen_expr(c);
                NexIRInstr push; push.op = NexOp::ARRAY_PUSH;
                push.src_a = dst; push.src_b = elem; emit(push);
            }
            return dst;
        }
        case NexNode::STRUCT_LITERAL: {
            StringName sname = StringName(node->str_val);
            int dst = alloc_slot(NexType::make_struct(sname));
            NexIRInstr alloc; alloc.op = NexOp::STRUCT_ALLOC;
            alloc.dst = dst; alloc.call_name = sname;
            if (structs.has(sname)) alloc.imm_int = structs[sname].total_size;
            emit(alloc);
            if (structs.has(sname)) {
                const NexAnalyzer::StructInfo &si = structs[sname];
                for (NexNode *f : node->children) {
                    if (f->kind == NexNode::FIELD_EXPR && !f->children.is_empty()) {
                        int val_slot = gen_expr(f->children[0]);
                        StringName fname = StringName(f->str_val);
                        if (si.field_index.has(fname)) {
                            int fidx = si.field_index[fname];
                            const NexAnalyzer::FieldInfo &fi = si.fields[fidx];
                            NexIRInstr store;
                            if (fi.type.is_integer()) store.op = NexOp::STORE_FIELD_INT;
                            else if (fi.type.is_float()) store.op = NexOp::STORE_FIELD_FLOAT;
                            else if (fi.type.base == NexBaseType::VECTOR2) store.op = NexOp::STORE_FIELD_VEC2;
                            else store.op = NexOp::STORE_FIELD_OBJ;
                            store.dst = dst; store.src_a = val_slot;
                            store.field_offset = fi.byte_offset;
                            emit(store);
                        }
                    }
                }
            }
            return dst;
        }
        case NexNode::OPTION_SOME_EXPR: {
            int inner = node->children.is_empty() ? -1 : gen_expr(node->children[0]);
            int dst = alloc_slot(result_type);
            NexIRInstr instr; instr.op = NexOp::OPTION_SOME;
            instr.dst = dst; instr.src_a = inner; emit(instr);
            return dst;
        }
        case NexNode::OPTION_NONE_EXPR: {
            int dst = alloc_slot(result_type);
            NexIRInstr instr; instr.op = NexOp::OPTION_NONE; instr.dst = dst; emit(instr);
            return dst;
        }
        case NexNode::RESULT_OK_EXPR: {
            int inner = node->children.is_empty() ? -1 : gen_expr(node->children[0]);
            int dst = alloc_slot(result_type);
            NexIRInstr instr; instr.op = NexOp::RESULT_OK;
            instr.dst = dst; instr.src_a = inner; emit(instr);
            return dst;
        }
        case NexNode::RESULT_ERR_EXPR: {
            int inner = node->children.is_empty() ? -1 : gen_expr(node->children[0]);
            int dst = alloc_slot(result_type);
            NexIRInstr instr; instr.op = NexOp::RESULT_ERR;
            instr.dst = dst; instr.src_a = inner; emit(instr);
            return dst;
        }
        case NexNode::QUESTION_EXPR: {
            int dst = alloc_slot(result_type);
            gen_question(node, dst);
            return dst;
        }
        case NexNode::DOLLAR_PATH_EXPR: {
            int dst = alloc_slot(NexType::make_object(StringName("Node")));
            NexIRInstr instr; instr.op = NexOp::GET_NODE;
            instr.dst = dst; instr.imm_str = intern_string(node->str_val); emit(instr);
            return dst;
        }
        case NexNode::INLINE_IF_EXPR: {
            if (node->children.size() < 2) return -1;
            int cond_slot = gen_expr(node->children[0]);
            int dst = alloc_slot(result_type);

            NexIRInstr jif; jif.op = NexOp::JUMP_IF_FALSE; jif.src_a = cond_slot;
            int jif_idx = current_fn->instructions.size();
            emit(jif);

            int true_slot = gen_expr(node->children[1]);
            NexIRInstr store_t; store_t.op = NexOp::STORE_LOCAL; store_t.dst = dst; store_t.src_a = true_slot;
            emit(store_t);

            NexIRInstr jmp; jmp.op = NexOp::JUMP;
            int jmp_idx = current_fn->instructions.size();
            emit(jmp);

            int else_start = current_fn->instructions.size();
            patch_label(jif_idx, else_start);

            if (node->children.size() > 2) {
                int false_slot = gen_expr(node->children[2]);
                NexIRInstr store_f; store_f.op = NexOp::STORE_LOCAL; store_f.dst = dst; store_f.src_a = false_slot;
                emit(store_f);
            }

            patch_label(jmp_idx, current_fn->instructions.size());
            return dst;
        }
        case NexNode::TUPLE_EXPR: {
            int dst = alloc_slot(result_type);
            for (NexNode *c : node->children) gen_expr(c);
            return dst;
        }
        case NexNode::AWAIT_EXPR: {
            if (!node->children.is_empty()) {
                int sig_slot = gen_expr(node->children[0]);
                int dst = alloc_slot(result_type);
                NexIRInstr instr; instr.op = NexOp::AWAIT_SIGNAL;
                instr.dst = dst; instr.src_a = sig_slot; emit(instr);
                return dst;
            }
            return -1;
        }
        case NexNode::AS_CAST_EXPR: {
            if (node->children.is_empty()) return -1;
            int src = gen_expr(node->children[0]);
            int dst = alloc_slot(node->type_annotation);
            NexIRInstr instr; instr.op = NexOp::FROM_VARIANT;
            instr.dst = dst; instr.src_a = src; instr.slot_type = node->type_annotation;
            emit(instr);
            return dst;
        }
        case NexNode::CLOSURE_EXPR: {
            int dst = alloc_slot(result_type);
            NexIRInstr instr; instr.op = NexOp::LOAD_NULL; instr.dst = dst; emit(instr);
            return dst;
        }
        default:
            gen_stmt(node);
            return -1;
    }
}

void NexIRGen::gen_field_access(NexNode *field_node, int val_slot) {
    if (!field_node || field_node->children.is_empty()) return;
    NexNode *obj = field_node->children[0];
    StringName name = StringName(obj->str_val);
    if (!local_slots.has(name)) return;
    int obj_slot = local_slots[name];
    NexType obj_type = obj->resolved_type;
    StringName field_name = StringName(field_node->str_val);

    if (obj_type.base == NexBaseType::STRUCT && structs.has(obj_type.name)) {
        const NexAnalyzer::StructInfo &si = structs[obj_type.name];
        if (si.field_index.has(field_name)) {
            int idx = si.field_index[field_name];
            const NexAnalyzer::FieldInfo &fi = si.fields[idx];
            NexIRInstr store;
            if (fi.type.is_integer()) store.op = NexOp::STORE_FIELD_INT;
            else if (fi.type.is_float()) store.op = NexOp::STORE_FIELD_FLOAT;
            else store.op = NexOp::STORE_FIELD_OBJ;
            store.dst = obj_slot; store.src_a = val_slot;
            store.field_offset = fi.byte_offset;
            emit(store);
        }
    }
}

void NexIRGen::gen_for_range(NexNode *for_node) {
    if (for_node->children.is_empty()) return;
    NexNode *range = for_node->children[0];
    NexNode *step_node = (for_node->children.size() == 3) ? for_node->children[1] : nullptr;
    NexNode *body = for_node->children[for_node->children.size() - 1];

    int start_slot = -1, end_slot = -1, step_slot = -1;
    if (range && range->kind == NexNode::RANGE_EXPR && range->children.size() == 2) {
        start_slot = gen_expr(range->children[0]);
        end_slot   = gen_expr(range->children[1]);
    } else {
        start_slot = alloc_slot(NexType::make_int());
        NexIRInstr zero; zero.op = NexOp::LOAD_INT_CONST; zero.dst = start_slot; zero.imm_int = 0; emit(zero);
        end_slot = range ? gen_expr(range) : start_slot;
    }

    if (step_node) {
        step_slot = gen_expr(step_node);
    } else {
        step_slot = alloc_slot(NexType::make_int());
        NexIRInstr one; one.op = NexOp::LOAD_INT_CONST; one.dst = step_slot; one.imm_int = 1; emit(one);
    }

    int counter_slot = alloc_slot(NexType::make_int());
    NexIRInstr init; init.op = NexOp::STORE_LOCAL; init.dst = counter_slot; init.src_a = start_slot; emit(init);
    local_slots[StringName(for_node->str_val)] = counter_slot;

    int loop_start = current_fn->instructions.size();

    // Loop condition: counter < end
    int cond_slot = alloc_slot(NexType::make_bool());
    NexIRInstr cmp; cmp.op = NexOp::INT_LT; cmp.dst = cond_slot;
    cmp.src_a = counter_slot; cmp.src_b = end_slot; emit(cmp);

    NexIRInstr jif; jif.op = NexOp::JUMP_IF_FALSE; jif.src_a = cond_slot;
    int jif_idx = current_fn->instructions.size();
    emit(jif);

    gen_block(body);

    // counter += step
    int new_counter = alloc_slot(NexType::make_int());
    NexIRInstr add; add.op = NexOp::INT_ADD; add.dst = new_counter;
    add.src_a = counter_slot; add.src_b = step_slot; emit(add);
    NexIRInstr store; store.op = NexOp::STORE_LOCAL;
    store.dst = counter_slot; store.src_a = new_counter; emit(store);

    NexIRInstr jmp; jmp.op = NexOp::JUMP; jmp.label = loop_start; emit(jmp);

    int loop_end = current_fn->instructions.size();
    patch_label(jif_idx, loop_end);
}

void NexIRGen::gen_for_each(NexNode *for_node) {
    if (for_node->children.size() < 2) return;
    NexNode *iterable = for_node->children[0];
    NexNode *body = for_node->children[1];

    int arr_slot = gen_expr(iterable);
    int len_slot = alloc_slot(NexType::make_int());
    NexIRInstr get_len; get_len.op = NexOp::ARRAY_LEN;
    get_len.dst = len_slot; get_len.src_a = arr_slot; emit(get_len);

    int counter_slot = alloc_slot(NexType::make_int());
    NexIRInstr init; init.op = NexOp::LOAD_INT_CONST; init.dst = counter_slot; init.imm_int = 0; emit(init);

    int loop_start = current_fn->instructions.size();

    int cond_slot = alloc_slot(NexType::make_bool());
    NexIRInstr cmp; cmp.op = NexOp::INT_LT; cmp.dst = cond_slot;
    cmp.src_a = counter_slot; cmp.src_b = len_slot; emit(cmp);

    NexIRInstr jif; jif.op = NexOp::JUMP_IF_FALSE; jif.src_a = cond_slot;
    int jif_idx = current_fn->instructions.size();
    emit(jif);

    String var_name = for_node->str_val;
    int item_slot = alloc_slot(NexType::make_variant());
    NexIRInstr get_item; get_item.op = NexOp::LOAD_INDEX;
    get_item.dst = item_slot; get_item.src_a = arr_slot; get_item.src_b = counter_slot; emit(get_item);
    local_slots[StringName(var_name)] = item_slot;

    gen_block(body);

    int one_slot = alloc_slot(NexType::make_int());
    NexIRInstr one; one.op = NexOp::LOAD_INT_CONST; one.dst = one_slot; one.imm_int = 1; emit(one);
    int new_counter = alloc_slot(NexType::make_int());
    NexIRInstr add; add.op = NexOp::INT_ADD; add.dst = new_counter;
    add.src_a = counter_slot; add.src_b = one_slot; emit(add);
    NexIRInstr store; store.op = NexOp::STORE_LOCAL;
    store.dst = counter_slot; store.src_a = new_counter; emit(store);

    NexIRInstr jmp; jmp.op = NexOp::JUMP; jmp.label = loop_start; emit(jmp);
    patch_label(jif_idx, current_fn->instructions.size());
}

void NexIRGen::gen_match(NexNode *match_node) {
    if (match_node->children.is_empty()) return;
    int subject_slot = gen_expr(match_node->children[0]);
    NexType subject_type = match_node->children[0]->resolved_type;

    bool use_jump_table = subject_type.is_integer() || subject_type.base == NexBaseType::ENUM;

    if (use_jump_table) {
        // Collect integer arm values
        int64_t max_val = -1;
        bool all_int_literals = true;
        for (int i = 1; i < match_node->children.size(); i++) {
            NexNode *arm = match_node->children[i];
            if (arm->children.is_empty()) continue;
            NexNode *pattern = arm->children[0];
            if (pattern->kind != NexNode::INT_LITERAL && pattern->str_val != "_") { all_int_literals = false; break; }
            if (pattern->kind == NexNode::INT_LITERAL && pattern->int_val > max_val) max_val = pattern->int_val;
        }

        if (all_int_literals && max_val >= 0 && max_val < 256) {
            NexIRInstr jt; jt.op = NexOp::JUMP_TABLE; jt.src_a = subject_slot;
            jt.jump_table.resize(max_val + 1);
            for (int k = 0; k <= max_val; k++) jt.jump_table.write[k] = -1;
            int jt_idx = current_fn->instructions.size();
            emit(jt);

            Vector<int> arm_end_jumps;
            Vector<int> arm_starts;
            int default_start = -1;

            for (int i = 1; i < match_node->children.size(); i++) {
                NexNode *arm = match_node->children[i];
                if (arm->children.is_empty()) continue;
                NexNode *pattern = arm->children[0];
                int arm_start = current_fn->instructions.size();
                arm_starts.push_back(arm_start);

                if (pattern->kind == NexNode::INT_LITERAL) {
                    int64_t v = pattern->int_val;
                    if (v >= 0 && v <= max_val) {
                        current_fn->instructions.write[jt_idx].jump_table.write[v] = arm_start;
                    }
                } else {
                    default_start = arm_start;
                    current_fn->instructions.write[jt_idx].label = arm_start;
                }

                if (arm->children.size() > 1) gen_block(arm->children[1]);
                NexIRInstr jmp; jmp.op = NexOp::JUMP;
                arm_end_jumps.push_back(current_fn->instructions.size());
                emit(jmp);
            }

            int match_end = current_fn->instructions.size();
            for (int j : arm_end_jumps) patch_label(j, match_end);
            for (int k = 0; k <= max_val; k++) {
                if (current_fn->instructions[jt_idx].jump_table[k] == -1) {
                    current_fn->instructions.write[jt_idx].jump_table.write[k] = match_end;
                }
            }
            return;
        }
    }

    // Fallback: chain of JUMP_IF_FALSE
    Vector<int> arm_end_jumps;
    for (int i = 1; i < match_node->children.size(); i++) {
        NexNode *arm = match_node->children[i];
        if (arm->children.is_empty()) continue;
        NexNode *pattern = arm->children[0];
        NexNode *body    = (arm->children.size() > 1) ? arm->children[1] : nullptr;

        if (pattern->str_val == "_") {
            if (body) gen_block(body);
            NexIRInstr jmp; jmp.op = NexOp::JUMP;
            arm_end_jumps.push_back(current_fn->instructions.size());
            emit(jmp);
        } else {
            int pat_slot = gen_expr(pattern);
            int cond_slot = alloc_slot(NexType::make_bool());
            NexIRInstr cmp_instr;
            cmp_instr.op = cmp_op("==", subject_type);
            cmp_instr.dst = cond_slot; cmp_instr.src_a = subject_slot; cmp_instr.src_b = pat_slot;
            emit(cmp_instr);

            NexIRInstr jif; jif.op = NexOp::JUMP_IF_FALSE; jif.src_a = cond_slot;
            int jif_idx = current_fn->instructions.size();
            emit(jif);

            if (body) gen_block(body);
            NexIRInstr jmp; jmp.op = NexOp::JUMP;
            arm_end_jumps.push_back(current_fn->instructions.size());
            emit(jmp);

            patch_label(jif_idx, current_fn->instructions.size());
        }
    }
    int match_end = current_fn->instructions.size();
    for (int j : arm_end_jumps) patch_label(j, match_end);
}

void NexIRGen::gen_question(NexNode *q_node, int dst_slot) {
    if (q_node->children.is_empty()) return;
    int result_slot = gen_expr(q_node->children[0]);

    int is_ok_slot = alloc_slot(NexType::make_bool());
    NexIRInstr chk; chk.op = NexOp::RESULT_IS_OK;
    chk.dst = is_ok_slot; chk.src_a = result_slot; emit(chk);

    NexIRInstr jif; jif.op = NexOp::JUMP_IF_FALSE; jif.src_a = is_ok_slot;
    int jif_idx = current_fn->instructions.size();
    emit(jif);

    // Happy path: unwrap ok value
    NexIRInstr unwrap; unwrap.op = NexOp::RESULT_UNWRAP_OK;
    unwrap.dst = dst_slot; unwrap.src_a = result_slot; emit(unwrap);

    NexIRInstr jmp_skip; jmp_skip.op = NexOp::JUMP;
    int jmp_skip_idx = current_fn->instructions.size();
    emit(jmp_skip);

    // Error path: early return
    int err_label = current_fn->instructions.size();
    patch_label(jif_idx, err_label);

    NexIRInstr unwrap_err; unwrap_err.op = NexOp::RESULT_UNWRAP_ERR;
    unwrap_err.dst = dst_slot; unwrap_err.src_a = result_slot; emit(unwrap_err);
    NexIRInstr ret; ret.op = NexOp::RETURN; ret.src_a = dst_slot; emit(ret);

    patch_label(jmp_skip_idx, current_fn->instructions.size());
}

void NexIRGen::gen_if(NexNode *if_node) {
    if (if_node->children.is_empty()) return;
    int cond_slot = gen_expr(if_node->children[0]);

    NexIRInstr jif; jif.op = NexOp::JUMP_IF_FALSE; jif.src_a = cond_slot;
    int jif_idx = current_fn->instructions.size();
    emit(jif);

    if (if_node->children.size() > 1) gen_block(if_node->children[1]);

    Vector<int> end_jumps;
    NexIRInstr jmp; jmp.op = NexOp::JUMP;
    end_jumps.push_back(current_fn->instructions.size());
    emit(jmp);

    patch_label(jif_idx, current_fn->instructions.size());

    for (int i = 2; i < if_node->children.size(); i++) {
        NexNode *child = if_node->children[i];
        if (child->kind == NexNode::IF_STMT) {
            gen_if(child);
        } else if (child->kind == NexNode::BLOCK) {
            gen_block(child);
            NexIRInstr j; j.op = NexOp::JUMP;
            end_jumps.push_back(current_fn->instructions.size());
            emit(j);
        }
    }

    int end = current_fn->instructions.size();
    for (int j : end_jumps) patch_label(j, end);
}

void NexIRGen::gen_while(NexNode *while_node) {
    if (while_node->children.size() < 2) return;
    int loop_start = current_fn->instructions.size();
    int cond_slot = gen_expr(while_node->children[0]);

    NexIRInstr jif; jif.op = NexOp::JUMP_IF_FALSE; jif.src_a = cond_slot;
    int jif_idx = current_fn->instructions.size();
    emit(jif);

    gen_block(while_node->children[1]);

    NexIRInstr jmp; jmp.op = NexOp::JUMP; jmp.label = loop_start; emit(jmp);
    patch_label(jif_idx, current_fn->instructions.size());
}

void NexIRGen::gen_loop(NexNode *loop_node) {
    if (loop_node->children.is_empty()) return;
    int loop_start = current_fn->instructions.size();
    gen_block(loop_node->children[0]);
    NexIRInstr jmp; jmp.op = NexOp::JUMP; jmp.label = loop_start; emit(jmp);
}

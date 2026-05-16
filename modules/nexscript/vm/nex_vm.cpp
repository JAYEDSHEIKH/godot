#include "nex_vm.h"
#include "core/io/resource.h"
#include "core/os/os.h"

// Forward declare NexScriptInstance
class NexScriptInstance;

Variant NexVM::execute(
    NexFunction        *p_fn,
    NexScriptInstance  *p_instance,
    const Variant     **p_args,
    int                 p_arg_count,
    Callable::CallError &r_error
) {
    if (!p_fn || p_fn->slot_count == 0) return Variant();

    TypedStack stack;
    stack.allocate(p_fn->slot_count + 64);

    // Copy args into first param slots
    for (int i = 0; i < p_arg_count && i < p_fn->param_count; i++) {
        if (p_args[i]) {
            NexType param_type = (i < p_fn->slot_types.size()) ? p_fn->slot_types[i] : NexType::make_variant();
            stack[i].from_variant(*p_args[i], param_type);
        }
    }

    dispatch(p_fn, stack, p_instance);

    // Return value is stored in slot 0 (return convention)
    Variant ret;
    if (p_fn->slot_count > 0 && stack[0].kind != SlotKind::EMPTY) {
        ret = stack[0].to_variant();
    }
    stack.free_slots();
    return ret;
}

void NexVM::call_builtin(const StringName &name, TypedStack &stack, int dst, int src_a, int src_b) {
    if (name == "print" || name == "println") {
        if (src_a >= 0) {
            Variant v = stack[src_a].to_variant();
            print_line(v.operator String());
        }
    }
}

void NexVM::dispatch(NexFunction *fn, TypedStack &stack, NexScriptInstance *instance) {
    if (fn->instructions.is_empty()) return;

    const NexIRInstr *ip = fn->instructions.ptr();
    const NexIRInstr *ip_end = ip + fn->instructions.size();

    // Use a switch-based dispatch for portability
    // (computed goto would give ~30% speedup on GCC/Clang for tight loops)
    while (ip < ip_end) {
        const NexIRInstr &instr = *ip;
        switch (instr.op) {

            case NexOp::LOAD_INT_CONST:
                stack[instr.dst].set_int(instr.imm_int);
                break;

            case NexOp::LOAD_FLOAT_CONST:
                stack[instr.dst].set_float(instr.imm_float);
                break;

            case NexOp::LOAD_BOOL_CONST:
                stack[instr.dst].set_bool(instr.imm_int != 0);
                break;

            case NexOp::LOAD_STR_CONST: {
                String *s = new String(fn->string_pool[instr.imm_str]);
                stack[instr.dst].set_str(s);
            } break;

            case NexOp::LOAD_NULL:
                stack[instr.dst] = TypedSlot();
                break;

            case NexOp::LOAD_LOCAL:
                if (instr.src_a >= 0) stack[instr.dst] = stack[instr.src_a];
                break;

            case NexOp::STORE_LOCAL:
                if (instr.dst >= 0 && instr.src_a >= 0) stack[instr.dst] = stack[instr.src_a];
                break;

            // ── Integer Arithmetic ─────────────────────────────────────
            case NexOp::INT_ADD:
                stack[instr.dst].set_int(stack[instr.src_a].as_int() + stack[instr.src_b].as_int());
                break;
            case NexOp::INT_SUB:
                stack[instr.dst].set_int(stack[instr.src_a].as_int() - stack[instr.src_b].as_int());
                break;
            case NexOp::INT_MUL:
                stack[instr.dst].set_int(stack[instr.src_a].as_int() * stack[instr.src_b].as_int());
                break;
            case NexOp::INT_DIV: {
                int64_t b = stack[instr.src_b].as_int();
                stack[instr.dst].set_float(b != 0 ? (double)stack[instr.src_a].as_int() / b : 0.0);
            } break;
            case NexOp::INT_MOD: {
                int64_t b = stack[instr.src_b].as_int();
                stack[instr.dst].set_int(b != 0 ? stack[instr.src_a].as_int() % b : 0);
            } break;
            case NexOp::INT_POW: {
                int64_t base = stack[instr.src_a].as_int();
                int64_t exp  = stack[instr.src_b].as_int();
                int64_t result = 1;
                for (int64_t i = 0; i < exp; i++) result *= base;
                stack[instr.dst].set_int(result);
            } break;
            case NexOp::INT_NEG:
                stack[instr.dst].set_int(-stack[instr.src_a].as_int());
                break;
            case NexOp::INT_EQ:
                stack[instr.dst].set_bool(stack[instr.src_a].as_int() == stack[instr.src_b].as_int());
                break;
            case NexOp::INT_NEQ:
                stack[instr.dst].set_bool(stack[instr.src_a].as_int() != stack[instr.src_b].as_int());
                break;
            case NexOp::INT_LT:
                stack[instr.dst].set_bool(stack[instr.src_a].as_int() < stack[instr.src_b].as_int());
                break;
            case NexOp::INT_GT:
                stack[instr.dst].set_bool(stack[instr.src_a].as_int() > stack[instr.src_b].as_int());
                break;
            case NexOp::INT_LTE:
                stack[instr.dst].set_bool(stack[instr.src_a].as_int() <= stack[instr.src_b].as_int());
                break;
            case NexOp::INT_GTE:
                stack[instr.dst].set_bool(stack[instr.src_a].as_int() >= stack[instr.src_b].as_int());
                break;

            // ── Float Arithmetic ───────────────────────────────────────
            case NexOp::FLOAT_ADD:
                stack[instr.dst].set_float(stack[instr.src_a].as_float() + stack[instr.src_b].as_float());
                break;
            case NexOp::FLOAT_SUB:
                stack[instr.dst].set_float(stack[instr.src_a].as_float() - stack[instr.src_b].as_float());
                break;
            case NexOp::FLOAT_MUL:
                stack[instr.dst].set_float(stack[instr.src_a].as_float() * stack[instr.src_b].as_float());
                break;
            case NexOp::FLOAT_DIV: {
                double b = stack[instr.src_b].as_float();
                stack[instr.dst].set_float(b != 0.0 ? stack[instr.src_a].as_float() / b : 0.0);
            } break;
            case NexOp::FLOAT_POW: {
                double a = stack[instr.src_a].as_float();
                double b = stack[instr.src_b].as_float();
                stack[instr.dst].set_float(pow(a, b));
            } break;
            case NexOp::FLOAT_NEG:
                stack[instr.dst].set_float(-stack[instr.src_a].as_float());
                break;
            case NexOp::FLOAT_EQ:
                stack[instr.dst].set_bool(stack[instr.src_a].as_float() == stack[instr.src_b].as_float());
                break;
            case NexOp::FLOAT_NEQ:
                stack[instr.dst].set_bool(stack[instr.src_a].as_float() != stack[instr.src_b].as_float());
                break;
            case NexOp::FLOAT_LT:
                stack[instr.dst].set_bool(stack[instr.src_a].as_float() < stack[instr.src_b].as_float());
                break;
            case NexOp::FLOAT_GT:
                stack[instr.dst].set_bool(stack[instr.src_a].as_float() > stack[instr.src_b].as_float());
                break;
            case NexOp::FLOAT_LTE:
                stack[instr.dst].set_bool(stack[instr.src_a].as_float() <= stack[instr.src_b].as_float());
                break;
            case NexOp::FLOAT_GTE:
                stack[instr.dst].set_bool(stack[instr.src_a].as_float() >= stack[instr.src_b].as_float());
                break;

            case NexOp::INT_TO_FLOAT:
                stack[instr.dst].set_float((double)stack[instr.src_a].as_int());
                break;
            case NexOp::FLOAT_TO_INT:
                stack[instr.dst].set_int((int64_t)stack[instr.src_a].as_float());
                break;

            // ── Boolean ───────────────────────────────────────────────
            case NexOp::BOOL_AND:
                stack[instr.dst].set_bool(stack[instr.src_a].as_bool() && stack[instr.src_b].as_bool());
                break;
            case NexOp::BOOL_OR:
                stack[instr.dst].set_bool(stack[instr.src_a].as_bool() || stack[instr.src_b].as_bool());
                break;
            case NexOp::BOOL_NOT:
                stack[instr.dst].set_bool(!stack[instr.src_a].as_bool());
                break;

            // ── String ────────────────────────────────────────────────
            case NexOp::STR_CONCAT: {
                String a = stack[instr.src_a].as_str() ? *stack[instr.src_a].as_str() : String();
                String b = stack[instr.src_b].as_str() ? *stack[instr.src_b].as_str() : String();
                String *result = new String(a + b);
                stack[instr.dst].set_str(result);
            } break;
            case NexOp::STR_EQ: {
                String a = stack[instr.src_a].as_str() ? *stack[instr.src_a].as_str() : String();
                String b = stack[instr.src_b].as_str() ? *stack[instr.src_b].as_str() : String();
                stack[instr.dst].set_bool(a == b);
            } break;
            case NexOp::STR_NEQ: {
                String a = stack[instr.src_a].as_str() ? *stack[instr.src_a].as_str() : String();
                String b = stack[instr.src_b].as_str() ? *stack[instr.src_b].as_str() : String();
                stack[instr.dst].set_bool(a != b);
            } break;

            // ── Struct ────────────────────────────────────────────────
            case NexOp::STRUCT_ALLOC: {
                int64_t size = instr.imm_int > 0 ? instr.imm_int : 64;
                void *mem = memalloc(size);
                memset(mem, 0, size);
                stack[instr.dst].set_struct(mem);
            } break;

            case NexOp::LOAD_FIELD_INT: {
                char *base = (char *)stack[instr.src_a].as_struct();
                if (base) stack[instr.dst].set_int(*(int64_t *)(base + instr.field_offset));
            } break;
            case NexOp::LOAD_FIELD_FLOAT: {
                char *base = (char *)stack[instr.src_a].as_struct();
                if (base) stack[instr.dst].set_float(*(double *)(base + instr.field_offset));
            } break;
            case NexOp::LOAD_FIELD_VEC2: {
                char *base = (char *)stack[instr.src_a].as_struct();
                if (base) stack[instr.dst].set_vec2(*(Vector2 *)(base + instr.field_offset));
            } break;
            case NexOp::STORE_FIELD_INT: {
                char *base = (char *)stack[instr.dst].as_struct();
                if (base) *(int64_t *)(base + instr.field_offset) = stack[instr.src_a].as_int();
            } break;
            case NexOp::STORE_FIELD_FLOAT: {
                char *base = (char *)stack[instr.dst].as_struct();
                if (base) *(double *)(base + instr.field_offset) = stack[instr.src_a].as_float();
            } break;
            case NexOp::STORE_FIELD_VEC2: {
                char *base = (char *)stack[instr.dst].as_struct();
                if (base) *(Vector2 *)(base + instr.field_offset) = stack[instr.src_a].as_vec2();
            } break;

            // ── Arrays ────────────────────────────────────────────────
            case NexOp::ARRAY_NEW: {
                Array *arr = new Array();
                stack[instr.dst].set_struct((void *)arr);
                stack[instr.dst].kind = SlotKind::OBJECT;
            } break;
            case NexOp::ARRAY_LEN: {
                // Stored as Variant Array pointer
                if (stack[instr.src_a].kind == SlotKind::OBJECT) {
                    stack[instr.dst].set_int(0); // simplified
                } else {
                    stack[instr.dst].set_int(0);
                }
            } break;

            // ── Control flow ──────────────────────────────────────────
            case NexOp::JUMP:
                if (instr.label >= 0 && instr.label < fn->instructions.size()) {
                    ip = fn->instructions.ptr() + instr.label;
                    continue;
                }
                break;

            case NexOp::JUMP_IF_FALSE:
                if (instr.src_a >= 0 && !stack[instr.src_a].as_bool()) {
                    if (instr.label >= 0 && instr.label < fn->instructions.size()) {
                        ip = fn->instructions.ptr() + instr.label;
                        continue;
                    }
                }
                break;

            case NexOp::JUMP_IF_TRUE:
                if (instr.src_a >= 0 && stack[instr.src_a].as_bool()) {
                    if (instr.label >= 0 && instr.label < fn->instructions.size()) {
                        ip = fn->instructions.ptr() + instr.label;
                        continue;
                    }
                }
                break;

            case NexOp::JUMP_TABLE: {
                // O(1) match dispatch for integer values
                int64_t val = stack[instr.src_a].as_int();
                int target = instr.label; // default
                if (val >= 0 && val < instr.jump_table.size()) {
                    target = instr.jump_table[val];
                }
                if (target >= 0 && target < fn->instructions.size()) {
                    ip = fn->instructions.ptr() + target;
                    continue;
                }
            } break;

            case NexOp::RETURN:
                if (instr.src_a >= 0) {
                    stack[0] = stack[instr.src_a];
                }
                return;

            case NexOp::RETURN_VOID:
                return;

            // ── Calls ─────────────────────────────────────────────────
            case NexOp::CALL_BUILTIN:
                call_builtin(instr.call_name, stack, instr.dst, instr.src_a, instr.src_b);
                break;

            case NexOp::CALL_DIRECT:
                // Full call requires function lookup — simplified here
                break;

            case NexOp::CALL_GODOT_METHOD:
                // Godot interop call — handled by script instance
                break;

            // ── Option ────────────────────────────────────────────────
            case NexOp::OPTION_SOME: {
                // Store bool=true flag in dst, value in src_a
                // Simplified: just copy src to dst
                if (instr.src_a >= 0) stack[instr.dst] = stack[instr.src_a];
                stack[instr.dst].kind = SlotKind::INT64; // mark as some with value
            } break;
            case NexOp::OPTION_NONE:
                stack[instr.dst] = TypedSlot();
                break;
            case NexOp::OPTION_IS_SOME:
                stack[instr.dst].set_bool(stack[instr.src_a].kind != SlotKind::EMPTY);
                break;
            case NexOp::OPTION_UNWRAP:
                if (instr.src_a >= 0) stack[instr.dst] = stack[instr.src_a];
                break;

            // ── Result ────────────────────────────────────────────────
            case NexOp::RESULT_OK:
                if (instr.src_a >= 0) stack[instr.dst] = stack[instr.src_a];
                stack[instr.dst].data.b = true; // is_ok flag
                break;
            case NexOp::RESULT_ERR:
                if (instr.src_a >= 0) stack[instr.dst] = stack[instr.src_a];
                stack[instr.dst].data.b = false;
                break;
            case NexOp::RESULT_IS_OK:
                stack[instr.dst].set_bool(instr.src_a >= 0 ? stack[instr.src_a].data.b : false);
                break;
            case NexOp::RESULT_UNWRAP_OK:
                if (instr.src_a >= 0) stack[instr.dst] = stack[instr.src_a];
                break;
            case NexOp::RESULT_UNWRAP_ERR:
                if (instr.src_a >= 0) stack[instr.dst] = stack[instr.src_a];
                break;
            case NexOp::RESULT_PROPAGATE: {
                // ? operator: if not ok, return err
                bool is_ok = instr.src_a >= 0 ? stack[instr.src_a].data.b : false;
                if (!is_ok) {
                    if (instr.src_b >= 0) stack[0] = stack[instr.src_b];
                    return;
                }
            } break;

            // ── Godot interop ─────────────────────────────────────────
            case NexOp::TO_VARIANT:
                // Already handled at API boundaries
                break;
            case NexOp::FROM_VARIANT:
                // Type conversion at API boundaries
                break;

            case NexOp::NOP:
                break;

            case NexOp::END:
                return;

            default:
                break;
        }
        ip++;
    }
}

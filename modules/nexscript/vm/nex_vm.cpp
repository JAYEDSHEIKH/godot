#include "nex_vm.h"
#include "core/io/resource.h"
#include "core/math/math_funcs.h"
#include "core/object/object.h"
#include "core/os/os.h"
#include "godot/nex_instance.h"
#include "godot/nex_script.h"

Variant NexVM::execute(
    NexFunction        *p_fn,
    NexScriptInstance  *p_instance,
    const Variant     **p_args,
    int                 p_arg_count,
    Callable::CallError &r_error
) {
    return vm_execute_fn(p_fn, p_instance, p_args, p_arg_count, r_error);
}

Variant NexVM::vm_execute_fn(
    NexFunction        *p_fn,
    NexScriptInstance  *p_instance,
    const Variant     **p_args,
    int                 p_arg_count,
    Callable::CallError &r_error
) {
    if (!p_fn || p_fn->slot_count == 0) return Variant();

    TypedStack stack;
    stack.allocate(p_fn->slot_count + 64);

    for (int i = 0; i < p_arg_count && i < p_fn->param_count; i++) {
        if (p_args[i]) {
            NexType param_type = (i < p_fn->slot_types.size()) ? p_fn->slot_types[i] : NexType::make_variant();
            stack[i].from_variant(*p_args[i], param_type);
        }
    }

    dispatch(p_fn, stack, p_instance);

    Variant ret;
    if (p_fn->slot_count > 0 && stack[0].kind != SlotKind::EMPTY) {
        ret = stack[0].to_variant();
    }
    stack.free_slots();
    return ret;
}

// Fix 6 — Full standard-library builtins
void NexVM::call_builtin(const StringName &name, TypedStack &stack, int dst, int src_a, int src_b) {
    // ── Output ─────────────────────────────────────────────────────────────
    if (name == "print" || name == "println") {
        if (src_a >= 0) print_line(stack[src_a].to_variant().operator String());
        return;
    }
    if (name == "print_err" || name == "printerr") {
        if (src_a >= 0) print_error(stack[src_a].to_variant().operator String());
        return;
    }

    // ── Type conversion ────────────────────────────────────────────────────
    if (name == "str") {
        Variant v = (src_a >= 0) ? stack[src_a].to_variant() : Variant();
        stack[dst].set_str(new String(v.operator String()));
        return;
    }
    if (name == "int") {
        Variant v = (src_a >= 0) ? stack[src_a].to_variant() : Variant();
        stack[dst].set_int((int64_t)v);
        return;
    }
    if (name == "float") {
        Variant v = (src_a >= 0) ? stack[src_a].to_variant() : Variant();
        stack[dst].set_float((double)v);
        return;
    }
    if (name == "bool") {
        Variant v = (src_a >= 0) ? stack[src_a].to_variant() : Variant();
        stack[dst].set_bool((bool)v);
        return;
    }

    // ── Math ───────────────────────────────────────────────────────────────
    if (name == "abs") {
        if (src_a >= 0) {
            if (stack[src_a].kind == SlotKind::INT64)
                stack[dst].set_int(Math::abs(stack[src_a].as_int()));
            else
                stack[dst].set_float(Math::abs(stack[src_a].as_float()));
        }
        return;
    }
    if (name == "sign") {
        if (src_a >= 0) {
            if (stack[src_a].kind == SlotKind::INT64)
                stack[dst].set_int(SIGN(stack[src_a].as_int()));
            else
                stack[dst].set_float(SIGN(stack[src_a].as_float()));
        }
        return;
    }
    if (name == "floor") {
        if (src_a >= 0) stack[dst].set_float(Math::floor(stack[src_a].as_float()));
        return;
    }
    if (name == "ceil") {
        if (src_a >= 0) stack[dst].set_float(Math::ceil(stack[src_a].as_float()));
        return;
    }
    if (name == "round") {
        if (src_a >= 0) stack[dst].set_float(Math::round(stack[src_a].as_float()));
        return;
    }
    if (name == "sqrt") {
        if (src_a >= 0) stack[dst].set_float(Math::sqrt(stack[src_a].as_float()));
        return;
    }
    if (name == "pow") {
        if (src_a >= 0 && src_b >= 0)
            stack[dst].set_float(Math::pow(stack[src_a].as_float(), stack[src_b].as_float()));
        return;
    }
    if (name == "log") {
        if (src_a >= 0) stack[dst].set_float(Math::log(stack[src_a].as_float()));
        return;
    }
    if (name == "exp") {
        if (src_a >= 0) stack[dst].set_float(Math::exp(stack[src_a].as_float()));
        return;
    }
    if (name == "sin") {
        if (src_a >= 0) stack[dst].set_float(Math::sin(stack[src_a].as_float()));
        return;
    }
    if (name == "cos") {
        if (src_a >= 0) stack[dst].set_float(Math::cos(stack[src_a].as_float()));
        return;
    }
    if (name == "tan") {
        if (src_a >= 0) stack[dst].set_float(Math::tan(stack[src_a].as_float()));
        return;
    }
    if (name == "asin") {
        if (src_a >= 0) stack[dst].set_float(Math::asin(stack[src_a].as_float()));
        return;
    }
    if (name == "acos") {
        if (src_a >= 0) stack[dst].set_float(Math::acos(stack[src_a].as_float()));
        return;
    }
    if (name == "atan") {
        if (src_a >= 0) stack[dst].set_float(Math::atan(stack[src_a].as_float()));
        return;
    }
    if (name == "atan2") {
        if (src_a >= 0 && src_b >= 0)
            stack[dst].set_float(Math::atan2(stack[src_a].as_float(), stack[src_b].as_float()));
        return;
    }

    // ── Min / max ──────────────────────────────────────────────────────────
    if (name == "min") {
        if (src_a >= 0 && src_b >= 0) {
            if (stack[src_a].kind == SlotKind::INT64)
                stack[dst].set_int(MIN(stack[src_a].as_int(), stack[src_b].as_int()));
            else
                stack[dst].set_float(MIN(stack[src_a].as_float(), stack[src_b].as_float()));
        }
        return;
    }
    if (name == "max") {
        if (src_a >= 0 && src_b >= 0) {
            if (stack[src_a].kind == SlotKind::INT64)
                stack[dst].set_int(MAX(stack[src_a].as_int(), stack[src_b].as_int()));
            else
                stack[dst].set_float(MAX(stack[src_a].as_float(), stack[src_b].as_float()));
        }
        return;
    }

    // ── Randomness ─────────────────────────────────────────────────────────
    if (name == "randf") {
        stack[dst].set_float(Math::randf());
        return;
    }
    if (name == "randi") {
        stack[dst].set_int((int64_t)Math::rand());
        return;
    }
    if (name == "randf_range") {
        if (src_a >= 0 && src_b >= 0)
            stack[dst].set_float(Math::random(stack[src_a].as_float(), stack[src_b].as_float()));
        return;
    }
    if (name == "randi_range") {
        if (src_a >= 0 && src_b >= 0)
            stack[dst].set_int(Math::random((int64_t)stack[src_a].as_int(), (int64_t)stack[src_b].as_int()));
        return;
    }

    // ── Utility ────────────────────────────────────────────────────────────
    if (name == "len") {
        Variant v = (src_a >= 0) ? stack[src_a].to_variant() : Variant();
        stack[dst].set_int((int64_t)v.call("size"));
        return;
    }
    if (name == "typeof") {
        Variant v = (src_a >= 0) ? stack[src_a].to_variant() : Variant();
        stack[dst].set_int((int64_t)v.get_type());
        return;
    }
    if (name == "is_nan") {
        if (src_a >= 0) stack[dst].set_bool(Math::is_nan(stack[src_a].as_float()));
        return;
    }
    if (name == "is_inf") {
        if (src_a >= 0) stack[dst].set_bool(Math::is_inf(stack[src_a].as_float()));
        return;
    }
    if (name == "deg_to_rad") {
        if (src_a >= 0) stack[dst].set_float(Math::deg_to_rad(stack[src_a].as_float()));
        return;
    }
    if (name == "rad_to_deg") {
        if (src_a >= 0) stack[dst].set_float(Math::rad_to_deg(stack[src_a].as_float()));
        return;
    }
    if (name == "clamp") {
        // src_a = value, src_b = min slot, dst = max slot
        // IR gen must emit 3 args in consecutive slots
        // Uses src_a=value, src_b=min; caller puts max at src_b+1
        if (src_a >= 0 && src_b >= 0) {
            if (stack[src_a].kind == SlotKind::INT64)
                stack[dst].set_int(CLAMP(stack[src_a].as_int(), stack[src_b].as_int(), stack[src_b + 1].as_int()));
            else
                stack[dst].set_float(CLAMP(stack[src_a].as_float(), stack[src_b].as_float(), stack[src_b + 1].as_float()));
        }
        return;
    }

    WARN_PRINT(vformat("NexScript: unknown builtin function '%s'", name));
}

void NexVM::dispatch(NexFunction *fn, TypedStack &stack, NexScriptInstance *instance) {
    if (fn->instructions.is_empty()) return;

    const NexIRInstr *ip     = fn->instructions.ptr();
    const NexIRInstr *ip_end = ip + fn->instructions.size();

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
                stack[instr.dst].set_str(new String(fn->string_pool[instr.imm_str]));
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

            // ── Integer Arithmetic ──────────────────────────────────────────
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
                // Fix 16 — integer division returns integer
                int64_t b = stack[instr.src_b].as_int();
                stack[instr.dst].set_int(b != 0 ? stack[instr.src_a].as_int() / b : 0);
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

            // ── Float Arithmetic ────────────────────────────────────────────
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
                stack[instr.dst].set_float(Math::pow(stack[instr.src_a].as_float(), stack[instr.src_b].as_float()));
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

            // ── Boolean ────────────────────────────────────────────────────
            case NexOp::BOOL_AND:
                stack[instr.dst].set_bool(stack[instr.src_a].as_bool() && stack[instr.src_b].as_bool());
                break;
            case NexOp::BOOL_OR:
                stack[instr.dst].set_bool(stack[instr.src_a].as_bool() || stack[instr.src_b].as_bool());
                break;
            case NexOp::BOOL_NOT:
                stack[instr.dst].set_bool(!stack[instr.src_a].as_bool());
                break;

            // ── String ─────────────────────────────────────────────────────
            case NexOp::STR_CONCAT: {
                String a = stack[instr.src_a].as_str() ? *stack[instr.src_a].as_str() : String();
                String b = stack[instr.src_b].as_str() ? *stack[instr.src_b].as_str() : String();
                stack[instr.dst].set_str(new String(a + b));
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

            // ── Struct ─────────────────────────────────────────────────────
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

            // ── Arrays ─────────────────────────────────────────────────────
            case NexOp::ARRAY_NEW: {
                Array *arr = new Array();
                stack[instr.dst].set_struct((void *)arr);
                stack[instr.dst].kind = SlotKind::OBJECT;
            } break;
            case NexOp::ARRAY_LEN: {
                // Fix 17 — properly dereference the Array pointer
                if (stack[instr.src_a].kind == SlotKind::OBJECT) {
                    Array *arr = reinterpret_cast<Array *>(stack[instr.src_a].as_struct());
                    stack[instr.dst].set_int(arr ? (int64_t)arr->size() : 0);
                } else {
                    stack[instr.dst].set_int(0);
                }
            } break;

            // ── Control flow ───────────────────────────────────────────────
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
                int64_t val = stack[instr.src_a].as_int();
                int target = instr.label;
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

            // ── Calls ──────────────────────────────────────────────────────
            case NexOp::CALL_BUILTIN:
                call_builtin(instr.call_name, stack, instr.dst, instr.src_a, instr.src_b);
                break;

            // Fix 5 — CALL_DIRECT: call another NexScript function
            case NexOp::CALL_DIRECT: {
                NexFunction *callee = nullptr;
                if (instance && instance->get_script().is_valid()) {
                    Ref<NexScript> nex_script = instance->get_script();
                    callee = nex_script->get_function(instr.call_name);
                }
                if (callee) {
                    int arg_count = instr.src_b;
                    Vector<Variant>         arg_variants;
                    Vector<const Variant *> arg_ptrs;
                    arg_variants.resize(arg_count);
                    arg_ptrs.resize(arg_count);
                    for (int i = 0; i < arg_count; i++) {
                        arg_variants.write[i] = stack[instr.src_a + i].to_variant();
                        arg_ptrs.write[i]     = &arg_variants[i];
                    }
                    Callable::CallError sub_err;
                    Variant result = vm_execute_fn(callee, instance, arg_ptrs.ptr(), arg_count, sub_err);
                    if (instr.dst >= 0) {
                        stack[instr.dst].from_variant(result, callee->return_type);
                    }
                }
            } break;

            // Fix 5 — CALL_GODOT_METHOD: call a method on a Godot Object
            case NexOp::CALL_GODOT_METHOD: {
                Object *obj = stack[instr.src_a].as_object();
                if (obj) {
                    int arg_count = instr.src_b;
                    Vector<Variant>         arg_variants;
                    Vector<const Variant *> arg_ptrs;
                    arg_variants.resize(arg_count);
                    arg_ptrs.resize(arg_count);
                    for (int i = 0; i < arg_count; i++) {
                        arg_variants.write[i] = stack[instr.src_a + 1 + i].to_variant();
                        arg_ptrs.write[i]     = &arg_variants[i];
                    }
                    Callable::CallError call_err;
                    Variant result;
                    obj->callp(instr.call_name, arg_ptrs.ptr(), arg_count, result, call_err);
                    if (instr.dst >= 0) {
                        stack[instr.dst].from_variant(result, NexType::make_variant());
                    }
                }
            } break;

            // ── Option ─────────────────────────────────────────────────────
            case NexOp::OPTION_SOME: {
                if (instr.src_a >= 0) stack[instr.dst] = stack[instr.src_a];
                stack[instr.dst].kind = SlotKind::INT64;
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

            // ── Result ─────────────────────────────────────────────────────
            case NexOp::RESULT_OK:
                if (instr.src_a >= 0) stack[instr.dst] = stack[instr.src_a];
                stack[instr.dst].data.b = true;
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
                bool is_ok = instr.src_a >= 0 ? stack[instr.src_a].data.b : false;
                if (!is_ok) {
                    if (instr.src_b >= 0) stack[0] = stack[instr.src_b];
                    return;
                }
            } break;

            // ── Godot interop ──────────────────────────────────────────────
            case NexOp::TO_VARIANT:
                break;
            case NexOp::FROM_VARIANT:
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

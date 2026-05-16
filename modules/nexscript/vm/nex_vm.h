#pragma once
#include "nex_typed_slot.h"
#include "modules/nexscript/ir/nex_ir.h"
#include "core/variant/variant.h"
#include "core/object/callable.h"

class NexScriptInstance;

class NexVM {
public:
    Variant execute(
        NexFunction        *p_fn,
        NexScriptInstance  *p_instance,
        const Variant     **p_args,
        int                 p_arg_count,
        Callable::CallError &r_error
    );

private:
    void dispatch(NexFunction *fn, TypedStack &stack, NexScriptInstance *instance);
    void call_builtin(const StringName &name, TypedStack &stack, int dst, int src_a, int src_b);
};

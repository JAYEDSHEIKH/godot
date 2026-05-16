#include "nex_instance.h"
#include "nex_language.h"
#include "modules/nexscript/vm/nex_vm.h"
#include "scene/main/node.h"

NexScriptInstance::NexScriptInstance(Object *p_owner, const Ref<NexScript> &p_script)
    : owner(p_owner), script(p_script) {
    // Initialize member slots with default values for exports
    for (const NexScript::ExportVar &ev : script->exports) {
        TypedSlot slot;
        slot.from_variant(ev.default_value, ev.type);
        member_slots[ev.name] = slot;
    }
}

NexScriptInstance::~NexScriptInstance() {}

bool NexScriptInstance::set(const StringName &p_name, const Variant &p_value) {
    if (member_slots.has(p_name)) {
        // Determine type from exports
        for (const NexScript::ExportVar &ev : script->exports) {
            if (ev.name == p_name) {
                member_slots[p_name].from_variant(p_value, ev.type);
                return true;
            }
        }
        member_slots[p_name].from_variant(p_value, NexType::make_variant());
        return true;
    }
    // Create new slot
    TypedSlot slot;
    slot.from_variant(p_value, NexType::make_variant());
    member_slots[p_name] = slot;
    return true;
}

bool NexScriptInstance::get(const StringName &p_name, Variant &r_ret) const {
    if (member_slots.has(p_name)) {
        r_ret = member_slots[p_name].to_variant();
        return true;
    }
    return false;
}

void NexScriptInstance::get_property_list(List<PropertyInfo> *p_properties) const {
    script->get_script_property_list(p_properties);
}

Variant::Type NexScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
    for (const NexScript::ExportVar &ev : script->exports) {
        if (ev.name == p_name) {
            if (r_is_valid) *r_is_valid = true;
            return ev.property_info.type;
        }
    }
    if (r_is_valid) *r_is_valid = false;
    return Variant::NIL;
}

void NexScriptInstance::call(
    const StringName &p_method,
    const Variant **p_args, int p_argcount,
    Variant &r_ret, Callable::CallError &r_error
) {
    NexFunction *fn = script->get_function(p_method);
    if (!fn) {
        r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
        return;
    }
    NexVM vm;
    r_ret = vm.execute(fn, this, p_args, p_argcount, r_error);
}

void NexScriptInstance::notification(int p_notification, bool p_reversed) {
    Callable::CallError err;

    switch (p_notification) {
        case Node::NOTIFICATION_READY: {
            if (script->has_method(StringName("_ready"))) {
                Variant ret;
                call(StringName("_ready"), nullptr, 0, ret, err);
            }
        } break;
        case Node::NOTIFICATION_PROCESS: {
            if (script->has_method(StringName("_process"))) {
                // dt passed separately in _process override
            }
        } break;
        case Node::NOTIFICATION_PHYSICS_PROCESS: {
            if (script->has_method(StringName("_physics_process"))) {
            }
        } break;
        case Node::NOTIFICATION_EXIT_TREE: {
            if (script->has_method(StringName("_exit_tree"))) {
                Variant ret;
                call(StringName("_exit_tree"), nullptr, 0, ret, err);
            }
        } break;
    }
}

ScriptLanguage *NexScriptInstance::get_language() {
    return NexLanguage::get_singleton();
}

bool NexScriptInstance::has_method(const StringName &p_method) const {
    return script->has_method(p_method);
}

void NexScriptInstance::get_method_list(List<MethodInfo> *p_list) const {
    script->get_script_method_list(p_list);
}

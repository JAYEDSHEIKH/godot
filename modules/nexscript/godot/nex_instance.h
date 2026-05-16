#pragma once
#include "core/object/script_language.h"
#include "modules/nexscript/vm/nex_typed_slot.h"
#include "nex_script.h"

class NexScriptInstance : public ScriptInstance {
    Object        *owner  = nullptr;
    Ref<NexScript> script;

    HashMap<StringName, TypedSlot> member_slots;

public:
    NexScriptInstance(Object *p_owner, const Ref<NexScript> &p_script);
    ~NexScriptInstance() override;

    bool set(const StringName &p_name, const Variant &p_value) override;
    bool get(const StringName &p_name, Variant &r_ret) const override;
    void get_property_list(List<PropertyInfo> *p_properties) const override;
    Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const override;
    void validate_property(PropertyInfo &p_property) const override {}

    bool property_can_revert(const StringName &p_name) const override { return false; }
    bool property_get_revert(const StringName &p_name, Variant &r_ret) const override { return false; }

    Object *get_owner() override { return owner; }

    void call(
        const StringName &p_method,
        const Variant **p_args, int p_argcount,
        Variant &r_ret, Callable::CallError &r_error
    ) override;

    void notification(int p_notification, bool p_reversed = false) override;

    String to_string(bool *r_valid) override { if (r_valid) *r_valid = false; return ""; }

    void refcount_incremented() override {}
    bool refcount_decremented() override { return false; }

    Ref<Script> get_script() const override { return script; }

    bool is_placeholder() const override { return false; }

    void property_set_fallback(const StringName &p_name, const Variant &p_value, bool *r_valid) override {}
    Variant property_get_fallback(const StringName &p_name, bool *r_valid) override { return Variant(); }

    ScriptLanguage *get_language() override;

    bool has_method(const StringName &p_method) const override;
    void get_method_list(List<MethodInfo> *p_list) const override;
};

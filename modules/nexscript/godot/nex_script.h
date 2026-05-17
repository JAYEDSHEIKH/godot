#pragma once
#include "core/object/script_language.h"
#include "modules/nexscript/ir/nex_ir.h"
#include "modules/nexscript/frontend/nex_analyzer.h"

class NexScript : public Script {
    GDCLASS(NexScript, Script);

    String     source_code;
    String     file_path;
    bool       is_compiled = false;
    StringName base_class;

    HashMap<StringName, NexFunction *> functions;
    HashMap<StringName, NexAnalyzer::StructInfo> structs;

public:
    struct ExportVar {
        StringName   name;
        NexType      type;
        Variant      default_value;
        PropertyInfo property_info;
    };

    struct SignalDecl {
        StringName name;
        MethodInfo method_info;
    };

    Vector<ExportVar> exports;
    Vector<SignalDecl> signals_list;

    NexScript() {}
    ~NexScript();

    Error compile();

    bool        can_instantiate() const override;
    StringName  get_instance_base_type() const override { return base_class; }
    ScriptInstance *instance_create(Object *p_this) override;

    PlaceholderScriptInstance *placeholder_instance_create(Object *p_this) override {
        PlaceholderScriptInstance *placeholder = memnew(PlaceholderScriptInstance(get_language(), Ref<Script>(this), p_this));
        return placeholder;
    }

    void get_script_property_list(List<PropertyInfo> *p_list) const override;
    void get_script_signal_list(List<MethodInfo> *p_list) const override;
    void get_script_method_list(List<MethodInfo> *p_list) const override;

    bool has_method(const StringName &p_method) const override;
    MethodInfo get_method_info(const StringName &p_method) const override;

    Error reload(bool p_keep_state) override;
    bool is_valid() const override { return is_compiled; }
    bool is_tool() const override { return false; }
    bool is_abstract() const override { return false; }

    bool inherits_script(const Ref<Script> &p_script) const override { return false; }

    Ref<Script> get_base_script() const override { return Ref<Script>(); }
    StringName get_global_name() const override { return StringName(); }

    bool has_source_code() const override { return !source_code.is_empty(); }
    String get_source_code() const override { return source_code; }
    void set_source_code(const String &p_code) override {
        source_code = p_code;
        is_compiled = false;
    }

    void set_path(const String &p_path, bool p_take_over = false) override {
        file_path = p_path;
        Resource::set_path(p_path, p_take_over);
    }

    String get_file_path() const { return file_path; }

    void get_constants(HashMap<StringName, Variant> *p_constants) override {}
    void get_members(HashSet<StringName> *p_members) override {}

    bool is_placeholder_fallback_enabled() const override { return false; }

    const Variant get_rpc_config() const override { return Variant(); }

    ScriptLanguage *get_language() const override;

    NexFunction *get_function(const StringName &name) {
        if (functions.has(name)) return functions[name];
        return nullptr;
    }

    const HashMap<StringName, NexAnalyzer::StructInfo> &get_structs() const { return structs; }

protected:
    static void _bind_methods();
};

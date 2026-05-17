#pragma once
#include "core/object/script_language.h"
#include "core/templates/hash_map.h"
#include "core/variant/variant.h"

class NexLanguage : public ScriptLanguage {
    static NexLanguage *singleton;

    HashMap<StringName, Variant> global_constants;
    HashMap<StringName, Variant> named_global_constants;

public:
    static NexLanguage *get_singleton() { return singleton; }

    NexLanguage();
    ~NexLanguage() override;

    String get_name() const override { return "NEX"; }
    String get_type() const override { return "NexScript"; }
    String get_extension() const override { return "nex"; }

    void init() override;
    void finish() override;

    void get_reserved_words(List<String> *p_words) const override;
    void get_comment_delimiters(List<String> *p_delimiters) const override;
    void get_string_delimiters(List<String> *p_delimiters) const override;

    Ref<Script> make_template(
        const String &p_template,
        const String &p_class_name,
        const String &p_base_class_name
    ) const override;

    bool validate(
        const String &p_script, const String &p_path,
        List<String> *r_functions,
        List<ScriptLanguage::ScriptError> *r_errors,
        List<ScriptLanguage::Warning> *r_warnings,
        HashSet<int> *r_safe_lines
    ) const override;

    Script *create_script() const override;

    bool has_named_classes() const override { return false; }
    bool supports_builtin_mode() const override { return true; }
    bool can_inherit_from_file() const override { return false; }

    int find_function(const String &p_function, const String &p_code) const override { return -1; }
    String make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const override { return ""; }

    Error complete_code(const String &p_code, const String &p_path, Object *p_owner, List<ScriptLanguage::CodeCompletionOption> *r_options, bool &r_forced, String &r_call_hint) override { return ERR_UNAVAILABLE; }

    void auto_indent_code(String &p_code, int p_from_line, int p_to_line) const override {}

    void add_global_constant(const StringName &p_variable, const Variant &p_value) override;
    void add_named_global_constant(const StringName &p_name, const Variant &p_value) override;
    void remove_named_global_constant(const StringName &p_name) override;

    const Variant *get_global_constant(const StringName &p_name) const;

    void thread_enter() override {}
    void thread_exit() override {}
    String debug_get_error() const override { return ""; }
    int debug_get_stack_level_count() const override { return 0; }
    int debug_get_stack_level_line(int p_level) const override { return 0; }
    String debug_get_stack_level_function(int p_level) const override { return ""; }
    Dictionary debug_get_stack_level_locals(int p_level, int p_max_subitems, int p_max_depth) override { return {}; }
    Dictionary debug_get_stack_level_members(int p_level, int p_max_subitems, int p_max_depth) override { return {}; }
    void debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) override {}
    String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems, int p_max_depth) override { return ""; }
    void reload_all_scripts() override {}
    void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) override {}
    void get_recognized_extensions(List<String> *p_extensions) const override;
    void get_public_functions(List<MethodInfo> *p_functions) const override {}
    void get_public_constants(List<Pair<String, Variant>> *p_constants) const override {}
    void get_public_annotations(List<MethodInfo> *p_annotations) const override {}
    void profiling_start() override {}
    void profiling_stop() override {}
    int profiling_get_accumulated_data(ScriptLanguage::ProfilingInfo *p_info_arr, int p_info_max) override { return 0; }
    int profiling_get_frame_data(ScriptLanguage::ProfilingInfo *p_info_arr, int p_info_max) override { return 0; }
    void frame() override {}

    bool handles_global_class_type(const String &p_type) const override { return p_type == "NexScript"; }
    void get_global_class_name(const String &p_path, String *r_name, String *r_base_type) const override;
};

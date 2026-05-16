#pragma once
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"
#include "core/templates/vector.h"

class AdvancedPrompting {
public:
    static String build_system_prompt(
        const String          &p_project_name,
        const String          &p_godot_version,
        const Vector<String>  &p_available_tools,
        const String          &p_tone = "Analytical"
    );

    static String build_user_prompt(
        const String          &p_user_message,
        const Dictionary      &p_context_blocks,
        const Vector<String>  &p_recent_facts,
        const Vector<String>  &p_relevant_code_snippets,
        int p_token_budget = 6000
    );

    static String handle_slash_command(
        const String          &p_command,
        const String          &p_args,
        const Dictionary      &p_context
    );

    static String get_code_generation_examples();
    static String get_debugging_examples();
    static String get_architecture_examples();

private:
    static String _format_context_for_llm(const Dictionary &p_blocks, int p_max_tokens);
    static String _rank_and_filter_context(const Vector<String> &p_blocks, int p_max_tokens);
};

#include "advanced_prompting.h"

String AdvancedPrompting::build_system_prompt(
    const String         &p_project_name,
    const String         &p_godot_version,
    const Vector<String> &p_available_tools,
    const String         &p_tone
) {
    String prompt = "You are a first-class AI Copilot built directly into Godot " + p_godot_version + ".\n";
    prompt += "Project: " + p_project_name + "\n\n";
    prompt += "Your role is to help the developer create, debug, and optimize their game.\n";
    prompt += "You have access to the full project context and can execute tools to modify the project.\n\n";

    prompt += "Tone: " + p_tone + "\n\n";

    if (!p_available_tools.is_empty()) {
        prompt += "## Available Tools\n";
        for (const String &tool : p_available_tools) {
            prompt += "- " + tool + "\n";
        }
        prompt += "\n";
    }

    prompt += "## Guidelines\n";
    prompt += "- Always use the most precise tool available rather than writing instructions\n";
    prompt += "- When creating code, prefer GDScript 4.x or NEX syntax\n";
    prompt += "- Explain what you're doing before executing destructive operations\n";
    prompt += "- Reference specific nodes, scripts, and scene paths when discussing the project\n";
    prompt += "- When fixing bugs, explain the root cause and the fix\n\n";

    prompt += get_code_generation_examples();

    return prompt;
}

String AdvancedPrompting::build_user_prompt(
    const String         &p_user_message,
    const Dictionary     &p_context_blocks,
    const Vector<String> &p_recent_facts,
    const Vector<String> &p_relevant_code_snippets,
    int p_token_budget
) {
    String prompt;

    if (!p_context_blocks.is_empty()) {
        prompt += "## Context\n";
        prompt += _format_context_for_llm(p_context_blocks, p_token_budget / 2);
        prompt += "\n";
    }

    if (!p_recent_facts.is_empty()) {
        prompt += "## Memory\n";
        for (int i = 0; i < p_recent_facts.size() && i < 5; i++) {
            prompt += "- " + p_recent_facts[i] + "\n";
        }
        prompt += "\n";
    }

    if (!p_relevant_code_snippets.is_empty()) {
        prompt += "## Relevant Code\n";
        for (int i = 0; i < p_relevant_code_snippets.size() && i < 3; i++) {
            prompt += "```\n" + p_relevant_code_snippets[i] + "\n```\n";
        }
        prompt += "\n";
    }

    prompt += "## Request\n" + p_user_message;
    return prompt;
}

String AdvancedPrompting::handle_slash_command(
    const String     &p_command,
    const String     &p_args,
    const Dictionary &p_context
) {
    if (p_command == "plan") {
        return "Create a detailed step-by-step implementation plan for: " + p_args + "\n"
               "Include: architecture decisions, files to create/modify, potential pitfalls.";
    }
    if (p_command == "debug") {
        return "Debug the following issue in my Godot project:\n" + p_args + "\n\n"
               "Analyze the scene tree and scripts to find the root cause.";
    }
    if (p_command == "brainstorm") {
        return "Brainstorm 5 different approaches for implementing: " + p_args + "\n"
               "For each approach, list pros, cons, and implementation complexity.";
    }
    if (p_command == "explain") {
        return "Explain in detail how this works: " + p_args + "\n"
               "Use the current project context to give specific examples.";
    }
    if (p_command == "refactor") {
        return "Refactor the following to be more maintainable and performant: " + p_args + "\n"
               "Show a diff with explanations of each change.";
    }
    if (p_command == "review") {
        return "Do a code review of: " + p_args + "\n"
               "Check for: bugs, performance issues, best practices, Godot idioms.";
    }
    if (p_command == "doc") {
        return "Generate comprehensive documentation for: " + p_args;
    }
    if (p_command == "test") {
        return "Write tests for: " + p_args + "\n"
               "Include edge cases and error conditions.";
    }
    // Default: treat as regular message
    return p_command + " " + p_args;
}

String AdvancedPrompting::get_code_generation_examples() {
    return
        "## Code Style Examples\n"
        "When writing GDScript 4.x:\n"
        "```gdscript\n"
        "extends CharacterBody2D\n\n"
        "@export var speed: float = 200.0\n\n"
        "func _physics_process(delta: float) -> void:\n"
        "\tvar direction := Input.get_axis(\"ui_left\", \"ui_right\")\n"
        "\tvelocity.x = direction * speed\n"
        "\tmove_and_slide()\n"
        "```\n\n";
}

String AdvancedPrompting::get_debugging_examples() {
    return
        "## Debugging Approach\n"
        "1. Read the error message carefully\n"
        "2. Check the scene tree for the failing node\n"
        "3. Review the script at the reported line\n"
        "4. Add print() statements to trace values\n"
        "5. Check signal connections\n\n";
}

String AdvancedPrompting::get_architecture_examples() {
    return
        "## Architecture Patterns for Godot\n"
        "- Use Autoloads for global state and services\n"
        "- Use Signals for loose coupling between nodes\n"
        "- Use Resources for data-driven design\n"
        "- Use State Machines for complex behavior\n\n";
}

String AdvancedPrompting::_format_context_for_llm(const Dictionary &p_blocks, int p_max_tokens) {
    String result;
    int used = 0;
    Array keys = p_blocks.keys();
    for (int i = 0; i < keys.size(); i++) {
        String key = keys[i];
        String val = p_blocks[key].operator String();
        int tokens = val.length() / 4;
        if (used + tokens > p_max_tokens) break;
        result += "\n### " + key + "\n" + val + "\n";
        used += tokens;
    }
    return result;
}

String AdvancedPrompting::_rank_and_filter_context(const Vector<String> &p_blocks, int p_max_tokens) {
    String result;
    int used = 0;
    for (const String &block : p_blocks) {
        int tokens = block.length() / 4;
        if (used + tokens > p_max_tokens) break;
        result += block + "\n";
        used += tokens;
    }
    return result;
}

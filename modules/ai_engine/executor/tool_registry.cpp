#include "tool_registry.h"

void ToolRegistry::register_tool_set(ToolSet *p_set) {
    p_set->register_tools(this);
}

void ToolRegistry::register_tool(const ToolDefinition &p_def, ToolSet *p_set) {
    definitions.push_back(p_def);
    tool_sets[p_def.name] = p_set;
    name_index[p_def.name] = &definitions[definitions.size() - 1];
}

bool ToolRegistry::execute(const String &p_name, const Dictionary &p_args, String &r_result) {
    if (!tool_sets.has(p_name)) {
        r_result = "Unknown tool: " + p_name;
        return false;
    }
    return tool_sets[p_name]->execute(p_name, p_args, r_result);
}

const ToolDefinition *ToolRegistry::get_definition(const String &p_name) const {
    for (int i = 0; i < definitions.size(); i++) {
        if (definitions[i].name == p_name) return &definitions[i];
    }
    return nullptr;
}

Vector<String> ToolRegistry::get_tool_names() const {
    Vector<String> names;
    for (const ToolDefinition &td : definitions) names.push_back(td.name);
    return names;
}

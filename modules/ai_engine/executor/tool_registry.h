#pragma once
#include "core/object/ref_counted.h"
#include "modules/ai_engine/core/ai_types.h"

class ToolSet;

class ToolRegistry : public RefCounted {
    GDCLASS(ToolRegistry, RefCounted);

    Vector<ToolDefinition>              definitions;
    HashMap<String, ToolSet *>          tool_sets;
    HashMap<String, const ToolDefinition *> name_index;

public:
    void register_tool_set(ToolSet *p_set);
    void register_tool(const ToolDefinition &p_def, ToolSet *p_set);

    bool execute(const String &p_name, const Dictionary &p_args, String &r_result);

    const ToolDefinition *get_definition(const String &p_name) const;
    const Vector<ToolDefinition> &get_all_definitions() const { return definitions; }
    Vector<String> get_tool_names() const;

protected:
    static void _bind_methods() {}
};

class ToolSet {
public:
    virtual ~ToolSet() {}
    virtual void register_tools(ToolRegistry *r) = 0;
    virtual bool execute(const String &p_name, const Dictionary &p_args, String &r_result) = 0;
};

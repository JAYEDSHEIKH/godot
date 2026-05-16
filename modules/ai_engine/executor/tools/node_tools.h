#pragma once
#include "modules/ai_engine/executor/tool_registry.h"

class NodeTools : public ToolSet {
public:
    void register_tools(ToolRegistry *r) override;
    bool execute(const String &p_name, const Dictionary &p_args, String &r_result) override;

private:
    String _get_scene_tree(const Dictionary &args);
    String _add_node(const Dictionary &args);
    String _delete_node(const Dictionary &args);
    String _set_node_property(const Dictionary &args);
    String _get_node_property(const Dictionary &args);
    String _reparent_node(const Dictionary &args);
    String _find_node_by_name(const Dictionary &args);
    String _get_node_signals(const Dictionary &args);
    String _connect_signal(const Dictionary &args);
};

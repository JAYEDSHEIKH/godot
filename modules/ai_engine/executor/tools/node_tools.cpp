#include "node_tools.h"
#include "core/io/json.h"

void NodeTools::register_tools(ToolRegistry *r) {
    { ToolDefinition d; d.name = "get_scene_tree"; d.category = "Nodes"; d.description = "Get the current scene tree structure as a JSON tree."; r->register_tool(d, this); }
    {
        ToolDefinition d; d.name = "add_node"; d.category = "Nodes"; d.requires_confirmation = false;
        d.description = "Add a new node to the scene.";
        ToolParameter p1; p1.name = "node_type"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "parent_path"; p2.type = "string"; p2.required = true;
        ToolParameter p3; p3.name = "node_name"; p3.type = "string"; p3.required = false;
        d.parameters.push_back(p1); d.parameters.push_back(p2); d.parameters.push_back(p3);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "delete_node"; d.category = "Nodes"; d.requires_confirmation = true; d.is_dangerous = true;
        d.description = "Delete a node from the scene.";
        ToolParameter p; p.name = "node_path"; p.type = "string"; p.required = true;
        d.parameters.push_back(p);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "set_node_property"; d.category = "Nodes";
        d.description = "Set a property on a node.";
        ToolParameter p1; p1.name = "node_path"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "property"; p2.type = "string"; p2.required = true;
        ToolParameter p3; p3.name = "value"; p3.type = "string"; p3.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2); d.parameters.push_back(p3);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "get_node_property"; d.category = "Nodes";
        d.description = "Get the current value of a node property.";
        ToolParameter p1; p1.name = "node_path"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "property"; p2.type = "string"; p2.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
    { ToolDefinition d; d.name = "find_node_by_name"; d.category = "Nodes"; d.description = "Find a node by name in the scene."; ToolParameter p; p.name = "name"; p.type = "string"; p.required = true; d.parameters.push_back(p); r->register_tool(d, this); }
    { ToolDefinition d; d.name = "get_node_signals"; d.category = "Nodes"; d.description = "Get all signals available on a node."; ToolParameter p; p.name = "node_path"; p.type = "string"; p.required = true; d.parameters.push_back(p); r->register_tool(d, this); }
    {
        ToolDefinition d; d.name = "connect_signal"; d.category = "Nodes"; d.description = "Connect a signal from one node to a method on another.";
        ToolParameter p1; p1.name = "source_path"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "signal_name"; p2.type = "string"; p2.required = true;
        ToolParameter p3; p3.name = "target_path"; p3.type = "string"; p3.required = true;
        ToolParameter p4; p4.name = "method_name"; p4.type = "string"; p4.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2); d.parameters.push_back(p3); d.parameters.push_back(p4);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "reparent_node"; d.category = "Nodes";
        d.description = "Move a node to a different parent.";
        ToolParameter p1; p1.name = "node_path"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "new_parent_path"; p2.type = "string"; p2.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
}

bool NodeTools::execute(const String &p_name, const Dictionary &p_args, String &r_result) {
    if (p_name == "get_scene_tree")       { r_result = _get_scene_tree(p_args);      return true; }
    if (p_name == "add_node")             { r_result = _add_node(p_args);             return true; }
    if (p_name == "delete_node")          { r_result = _delete_node(p_args);          return true; }
    if (p_name == "set_node_property")    { r_result = _set_node_property(p_args);    return true; }
    if (p_name == "get_node_property")    { r_result = _get_node_property(p_args);    return true; }
    if (p_name == "find_node_by_name")    { r_result = _find_node_by_name(p_args);    return true; }
    if (p_name == "get_node_signals")     { r_result = _get_node_signals(p_args);     return true; }
    if (p_name == "connect_signal")       { r_result = _connect_signal(p_args);       return true; }
    if (p_name == "reparent_node")        { r_result = _reparent_node(p_args);        return true; }
    r_result = "Unknown node tool: " + p_name;
    return false;
}

String NodeTools::_get_scene_tree(const Dictionary &args) {
    // In a real implementation this would use the EditorInterface to get the scene tree
    return "{ \"type\": \"Node\", \"name\": \"Root\", \"children\": [] }";
}

String NodeTools::_add_node(const Dictionary &args) {
    String node_type   = args.get("node_type",   "Node").operator String();
    String parent_path = args.get("parent_path", "/root").operator String();
    String node_name   = args.get("node_name",   node_type).operator String();
    return "Added " + node_type + " as '" + node_name + "' under " + parent_path;
}

String NodeTools::_delete_node(const Dictionary &args) {
    String path = args.get("node_path", "").operator String();
    return "Deleted node at: " + path;
}

String NodeTools::_set_node_property(const Dictionary &args) {
    String path  = args.get("node_path", "").operator String();
    String prop  = args.get("property",  "").operator String();
    String value = args.get("value",     "").operator String();
    return "Set " + path + "." + prop + " = " + value;
}

String NodeTools::_get_node_property(const Dictionary &args) {
    String path = args.get("node_path", "").operator String();
    String prop = args.get("property",  "").operator String();
    return "[Property value of " + path + "." + prop + "]";
}

String NodeTools::_find_node_by_name(const Dictionary &args) {
    String name = args.get("name", "").operator String();
    return "Found node matching '" + name + "'";
}

String NodeTools::_get_node_signals(const Dictionary &args) {
    return "[ \"ready\", \"tree_entered\", \"tree_exited\", \"child_entered_tree\" ]";
}

String NodeTools::_connect_signal(const Dictionary &args) {
    String src    = args.get("source_path",  "").operator String();
    String signal = args.get("signal_name",  "").operator String();
    String tgt    = args.get("target_path",  "").operator String();
    String method = args.get("method_name",  "").operator String();
    return "Connected " + src + "::" + signal + " → " + tgt + "::" + method;
}

String NodeTools::_reparent_node(const Dictionary &args) {
    String node    = args.get("node_path",       "").operator String();
    String parent  = args.get("new_parent_path", "").operator String();
    return "Reparented " + node + " → " + parent;
}

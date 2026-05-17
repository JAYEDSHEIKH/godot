#include "node_tools.h"
#include "core/io/json.h"
#include "scene/main/node.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_interface.h"
#include "editor/editor_undo_redo_manager.h"
#endif

void NodeTools::register_tools(ToolRegistry *r) {
    {
        ToolDefinition d; d.name = "get_scene_tree"; d.category = "Nodes";
        d.description = "Get the current scene tree structure as a text tree.";
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "add_node"; d.category = "Nodes";
        d.description = "Add a new node to the scene.";
        ToolParameter p1; p1.name = "node_type";   p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "parent_path"; p2.type = "string"; p2.required = true;
        ToolParameter p3; p3.name = "node_name";   p3.type = "string"; p3.required = false;
        d.parameters.push_back(p1); d.parameters.push_back(p2); d.parameters.push_back(p3);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "delete_node"; d.category = "Nodes";
        d.requires_confirmation = true; d.is_dangerous = true;
        d.description = "Delete a node from the scene.";
        ToolParameter p; p.name = "node_path"; p.type = "string"; p.required = true;
        d.parameters.push_back(p);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "set_node_property"; d.category = "Nodes";
        d.description = "Set a property on a node.";
        ToolParameter p1; p1.name = "node_path"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "property";  p2.type = "string"; p2.required = true;
        ToolParameter p3; p3.name = "value";     p3.type = "string"; p3.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2); d.parameters.push_back(p3);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "get_node_property"; d.category = "Nodes";
        d.description = "Get the current value of a node property.";
        ToolParameter p1; p1.name = "node_path"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "property";  p2.type = "string"; p2.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "find_node_by_name"; d.category = "Nodes";
        d.description = "Find a node by name in the scene.";
        ToolParameter p; p.name = "name"; p.type = "string"; p.required = true;
        d.parameters.push_back(p);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "get_node_signals"; d.category = "Nodes";
        d.description = "Get all signals available on a node.";
        ToolParameter p; p.name = "node_path"; p.type = "string"; p.required = true;
        d.parameters.push_back(p);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "connect_signal"; d.category = "Nodes";
        d.description = "Connect a signal from one node to a method on another.";
        ToolParameter p1; p1.name = "source_path"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "signal_name"; p2.type = "string"; p2.required = true;
        ToolParameter p3; p3.name = "target_path"; p3.type = "string"; p3.required = true;
        ToolParameter p4; p4.name = "method_name"; p4.type = "string"; p4.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        d.parameters.push_back(p3); d.parameters.push_back(p4);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "reparent_node"; d.category = "Nodes";
        d.description = "Move a node to a different parent.";
        ToolParameter p1; p1.name = "node_path";      p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "new_parent_path"; p2.type = "string"; p2.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
}

bool NodeTools::execute(const String &p_name, const Dictionary &p_args, String &r_result) {
    if (p_name == "get_scene_tree")    { r_result = _get_scene_tree(p_args);    return true; }
    if (p_name == "add_node")          { r_result = _add_node(p_args);           return !r_result.begins_with("[Error"); }
    if (p_name == "delete_node")       { r_result = _delete_node(p_args);        return !r_result.begins_with("[Error"); }
    if (p_name == "set_node_property") { r_result = _set_node_property(p_args);  return !r_result.begins_with("[Error"); }
    if (p_name == "get_node_property") { r_result = _get_node_property(p_args);  return true; }
    if (p_name == "find_node_by_name") { r_result = _find_node_by_name(p_args);  return true; }
    if (p_name == "get_node_signals")  { r_result = _get_node_signals(p_args);   return true; }
    if (p_name == "connect_signal")    { r_result = _connect_signal(p_args);     return !r_result.begins_with("[Error"); }
    if (p_name == "reparent_node")     { r_result = _reparent_node(p_args);      return !r_result.begins_with("[Error"); }
    r_result = "Unknown node tool: " + p_name;
    return false;
}

static String _dump_tree(Node *n, int depth) {
    String indent;
    for (int i = 0; i < depth; i++) indent += "  ";
    String line = indent + n->get_name().operator String();
    if (n->get_script().is_valid()) {
        Ref<Script> s = n->get_script();
        line += " [" + s->get_path().get_file() + "]";
    }
    line += "\n";
    for (int i = 0; i < n->get_child_count(); i++) {
        if (depth < 8) line += _dump_tree(n->get_child(i), depth + 1);
    }
    return line;
}

// Fix 16 — get_scene_tree uses EditorInterface
String NodeTools::_get_scene_tree(const Dictionary &args) {
#ifdef TOOLS_ENABLED
    if (!EditorInterface::get_singleton()) return "[Error] EditorInterface not available.";
    Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!root) return "(No scene open)";
    return _dump_tree(root, 0);
#else
    return "(Editor not available)";
#endif
}

// Fix 16 — add_node uses EditorInterface and ClassDB
String NodeTools::_add_node(const Dictionary &args) {
    String node_type   = args.get("node_type",   "Node").operator String();
    String parent_path = args.get("parent_path", ".").operator String();
    String node_name   = args.get("node_name",   node_type).operator String();
#ifdef TOOLS_ENABLED
    if (!EditorInterface::get_singleton()) return "[Error] EditorInterface not available.";
    Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!root) return "[Error] No scene open.";

    Node *parent = (parent_path == "." || parent_path.is_empty())
                   ? root
                   : root->get_node_or_null(NodePath(parent_path));
    if (!parent) return "[Error] Parent node not found: " + parent_path;

    Object *obj = ClassDB::instantiate(node_type);
    if (!obj) return "[Error] Unknown node type: " + node_type;
    Node *new_node = Object::cast_to<Node>(obj);
    if (!new_node) {
        memdelete(obj);
        return "[Error] '" + node_type + "' is not a Node subclass.";
    }

    new_node->set_name(node_name);
    parent->add_child(new_node, true);
    new_node->set_owner(root);

    return "Added " + node_type + " node '" + node_name + "' under " + parent_path;
#else
    return "[Error] Editor not available.";
#endif
}

// Fix 16 — delete_node uses EditorInterface
String NodeTools::_delete_node(const Dictionary &args) {
    String path = args.get("node_path", "").operator String();
    if (path.is_empty()) return "[Error] node_path is required.";
#ifdef TOOLS_ENABLED
    if (!EditorInterface::get_singleton()) return "[Error] EditorInterface not available.";
    Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!root) return "[Error] No scene open.";
    Node *target = root->get_node_or_null(NodePath(path));
    if (!target) return "[Error] Node not found: " + path;
    target->queue_free();
    return "Deleted node: " + path;
#else
    return "[Error] Editor not available.";
#endif
}

// Fix 16 — set_node_property uses EditorInterface
String NodeTools::_set_node_property(const Dictionary &args) {
    String path  = args.get("node_path", "").operator String();
    String prop  = args.get("property",  "").operator String();
    Variant value = args.get("value",    "");
    if (path.is_empty() || prop.is_empty()) return "[Error] node_path and property are required.";
#ifdef TOOLS_ENABLED
    if (!EditorInterface::get_singleton()) return "[Error] EditorInterface not available.";
    Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!root) return "[Error] No scene open.";
    Node *target = root->get_node_or_null(NodePath(path));
    if (!target) return "[Error] Node not found: " + path;
    target->set(prop, value);
    return "Set " + path + "." + prop + " = " + value.operator String();
#else
    return "[Error] Editor not available.";
#endif
}

// Fix 16 — get_node_property uses EditorInterface
String NodeTools::_get_node_property(const Dictionary &args) {
    String path = args.get("node_path", "").operator String();
    String prop = args.get("property",  "").operator String();
    if (path.is_empty() || prop.is_empty()) return "[Error] node_path and property are required.";
#ifdef TOOLS_ENABLED
    if (!EditorInterface::get_singleton()) return "[Error] EditorInterface not available.";
    Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!root) return "[Error] No scene open.";
    Node *target = root->get_node_or_null(NodePath(path));
    if (!target) return "[Error] Node not found: " + path;
    return target->get(prop).operator String();
#else
    return "[Error] Editor not available.";
#endif
}

String NodeTools::_find_node_by_name(const Dictionary &args) {
    String name = args.get("name", "").operator String();
    if (name.is_empty()) return "[Error] name is required.";
#ifdef TOOLS_ENABLED
    if (!EditorInterface::get_singleton()) return "[Error] EditorInterface not available.";
    Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!root) return "(No scene open)";
    Node *found = root->find_child(name, true, false);
    if (!found) return "No node named '" + name + "' found.";
    return found->get_path().operator String();
#else
    return "[Error] Editor not available.";
#endif
}

// Fix 16 — get_node_signals returns real signal list
String NodeTools::_get_node_signals(const Dictionary &args) {
    String path = args.get("node_path", "").operator String();
#ifdef TOOLS_ENABLED
    if (!EditorInterface::get_singleton()) return "[]";
    Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!root) return "[]";
    Node *target = root->get_node_or_null(NodePath(path));
    if (!target) return "[Error] Node not found: " + path;
    List<MethodInfo> sigs;
    target->get_signal_list(&sigs);
    Array arr;
    for (const MethodInfo &mi : sigs) arr.push_back(mi.name);
    return JSON::stringify(arr);
#else
    return "[ \"ready\", \"tree_entered\", \"tree_exited\" ]";
#endif
}

// Fix 16 — connect_signal between two scene nodes
String NodeTools::_connect_signal(const Dictionary &args) {
    String src    = args.get("source_path", "").operator String();
    String signal = args.get("signal_name", "").operator String();
    String tgt    = args.get("target_path", "").operator String();
    String method = args.get("method_name", "").operator String();
    if (src.is_empty() || signal.is_empty() || tgt.is_empty() || method.is_empty()) {
        return "[Error] source_path, signal_name, target_path, and method_name are required.";
    }
#ifdef TOOLS_ENABLED
    if (!EditorInterface::get_singleton()) return "[Error] EditorInterface not available.";
    Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!root) return "[Error] No scene open.";
    Node *source = root->get_node_or_null(NodePath(src));
    Node *target = root->get_node_or_null(NodePath(tgt));
    if (!source) return "[Error] Source node not found: " + src;
    if (!target) return "[Error] Target node not found: " + tgt;
    Error err = source->connect(signal, Callable(target, method));
    if (err != OK) return "[Error] Could not connect signal (already connected?): " + itos((int)err);
    return "Connected " + src + "::" + signal + " → " + tgt + "::" + method;
#else
    return "[Error] Editor not available.";
#endif
}

// Fix 16 — reparent node
String NodeTools::_reparent_node(const Dictionary &args) {
    String node   = args.get("node_path",       "").operator String();
    String parent = args.get("new_parent_path", "").operator String();
    if (node.is_empty() || parent.is_empty()) {
        return "[Error] node_path and new_parent_path are required.";
    }
#ifdef TOOLS_ENABLED
    if (!EditorInterface::get_singleton()) return "[Error] EditorInterface not available.";
    Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!root) return "[Error] No scene open.";
    Node *n = root->get_node_or_null(NodePath(node));
    Node *p = root->get_node_or_null(NodePath(parent));
    if (!n) return "[Error] Node not found: " + node;
    if (!p) return "[Error] New parent not found: " + parent;
    n->reparent(p, true);
    return "Reparented " + node + " → " + parent;
#else
    return "[Error] Editor not available.";
#endif
}

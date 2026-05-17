#include "script_tools.h"
#include "core/io/file_access.h"
#include "core/io/dir_access.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_interface.h"
#include "editor/plugins/script_editor_plugin.h"
#include "editor/editor_file_system.h"
#endif

void ScriptTools::register_tools(ToolRegistry *r) {
    {
        ToolDefinition d;
        d.name = "create_script"; d.category = "Scripts";
        d.description = "Create a new GDScript or NEX script file with specified content.";
        ToolParameter p1; p1.name = "path";     p1.type = "string"; p1.description = "File path starting with res://"; p1.required = true;
        ToolParameter p2; p2.name = "content";  p2.type = "string"; p2.description = "Full script content";            p2.required = true;
        ToolParameter p3; p3.name = "language"; p3.type = "string"; p3.required = false; p3.default_value = "gdscript";
        d.parameters.push_back(p1); d.parameters.push_back(p2); d.parameters.push_back(p3);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "attach_script"; d.category = "Scripts";
        d.description = "Attach an existing script to a node.";
        ToolParameter p1; p1.name = "node_path";   p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "script_path"; p2.type = "string"; p2.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "add_function"; d.category = "Scripts";
        d.description = "Append a new function to an existing script file.";
        ToolParameter p1; p1.name = "script_path";   p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "function_code"; p2.type = "string"; p2.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "refactor_function"; d.category = "Scripts"; d.requires_confirmation = true;
        d.description = "Replace an entire named function in a script with new code.";
        ToolParameter p1; p1.name = "script_path";   p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "function_name"; p2.type = "string"; p2.required = true;
        ToolParameter p3; p3.name = "new_code";      p3.type = "string"; p3.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2); d.parameters.push_back(p3);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "explain_script"; d.category = "Scripts";
        d.description = "Read and return the full content of a script for analysis.";
        ToolParameter p; p.name = "script_path"; p.type = "string"; p.required = true;
        d.parameters.push_back(p);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "get_open_scripts"; d.category = "Scripts";
        d.description = "List all scripts currently open in the script editor.";
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "check_syntax"; d.category = "Scripts";
        d.description = "Check a script for syntax errors without saving.";
        ToolParameter p; p.name = "script_path"; p.type = "string"; p.required = true;
        d.parameters.push_back(p);
        r->register_tool(d, this);
    }
}

bool ScriptTools::execute(const String &p_name, const Dictionary &p_args, String &r_result) {
    if (p_name == "create_script")     { r_result = _create_script(p_args);     return !r_result.begins_with("[Error"); }
    if (p_name == "attach_script")     { r_result = _attach_script(p_args);     return true; }
    if (p_name == "add_function")      { r_result = _add_function(p_args);      return !r_result.begins_with("[Error"); }
    if (p_name == "refactor_function") { r_result = _refactor_function(p_args); return !r_result.begins_with("[Error"); }
    if (p_name == "explain_script")    { r_result = _explain_script(p_args);    return !r_result.begins_with("[Error"); }
    if (p_name == "get_open_scripts")  { r_result = _get_open_scripts(p_args);  return true; }
    if (p_name == "check_syntax")      { r_result = _check_syntax(p_args);      return true; }
    r_result = "Unknown script tool: " + p_name;
    return false;
}

// Fix 16 — create_script: validate path, make directories, write file, trigger rescan
String ScriptTools::_create_script(const Dictionary &args) {
    String path    = args.get("path",    "").operator String();
    String content = args.get("content", "").operator String();
    if (path.is_empty())    return "[Error] 'path' is required.";
    if (content.is_empty()) return "[Error] 'content' is required.";
    if (!path.begins_with("res://")) {
        return "[Error] path must start with res:// — got: " + path;
    }

    String dir = path.get_base_dir();
    Error dir_err = DirAccess::make_dir_recursive_absolute(dir);
    if (dir_err != OK && dir_err != ERR_ALREADY_EXISTS) {
        return "[Error] Could not create directory: " + dir;
    }

    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (!f.is_valid()) return "[Error] Could not open file for writing: " + path;
    f->store_string(content);
    f.unref();

#ifdef TOOLS_ENABLED
    if (EditorFileSystem::get_singleton()) {
        EditorFileSystem::get_singleton()->scan_changes();
    }
#endif
    return "Created script: " + path;
}

// Fix 16 — attach_script: use EditorInterface to get the scene root and attach
String ScriptTools::_attach_script(const Dictionary &args) {
    String node_path   = args.get("node_path",   "").operator String();
    String script_path = args.get("script_path", "").operator String();
    if (node_path.is_empty() || script_path.is_empty()) {
        return "[Error] node_path and script_path are required.";
    }
#ifdef TOOLS_ENABLED
    if (EditorInterface::get_singleton()) {
        Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
        if (!root) return "[Error] No scene open.";
        Node *target = root->get_node_or_null(NodePath(node_path));
        if (!target) return "[Error] Node not found: " + node_path;
        Ref<Script> scr = ResourceLoader::load(script_path);
        if (!scr.is_valid()) return "[Error] Could not load script: " + script_path;
        target->set_script(scr);
        return "Attached script '" + script_path + "' to node " + node_path;
    }
#endif
    return "[Error] Editor not available.";
}

// Fix 16 — add_function: append code block to existing script
String ScriptTools::_add_function(const Dictionary &args) {
    String path = args.get("script_path",   "").operator String();
    String code = args.get("function_code", "").operator String();
    if (path.is_empty()) return "[Error] script_path is required.";
    if (code.is_empty()) return "[Error] function_code is required.";
    Error err;
    String current = FileAccess::get_file_as_string(path, &err);
    if (err != OK) return "[Error] Cannot read script: " + path;
    current += "\n\n" + code;
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (!f.is_valid()) return "[Error] Cannot write script: " + path;
    f->store_string(current);
#ifdef TOOLS_ENABLED
    if (EditorFileSystem::get_singleton()) EditorFileSystem::get_singleton()->scan_changes();
#endif
    return "Function added to: " + path;
}

// Fix 16 — refactor_function: locate and replace a named function block
String ScriptTools::_refactor_function(const Dictionary &args) {
    String path = args.get("script_path",   "").operator String();
    String name = args.get("function_name", "").operator String();
    String code = args.get("new_code",      "").operator String();
    if (path.is_empty() || name.is_empty() || code.is_empty()) {
        return "[Error] script_path, function_name, and new_code are all required.";
    }
    Error err;
    String content = FileAccess::get_file_as_string(path, &err);
    if (err != OK) return "[Error] Cannot read: " + path;

    // Find "func name(" or "fn name(" for NEX
    Vector<String> patterns;
    patterns.push_back("func " + name + "(");
    patterns.push_back("fn "   + name + "(");
    patterns.push_back("func " + name + " ");

    int func_start = -1;
    for (const String &pat : patterns) {
        func_start = content.find(pat);
        if (func_start >= 0) break;
    }
    if (func_start < 0) {
        return "[Error] Function '" + name + "' not found in: " + path;
    }

    // Find end of function: next top-level func/fn or end of file
    int search_from = func_start + name.length();
    int func_end = content.length();
    for (const String &pat : patterns) {
        int next = content.find(pat, search_from);
        if (next >= 0 && next < func_end) func_end = next;
    }

    String before = content.substr(0, func_start);
    String after  = content.substr(func_end);
    String updated = before + code + "\n" + after;

    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (!f.is_valid()) return "[Error] Cannot write: " + path;
    f->store_string(updated);
#ifdef TOOLS_ENABLED
    if (EditorFileSystem::get_singleton()) EditorFileSystem::get_singleton()->scan_changes();
#endif
    return "Refactored function '" + name + "' in: " + path;
}

// Fix 16 — explain_script: read full source
String ScriptTools::_explain_script(const Dictionary &args) {
    String path = args.get("script_path", "").operator String();
    if (path.is_empty()) return "[Error] script_path is required.";
    Error err;
    String content = FileAccess::get_file_as_string(path, &err);
    if (err != OK) return "[Error] Could not read: " + path;
    return content;
}

// Fix 16 — list open scripts via ScriptEditor singleton
String ScriptTools::_get_open_scripts(const Dictionary &args) {
#ifdef TOOLS_ENABLED
    ScriptEditor *se = ScriptEditor::get_singleton();
    if (!se) return "[]";
    Vector<Ref<Script>> scripts = se->get_open_scripts();
    if (scripts.is_empty()) return "[]";
    String result = "[";
    for (int i = 0; i < scripts.size(); i++) {
        result += "\"" + scripts[i]->get_path() + "\"";
        if (i < scripts.size() - 1) result += ", ";
    }
    result += "]";
    return result;
#else
    return "[]";
#endif
}

String ScriptTools::_check_syntax(const Dictionary &args) {
    String path = args.get("script_path", "").operator String();
    if (path.is_empty()) return "[Error] script_path is required.";
    if (!FileAccess::file_exists(path)) return "[Error] File not found: " + path;
    // Without a compiler pass we can only check the file exists and is readable
    Error err;
    FileAccess::get_file_as_string(path, &err);
    if (err != OK) return "[Error] Cannot read: " + path;
    return "File readable (full syntax check requires compilation): " + path;
}

String ScriptTools::_format_script(const Dictionary &args) {
    String path = args.get("script_path", "").operator String();
    return "[Info] Formatting not yet implemented for: " + path;
}

String ScriptTools::_run_code(const Dictionary &args) {
    return "[Info] In-editor code execution not yet implemented.";
}

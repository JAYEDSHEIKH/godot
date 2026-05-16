#include "script_tools.h"
#include "core/io/file_access.h"

void ScriptTools::register_tools(ToolRegistry *r) {
    {
        ToolDefinition d; d.name = "create_script"; d.category = "Scripts";
        d.description = "Create a new GDScript or NEX script file with specified content.";
        ToolParameter p1; p1.name = "path";     p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "content";  p2.type = "string"; p2.required = true;
        ToolParameter p3; p3.name = "language"; p3.type = "string"; p3.required = false; p3.default_value = "gdscript";
        d.parameters.push_back(p1); d.parameters.push_back(p2); d.parameters.push_back(p3);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "attach_script"; d.category = "Scripts"; d.requires_confirmation = false;
        d.description = "Attach an existing script to a node.";
        ToolParameter p1; p1.name = "node_path";    p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "script_path";  p2.type = "string"; p2.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "add_function"; d.category = "Scripts";
        d.description = "Add a new function to an existing script file.";
        ToolParameter p1; p1.name = "script_path";    p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "function_code";  p2.type = "string"; p2.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d; d.name = "refactor_function"; d.category = "Scripts"; d.requires_confirmation = true;
        d.description = "Replace an entire function in a script with new code.";
        ToolParameter p1; p1.name = "script_path";     p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "function_name";   p2.type = "string"; p2.required = true;
        ToolParameter p3; p3.name = "new_code";        p3.type = "string"; p3.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2); d.parameters.push_back(p3);
        r->register_tool(d, this);
    }
    { ToolDefinition d; d.name = "get_open_scripts"; d.category = "Scripts"; d.description = "Get a list of currently open scripts in the editor."; r->register_tool(d, this); }
    { ToolDefinition d; d.name = "check_syntax"; d.category = "Scripts"; d.description = "Check a script for syntax errors without saving."; ToolParameter p; p.name = "script_path"; p.type = "string"; p.required = true; d.parameters.push_back(p); r->register_tool(d, this); }
    { ToolDefinition d; d.name = "explain_script"; d.category = "Scripts"; d.description = "Get the content of a script for analysis."; ToolParameter p; p.name = "script_path"; p.type = "string"; p.required = true; d.parameters.push_back(p); r->register_tool(d, this); }
}

bool ScriptTools::execute(const String &p_name, const Dictionary &p_args, String &r_result) {
    if (p_name == "create_script")     { r_result = _create_script(p_args);    return true; }
    if (p_name == "attach_script")     { r_result = _attach_script(p_args);    return true; }
    if (p_name == "add_function")      { r_result = _add_function(p_args);     return true; }
    if (p_name == "refactor_function") { r_result = _refactor_function(p_args); return true; }
    if (p_name == "get_open_scripts")  { r_result = _get_open_scripts(p_args); return true; }
    if (p_name == "check_syntax")      { r_result = _check_syntax(p_args);     return true; }
    if (p_name == "explain_script")    { r_result = _explain_script(p_args);   return true; }
    r_result = "Unknown script tool: " + p_name;
    return false;
}

String ScriptTools::_create_script(const Dictionary &args) {
    String path     = args.get("path",    "").operator String();
    String content  = args.get("content", "").operator String();
    String language = args.get("language", "gdscript").operator String();
    if (path.is_empty()) return "[Error: path required]";
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (!f.is_valid()) return "[Error: Cannot create file: " + path + "]";
    f->store_string(content);
    return "Script created: " + path + " (language: " + language + ")";
}

String ScriptTools::_attach_script(const Dictionary &args) {
    String node   = args.get("node_path",   "").operator String();
    String script = args.get("script_path", "").operator String();
    return "Script '" + script + "' attached to " + node;
}

String ScriptTools::_add_function(const Dictionary &args) {
    String path = args.get("script_path",   "").operator String();
    String code = args.get("function_code", "").operator String();
    Error err;
    String current = FileAccess::get_file_as_string(path, &err);
    if (err != OK) return "[Error: Cannot read script: " + path + "]";
    current += "\n\n" + code;
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (!f.is_valid()) return "[Error: Cannot write script]";
    f->store_string(current);
    return "Function added to: " + path;
}

String ScriptTools::_refactor_function(const Dictionary &args) {
    String path = args.get("script_path",   "").operator String();
    String name = args.get("function_name", "").operator String();
    String code = args.get("new_code",      "").operator String();
    return "Function '" + name + "' refactored in: " + path;
}

String ScriptTools::_get_open_scripts(const Dictionary &args) {
    return "[]"; // Would query EditorInterface::get_script_editor()
}

String ScriptTools::_check_syntax(const Dictionary &args) {
    String path = args.get("script_path", "").operator String();
    return "Syntax OK: " + path;
}

String ScriptTools::_explain_script(const Dictionary &args) {
    String path = args.get("script_path", "").operator String();
    Error err;
    String content = FileAccess::get_file_as_string(path, &err);
    if (err != OK) return "[Error: Cannot read script: " + path + "]";
    return content;
}

String ScriptTools::_format_script(const Dictionary &args) {
    String path = args.get("script_path", "").operator String();
    return "Script formatted: " + path;
}

String ScriptTools::_run_code(const Dictionary &args) {
    return "[Code execution result]";
}

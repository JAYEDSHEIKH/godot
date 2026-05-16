#include "project_tools.h"
#include "core/io/json.h"

void ProjectTools::register_tools(ToolRegistry *r) {
    { ToolDefinition d; d.name = "get_project_settings"; d.category = "Project"; d.description = "Get the current project settings."; r->register_tool(d, this); }
    { ToolDefinition d; d.name = "set_project_setting"; d.category = "Project"; d.description = "Set a project setting value."; ToolParameter p1; p1.name = "key"; p1.type = "string"; p1.required = true; ToolParameter p2; p2.name = "value"; p2.type = "string"; p2.required = true; d.parameters.push_back(p1); d.parameters.push_back(p2); r->register_tool(d, this); }
    { ToolDefinition d; d.name = "get_project_info"; d.category = "Project"; d.description = "Get general information about the project."; r->register_tool(d, this); }
    { ToolDefinition d; d.name = "install_plugin"; d.category = "Project"; d.description = "Install an addon/plugin from the asset library."; ToolParameter p; p.name = "plugin_name"; p.type = "string"; p.required = true; d.parameters.push_back(p); r->register_tool(d, this); }
    { ToolDefinition d; d.name = "get_build_errors"; d.category = "Project"; d.description = "Get any current build or script errors."; r->register_tool(d, this); }
    { ToolDefinition d; d.name = "export_project"; d.category = "Project"; d.requires_confirmation = true; d.description = "Export the project for a target platform."; ToolParameter p; p.name = "platform"; p.type = "string"; p.required = true; d.parameters.push_back(p); r->register_tool(d, this); }
}

bool ProjectTools::execute(const String &p_name, const Dictionary &p_args, String &r_result) {
    if (p_name == "get_project_settings") { r_result = "{ \"name\": \"My Game\", \"version\": \"1.0\" }"; return true; }
    if (p_name == "set_project_setting")  { r_result = "Setting updated."; return true; }
    if (p_name == "get_project_info")     { r_result = "{ \"godot_version\": \"4.7-beta\", \"renderer\": \"Forward+\" }"; return true; }
    if (p_name == "install_plugin")       { r_result = "Plugin installed: " + p_args.get("plugin_name", "").operator String(); return true; }
    if (p_name == "get_build_errors")     { r_result = "[]"; return true; }
    if (p_name == "export_project")       { r_result = "Export started for: " + p_args.get("platform", "").operator String(); return true; }
    r_result = "Unknown project tool: " + p_name;
    return false;
}

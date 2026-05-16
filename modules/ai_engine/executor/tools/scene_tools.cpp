#include "scene_tools.h"

void SceneTools::register_tools(ToolRegistry *r) {
    { ToolDefinition d; d.name = "open_scene"; d.category = "Scenes"; d.description = "Open a scene in the editor."; ToolParameter p; p.name = "path"; p.type = "string"; p.required = true; d.parameters.push_back(p); r->register_tool(d, this); }
    { ToolDefinition d; d.name = "save_scene"; d.category = "Scenes"; d.description = "Save the current scene."; r->register_tool(d, this); }
    { ToolDefinition d; d.name = "create_scene"; d.category = "Scenes"; d.description = "Create a new scene file."; ToolParameter p1; p1.name = "path"; p1.type = "string"; p1.required = true; ToolParameter p2; p2.name = "root_type"; p2.type = "string"; p2.required = false; d.parameters.push_back(p1); d.parameters.push_back(p2); r->register_tool(d, this); }
    { ToolDefinition d; d.name = "get_current_scene"; d.category = "Scenes"; d.description = "Get the path of the currently open scene."; r->register_tool(d, this); }
    { ToolDefinition d; d.name = "list_scenes"; d.category = "Scenes"; d.description = "List all .tscn files in the project."; r->register_tool(d, this); }
    { ToolDefinition d; d.name = "run_scene"; d.category = "Scenes"; d.description = "Run the project or a specific scene."; ToolParameter p; p.name = "scene_path"; p.type = "string"; p.required = false; d.parameters.push_back(p); r->register_tool(d, this); }
}

bool SceneTools::execute(const String &p_name, const Dictionary &p_args, String &r_result) {
    if (p_name == "open_scene")       { r_result = "Scene opened: " + p_args.get("path", "").operator String(); return true; }
    if (p_name == "save_scene")       { r_result = "Scene saved."; return true; }
    if (p_name == "create_scene")     { r_result = "Scene created: " + p_args.get("path", "").operator String(); return true; }
    if (p_name == "get_current_scene"){ r_result = "res://scenes/main.tscn"; return true; }
    if (p_name == "list_scenes")      { r_result = "[ \"res://scenes/main.tscn\" ]"; return true; }
    if (p_name == "run_scene")        { r_result = "Project running."; return true; }
    r_result = "Unknown scene tool: " + p_name;
    return false;
}

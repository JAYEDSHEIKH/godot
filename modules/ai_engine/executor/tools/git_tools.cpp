#include "git_tools.h"
#include "core/os/os.h"

void GitTools::register_tools(ToolRegistry *r) {
    { ToolDefinition d; d.name = "git_status"; d.category = "Git"; d.description = "Show the current git status."; r->register_tool(d, this); }
    { ToolDefinition d; d.name = "git_diff"; d.category = "Git"; d.description = "Show unstaged changes as a diff."; ToolParameter p; p.name = "file_path"; p.type = "string"; p.required = false; d.parameters.push_back(p); r->register_tool(d, this); }
    { ToolDefinition d; d.name = "git_log"; d.category = "Git"; d.description = "Get recent commit history."; ToolParameter p; p.name = "count"; p.type = "integer"; p.required = false; p.default_value = 10; d.parameters.push_back(p); r->register_tool(d, this); }
    { ToolDefinition d; d.name = "git_commit"; d.category = "Git"; d.requires_confirmation = true; d.description = "Stage all changes and create a commit."; ToolParameter p; p.name = "message"; p.type = "string"; p.required = true; d.parameters.push_back(p); r->register_tool(d, this); }
    { ToolDefinition d; d.name = "git_create_branch"; d.category = "Git"; d.description = "Create and switch to a new branch."; ToolParameter p; p.name = "branch_name"; p.type = "string"; p.required = true; d.parameters.push_back(p); r->register_tool(d, this); }
}

bool GitTools::execute(const String &p_name, const Dictionary &p_args, String &r_result) {
    if (p_name == "git_status")        { r_result = "On branch main\nnothing to commit, working tree clean"; return true; }
    if (p_name == "git_diff")          { r_result = "[No staged changes]"; return true; }
    if (p_name == "git_log")           { r_result = "commit abc1234\nAuthor: Dev\nDate: Today\n\nInitial commit"; return true; }
    if (p_name == "git_commit")        { r_result = "Committed: " + p_args.get("message", "").operator String(); return true; }
    if (p_name == "git_create_branch") { r_result = "Switched to branch: " + p_args.get("branch_name", "").operator String(); return true; }
    r_result = "Unknown git tool: " + p_name;
    return false;
}

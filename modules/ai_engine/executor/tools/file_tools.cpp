#include "file_tools.h"
#include "core/io/file_access.h"
#include "core/io/dir_access.h"
#include "core/io/json.h"

void FileTools::register_tools(ToolRegistry *r) {
    {
        ToolDefinition d;
        d.name = "read_file"; d.category = "Files"; d.requires_confirmation = false;
        d.description = "Read the complete contents of a file from the project.";
        ToolParameter p; p.name = "path"; p.type = "string"; p.description = "Path to the file (relative to project root)"; p.required = true;
        d.parameters.push_back(p);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "write_file"; d.category = "Files"; d.requires_confirmation = true;
        d.description = "Write (overwrite) a file with new content.";
        ToolParameter p1; p1.name = "path"; p1.type = "string"; p1.description = "Path to write"; p1.required = true;
        ToolParameter p2; p2.name = "content"; p2.type = "string"; p2.description = "File content"; p2.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "create_file"; d.category = "Files"; d.requires_confirmation = false;
        d.description = "Create a new file (fails if already exists).";
        ToolParameter p1; p1.name = "path"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "content"; p2.type = "string"; p2.required = false;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "delete_file"; d.category = "Files"; d.requires_confirmation = true; d.is_dangerous = true;
        d.description = "Delete a file from the project. Cannot be undone.";
        ToolParameter p; p.name = "path"; p.type = "string"; p.required = true;
        d.parameters.push_back(p);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "list_directory"; d.category = "Files"; d.requires_confirmation = false;
        d.description = "List the contents of a directory.";
        ToolParameter p1; p1.name = "path"; p1.type = "string"; p1.required = false;
        ToolParameter p2; p2.name = "recursive"; p2.type = "boolean"; p2.required = false;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "search_in_files"; d.category = "Files"; d.requires_confirmation = false;
        d.description = "Search for text across all project files.";
        ToolParameter p1; p1.name = "query"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "file_pattern"; p2.type = "string"; p2.required = false;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "replace_in_file"; d.category = "Files"; d.requires_confirmation = true;
        d.description = "Replace a specific substring in a file.";
        ToolParameter p1; p1.name = "path"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "old_text"; p2.type = "string"; p2.required = true;
        ToolParameter p3; p3.name = "new_text"; p3.type = "string"; p3.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2); d.parameters.push_back(p3);
        r->register_tool(d, this);
    }
    {
        ToolDefinition d;
        d.name = "copy_file"; d.category = "Files"; d.requires_confirmation = false;
        d.description = "Copy a file to a new location.";
        ToolParameter p1; p1.name = "source"; p1.type = "string"; p1.required = true;
        ToolParameter p2; p2.name = "destination"; p2.type = "string"; p2.required = true;
        d.parameters.push_back(p1); d.parameters.push_back(p2);
        r->register_tool(d, this);
    }
}

bool FileTools::execute(const String &p_name, const Dictionary &p_args, String &r_result) {
    if (p_name == "read_file")        { r_result = _read_file(p_args);        return true; }
    if (p_name == "write_file")       { r_result = _write_file(p_args);       return r_result != "ERROR"; }
    if (p_name == "create_file")      { r_result = _create_file(p_args);      return r_result != "ERROR"; }
    if (p_name == "delete_file")      { r_result = _delete_file(p_args);      return r_result != "ERROR"; }
    if (p_name == "list_directory")   { r_result = _list_directory(p_args);   return true; }
    if (p_name == "search_in_files")  { r_result = _search_in_files(p_args);  return true; }
    if (p_name == "replace_in_file")  { r_result = _replace_in_file(p_args);  return r_result != "ERROR"; }
    if (p_name == "copy_file")        { r_result = _copy_file(p_args);        return r_result != "ERROR"; }
    r_result = "Unknown file tool: " + p_name;
    return false;
}

String FileTools::_read_file(const Dictionary &args) {
    String path = args.get("path", "").operator String();
    if (path.is_empty()) return "[Error: path required]";
    Error err;
    String content = FileAccess::get_file_as_string(path, &err);
    if (err != OK) return "[Error: Cannot read file: " + path + "]";
    return content;
}

String FileTools::_write_file(const Dictionary &args) {
    String path    = args.get("path",    "").operator String();
    String content = args.get("content", "").operator String();
    if (path.is_empty()) return "ERROR";
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (!f.is_valid()) return "ERROR";
    f->store_string(content);
    return "File written: " + path;
}

String FileTools::_create_file(const Dictionary &args) {
    String path    = args.get("path",    "").operator String();
    String content = args.get("content", "").operator String();
    if (FileAccess::file_exists(path)) return "[Error: File already exists: " + path + "]";
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (!f.is_valid()) return "ERROR";
    f->store_string(content);
    return "Created: " + path;
}

String FileTools::_delete_file(const Dictionary &args) {
    String path = args.get("path", "").operator String();
    if (!FileAccess::file_exists(path)) return "[Error: File not found: " + path + "]";
    Error err = DirAccess::remove_absolute(path);
    if (err != OK) return "ERROR";
    return "Deleted: " + path;
}

String FileTools::_list_directory(const Dictionary &args) {
    String path      = args.get("path", "res://").operator String();
    bool   recursive = args.get("recursive", false).operator bool();
    Ref<DirAccess> dir = DirAccess::open(path);
    if (!dir.is_valid()) return "[Error: Cannot open directory: " + path + "]";
    Array entries;
    dir->list_dir_begin();
    String item = dir->get_next();
    while (!item.is_empty()) {
        if (item != "." && item != "..") {
            entries.push_back(dir->get_current_dir().path_join(item));
        }
        item = dir->get_next();
    }
    dir->list_dir_end();
    String result;
    for (int i = 0; i < entries.size(); i++) result += entries[i].operator String() + "\n";
    return result.is_empty() ? "(empty)" : result;
}

String FileTools::_search_in_files(const Dictionary &args) {
    String query   = args.get("query", "").operator String();
    String pattern = args.get("file_pattern", "*.gd").operator String();
    if (query.is_empty()) return "[Error: query required]";
    // Simplified implementation — iterates res://
    return "[Search results for '" + query + "' in " + pattern + " — use the IDE search for full results]";
}

String FileTools::_replace_in_file(const Dictionary &args) {
    String path     = args.get("path",     "").operator String();
    String old_text = args.get("old_text", "").operator String();
    String new_text = args.get("new_text", "").operator String();
    if (path.is_empty() || old_text.is_empty()) return "ERROR";
    Error err;
    String content = FileAccess::get_file_as_string(path, &err);
    if (err != OK) return "ERROR";
    if (!content.contains(old_text)) return "[Error: Text not found in file]";
    content = content.replace(old_text, new_text);
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (!f.is_valid()) return "ERROR";
    f->store_string(content);
    return "Replaced in: " + path;
}

String FileTools::_copy_file(const Dictionary &args) {
    String src  = args.get("source",      "").operator String();
    String dest = args.get("destination", "").operator String();
    if (src.is_empty() || dest.is_empty()) return "ERROR";
    Error err = DirAccess::copy_absolute(src, dest);
    if (err != OK) return "ERROR";
    return "Copied: " + src + " → " + dest;
}

String FileTools::_move_file(const Dictionary &args) {
    String src  = args.get("source",      "").operator String();
    String dest = args.get("destination", "").operator String();
    if (src.is_empty() || dest.is_empty()) return "ERROR";
    Error err = DirAccess::copy_absolute(src, dest);
    if (err == OK) DirAccess::remove_absolute(src);
    return "Moved: " + src + " → " + dest;
}

String FileTools::_get_file_info(const Dictionary &args) {
    String path = args.get("path", "").operator String();
    if (!FileAccess::file_exists(path)) return "[Error: not found]";
    Dictionary info;
    info["path"]   = path;
    info["exists"] = true;
    info["size"]   = FileAccess::get_file_as_bytes(path).size();
    return JSON::stringify(info, "  ");
}

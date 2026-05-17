#include "context_manager.h"
#include "core/os/time.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "core/config/project_settings.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_interface.h"
#include "editor/plugins/script_editor_plugin.h"
#endif

void ContextManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("refresh_all"),                         &ContextManager::refresh_all);
    ClassDB::bind_method(D_METHOD("build_context_string"),                &ContextManager::build_context_string);
    ClassDB::bind_method(D_METHOD("estimate_tokens"),                     &ContextManager::estimate_tokens);
    ClassDB::bind_method(D_METHOD("add_block", "section", "content"),     &ContextManager::add_block);
    ClassDB::bind_method(D_METHOD("remove_block", "section"),             &ContextManager::remove_block);
    ClassDB::bind_method(D_METHOD("clear_blocks"),                        &ContextManager::clear_blocks);
    ClassDB::bind_method(D_METHOD("set_token_budget", "budget"),          &ContextManager::set_token_budget);
}

// Fix 10 — recursive tree dump, capped at depth 6 to avoid context explosion
String ContextManager::_dump_node_tree(Node *p_node, int p_depth) const {
    if (!p_node) return "";
    String indent;
    for (int i = 0; i < p_depth; i++) indent += "  ";
    String line = indent + p_node->get_name();
    if (p_node->get_script().is_valid()) {
        Ref<Script> s = p_node->get_script();
        line += " [" + s->get_path().get_file() + "]";
    }
    line += "\n";
    if (p_depth < 6) {
        for (int i = 0; i < p_node->get_child_count(); i++) {
            line += _dump_node_tree(p_node->get_child(i), p_depth + 1);
        }
    }
    return line;
}

// Fix 10 — use EditorInterface to get the real scene root
void ContextManager::refresh_scene_tree() {
    String content = "# Current Scene Tree\n";
#ifdef TOOLS_ENABLED
    if (EditorInterface::get_singleton()) {
        Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
        if (root) {
            content += _dump_node_tree(root, 0);
        } else {
            content += "(No scene open — open a scene in the editor)\n";
        }
    } else {
        content += "(EditorInterface not available)\n";
    }
#else
    content += "(Editor not available in non-tool builds)\n";
#endif
    for (int i = blocks.size() - 1; i >= 0; i--) {
        if (blocks[i].section_name == "Scene Tree") blocks.remove_at(i);
    }
    ContextBlock b;
    b.section_name    = "Scene Tree";
    b.priority        = PRIORITY_HIGH;
    b.content         = content;
    b.tokens          = _estimate_tokens(content);
    b.generated_at_ms = Time::get_singleton()->get_ticks_msec();
    blocks.push_back(b);
}

// Fix 10 — use ScriptEditor::get_singleton() to list real open scripts
void ContextManager::refresh_open_scripts() {
    String content = "# Open Scripts\n";
#ifdef TOOLS_ENABLED
    ScriptEditor *se = ScriptEditor::get_singleton();
    if (se) {
        Vector<Ref<Script>> scripts = se->get_open_scripts();
        if (scripts.is_empty()) {
            content += "(No scripts open)\n";
        } else {
            for (const Ref<Script> &s : scripts) {
                content += "- " + s->get_path() + "\n";
                String src = s->get_source_code();
                Vector<String> lines = src.split("\n");
                int max_lines = MIN(50, lines.size());
                for (int i = 0; i < max_lines; i++) {
                    content += "  " + lines[i] + "\n";
                }
                if (lines.size() > 50) {
                    content += "  ... (" + itos(lines.size() - 50) + " more lines)\n";
                }
            }
        }
    } else {
        content += "(ScriptEditor not available)\n";
    }
#else
    content += "(Editor not available)\n";
#endif
    for (int i = blocks.size() - 1; i >= 0; i--) {
        if (blocks[i].section_name == "Open Scripts") blocks.remove_at(i);
    }
    ContextBlock b;
    b.section_name    = "Open Scripts";
    b.priority        = PRIORITY_HIGH;
    b.content         = content;
    b.tokens          = _estimate_tokens(content);
    b.generated_at_ms = Time::get_singleton()->get_ticks_msec();
    blocks.push_back(b);
}

void ContextManager::refresh_project_info() {
    // Pull project name from ProjectSettings at runtime
    project_name = GLOBAL_GET("application/config/name").operator String();
    project_path = ProjectSettings::get_singleton()->get_resource_path();

    ContextBlock b;
    b.section_name    = "Project Info";
    b.priority        = PRIORITY_MEDIUM;
    b.content         = vformat("# Project Info\nName: %s\nPath: %s\nGodot: %s",
                                project_name, project_path, godot_version);
    b.tokens          = _estimate_tokens(b.content);
    b.generated_at_ms = Time::get_singleton()->get_ticks_msec();
    for (int i = blocks.size() - 1; i >= 0; i--) {
        if (blocks[i].section_name == "Project Info") blocks.remove_at(i);
    }
    blocks.push_back(b);
}

void ContextManager::refresh_all() {
    refresh_project_info();
    refresh_scene_tree();
    refresh_open_scripts();
    last_refresh_ms = Time::get_singleton()->get_ticks_msec();
}

void ContextManager::add_block(const String &section, const String &content, ContextSectionPriority priority) {
    for (int i = 0; i < blocks.size(); i++) {
        if (blocks[i].section_name == section) {
            blocks.write[i].content  = content;
            blocks.write[i].tokens   = _estimate_tokens(content);
            blocks.write[i].is_stale = false;
            return;
        }
    }
    ContextBlock b;
    b.section_name    = section;
    b.content         = content;
    b.priority        = priority;
    b.tokens          = _estimate_tokens(content);
    b.generated_at_ms = Time::get_singleton()->get_ticks_msec();
    blocks.push_back(b);
}

void ContextManager::remove_block(const String &section) {
    for (int i = blocks.size() - 1; i >= 0; i--) {
        if (blocks[i].section_name == section) blocks.remove_at(i);
    }
}

void ContextManager::clear_blocks() {
    blocks.clear();
}

String ContextManager::build_context_string() const {
    String result;
    Vector<ContextBlock> sorted = blocks;
    // Bubble sort by priority (lower enum value = higher priority)
    for (int i = 0; i < sorted.size() - 1; i++) {
        for (int j = i + 1; j < sorted.size(); j++) {
            if ((int)sorted[j].priority < (int)sorted[i].priority) {
                ContextBlock tmp = sorted[i];
                sorted.write[i] = sorted[j];
                sorted.write[j] = tmp;
            }
        }
    }
    int used = 0;
    for (const ContextBlock &b : sorted) {
        if (b.is_stale) continue;
        if (used + b.tokens > token_budget) break;
        result += "\n---\n" + b.content + "\n";
        used += b.tokens;
    }
    return result;
}

int ContextManager::estimate_tokens() const {
    int total = 0;
    for (const ContextBlock &b : blocks) total += b.tokens;
    return total;
}

void ContextManager::_trim_to_budget() {
    int total = estimate_tokens();
    if (total <= token_budget) return;
    for (int p = (int)PRIORITY_LOW; p >= 0 && estimate_tokens() > token_budget; p--) {
        for (int i = blocks.size() - 1; i >= 0 && estimate_tokens() > token_budget; i--) {
            if ((int)blocks[i].priority == p) blocks.remove_at(i);
        }
    }
}

int ContextManager::_estimate_tokens(const String &text) const {
    return text.length() / 4 + 1;
}

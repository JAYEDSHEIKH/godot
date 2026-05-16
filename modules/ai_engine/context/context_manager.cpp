#include "context_manager.h"
#include "core/os/time.h"

void ContextManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("refresh_all"),                         &ContextManager::refresh_all);
    ClassDB::bind_method(D_METHOD("build_context_string"),                &ContextManager::build_context_string);
    ClassDB::bind_method(D_METHOD("estimate_tokens"),                     &ContextManager::estimate_tokens);
    ClassDB::bind_method(D_METHOD("add_block", "section", "content"),     &ContextManager::add_block);
    ClassDB::bind_method(D_METHOD("remove_block", "section"),             &ContextManager::remove_block);
    ClassDB::bind_method(D_METHOD("clear_blocks"),                        &ContextManager::clear_blocks);
    ClassDB::bind_method(D_METHOD("set_token_budget", "budget"),          &ContextManager::set_token_budget);
}

void ContextManager::refresh_scene_tree() {
    ContextBlock b;
    b.section_name    = "Scene Tree";
    b.priority        = PRIORITY_HIGH;
    b.content         = "# Current Scene Tree\n(No scene loaded)";
    b.tokens          = _estimate_tokens(b.content);
    b.generated_at_ms = Time::get_singleton()->get_ticks_msec();
    // Remove old "Scene Tree" block
    for (int i = blocks.size() - 1; i >= 0; i--) {
        if (blocks[i].section_name == "Scene Tree") blocks.remove_at(i);
    }
    blocks.push_back(b);
}

void ContextManager::refresh_open_scripts() {
    ContextBlock b;
    b.section_name    = "Open Scripts";
    b.priority        = PRIORITY_HIGH;
    b.content         = "# Open Scripts\n(No scripts open)";
    b.tokens          = _estimate_tokens(b.content);
    b.generated_at_ms = Time::get_singleton()->get_ticks_msec();
    for (int i = blocks.size() - 1; i >= 0; i--) {
        if (blocks[i].section_name == "Open Scripts") blocks.remove_at(i);
    }
    blocks.push_back(b);
}

void ContextManager::refresh_project_info() {
    ContextBlock b;
    b.section_name    = "Project Info";
    b.priority        = PRIORITY_MEDIUM;
    b.content         = vformat("# Project Info\nName: %s\nGodot: %s", project_name, godot_version);
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
    // Sort by priority
    Vector<ContextBlock> sorted = blocks;
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
    // Remove lowest priority, oldest blocks first
    for (int p = (int)PRIORITY_LOW; p >= 0 && estimate_tokens() > token_budget; p--) {
        for (int i = blocks.size() - 1; i >= 0 && estimate_tokens() > token_budget; i--) {
            if ((int)blocks[i].priority == p) blocks.remove_at(i);
        }
    }
}

int ContextManager::_estimate_tokens(const String &text) const {
    // Rough estimate: 1 token per ~4 characters
    return text.length() / 4 + 1;
}

#pragma once
#include "core/object/ref_counted.h"
#include "modules/ai_engine/core/ai_types.h"

class Node;

class ContextManager : public RefCounted {
    GDCLASS(ContextManager, RefCounted);

    Vector<ContextBlock> blocks;
    int                  token_budget    = 6000;
    int64_t              last_refresh_ms = 0;
    bool                 auto_refresh    = true;
    String               godot_version   = "4.7-beta";
    String               project_name;
    String               project_path;

public:
    void set_token_budget(int budget) { token_budget = budget; }
    void set_auto_refresh(bool v)     { auto_refresh = v; }

    void refresh_scene_tree();
    void refresh_open_scripts();
    void refresh_project_info();
    void refresh_all();

    String build_context_string() const;
    int    estimate_tokens() const;

    void add_block(const String &section, const String &content,
        ContextSectionPriority priority = PRIORITY_MEDIUM);
    void remove_block(const String &section);
    void clear_blocks();

    const Vector<ContextBlock> &get_blocks() const { return blocks; }

    String get_godot_version() const { return godot_version; }
    String get_project_name()  const { return project_name; }
    String get_project_path()  const { return project_path; }

protected:
    static void _bind_methods();

private:
    void   _trim_to_budget();
    int    _estimate_tokens(const String &text) const;
    // Fix 10 — recursive scene tree dump helper
    String _dump_node_tree(Node *p_node, int p_depth) const;
};

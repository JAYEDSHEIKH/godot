#include "ai_tools_panel.h"
#include "scene/gui/separator.h"

AIToolsPanel::AIToolsPanel() {}

void AIToolsPanel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("log_tool_execution", "name", "result", "success"), &AIToolsPanel::log_tool_execution);
    ClassDB::bind_method(D_METHOD("refresh_execution_log"), &AIToolsPanel::refresh_execution_log);
}

void AIToolsPanel::_ready() {
    tab_container = memnew(TabContainer);
    tab_container->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(tab_container);

    // ── Execution Log Tab ─────────────────────────
    log_vbox = memnew(VBoxContainer);
    log_vbox->set_name("Execution Log");
    tab_container->add_child(log_vbox);

    // Status row
    status_hbox = memnew(HBoxContainer);
    log_vbox->add_child(status_hbox);

    tool_status_label = memnew(Label);
    tool_status_label->set_text("Ready");
    tool_status_label->set_h_size_flags(SIZE_EXPAND_FILL);
    status_hbox->add_child(tool_status_label);

    tool_progress = memnew(ProgressBar);
    tool_progress->set_custom_minimum_size(Vector2(80, 0));
    tool_progress->set_visible(false);
    status_hbox->add_child(tool_progress);

    // Log controls
    HBoxContainer *log_ctrl = memnew(HBoxContainer);
    log_vbox->add_child(log_ctrl);

    success_count_lbl = memnew(Label);
    success_count_lbl->set_text("✓ 0");
    success_count_lbl->set_h_size_flags(SIZE_EXPAND_FILL);
    log_ctrl->add_child(success_count_lbl);

    error_count_lbl = memnew(Label);
    error_count_lbl->set_text("✗ 0");
    error_count_lbl->set_h_size_flags(SIZE_EXPAND_FILL);
    log_ctrl->add_child(error_count_lbl);

    auto_scroll_cb = memnew(CheckBox);
    auto_scroll_cb->set_text("Auto-scroll");
    auto_scroll_cb->set_pressed(true);
    log_ctrl->add_child(auto_scroll_cb);

    clear_log_button = memnew(Button);
    clear_log_button->set_text("Clear");
    log_ctrl->add_child(clear_log_button);

    // Execution list
    execution_list = memnew(ItemList);
    execution_list->set_v_size_flags(SIZE_EXPAND_FILL);
    execution_list->set_custom_minimum_size(Vector2(0, 100));
    log_vbox->add_child(execution_list);

    execution_detail = memnew(TextEdit);
    execution_detail->set_custom_minimum_size(Vector2(0, 80));
    execution_detail->set_editable(false);
    log_vbox->add_child(execution_detail);

    // ── Available Tools Tab ───────────────────────
    VBoxContainer *tools_vbox = memnew(VBoxContainer);
    tools_vbox->set_name("Available Tools");
    tab_container->add_child(tools_vbox);

    tools_tree = memnew(Tree);
    tools_tree->set_v_size_flags(SIZE_EXPAND_FILL);
    tools_tree->set_custom_minimum_size(Vector2(0, 150));
    tools_tree->set_hide_root(true);
    tools_tree->set_column_count(2);
    tools_tree->set_column_title(0, "Tool Name");
    tools_tree->set_column_title(1, "Category");
    tools_vbox->add_child(tools_tree);

    tool_documentation = memnew(TextEdit);
    tool_documentation->set_custom_minimum_size(Vector2(0, 100));
    tool_documentation->set_editable(false);
    tools_vbox->add_child(tool_documentation);

    copy_tool_def_btn = memnew(Button);
    copy_tool_def_btn->set_text("Copy Tool Definition");
    tools_vbox->add_child(copy_tool_def_btn);

    // Connect
    clear_log_button->connect("pressed", callable_mp(this, &AIToolsPanel::_on_clear_log_pressed));

    _populate_tools_tree();
}

void AIToolsPanel::_populate_tools_tree() {
    if (!tools_tree) return;
    tools_tree->clear();
    TreeItem *root = tools_tree->create_item();

    static const struct { const char *name; const char *cat; } tools[] = {
        {"read_file",            "Files"},
        {"write_file",           "Files"},
        {"create_file",          "Files"},
        {"delete_file",          "Files"},
        {"list_directory",       "Files"},
        {"search_in_files",      "Files"},
        {"replace_in_file",      "Files"},
        {"copy_file",            "Files"},
        {"get_scene_tree",       "Nodes"},
        {"add_node",             "Nodes"},
        {"delete_node",          "Nodes"},
        {"set_node_property",    "Nodes"},
        {"get_node_property",    "Nodes"},
        {"connect_signal",       "Nodes"},
        {"create_script",        "Scripts"},
        {"attach_script",        "Scripts"},
        {"add_function",         "Scripts"},
        {"refactor_function",    "Scripts"},
        {"open_scene",           "Scenes"},
        {"save_scene",           "Scenes"},
        {"run_scene",            "Scenes"},
        {"get_project_settings", "Project"},
        {"get_build_errors",     "Project"},
        {"git_status",           "Git"},
        {"git_commit",           "Git"},
        {"audit_performance",    "Audit"},
        {"generate_documentation","Audit"},
        {nullptr, nullptr}
    };

    HashMap<String, TreeItem *> categories;
    for (int i = 0; tools[i].name; i++) {
        String cat = tools[i].cat;
        if (!categories.has(cat)) {
            TreeItem *cat_item = tools_tree->create_item(root);
            cat_item->set_text(0, cat);
            cat_item->set_selectable(0, false);
            categories[cat] = cat_item;
        }
        TreeItem *item = tools_tree->create_item(categories[cat]);
        item->set_text(0, tools[i].name);
        item->set_text(1, cat);
    }
}

void AIToolsPanel::_show_tool_documentation(const String &tool_name) {
    if (!tool_documentation) return;
    tool_documentation->set_text(
        "Tool: " + tool_name + "\n"
        "Use this tool to perform actions in the Godot project.\n"
        "Parameters depend on the specific tool."
    );
}

void AIToolsPanel::log_tool_execution(const String &tool_name, const String &result, bool success) {
    if (!execution_list) return;
    String prefix = success ? "✓ " : "✗ ";
    execution_list->add_item(prefix + tool_name);
    if (success) success_count++; else error_count++;
    if (success_count_lbl) success_count_lbl->set_text("✓ " + itos(success_count));
    if (error_count_lbl)   error_count_lbl->set_text("✗ " + itos(error_count));
    if (auto_scroll_cb && auto_scroll_cb->is_pressed()) {
        execution_list->ensure_current_is_visible();
    }
}

void AIToolsPanel::refresh_execution_log() {}

void AIToolsPanel::_on_tool_selected(int p_index) {}
void AIToolsPanel::_on_execution_selected(int p_index) {}

void AIToolsPanel::_on_clear_log_pressed() {
    if (execution_list) execution_list->clear();
    success_count = 0;
    error_count   = 0;
    if (success_count_lbl) success_count_lbl->set_text("✓ 0");
    if (error_count_lbl)   error_count_lbl->set_text("✗ 0");
}

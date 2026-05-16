#include "ai_memory_panel.h"
#include "scene/gui/separator.h"

AIMemoryPanel::AIMemoryPanel() {}

void AIMemoryPanel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("refresh_display"), &AIMemoryPanel::refresh_display);
}

void AIMemoryPanel::_ready() {
    set_name("Memory");

    HBoxContainer *header_row = memnew(HBoxContainer);
    add_child(header_row);
    Label *title = memnew(Label); title->set_text("AI Memory"); title->set_h_size_flags(SIZE_EXPAND_FILL);
    header_row->add_child(title);

    split = memnew(HSplitContainer);
    split->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(split);

    // ── Left Panel ────────────────────────────────
    left_panel = memnew(VBoxContainer);
    left_panel->set_h_size_flags(SIZE_EXPAND_FILL);
    left_panel->set_custom_minimum_size(Vector2(150, 0));
    split->add_child(left_panel);

    search_row = memnew(HBoxContainer);
    left_panel->add_child(search_row);

    search_field = memnew(LineEdit);
    search_field->set_placeholder_text("Search...");
    search_field->set_h_size_flags(SIZE_EXPAND_FILL);
    search_row->add_child(search_field);

    refresh_btn = memnew(Button);
    refresh_btn->set_text("↻");
    search_row->add_child(refresh_btn);

    category_filter = memnew(OptionButton);
    category_filter->add_item("All Categories");
    category_filter->add_item("developer");
    category_filter->add_item("project_type");
    category_filter->add_item("preferences");
    category_filter->add_item("code_patterns");
    category_filter->add_item("bugs");
    left_panel->add_child(category_filter);

    memory_tree = memnew(Tree);
    memory_tree->set_v_size_flags(SIZE_EXPAND_FILL);
    memory_tree->set_hide_root(true);
    memory_tree->set_column_count(2);
    memory_tree->set_column_title(0, "Fact");
    memory_tree->set_column_title(1, "Category");
    left_panel->add_child(memory_tree);

    memory_btn_row = memnew(HBoxContainer);
    left_panel->add_child(memory_btn_row);

    delete_btn = memnew(Button); delete_btn->set_text("Delete"); memory_btn_row->add_child(delete_btn);
    clear_all_btn = memnew(Button); clear_all_btn->set_text("Clear All"); memory_btn_row->add_child(clear_all_btn);

    count_label = memnew(Label);
    count_label->set_text("0 facts stored");
    count_label->add_theme_font_size_override("font_size", 10);
    left_panel->add_child(count_label);

    // ── Right Panel ───────────────────────────────
    right_panel = memnew(VBoxContainer);
    right_panel->set_h_size_flags(SIZE_EXPAND_FILL);
    split->add_child(right_panel);

    detail_header = memnew(Label); detail_header->set_text("Selected Fact:"); right_panel->add_child(detail_header);
    detail_text = memnew(TextEdit);
    detail_text->set_custom_minimum_size(Vector2(0, 80));
    detail_text->set_editable(false);
    right_panel->add_child(detail_text);

    HSeparator *sep = memnew(HSeparator); right_panel->add_child(sep);

    add_header = memnew(Label); add_header->set_text("Add Memory Fact:"); right_panel->add_child(add_header);

    add_category = memnew(OptionButton);
    add_category->add_item("developer");
    add_category->add_item("project_type");
    add_category->add_item("preferences");
    add_category->add_item("code_patterns");
    add_category->add_item("bugs");
    add_category->add_item("custom");
    right_panel->add_child(add_category);

    add_content = memnew(TextEdit);
    add_content->set_placeholder_text("Enter a fact to remember...");
    add_content->set_custom_minimum_size(Vector2(0, 60));
    right_panel->add_child(add_content);

    add_btn = memnew(Button); add_btn->set_text("Add Fact"); right_panel->add_child(add_btn);

    // Connect
    search_field->connect("text_changed",   callable_mp(this, &AIMemoryPanel::_on_search_changed));
    delete_btn->connect("pressed",          callable_mp(this, &AIMemoryPanel::_on_delete_pressed));
    clear_all_btn->connect("pressed",       callable_mp(this, &AIMemoryPanel::_on_clear_all_pressed));
    add_btn->connect("pressed",             callable_mp(this, &AIMemoryPanel::_on_add_pressed));
    refresh_btn->connect("pressed",         callable_mp(this, &AIMemoryPanel::_on_refresh_pressed));
    memory_tree->connect("item_selected",   callable_mp(this, &AIMemoryPanel::_on_memory_selected));
}

void AIMemoryPanel::refresh_display() { _populate_tree(); }

void AIMemoryPanel::_populate_tree(const String &filter, const String &category) {
    if (!memory_tree) return;
    memory_tree->clear();
    TreeItem *root = memory_tree->create_item();
    // Would populate from MemoryManager::get_singleton()
    count_label->set_text("0 facts stored");
}

void AIMemoryPanel::add_manual_fact(const String &category, const String &content) {
    _populate_tree();
}

void AIMemoryPanel::_on_search_changed(const String &text) { _populate_tree(text); }
void AIMemoryPanel::_on_memory_selected() {}
void AIMemoryPanel::_on_delete_pressed() {}
void AIMemoryPanel::_on_clear_all_pressed() {}
void AIMemoryPanel::_on_add_pressed() {
    if (!add_content || !add_category) return;
    String content = add_content->get_text().strip_edges();
    if (content.is_empty()) return;
    String cat = add_category->get_item_text(add_category->get_selected());
    add_manual_fact(cat, content);
    add_content->set_text("");
}
void AIMemoryPanel::_on_refresh_pressed() { refresh_display(); }

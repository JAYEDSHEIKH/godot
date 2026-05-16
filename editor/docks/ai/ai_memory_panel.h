#pragma once
#include "scene/gui/box_container.h"
#include "scene/gui/tree.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/option_button.h"

class AIMemoryPanel : public VBoxContainer {
    GDCLASS(AIMemoryPanel, VBoxContainer);

public:
    AIMemoryPanel();

    void refresh_display();
    void add_manual_fact(const String &category, const String &content);

protected:
    void _ready() override;
    static void _bind_methods();

private:
    HSplitContainer  *split           = nullptr;
    VBoxContainer    *left_panel      = nullptr;
    VBoxContainer    *right_panel     = nullptr;

    // Left: memory browser
    HBoxContainer    *search_row      = nullptr;
    LineEdit         *search_field    = nullptr;
    OptionButton     *category_filter = nullptr;
    Button           *refresh_btn     = nullptr;
    Tree             *memory_tree     = nullptr;
    HBoxContainer    *memory_btn_row  = nullptr;
    Button           *delete_btn      = nullptr;
    Button           *clear_all_btn   = nullptr;
    Label            *count_label     = nullptr;

    // Right: detail + add
    Label            *detail_header   = nullptr;
    TextEdit         *detail_text     = nullptr;
    Label            *add_header      = nullptr;
    OptionButton     *add_category    = nullptr;
    TextEdit         *add_content     = nullptr;
    Button           *add_btn         = nullptr;

    void _on_search_changed(const String &text);
    void _on_memory_selected();
    void _on_delete_pressed();
    void _on_clear_all_pressed();
    void _on_add_pressed();
    void _on_refresh_pressed();
    void _populate_tree(const String &filter = "", const String &category = "");
};

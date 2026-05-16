#pragma once
#include "scene/gui/box_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/tree.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/check_box.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/progress_bar.h"

class AIToolsPanel : public VBoxContainer {
    GDCLASS(AIToolsPanel, VBoxContainer);

public:
    AIToolsPanel();

    void log_tool_execution(const String &tool_name, const String &result, bool success);
    void refresh_execution_log();

protected:
    void _ready() override;
    static void _bind_methods();

private:
    TabContainer    *tab_container      = nullptr;

    // Execution Log Tab
    VBoxContainer   *log_vbox           = nullptr;
    ItemList        *execution_list     = nullptr;
    TextEdit        *execution_detail   = nullptr;
    Button          *clear_log_button   = nullptr;
    CheckBox        *auto_scroll_cb     = nullptr;
    Label           *success_count_lbl  = nullptr;
    Label           *error_count_lbl    = nullptr;

    // Available Tools Tab
    Tree            *tools_tree         = nullptr;
    TextEdit        *tool_documentation = nullptr;
    Button          *copy_tool_def_btn  = nullptr;

    // Status
    HBoxContainer   *status_hbox        = nullptr;
    Label           *tool_status_label  = nullptr;
    ProgressBar     *tool_progress      = nullptr;

    int success_count = 0;
    int error_count   = 0;

    void _on_tool_selected(int p_index);
    void _on_execution_selected(int p_index);
    void _on_clear_log_pressed();
    void _populate_tools_tree();
    void _show_tool_documentation(const String &tool_name);
};

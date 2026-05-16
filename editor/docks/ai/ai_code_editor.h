#pragma once
#include "scene/gui/box_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/code_edit.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/split_container.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/option_button.h"

// Diff viewer and code comparison panel
class AIDiffViewer : public VBoxContainer {
    GDCLASS(AIDiffViewer, VBoxContainer);

public:
    AIDiffViewer();

    void set_diff(const String &p_original, const String &p_modified, const String &p_file_path = "");
    void clear_diff();
    bool has_diff() const { return !original_code.is_empty(); }

    String get_modified() const { return modified_code; }

protected:
    void _ready() override;
    static void _bind_methods();

private:
    String original_code;
    String modified_code;
    String file_path;

    HBoxContainer  *header_row     = nullptr;
    Label          *file_label     = nullptr;
    Label          *stats_label    = nullptr;
    Button         *apply_btn      = nullptr;
    Button         *reject_btn     = nullptr;
    Button         *copy_btn       = nullptr;
    OptionButton   *view_mode      = nullptr;

    HSplitContainer *split         = nullptr;
    PanelContainer  *left_panel    = nullptr;
    PanelContainer  *right_panel   = nullptr;
    CodeEdit        *original_view = nullptr;
    CodeEdit        *modified_view = nullptr;

    RichTextLabel   *unified_view  = nullptr;

    void _show_split_view();
    void _show_unified_view();
    void _compute_diff();
    String _highlight_diff_line(const String &line, bool is_add);
    void _on_apply_pressed();
    void _on_reject_pressed();
    void _on_copy_pressed();
    void _on_view_mode_changed(int idx);
};

#include "ai_code_editor.h"
#include "scene/gui/separator.h"
#include "core/io/file_access.h"

AIDiffViewer::AIDiffViewer() {}

void AIDiffViewer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_diff", "original", "modified", "file_path"), &AIDiffViewer::set_diff);
    ClassDB::bind_method(D_METHOD("clear_diff"),                                     &AIDiffViewer::clear_diff);
    ClassDB::bind_method(D_METHOD("has_diff"),                                       &AIDiffViewer::has_diff);
    ClassDB::bind_method(D_METHOD("get_modified"),                                   &AIDiffViewer::get_modified);
}

void AIDiffViewer::_ready() {
    // Header
    header_row = memnew(HBoxContainer);
    add_child(header_row);

    file_label = memnew(Label);
    file_label->set_text("No diff loaded");
    file_label->set_h_size_flags(SIZE_EXPAND_FILL);
    file_label->add_theme_font_size_override("font_size", 11);
    header_row->add_child(file_label);

    stats_label = memnew(Label);
    stats_label->set_text("");
    header_row->add_child(stats_label);

    view_mode = memnew(OptionButton);
    view_mode->add_item("Unified");
    view_mode->add_item("Split");
    header_row->add_child(view_mode);

    copy_btn = memnew(Button);
    copy_btn->set_text("Copy");
    header_row->add_child(copy_btn);

    apply_btn = memnew(Button);
    apply_btn->set_text("✓ Apply");
    apply_btn->set_visible(false);
    header_row->add_child(apply_btn);

    reject_btn = memnew(Button);
    reject_btn->set_text("✗ Reject");
    reject_btn->set_visible(false);
    header_row->add_child(reject_btn);

    // Split view
    split = memnew(HSplitContainer);
    split->set_v_size_flags(SIZE_EXPAND_FILL);
    split->set_visible(false);
    add_child(split);

    left_panel = memnew(PanelContainer);
    left_panel->set_h_size_flags(SIZE_EXPAND_FILL);
    split->add_child(left_panel);

    original_view = memnew(CodeEdit);
    original_view->set_editable(false);
    original_view->set_h_size_flags(SIZE_EXPAND_FILL);
    original_view->set_v_size_flags(SIZE_EXPAND_FILL);
    left_panel->add_child(original_view);

    right_panel = memnew(PanelContainer);
    right_panel->set_h_size_flags(SIZE_EXPAND_FILL);
    split->add_child(right_panel);

    modified_view = memnew(CodeEdit);
    modified_view->set_editable(true);
    modified_view->set_h_size_flags(SIZE_EXPAND_FILL);
    modified_view->set_v_size_flags(SIZE_EXPAND_FILL);
    right_panel->add_child(modified_view);

    // Unified view
    unified_view = memnew(RichTextLabel);
    unified_view->set_use_bbcode(true);
    unified_view->set_selection_enabled(true);
    unified_view->set_v_size_flags(SIZE_EXPAND_FILL);
    unified_view->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(unified_view);

    // Connect
    apply_btn->connect("pressed",               callable_mp(this, &AIDiffViewer::_on_apply_pressed));
    reject_btn->connect("pressed",              callable_mp(this, &AIDiffViewer::_on_reject_pressed));
    copy_btn->connect("pressed",                callable_mp(this, &AIDiffViewer::_on_copy_pressed));
    view_mode->connect("item_selected",         callable_mp(this, &AIDiffViewer::_on_view_mode_changed));
}

void AIDiffViewer::set_diff(const String &p_original, const String &p_modified, const String &p_fp) {
    original_code = p_original;
    modified_code = p_modified;
    file_path     = p_fp;
    file_label->set_text(p_fp.is_empty() ? "Untitled diff" : p_fp);
    apply_btn->set_visible(!p_fp.is_empty());
    reject_btn->set_visible(true);
    _compute_diff();
}

void AIDiffViewer::clear_diff() {
    original_code = "";
    modified_code = "";
    file_path     = "";
    file_label->set_text("No diff loaded");
    if (unified_view) unified_view->clear();
    if (original_view) original_view->set_text("");
    if (modified_view) modified_view->set_text("");
    apply_btn->set_visible(false);
    reject_btn->set_visible(false);
}

void AIDiffViewer::_compute_diff() {
    if (!unified_view) return;
    unified_view->clear();

    Vector<String> orig_lines = original_code.split("\n");
    Vector<String> mod_lines  = modified_code.split("\n");

    int additions = 0, removals = 0;
    String bbcode;

    // Simplified line-by-line diff
    int max_lines = MAX(orig_lines.size(), mod_lines.size());
    for (int i = 0; i < max_lines; i++) {
        String orig = (i < orig_lines.size()) ? orig_lines[i] : "";
        String mod  = (i < mod_lines.size())  ? mod_lines[i]  : "";
        if (orig == mod) {
            bbcode += "[color=#888888]  " + itos(i + 1) + " | " + orig.xml_escape() + "[/color]\n";
        } else if (orig.is_empty()) {
            bbcode += "[bgcolor=#1a3a1a][color=#90ff90]+ " + itos(i + 1) + " | " + mod.xml_escape() + "[/color][/bgcolor]\n";
            additions++;
        } else if (mod.is_empty()) {
            bbcode += "[bgcolor=#3a1a1a][color=#ff9090]- " + itos(i + 1) + " | " + orig.xml_escape() + "[/color][/bgcolor]\n";
            removals++;
        } else {
            bbcode += "[bgcolor=#3a1a1a][color=#ff9090]- " + itos(i + 1) + " | " + orig.xml_escape() + "[/color][/bgcolor]\n";
            bbcode += "[bgcolor=#1a3a1a][color=#90ff90]+ " + itos(i + 1) + " | " + mod.xml_escape()  + "[/color][/bgcolor]\n";
            additions++; removals++;
        }
    }

    unified_view->append_text(bbcode);
    stats_label->set_text(vformat("+%d -%d", additions, removals));

    if (original_view) original_view->set_text(original_code);
    if (modified_view) modified_view->set_text(modified_code);
}

String AIDiffViewer::_highlight_diff_line(const String &line, bool is_add) {
    return is_add ? ("[color=#90ff90]" + line.xml_escape() + "[/color]")
                  : ("[color=#ff9090]" + line.xml_escape() + "[/color]");
}

void AIDiffViewer::_on_apply_pressed() {
    if (file_path.is_empty()) return;
    Ref<FileAccess> f = FileAccess::open(file_path, FileAccess::WRITE);
    if (f.is_valid()) {
        f->store_string(modified_code);
        emit_signal("diff_applied", file_path);
        clear_diff();
    }
}

void AIDiffViewer::_on_reject_pressed() {
    clear_diff();
    emit_signal("diff_rejected");
}

void AIDiffViewer::_on_copy_pressed() {
    // Copy modified to clipboard
}

void AIDiffViewer::_on_view_mode_changed(int idx) {
    split->set_visible(idx == 1);
    unified_view->set_visible(idx == 0);
}

void AIDiffViewer::_show_split_view() { split->set_visible(true); unified_view->set_visible(false); }
void AIDiffViewer::_show_unified_view() { split->set_visible(false); unified_view->set_visible(true); }

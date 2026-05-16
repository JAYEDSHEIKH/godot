#pragma once
#include "scene/gui/box_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/check_box.h"
#include "scene/gui/split_container.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/progress_bar.h"

class AIChatPanel : public VBoxContainer {
    GDCLASS(AIChatPanel, VBoxContainer);

public:
    AIChatPanel();

    void append_user_message(const String &p_text);
    void append_assistant_message(const String &p_text);
    void append_tool_call(const String &p_tool_name, const String &p_result);
    void append_system_message(const String &p_text);
    void clear_conversation();
    void set_streaming_in_progress(bool p_streaming);
    void append_streaming_chunk(const String &p_chunk);

    String get_input_text() const;
    void   clear_input();

protected:
    void _ready() override;
    static void _bind_methods();

private:
    VSplitContainer  *split        = nullptr;
    PanelContainer   *chat_bg      = nullptr;
    RichTextLabel    *chat_history = nullptr;

    VBoxContainer    *input_area   = nullptr;
    HBoxContainer    *input_hbox   = nullptr;
    TextEdit         *input_field  = nullptr;
    VBoxContainer    *btn_vbox     = nullptr;
    Button           *send_btn     = nullptr;
    Button           *stop_btn     = nullptr;

    HBoxContainer    *options_hbox = nullptr;
    CheckBox         *auto_context_cb = nullptr;
    CheckBox         *use_memory_cb   = nullptr;
    CheckBox         *use_rag_cb      = nullptr;

    Label            *token_label  = nullptr;
    ProgressBar      *streaming_bar = nullptr;

    bool is_streaming = false;
    String current_stream_text;

    void _on_send_pressed();
    void _on_stop_pressed();
    void _on_input_text_changed();
    void _submit_message();
    String _format_user_bubble(const String &text);
    String _format_assistant_bubble(const String &text);
    String _format_tool_bubble(const String &name, const String &result);
    String _format_system_message(const String &text);
};

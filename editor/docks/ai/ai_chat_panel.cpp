#include "ai_chat_panel.h"
#include "scene/gui/separator.h"
#include "core/os/time.h"

AIChatPanel::AIChatPanel() {}

void AIChatPanel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("append_user_message",      "text"), &AIChatPanel::append_user_message);
    ClassDB::bind_method(D_METHOD("append_assistant_message", "text"), &AIChatPanel::append_assistant_message);
    ClassDB::bind_method(D_METHOD("clear_conversation"),               &AIChatPanel::clear_conversation);
    ClassDB::bind_method(D_METHOD("set_streaming_in_progress", "v"),   &AIChatPanel::set_streaming_in_progress);
    ClassDB::bind_method(D_METHOD("append_streaming_chunk",    "chunk"),&AIChatPanel::append_streaming_chunk);
    ClassDB::bind_method(D_METHOD("get_input_text"),                   &AIChatPanel::get_input_text);
    ClassDB::bind_method(D_METHOD("clear_input"),                      &AIChatPanel::clear_input);
}

void AIChatPanel::_ready() {
    set_custom_minimum_size(Vector2(0, 300));

    // Main split: chat history / input area
    split = memnew(VSplitContainer);
    split->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(split);

    // Chat history
    chat_bg = memnew(PanelContainer);
    chat_bg->set_v_size_flags(SIZE_EXPAND_FILL);
    chat_bg->set_custom_minimum_size(Vector2(0, 200));
    split->add_child(chat_bg);

    chat_history = memnew(RichTextLabel);
    chat_history->set_use_bbcode(true);
    chat_history->set_scroll_follow(true);
    chat_history->set_selection_enabled(true);
    chat_history->set_context_menu_enabled(true);
    chat_history->set_v_size_flags(SIZE_EXPAND_FILL);
    chat_history->set_h_size_flags(SIZE_EXPAND_FILL);
    chat_bg->add_child(chat_history);

    // Streaming indicator
    streaming_bar = memnew(ProgressBar);
    streaming_bar->set_indeterminate(true);
    streaming_bar->set_visible(false);
    add_child(streaming_bar);

    // Input area
    input_area = memnew(VBoxContainer);
    input_area->set_custom_minimum_size(Vector2(0, 80));
    split->add_child(input_area);

    // Options row
    options_hbox = memnew(HBoxContainer);
    input_area->add_child(options_hbox);

    auto_context_cb = memnew(CheckBox);
    auto_context_cb->set_text("Context");
    auto_context_cb->set_pressed(true);
    auto_context_cb->set_tooltip_text("Include scene/script context automatically");
    options_hbox->add_child(auto_context_cb);

    use_memory_cb = memnew(CheckBox);
    use_memory_cb->set_text("Memory");
    use_memory_cb->set_pressed(true);
    use_memory_cb->set_tooltip_text("Include memory facts in context");
    options_hbox->add_child(use_memory_cb);

    use_rag_cb = memnew(CheckBox);
    use_rag_cb->set_text("RAG");
    use_rag_cb->set_pressed(true);
    use_rag_cb->set_tooltip_text("Search codebase for relevant snippets");
    options_hbox->add_child(use_rag_cb);

    token_label = memnew(Label);
    token_label->set_text("~0 tokens");
    token_label->set_h_size_flags(SIZE_EXPAND_FILL);
    token_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
    token_label->add_theme_font_size_override("font_size", 10);
    options_hbox->add_child(token_label);

    // Input row
    input_hbox = memnew(HBoxContainer);
    input_hbox->set_v_size_flags(SIZE_EXPAND_FILL);
    input_area->add_child(input_hbox);

    input_field = memnew(TextEdit);
    input_field->set_h_size_flags(SIZE_EXPAND_FILL);
    input_field->set_v_size_flags(SIZE_EXPAND_FILL);
    input_field->set_placeholder_text("Ask your AI Copilot... (Shift+Enter to send, /plan /debug /brainstorm)");
    input_field->set_wrap_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
    input_hbox->add_child(input_field);

    btn_vbox = memnew(VBoxContainer);
    input_hbox->add_child(btn_vbox);

    send_btn = memnew(Button);
    send_btn->set_text("▶");
    send_btn->set_tooltip_text("Send message (Shift+Enter)");
    send_btn->set_custom_minimum_size(Vector2(40, 0));
    send_btn->set_v_size_flags(SIZE_EXPAND_FILL);
    btn_vbox->add_child(send_btn);

    stop_btn = memnew(Button);
    stop_btn->set_text("■");
    stop_btn->set_tooltip_text("Stop generation");
    stop_btn->set_custom_minimum_size(Vector2(40, 0));
    stop_btn->set_visible(false);
    btn_vbox->add_child(stop_btn);

    // Welcome message
    append_system_message("Welcome to **Godot AI Copilot** — your built-in AI assistant.\n\n"
        "• Chat naturally about your project\n"
        "• Use /plan, /debug, /brainstorm, /review\n"
        "• I can read files, modify scripts, add nodes, and more\n"
        "• Configure your API key in **Settings**\n");

    // Connect signals
    send_btn->connect("pressed", callable_mp(this, &AIChatPanel::_on_send_pressed));
    stop_btn->connect("pressed", callable_mp(this, &AIChatPanel::_on_stop_pressed));
    input_field->connect("text_changed", callable_mp(this, &AIChatPanel::_on_input_text_changed));
}

void AIChatPanel::_on_send_pressed() { _submit_message(); }

void AIChatPanel::_on_stop_pressed() {
    set_streaming_in_progress(false);
    append_system_message("Generation stopped.");
}

void AIChatPanel::_on_input_text_changed() {
    String text = input_field->get_text();
    int tokens = text.length() / 4;
    token_label->set_text(vformat("~%d tokens", tokens));
}

void AIChatPanel::_submit_message() {
    String text = input_field->get_text().strip_edges();
    if (text.is_empty()) return;
    clear_input();
    append_user_message(text);
    set_streaming_in_progress(true);
    // Dispatch to AI engine would happen here
    emit_signal("message_submitted", text);
}

String AIChatPanel::_format_user_bubble(const String &text) {
    return "[bgcolor=#2a4a6a][color=#ffffff][b]You:[/b][/color]\n" + text + "[/bgcolor]\n\n";
}

String AIChatPanel::_format_assistant_bubble(const String &text) {
    return "[bgcolor=#1a3a1a][color=#90ff90][b]AI Copilot:[/b][/color]\n" + text + "[/bgcolor]\n\n";
}

String AIChatPanel::_format_tool_bubble(const String &name, const String &result) {
    return "[color=#888888][i]🔧 Tool: " + name + "[/i]\n" + result + "[/color]\n\n";
}

String AIChatPanel::_format_system_message(const String &text) {
    return "[color=#888888][i]" + text + "[/i][/color]\n\n";
}

void AIChatPanel::append_user_message(const String &p_text) {
    chat_history->append_text(_format_user_bubble(p_text));
}

void AIChatPanel::append_assistant_message(const String &p_text) {
    if (is_streaming) current_stream_text = "";
    chat_history->append_text(_format_assistant_bubble(p_text));
    set_streaming_in_progress(false);
}

void AIChatPanel::append_tool_call(const String &p_tool_name, const String &p_result) {
    chat_history->append_text(_format_tool_bubble(p_tool_name, p_result));
}

void AIChatPanel::append_system_message(const String &p_text) {
    chat_history->append_text(_format_system_message(p_text));
}

void AIChatPanel::append_streaming_chunk(const String &p_chunk) {
    current_stream_text += p_chunk;
    // Update last message in-place (simplified: just append)
    chat_history->append_text(p_chunk);
}

void AIChatPanel::clear_conversation() {
    chat_history->clear();
    append_system_message("Conversation cleared.");
}

void AIChatPanel::set_streaming_in_progress(bool p_streaming) {
    is_streaming = p_streaming;
    if (streaming_bar) streaming_bar->set_visible(p_streaming);
    if (send_btn) send_btn->set_visible(!p_streaming);
    if (stop_btn) stop_btn->set_visible(p_streaming);
}

String AIChatPanel::get_input_text() const {
    return input_field ? input_field->get_text() : "";
}

void AIChatPanel::clear_input() {
    if (input_field) input_field->set_text("");
}

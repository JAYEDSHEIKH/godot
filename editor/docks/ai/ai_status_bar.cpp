#include "ai_status_bar.h"
#include "scene/gui/separator.h"

AIStatusBar::AIStatusBar() {}

void AIStatusBar::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_status", "state", "message"),  &AIStatusBar::set_status);
    ClassDB::bind_method(D_METHOD("set_progress", "value"),           &AIStatusBar::set_progress);
    ClassDB::bind_method(D_METHOD("set_token_count", "count"),        &AIStatusBar::set_token_count);
    ClassDB::bind_method(D_METHOD("set_context_usage", "percentage"), &AIStatusBar::set_context_usage);
}

void AIStatusBar::_ready() {
    set_custom_minimum_size(Vector2(0, 24));

    status_label = memnew(Label);
    status_label->set_text("● Ready");
    status_label->set_custom_minimum_size(Vector2(80, 0));
    add_child(status_label);

    VSeparator *sep1 = memnew(VSeparator);
    add_child(sep1);

    message_label = memnew(Label);
    message_label->set_text("");
    message_label->set_h_size_flags(SIZE_EXPAND_FILL);
    message_label->add_theme_font_size_override("font_size", 10);
    add_child(message_label);

    VSeparator *sep2 = memnew(VSeparator);
    add_child(sep2);

    progress_bar = memnew(ProgressBar);
    progress_bar->set_custom_minimum_size(Vector2(80, 0));
    progress_bar->set_visible(false);
    progress_bar->set_value(0);
    add_child(progress_bar);

    token_label = memnew(Label);
    token_label->set_text("0 tokens");
    token_label->add_theme_font_size_override("font_size", 10);
    add_child(token_label);

    VSeparator *sep3 = memnew(VSeparator);
    add_child(sep3);

    context_usage_label = memnew(Label);
    context_usage_label->set_text("Context: 0%");
    context_usage_label->add_theme_font_size_override("font_size", 10);
    add_child(context_usage_label);
}

void AIStatusBar::_process(double p_delta) {
    if (current_state == STATUS_THINKING || current_state == STATUS_EXECUTING_TOOL) {
        pulse_time += p_delta;
        int dots = ((int)(pulse_time * 3.0f) % 4);
        String ellipsis;
        for (int i = 0; i < dots; i++) ellipsis += ".";
        message_label->set_text(String("Processing") + ellipsis);
    }
}

void AIStatusBar::set_status(StatusState p_state, const String &p_message) {
    current_state = p_state;
    pulse_time = 0.0f;
    switch (p_state) {
        case STATUS_IDLE:           status_label->set_text("● Ready"); break;
        case STATUS_THINKING:       status_label->set_text("◉ Thinking"); break;
        case STATUS_EXECUTING_TOOL: status_label->set_text("⚙ Tool"); break;
        case STATUS_INDEXING:       status_label->set_text("⟳ Indexing"); break;
        case STATUS_ERROR:          status_label->set_text("✗ Error"); break;
    }
    if (!p_message.is_empty()) message_label->set_text(p_message);
    if (progress_bar) progress_bar->set_visible(p_state != STATUS_IDLE && p_state != STATUS_ERROR);
}

void AIStatusBar::set_progress(float p_value) {
    if (progress_bar) {
        progress_bar->set_visible(true);
        progress_bar->set_value(p_value * 100.0f);
    }
}

void AIStatusBar::set_token_count(int p_count) {
    if (token_label) token_label->set_text(vformat("%d tokens", p_count));
}

void AIStatusBar::set_context_usage(float p_percentage) {
    if (context_usage_label) context_usage_label->set_text(vformat("Context: %d%%", (int)(p_percentage * 100)));
}

void AIStatusBar::_update_display() {}

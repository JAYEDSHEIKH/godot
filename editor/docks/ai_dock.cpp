#include "ai_dock.h"
#include "ai/ai_chat_panel.h"
#include "ai/ai_settings_panel.h"
#include "ai/ai_memory_panel.h"
#include "ai/ai_tools_panel.h"
#include "ai/ai_status_bar.h"
#include "scene/gui/separator.h"

AIDock::AIDock() {
    set_title("AI Copilot");
    set_layout_key("AIAssistant");
    set_default_slot(DOCK_SLOT_RIGHT_BL);
}

AIDock::~AIDock() {}

void AIDock::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize"), &AIDock::initialize);
}

void AIDock::_ready() {
    _setup_ui();
    _connect_signals();
    _apply_theme();
}

void AIDock::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED) {
        _apply_theme();
    }
}

void AIDock::initialize() {
    _setup_providers();
}

void AIDock::_setup_ui() {
    vbox = memnew(VBoxContainer);
    vbox->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
    add_child(vbox);

    // Toolbar
    toolbar_hbox = memnew(HBoxContainer);
    vbox->add_child(toolbar_hbox);

    new_session_btn = memnew(Button);
    new_session_btn->set_text("New");
    new_session_btn->set_tooltip_text("Start a new conversation session");
    toolbar_hbox->add_child(new_session_btn);

    save_session_btn = memnew(Button);
    save_session_btn->set_text("Save");
    toolbar_hbox->add_child(save_session_btn);

    sessions_dropdown = memnew(OptionButton);
    sessions_dropdown->set_h_size_flags(SIZE_EXPAND_FILL);
    sessions_dropdown->set_tooltip_text("Switch sessions");
    toolbar_hbox->add_child(sessions_dropdown);

    clear_history_btn = memnew(Button);
    clear_history_btn->set_text("Clear");
    toolbar_hbox->add_child(clear_history_btn);

    settings_button = memnew(Button);
    settings_button->set_text(U"\u2699");
    toolbar_hbox->add_child(settings_button);

    // Provider label
    provider_label = memnew(Label);
    provider_label->set_text("No Provider");
    provider_label->add_theme_font_size_override("font_size", 10);
    vbox->add_child(provider_label);

    HSeparator *sep = memnew(HSeparator);
    vbox->add_child(sep);

    // Main tabs
    main_tabs = memnew(TabContainer);
    main_tabs->set_v_size_flags(SIZE_EXPAND_FILL);
    vbox->add_child(main_tabs);

    chat_panel = memnew(AIChatPanel);
    chat_panel->set_name("Chat");
    main_tabs->add_child(chat_panel);

    memory_panel = memnew(AIMemoryPanel);
    memory_panel->set_name("Memory");
    main_tabs->add_child(memory_panel);

    tools_panel = memnew(AIToolsPanel);
    tools_panel->set_name("Tools");
    main_tabs->add_child(tools_panel);

    settings_panel = memnew(AISettingsPanel);
    settings_panel->set_name("Settings");
    main_tabs->add_child(settings_panel);

    // Status bar at bottom
    HSeparator *sep2 = memnew(HSeparator);
    vbox->add_child(sep2);

    status_bar = memnew(AIStatusBar);
    vbox->add_child(status_bar);
}

void AIDock::_setup_providers() {}

void AIDock::_connect_signals() {
    if (new_session_btn) {
        new_session_btn->connect("pressed", callable_mp(this, &AIDock::_on_new_session));
    }
    if (save_session_btn) {
        save_session_btn->connect("pressed", callable_mp(this, &AIDock::_on_save_session));
    }
    if (clear_history_btn) {
        clear_history_btn->connect("pressed", callable_mp(this, &AIDock::_on_clear_history));
    }
    if (main_tabs) {
        main_tabs->connect("tab_changed", callable_mp(this, &AIDock::_on_tab_changed));
    }
}

void AIDock::_apply_theme() {}

void AIDock::_on_new_session() {
    if (chat_panel) {
        chat_panel->clear_conversation();
    }
    if (status_bar) {
        status_bar->set_status(AIStatusBar::STATUS_IDLE, "New session");
    }
}

void AIDock::_on_save_session() {}

void AIDock::_on_clear_history() {
    if (chat_panel) {
        chat_panel->clear_conversation();
    }
}

void AIDock::_on_tab_changed(int p_index) {}

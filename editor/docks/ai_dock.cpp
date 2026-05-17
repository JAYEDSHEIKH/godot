#include "ai_dock.h"
#include "ai/ai_chat_panel.h"
#include "ai/ai_settings_panel.h"
#include "ai/ai_memory_panel.h"
#include "ai/ai_tools_panel.h"
#include "ai/ai_status_bar.h"
#include "scene/gui/separator.h"
#include "core/io/json.h"
#include "modules/ai_engine/core/ai_engine.h"
#include "modules/ai_engine/provider/ai_provider.h"

AIDock::AIDock() {
    set_title("AI Copilot");
    set_layout_key("AIAssistant");
    set_default_slot(DOCK_SLOT_RIGHT_BL);
}

AIDock::~AIDock() {}

void AIDock::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize"), &AIDock::initialize);
    ClassDB::bind_method(D_METHOD("_on_response_chunk", "chunk"),
        &AIDock::_on_response_chunk);
    ClassDB::bind_method(D_METHOD("_on_tool_output", "call_id", "result"),
        &AIDock::_on_tool_output);
    ClassDB::bind_method(D_METHOD("_on_provider_error", "error"),
        &AIDock::_on_provider_error);
}

void AIDock::_ready() {
    _setup_ui();
    _connect_signals();
    _apply_theme();
    // Wire to AIEngine after we're in the scene tree
    _setup_providers();
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

// Fix 8 — fully wire dock to AIEngine and inject HTTPRequest into provider
void AIDock::_setup_providers() {
    AIEngine *engine = AIEngine::get_singleton();
    if (!engine) return;

    // initialize() creates providers (still RefCounted) and wires their signals
    engine->initialize();

    // Inject the HTTPRequest node into the provider so HTTP calls work (Fix 5)
    Ref<AIProvider> prov = engine->get_provider();
    if (prov.is_valid()) {
        if (!provider_http) {
            provider_http = memnew(HTTPRequest);
            add_child(provider_http);
        }
        prov->setup_http_request(provider_http);
    }

    // Wire AIEngine signals → dock UI handlers
    if (!engine->is_connected("response_chunk_received",
            callable_mp(this, &AIDock::_on_response_chunk))) {
        engine->connect("response_chunk_received",
            callable_mp(this, &AIDock::_on_response_chunk));
    }
    if (!engine->is_connected("tool_output_received",
            callable_mp(this, &AIDock::_on_tool_output))) {
        engine->connect("tool_output_received",
            callable_mp(this, &AIDock::_on_tool_output));
    }
    if (!engine->is_connected("tool_confirmation_required",
            callable_mp(this, &AIDock::_on_tool_confirmation_required))) {
        engine->connect("tool_confirmation_required",
            callable_mp(this, &AIDock::_on_tool_confirmation_required));
    }

    // Wire provider status → status bar
    if (prov.is_valid()) {
        if (!prov->is_connected("response_error",
                callable_mp(this, &AIDock::_on_provider_error))) {
            prov->connect("response_error",
                callable_mp(this, &AIDock::_on_provider_error));
        }
    }

    // Fix 9 — wire chat panel message → AIEngine::send_message
    if (chat_panel && !chat_panel->is_connected("message_submitted",
            callable_mp(engine, &AIEngine::send_message))) {
        chat_panel->connect("message_submitted",
            callable_mp(engine, &AIEngine::send_message));
    }

    // Update provider label
    if (provider_label && prov.is_valid()) {
        provider_label->set_text(prov->get_provider_name() + " / " + prov->get_model_name());
    }
}

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

// Fix 8 — response chunk forwarded to chat panel streaming bubble
void AIDock::_on_response_chunk(const String &p_chunk) {
    if (chat_panel) chat_panel->append_streaming_chunk(p_chunk);
}

// Fix 8 — tool output shown in chat
void AIDock::_on_tool_output(const String &p_call_id, const String &p_result) {
    if (chat_panel) chat_panel->append_tool_call(p_call_id, p_result);
}

// Fix 8 — tool needs confirmation, notify user
void AIDock::_on_tool_confirmation_required(const String &p_tool, const Dictionary &p_args) {
    if (chat_panel) {
        String msg = "[color=#ffaa00][b]Tool Approval Required:[/b][/color]\n"
            "AI wants to run: [b]" + p_tool + "[/b]\n\nArgs:\n" +
            JSON::stringify(p_args, "  ") + "\n\n"
            "[i]Use the Tools tab to Approve or Deny.[/i]";
        chat_panel->append_system_message(msg);
    }
    if (status_bar) {
        status_bar->set_status(AIStatusBar::STATUS_THINKING, "Waiting for approval: " + p_tool);
    }
}

// Fix 8 — provider error shown in chat and status bar
void AIDock::_on_provider_error(const String &p_error) {
    if (chat_panel) {
        chat_panel->append_system_message("[color=red][b]Error:[/b] " + p_error + "[/color]");
        chat_panel->set_streaming_in_progress(false);
    }
    if (status_bar) {
        status_bar->set_status(AIStatusBar::STATUS_ERROR, p_error);
    }
}

void AIDock::_on_new_session() {
    if (chat_panel) chat_panel->clear_conversation();
    if (status_bar)  status_bar->set_status(AIStatusBar::STATUS_IDLE, "New session");
}

void AIDock::_on_save_session() {}

void AIDock::_on_clear_history() {
    if (chat_panel) chat_panel->clear_conversation();
}

void AIDock::_on_tab_changed(int p_index) {}

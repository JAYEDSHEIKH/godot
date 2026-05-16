#include "ai_settings_panel.h"
#include "scene/gui/separator.h"
#include "scene/gui/scroll_container.h"

AISettingsPanel::AISettingsPanel() {}

void AISettingsPanel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_settings"), &AISettingsPanel::load_settings);
    ClassDB::bind_method(D_METHOD("save_settings"), &AISettingsPanel::save_settings);
}

void AISettingsPanel::_ready() {
    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(scroll);

    VBoxContainer *content = memnew(VBoxContainer);
    content->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->add_child(content);

    tabs = memnew(TabContainer);
    tabs->set_v_size_flags(SIZE_EXPAND_FILL);
    content->add_child(tabs);

    // ── Provider Tab ─────────────────────────────
    VBoxContainer *provider_tab = memnew(VBoxContainer);
    provider_tab->set_name("Provider");
    tabs->add_child(provider_tab);

    Label *prov_lbl = memnew(Label); prov_lbl->set_text("AI Provider:"); provider_tab->add_child(prov_lbl);
    provider_dropdown = memnew(OptionButton);
    provider_dropdown->add_item("OpenAI / OpenRouter");
    provider_dropdown->add_item("Google Gemini");
    provider_dropdown->add_item("Local (Ollama/LMStudio)");
    provider_tab->add_child(provider_dropdown);

    Label *key_lbl = memnew(Label); key_lbl->set_text("OpenAI API Key:"); provider_tab->add_child(key_lbl);
    api_key_field = memnew(LineEdit);
    api_key_field->set_secret(true);
    api_key_field->set_placeholder_text("sk-...");
    provider_tab->add_child(api_key_field);

    Label *gem_lbl = memnew(Label); gem_lbl->set_text("Gemini API Key:"); provider_tab->add_child(gem_lbl);
    gemini_key_field = memnew(LineEdit);
    gemini_key_field->set_secret(true);
    gemini_key_field->set_placeholder_text("AIza...");
    provider_tab->add_child(gemini_key_field);

    Label *ep_lbl = memnew(Label); ep_lbl->set_text("Local Endpoint:"); provider_tab->add_child(ep_lbl);
    local_endpoint = memnew(LineEdit);
    local_endpoint->set_text("http://localhost:11434/v1");
    provider_tab->add_child(local_endpoint);

    Label *mdl_lbl = memnew(Label); mdl_lbl->set_text("Model:"); provider_tab->add_child(mdl_lbl);
    model_dropdown = memnew(OptionButton);
    provider_tab->add_child(model_dropdown);

    Label *temp_lbl = memnew(Label); temp_lbl->set_text("Temperature:"); provider_tab->add_child(temp_lbl);
    temperature_slider = memnew(HSlider);
    temperature_slider->set_min(0.0);
    temperature_slider->set_max(2.0);
    temperature_slider->set_step(0.1);
    temperature_slider->set_value(0.7);
    provider_tab->add_child(temperature_slider);
    temperature_label = memnew(Label); temperature_label->set_text("0.7"); provider_tab->add_child(temperature_label);

    Label *max_lbl = memnew(Label); max_lbl->set_text("Max Tokens:"); provider_tab->add_child(max_lbl);
    max_tokens_spin = memnew(SpinBox);
    max_tokens_spin->set_min(256);
    max_tokens_spin->set_max(32000);
    max_tokens_spin->set_step(256);
    max_tokens_spin->set_value(4096);
    provider_tab->add_child(max_tokens_spin);

    // ── Context Tab ───────────────────────────────
    VBoxContainer *ctx_tab = memnew(VBoxContainer);
    ctx_tab->set_name("Context");
    tabs->add_child(ctx_tab);

    Label *budget_lbl = memnew(Label); budget_lbl->set_text("Context Token Budget:"); ctx_tab->add_child(budget_lbl);
    token_budget_spin = memnew(SpinBox);
    token_budget_spin->set_min(1000);
    token_budget_spin->set_max(100000);
    token_budget_spin->set_step(1000);
    token_budget_spin->set_value(6000);
    ctx_tab->add_child(token_budget_spin);

    auto_refresh_cb = memnew(CheckBox);
    auto_refresh_cb->set_text("Auto-refresh context");
    auto_refresh_cb->set_pressed(true);
    ctx_tab->add_child(auto_refresh_cb);

    include_scene_cb = memnew(CheckBox);
    include_scene_cb->set_text("Include scene tree");
    include_scene_cb->set_pressed(true);
    ctx_tab->add_child(include_scene_cb);

    include_scripts_cb = memnew(CheckBox);
    include_scripts_cb->set_text("Include open scripts");
    include_scripts_cb->set_pressed(true);
    ctx_tab->add_child(include_scripts_cb);

    refresh_ctx_btn = memnew(Button);
    refresh_ctx_btn->set_text("Refresh Context Now");
    ctx_tab->add_child(refresh_ctx_btn);

    reindex_btn = memnew(Button);
    reindex_btn->set_text("Re-index Codebase (RAG)");
    ctx_tab->add_child(reindex_btn);

    // ── UI Tab ────────────────────────────────────
    VBoxContainer *ui_tab = memnew(VBoxContainer);
    ui_tab->set_name("UI");
    tabs->add_child(ui_tab);

    streaming_cb = memnew(CheckBox);
    streaming_cb->set_text("Stream responses");
    streaming_cb->set_pressed(true);
    ui_tab->add_child(streaming_cb);

    show_thinking_cb = memnew(CheckBox);
    show_thinking_cb->set_text("Show thinking indicator");
    show_thinking_cb->set_pressed(true);
    ui_tab->add_child(show_thinking_cb);

    Label *fs_lbl = memnew(Label); fs_lbl->set_text("Font size:"); ui_tab->add_child(fs_lbl);
    font_size_spin = memnew(SpinBox);
    font_size_spin->set_min(8);
    font_size_spin->set_max(24);
    font_size_spin->set_value(14);
    ui_tab->add_child(font_size_spin);

    // Save/Reset
    HBoxContainer *btn_row = memnew(HBoxContainer);
    content->add_child(btn_row);

    save_btn = memnew(Button); save_btn->set_text("Save Settings"); save_btn->set_h_size_flags(SIZE_EXPAND_FILL); btn_row->add_child(save_btn);
    reset_btn = memnew(Button); reset_btn->set_text("Reset to Defaults"); btn_row->add_child(reset_btn);

    // Connect signals
    provider_dropdown->connect("item_selected", callable_mp(this, &AISettingsPanel::_on_provider_changed));
    temperature_slider->connect("value_changed", callable_mp(this, &AISettingsPanel::_on_temperature_changed));
    save_btn->connect("pressed", callable_mp(this, &AISettingsPanel::_on_save_pressed));
    reset_btn->connect("pressed", callable_mp(this, &AISettingsPanel::_on_reset_pressed));
    if (reindex_btn) reindex_btn->connect("pressed", callable_mp(this, &AISettingsPanel::_on_reindex_pressed));

    _populate_model_dropdown("OpenAI");
    load_settings();
}

void AISettingsPanel::_populate_model_dropdown(const String &provider) {
    if (!model_dropdown) return;
    model_dropdown->clear();
    if (provider == "OpenAI" || provider.contains("OpenAI")) {
        model_dropdown->add_item("gpt-4o");
        model_dropdown->add_item("gpt-4o-mini");
        model_dropdown->add_item("gpt-4-turbo");
        model_dropdown->add_item("o1-preview");
        model_dropdown->add_item("o1-mini");
    } else if (provider.contains("Gemini")) {
        model_dropdown->add_item("gemini-1.5-pro");
        model_dropdown->add_item("gemini-1.5-flash");
        model_dropdown->add_item("gemini-2.0-flash");
    } else {
        model_dropdown->add_item("llama3.2");
        model_dropdown->add_item("mistral");
        model_dropdown->add_item("codestral");
        model_dropdown->add_item("deepseek-coder-v2");
    }
}

void AISettingsPanel::_on_provider_changed(int idx) {
    String names[] = {"OpenAI", "Gemini", "Local"};
    if (idx < 3) _populate_model_dropdown(names[idx]);
}

void AISettingsPanel::_on_temperature_changed(float val) {
    if (temperature_label) temperature_label->set_text(String::num(val, 1));
}

void AISettingsPanel::_on_save_pressed() { save_settings(); }
void AISettingsPanel::_on_reset_pressed() {}
void AISettingsPanel::_on_reindex_pressed() {}

void AISettingsPanel::load_settings() {}
void AISettingsPanel::save_settings() {}

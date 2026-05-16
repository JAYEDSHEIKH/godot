#pragma once
#include "scene/gui/box_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/label.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tab_container.h"

class AISettingsPanel : public VBoxContainer {
    GDCLASS(AISettingsPanel, VBoxContainer);

public:
    AISettingsPanel();
    void load_settings();
    void save_settings();

protected:
    void _ready() override;
    static void _bind_methods();

private:
    TabContainer *tabs               = nullptr;

    // Provider tab
    OptionButton *provider_dropdown  = nullptr;
    LineEdit     *api_key_field      = nullptr;
    LineEdit     *gemini_key_field   = nullptr;
    LineEdit     *local_endpoint     = nullptr;
    OptionButton *model_dropdown     = nullptr;
    HSlider      *temperature_slider = nullptr;
    Label        *temperature_label  = nullptr;
    SpinBox      *max_tokens_spin    = nullptr;

    // Context tab
    SpinBox      *token_budget_spin  = nullptr;
    CheckBox     *auto_refresh_cb    = nullptr;
    CheckBox     *include_scene_cb   = nullptr;
    CheckBox     *include_scripts_cb = nullptr;
    Button       *refresh_ctx_btn    = nullptr;
    Button       *reindex_btn        = nullptr;

    // UI tab
    CheckBox     *streaming_cb       = nullptr;
    CheckBox     *show_thinking_cb   = nullptr;
    SpinBox      *font_size_spin     = nullptr;

    Button       *save_btn           = nullptr;
    Button       *reset_btn          = nullptr;

    void _on_provider_changed(int idx);
    void _on_temperature_changed(float val);
    void _on_save_pressed();
    void _on_reset_pressed();
    void _on_reindex_pressed();
    void _populate_model_dropdown(const String &provider);
};

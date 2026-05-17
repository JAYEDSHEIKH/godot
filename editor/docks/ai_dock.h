#pragma once
#include "editor/docks/editor_dock.h"
#include "scene/gui/box_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/label.h"
#include "scene/main/http_request.h"

class AIChatPanel;
class AISettingsPanel;
class AIMemoryPanel;
class AIToolsPanel;
class AIStatusBar;

class AIDock : public EditorDock {
    GDCLASS(AIDock, EditorDock);

public:
    AIDock();
    ~AIDock() override;

    void initialize();

protected:
    void _ready() override;
    void _notification(int p_what) override;
    static void _bind_methods();

private:
    VBoxContainer    *vbox             = nullptr;
    TabContainer     *main_tabs        = nullptr;
    AIChatPanel      *chat_panel       = nullptr;
    AISettingsPanel  *settings_panel   = nullptr;
    AIMemoryPanel    *memory_panel     = nullptr;
    AIToolsPanel     *tools_panel      = nullptr;
    AIStatusBar      *status_bar       = nullptr;

    HBoxContainer    *toolbar_hbox     = nullptr;
    Button           *new_session_btn  = nullptr;
    Button           *save_session_btn = nullptr;
    OptionButton     *sessions_dropdown = nullptr;
    Button           *clear_history_btn = nullptr;
    Button           *settings_button  = nullptr;
    Label            *provider_label   = nullptr;

    // Owned HTTPRequest node used by the active provider (Fix 8 / Fix 5)
    HTTPRequest      *provider_http    = nullptr;

    void _setup_ui();
    void _setup_providers();
    void _connect_signals();
    void _apply_theme();

    void _on_new_session();
    void _on_save_session();
    void _on_clear_history();
    void _on_tab_changed(int p_index);

    // Fix 8 — engine signal handlers
    void _on_response_chunk(const String &p_chunk);
    void _on_tool_output(const String &p_call_id, const String &p_result);
    void _on_tool_confirmation_required(const String &p_tool, const Dictionary &p_args);
    void _on_provider_error(const String &p_error);
};

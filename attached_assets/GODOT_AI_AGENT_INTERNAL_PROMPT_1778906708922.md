# Godot AI Agent — Premium Internal C++ Implementation Prompt
> **Build a world-class, production-grade AI assistant that feels native to Godot.**
> Not a plugin. Not an addon. **A first-class editor feature** with enterprise-grade UX/IX.

---

## EXECUTIVE VISION

The Godot AI Agent is the **intelligent copilot that never leaves the editor**. It understands your project deeply, thinks about architecture, catches bugs before you do, and executes complex multi-step operations autonomously.

### Key Experience Principles
- ✨ **Zero friction** — AI is always available, always context-aware
- 🎯 **Transparency** — User sees exactly what the AI is thinking and doing
- 🛡️ **Safety** — Diff previews, confirmations, full audit trail
- ⚡ **Responsiveness** — Streaming output, progressive context loading
- 🧠 **Intelligence** — Deep project awareness, semantic understanding, memory
- 🎨 **Beauty** — Premium UI that makes developers want to use it every day

---

## REAL GODOT 4 SOURCE STRUCTURE

```
godotengine/godot/
├── core/
│   ├── object/         ← Object, ClassDB, signals
│   ├── io/             ← FileAccess, HTTPClient
│   ├── math/           ← Vector2, Vector3, etc.
│   ├── variant/        ← Variant type system
│   └── os/             ← threading, time
├── editor/
│   ├── docks/          ← editor dock panels (AI DOCK LIVES HERE)
│   │   ├── editor_dock.h/.cpp
│   │   ├── filesystem_dock.h/.cpp  ← reference
│   │   └── ai_dock.h/.cpp          ← NEW: THE AI ASSISTANT
│   ├── gui/            ← editor UI components
│   │   ├── editor_themes.h/.cpp    ← styling system
│   │   └── editor_help.h/.cpp      ← reference UI pattern
│   ├── editor_node.h/.cpp          ← main editor (register dock here)
│   ├── editor_interface.h/.cpp     ← editor scripting API
│   └── SCsub
├── modules/
│   ├── ai_engine/      ← NEW: Core AI module (logic + signals)
│   └── SCsub
└── servers/
```

---

## TARGET FOLDER STRUCTURE

### Core AI Module (Logic Layer):
```
modules/ai_engine/
├── config.py
├── SCsub
├── register_types.h/.cpp
│
├── core/
│   ├── ai_engine.h/.cpp            ← singleton orchestrator
│   └── ai_types.h                  ← shared types & constants
│
├── provider/
│   ├── ai_provider.h/.cpp          ← abstract base
│   ├── openai_provider.h/.cpp      ← OpenAI + OpenRouter + local
│   ├── gemini_provider.h/.cpp      ← Google Gemini
│   ├── provider_factory.h/.cpp     ← create provider instances
│   └── stream_parser.h/.cpp        ← handle streaming responses
│
├── executor/
│   ├── tool_executor.h/.cpp        ← orchestrates tool execution
│   ├── tool_registry.h/.cpp        ← tool definitions + validation
│   └── tools/
│       ├── file_tools.h/.cpp
│       ├── node_tools.h/.cpp
│       ├── script_tools.h/.cpp
│       ├── scene_tools.h/.cpp
│       ├── project_tools.h/.cpp
│       ├── git_tools.h/.cpp
│       └── audit_tools.h/.cpp
│
├── context/
│   ├── context_manager.h/.cpp      ← build AI context
│   ├── scene_analyzer.h/.cpp       ← scene tree analysis
│   ├── code_analyzer.h/.cpp        ← code structure analysis
│   └── project_indexer.h/.cpp      ← codebase scanning
│
├── vector_db/
│   ├── vector_db.h/.cpp            ← semantic search engine
│   ├── embeddings.h/.cpp           ← embedding generation
│   ├── similarity.h/.cpp           ← cosine similarity, reranking
│   └── chunker.h/.cpp              ← intelligent code chunking
│
├── memory/
│   ├── memory_manager.h/.cpp       ← long-term project knowledge
│   ├── session_manager.h/.cpp      ← conversation history
│   └── memory_store.h/.cpp         ← file-based persistence
│
└── utils/
    ├── logger.h/.cpp               ← comprehensive logging
    ├── metrics.h/.cpp              ← performance tracking
    ├── cache.h/.cpp                ← request caching
    └── rate_limiter.h/.cpp         ← API rate limiting
```

### Premium UI Layer:
```
editor/docks/
├── ai_dock.h/.cpp                  ← main container (EditorDock subclass)
└── ai/                             ← AI-specific UI components
    ├── ai_chat_panel.h/.cpp        ← chat interface with streaming
    ├── ai_code_editor.h/.cpp       ← syntax-highlighted diff viewer
    ├── ai_settings_panel.h/.cpp    ← configuration UI
    ├── ai_memory_panel.h/.cpp      ← memory/context browser
    ├── ai_tools_panel.h/.cpp       ← tool history & execution log
    ├── ai_status_bar.h/.cpp        ← real-time status indicator
    ├── ai_suggestion_widget.h/.cpp ← inline code suggestions
    └── themes/
        ├── ai_theme.h/.cpp         ← custom theme for AI UI
        └── ai_icons.h/.cpp         ← icon definitions
```

---

## PART 1: CORE DATA STRUCTURES & TYPES

### File: `modules/ai_engine/core/ai_types.h`

```cpp
#pragma once
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"
#include "core/variant/array.h"
#include "core/templates/vector.h"

// ============================================================================
// ENUMS & CONSTANTS
// ============================================================================

enum AIProviderType {
    AI_PROVIDER_OPENAI = 0,
    AI_PROVIDER_GEMINI = 1,
    AI_PROVIDER_LOCAL = 2,
};

enum AIModelFamily {
    MODEL_FAMILY_GPT = 0,
    MODEL_FAMILY_CLAUDE = 1,
    MODEL_FAMILY_GEMINI = 2,
    MODEL_FAMILY_OPEN_SOURCE = 3,
};

enum ToolExecutionStatus {
    TOOL_PENDING = 0,
    TOOL_RUNNING = 1,
    TOOL_SUCCESS = 2,
    TOOL_FAILED = 3,
    TOOL_CANCELLED = 4,
    TOOL_REQUIRES_CONFIRMATION = 5,
};

enum ContextSectionPriority {
    PRIORITY_CRITICAL = 0,      // Current scene, selected nodes
    PRIORITY_HIGH = 1,          // Open scripts, recent changes
    PRIORITY_MEDIUM = 2,        // Project structure, available tools
    PRIORITY_LOW = 3,           // General documentation, reference
};

// ============================================================================
// MESSAGE & HISTORY
// ============================================================================

struct AIMessage {
    String id;                                  // UUID for tracking
    String role;                                // "user", "assistant", "tool", "system"
    String content;                             // text content
    int64_t timestamp_ms = 0;
    int tokens_used = 0;
    int input_tokens = 0;
    int output_tokens = 0;
    Dictionary metadata;                        // extra info (tool_calls, citations, etc.)

    bool is_streaming = false;
    float confidence = 1.0f;                    // AI's confidence in response (0-1)
    Array citations;                            // [{ file, line_start, line_end }, ...]
};

// ============================================================================
// TOOL SYSTEM
// ============================================================================

struct ToolParameter {
    String name;
    String type;                                // "string", "integer", "float", "boolean", "array", "object"
    String description;
    bool required = true;
    Variant default_value;
    Array enum_values;                          // if restricted to specific values
    Dictionary validation;                      // {"pattern": "regex", "min": 0, "max": 100}
};

struct ToolDefinition {
    String name;
    String description;
    String category;                            // "file", "node", "script", "project", "git", "audit"
    bool requires_confirmation = false;         // safety flag
    bool is_dangerous = false;                  // destructive operations
    Vector<ToolParameter> parameters;
    String return_type;                         // what the tool returns
    int priority = 0;                           // execution order hint
};

struct ToolCall {
    String id;                                  // UUID
    String name;
    Dictionary arguments;
    int64_t requested_at_ms = 0;
    int64_t started_at_ms = 0;
    int64_t completed_at_ms = 0;
    ToolExecutionStatus status = TOOL_PENDING;
    String result;                              // tool output
    String error_message;
    bool requires_user_confirmation = false;
};

// ============================================================================
// CONTEXT SYSTEM
// ============================================================================

struct ContextBlock {
    String section_name;                        // "Scene Tree", "Open Scripts", etc.
    String content;
    ContextSectionPriority priority;
    int tokens = 0;                             // estimated token count
    int64_t generated_at_ms = 0;
    bool is_stale = false;
};

// ============================================================================
// EMBEDDING & VECTOR DB
// ============================================================================

struct CodeChunk {
    String file_path;
    String chunk_id;                            // function_name, class_name, etc.
    int line_start = 0;
    int line_end = 0;
    String source_code;
    Vector<float> embedding;                    // 1536 or 3072 dims
    String embedding_model;
    int64_t indexed_at_ms = 0;
    String content_hash;                        // MD5 for change detection
};

struct SearchResult {
    String file_path;
    String chunk_id;
    float relevance_score = 0.0f;               // 0-1, after reranking
    String preview;                             // first 200 chars of code
    int line_number = 0;
    Array keyword_matches;                      // which keywords matched
};

// ============================================================================
// SESSION & MEMORY
// ============================================================================

struct SessionMetadata {
    String session_id;
    int64_t created_at_ms = 0;
    int64_t last_accessed_at_ms = 0;
    String project_name;
    String project_path;
    int message_count = 0;
    int total_tokens_used = 0;
    Dictionary tags;                            // user-defined session tags
};

struct MemoryFact {
    String id;
    String category;                            // "architecture", "convention", "bug_fix", "pattern", "warning"
    String content;
    Array related_files;
    int confidence = 100;                       // 0-100, how sure we are about this
    int64_t created_at_ms = 0;
    int access_count = 0;                       // how often this fact is used
};

// ============================================================================
// METRICS & LOGGING
// ============================================================================

struct AIMetrics {
    int total_messages_sent = 0;
    int total_messages_received = 0;
    int total_tokens_used = 0;
    int total_tool_calls = 0;
    int successful_tool_calls = 0;
    int failed_tool_calls = 0;
    float average_response_time_ms = 0.0f;
    float average_token_time_ms = 0.0f;
    Dictionary provider_usage;                  // { "openai": 50, "gemini": 30, ... }
};
```

---

## PART 2: PREMIUM CHAT UI

### File: `editor/docks/ai/ai_chat_panel.h`

```cpp
#pragma once
#include "scene/gui/control.h"
#include "scene/gui/box_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/button.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/panel_container.h"
#include "core/templates/vector.h"

class AIProvider;
struct AIMessage;

// ============================================================================
// ADVANCED CHAT PANEL WITH STREAMING, CITATIONS, & RICH FORMATTING
// ============================================================================

class AIChatPanel : public Control {
    GDCLASS(AIChatPanel, Control);

public:
    AIChatPanel();
    ~AIChatPanel();

    // --- Setup ---
    void set_ai_provider(Ref<class AIProvider> p_provider);
    void set_context_manager(Ref<class ContextManager> p_context_mgr);

    // --- Core chat operations ---
    void send_message(const String &p_message);
    void clear_chat();
    void cancel_current_request();

    // --- Display options ---
    void set_show_timestamps(bool p_show);
    void set_show_token_counts(bool p_show);
    void set_stream_updates(bool p_enable);
    void set_line_wrap(bool p_enable);

    // --- Load/save session ---
    void load_session(const String &p_session_id);
    void save_session(const String &p_session_id);

protected:
    void _ready() override;
    void _process(double p_delta) override;
    void _gui_input(const Ref<InputEvent> &p_event) override;
    static void _bind_methods();

private:
    // --- Providers ---
    Ref<AIProvider>          ai_provider;
    Ref<class ContextManager> context_manager;

    // --- UI components ---
    VBoxContainer        *main_vbox        = nullptr;
    
    // Chat history display
    ScrollContainer      *scroll_container = nullptr;
    VBoxContainer        *messages_vbox    = nullptr;   // dynamic message bubbles
    
    // Input area
    HBoxContainer        *input_hbox       = nullptr;
    TextEdit             *input_field      = nullptr;   // multi-line with Ctrl+Enter to send
    Button               *send_button      = nullptr;
    Button               *stop_button      = nullptr;
    Button               *attachment_btn   = nullptr;   // drag-drop files/nodes
    OptionButton         *tone_selector    = nullptr;   // "Analytical", "Creative", "Cautious"
    
    // Status bar
    HBoxContainer        *status_hbar      = nullptr;
    Label                *status_label     = nullptr;   // "Thinking...", "Ready", etc.
    Label                *token_count_label = nullptr;  // "≈250 tokens"
    ProgressBar          *thinking_progress = nullptr;
    
    // --- Internal state ---
    Vector<AIMessage>    message_history;
    String               current_session_id;
    bool                 is_waiting_for_response = false;
    String               current_streaming_content;
    int                  current_message_token_count = 0;
    String               selected_tone = "Analytical";
    
    // --- Message bubble cache ---
    struct MessageBubble {
        Control         *container = nullptr;
        RichTextLabel   *text_label = nullptr;
        Label           *metadata_label = nullptr;  // timestamp, tokens
        Control         *citation_widget = nullptr; // code references
        Vector<Control*> action_buttons;             // copy, edit, regenerate, etc.
    };
    Vector<MessageBubble> message_bubbles;

    // --- Signal handlers from provider ---
    void _on_response_started();
    void _on_response_chunk_received(const String &p_chunk);
    void _on_response_complete(const String &p_full_response);
    void _on_response_error(const String &p_error);
    void _on_status_changed(const String &p_status);

    // --- UI actions ---
    void _on_send_button_pressed();
    void _on_stop_button_pressed();
    void _on_input_text_submitted(const String &p_text);
    void _on_attachment_dropped(const Array &p_files);
    void _on_tone_changed(int p_index);
    void _on_message_action(const String &p_action, int p_message_index);

    // --- Message rendering ---
    void _add_message_bubble(const AIMessage &p_message, bool p_is_streaming = false);
    void _update_last_bubble_with_chunk(const String &p_chunk);
    void _finalize_last_bubble();
    void _render_message_with_citations(
        Control         *p_bubble,
        const AIMessage &p_message
    );
    void _apply_syntax_highlighting(
        RichTextLabel   *p_label,
        const String    &p_text,
        const String    &p_language = ""
    );

    // --- Input processing ---
    void _process_input_commands();  // /plan, /debug, /brainstorm, etc.
    String _build_enhanced_prompt(const String &p_user_input);

    // --- Styling & theme ---
    void _setup_theme();
    void _apply_bubble_style(Control *p_bubble, const String &p_role);
    void _setup_markdown_styles();

    // --- Accessibility ---
    void _setup_accessibility();
};
```

---

## PART 3: INTERACTIVE DIFF VIEWER

### File: `editor/docks/ai/ai_code_editor.h`

```cpp
#pragma once
#include "scene/gui/control.h"
#include "scene/gui/split_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"
#include "core/templates/vector.h"

// ============================================================================
// ADVANCED DIFF VIEWER WITH SYNTAX HIGHLIGHTING & INTERACTIVITY
// ============================================================================

class AIDiffViewer : public Control {
    GDCLASS(AIDiffViewer, Control);

public:
    enum DiffMode {
        DIFF_MODE_SIDE_BY_SIDE = 0,
        DIFF_MODE_UNIFIED = 1,
        DIFF_MODE_INLINE = 2,
    };

    AIDiffViewer();

    // --- Set content to compare ---
    void set_diff(
        const String &p_file_path,
        const String &p_old_content,
        const String &p_new_content,
        const String &p_language = "gdscript"
    );

    // --- Display options ---
    void set_diff_mode(DiffMode p_mode);
    void set_show_line_numbers(bool p_show);
    void set_show_change_statistics(bool p_show);
    void set_context_lines(int p_count);  // lines around changes to show

    // --- User actions ---
    void accept_changes();
    void reject_changes();
    void accept_selective_changes(const Vector<int> &p_hunk_indices);

    // --- Signals ---
    // diff_accepted()
    // diff_rejected()
    // diff_partially_accepted(Vector<int> hunks)

protected:
    void _ready() override;
    void _gui_input(const Ref<InputEvent> &p_event) override;
    static void _bind_methods();

private:
    String file_path;
    String old_content;
    String new_content;
    String language_hint;
    DiffMode display_mode = DIFF_MODE_SIDE_BY_SIDE;
    int context_line_count = 3;

    // --- UI layout ---
    VBoxContainer        *main_vbox        = nullptr;
    
    // Header with file info & options
    HBoxContainer        *header_hbox      = nullptr;
    Label                *file_label       = nullptr;      // "Modifying: res://path/to/file.gd"
    Label                *stats_label      = nullptr;      // "+45 lines, -12 lines"
    Button               *mode_button      = nullptr;      // toggle view mode
    CheckBox             *line_numbers_cb  = nullptr;
    Button               *accept_button    = nullptr;
    Button               *reject_button    = nullptr;

    // Diff display area
    HSplitContainer      *diff_split       = nullptr;
    TextEdit             *old_editor       = nullptr;      // left side (original)
    TextEdit             *new_editor       = nullptr;      // right side (modified)
    
    // Unified diff view
    TextEdit             *unified_editor   = nullptr;
    
    // Change indicators
    VBoxContainer        *hunks_list       = nullptr;      // list of change blocks
    
    // --- Diff data ---
    struct Hunk {
        int old_start = 0;
        int old_end = 0;
        int new_start = 0;
        int new_end = 0;
        String content;
        Vector<int> added_lines;
        Vector<int> removed_lines;
    };
    Vector<Hunk> hunks;
    Vector<bool> hunk_selected;  // track user's accept/reject per hunk

    // --- Rendering ---
    void _compute_diff();
    void _render_side_by_side();
    void _render_unified();
    void _render_inline();
    void _apply_diff_colors(TextEdit *p_editor, const Vector<int> &p_added, const Vector<int> &p_removed);
    void _render_hunk_selector();

    // --- Interactivity ---
    void _on_accept_all_pressed();
    void _on_reject_all_pressed();
    void _on_hunk_toggled(int p_hunk_index, bool p_selected);
    void _on_accept_button_pressed();
};
```

---

## PART 4: INTELLIGENT SETTINGS WITH PROFILES

### File: `editor/docks/ai/ai_settings_panel.h`

```cpp
#pragma once
#include "scene/gui/control.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/check_box.h"
#include "scene/gui/option_button.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "core/templates/hash_map.h"

// ============================================================================
// PROFESSIONAL SETTINGS PANEL WITH PROFILES & PRESETS
// ============================================================================

class AISettingsPanel : public Control {
    GDCLASS(AISettingsPanel, Control);

public:
    AISettingsPanel();

    // --- Save/load entire configuration ---
    Dictionary get_current_config() const;
    void apply_config(const Dictionary &p_config);
    void save_current_profile(const String &p_profile_name);
    void load_profile(const String &p_profile_name);
    Array get_all_profiles() const;
    void delete_profile(const String &p_profile_name);

    // --- Get individual settings ---
    String get_api_provider() const;
    String get_api_key() const;
    String get_base_url() const;
    String get_model_name() const;
    float get_temperature() const;
    int get_max_tokens() const;
    String get_system_prompt() const;

    // --- Signals ---
    // settings_changed(Dictionary new_config)
    // profile_changed(String profile_name)

protected:
    void _ready() override;
    void _gui_input(const Ref<InputEvent> &p_event) override;
    static void _bind_methods();

private:
    TabContainer *tab_container = nullptr;

    // --- Provider Tab ---
    OptionButton      *provider_select  = nullptr;    // OpenAI, Gemini, Local
    OptionButton      *model_select     = nullptr;    // GPT-4, Gemini Pro, etc.
    LineEdit          *api_key_input    = nullptr;    // masked input
    Button            *api_key_toggle   = nullptr;    // show/hide
    Label             *api_key_status   = nullptr;    // "✓ Valid", "✗ Invalid"
    LineEdit          *base_url_input   = nullptr;    // for local/custom endpoints
    Button            *test_connection  = nullptr;    // verify connectivity
    Label             *connection_status = nullptr;

    // --- Model Parameters Tab ---
    SpinBox           *temperature_slider = nullptr;  // 0.0 - 2.0
    Label             *temperature_hint   = nullptr;  // "More creative" / "More focused"
    SpinBox           *top_p_slider       = nullptr;  // nucleus sampling
    SpinBox           *top_k_spinner      = nullptr;  // top-k sampling
    SpinBox           *max_tokens_spinner = nullptr;  // response length limit
    SpinBox           *frequency_penalty_slider = nullptr;
    SpinBox           *presence_penalty_slider = nullptr;
    OptionButton      *response_format    = nullptr;  // text, json, structured
    CheckBox          *stream_responses   = nullptr;  // enable streaming

    // --- Behavior Tab ---
    CheckBox          *auto_include_context  = nullptr;  // RAG context
    CheckBox          *auto_cite_sources     = nullptr;  // show references
    CheckBox          *require_confirmation  = nullptr;  // ask before destructive ops
    CheckBox          *show_diff_previews    = nullptr;  // visual diffs before apply
    CheckBox          *enable_memory         = nullptr;  // long-term project memory
    CheckBox          *auto_save_sessions    = nullptr;  // persist conversations
    OptionButton      *default_tone          = nullptr;  // Analytical, Creative, Cautious, Debug
    SpinBox           *context_budget_tokens = nullptr;  // max context size

    // --- RAG / Knowledge Tab ---
    CheckBox          *enable_rag            = nullptr;  // vector DB search
    SpinBox           *rag_top_k             = nullptr;  // return top K results
    SpinBox           *rag_threshold         = nullptr;  // minimum similarity threshold
    Button            *reindex_project       = nullptr;  // trigger full reindex
    Label             *index_status          = nullptr;  // "Ready", "Indexing...", "36,542 chunks"
    CheckBox          *auto_update_index     = nullptr;  // watch for file changes
    SpinBox           *index_retention_days  = nullptr;  // cleanup old chunks

    // --- Privacy & Security Tab ---
    CheckBox          *local_processing_only = nullptr;  // don't send code to cloud
    CheckBox          *clear_history_on_exit = nullptr;  // delete sessions when closing
    CheckBox          *encrypt_sessions      = nullptr;  // AES-256 for stored conversations
    Button            *clear_all_data        = nullptr;  // nuclear option
    CheckBox          *send_usage_analytics  = nullptr;  // opt-in telemetry
    Label             *privacy_notice        = nullptr;  // GDPR/privacy statement

    // --- Profiles Management ---
    HBoxContainer     *profiles_hbox         = nullptr;
    OptionButton      *profile_selector      = nullptr;
    LineEdit          *new_profile_name      = nullptr;
    Button            *save_profile_button   = nullptr;
    Button            *delete_profile_button = nullptr;
    Button            *reset_to_defaults     = nullptr;

    // --- Advanced Tab ---
    TextEdit          *system_prompt_editor  = nullptr;  // edit AI behavior
    TextEdit          *advanced_config_json  = nullptr;  // raw JSON config for power users
    CheckBox          *enable_debug_logging  = nullptr;
    Button            *open_logs_folder      = nullptr;
    SpinBox           *request_timeout_secs  = nullptr;
    SpinBox           *max_retries           = nullptr;

    // --- Internal state ---
    HashMap<String, Dictionary> profiles;  // saved profiles
    Dictionary current_config;
    bool unsaved_changes = false;

    // --- Signal handlers ---
    void _on_provider_changed(int p_index);
    void _on_model_changed(int p_index);
    void _on_test_connection_pressed();
    void _on_settings_changed();
    void _on_save_profile_pressed();
    void _on_load_profile(int p_index);
    void _on_delete_profile_pressed();
    void _on_reset_to_defaults_pressed();
    void _on_reindex_project_pressed();
    void _on_clear_all_data_pressed();

    // --- Helpers ---
    void _load_profiles_from_disk();
    void _save_profiles_to_disk();
    void _validate_api_key();
    void _update_model_list(int p_provider);
    void _update_hint_labels();
};
```

---

## PART 5: MEMORY BROWSER & KNOWLEDGE MANAGEMENT

### File: `editor/docks/ai/ai_memory_panel.h`

```cpp
#pragma once
#include "scene/gui/control.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/item_list.h"
#include "core/templates/vector.h"

// ============================================================================
// MEMORY BROWSER — VIEW/MANAGE LONG-TERM PROJECT KNOWLEDGE
// ============================================================================

class AIMemoryPanel : public Control {
    GDCLASS(AIMemoryPanel, Control);

public:
    AIMemoryPanel();

    void set_memory_manager(Ref<class MemoryManager> p_mgr);
    void set_vector_db(Ref<class VectorDB> p_db);

    void refresh_memory_view();
    void search_memory(const String &p_query);

protected:
    void _ready() override;
    static void _bind_methods();

private:
    Ref<class MemoryManager> memory_manager;
    Ref<class VectorDB> vector_db;

    TabContainer *tab_container = nullptr;

    // --- Memory Facts Tab ---
    VBoxContainer     *facts_vbox        = nullptr;
    HBoxContainer     *search_hbox       = nullptr;
    LineEdit          *search_input      = nullptr;
    OptionButton      *category_filter   = nullptr;  // architecture, convention, bug_fix, etc.
    Tree              *facts_tree        = nullptr;  // hierarchical view of facts
    TextEdit          *fact_detail_view  = nullptr;  // show selected fact's full content
    HBoxContainer     *fact_actions_hbox = nullptr;
    Button            *edit_fact_button  = nullptr;
    Button            *delete_fact_button = nullptr;
    Button            *new_fact_button   = nullptr;

    // --- Context Snapshot Tab ---
    TabContainer      *snapshot_tabs     = nullptr;  // Scene, Scripts, Project Info
    RichTextLabel     *scene_snapshot    = nullptr;  // current scene tree dump
    RichTextLabel     *scripts_snapshot  = nullptr;  // open files
    RichTextLabel     *project_snapshot  = nullptr;  // project settings

    // --- RAG Index Tab ---
    VBoxContainer     *rag_vbox          = nullptr;
    ItemList          *indexed_files     = nullptr;  // list of indexed .gd files
    Label             *index_stats_label = nullptr;  // "1,245 chunks, 2.1 MB, last updated 2 hours ago"
    Button            *refresh_index     = nullptr;
    Button            *clear_index       = nullptr;
    ProgressBar       *indexing_progress = nullptr;
    CheckBox          *auto_index_checkbox = nullptr;

    // --- Search Results Tab ---
    TabContainer      *search_results_tabs = nullptr;
    ItemList          *relevant_snippets   = nullptr;  // search results
    TextEdit          *snippet_preview     = nullptr;  // show matching code
    Label             *relevance_label     = nullptr;  // "89% match"

    // --- Session History Tab ---
    Tree              *sessions_tree       = nullptr;  // sessions grouped by date
    TextEdit          *session_preview     = nullptr;  // show conversation
    Button            *load_session_button = nullptr;
    Button            *delete_session_button = nullptr;
    Button            *export_session_button = nullptr;

    // --- Signal handlers ---
    void _on_fact_selected(TreeItem *p_item);
    void _on_edit_fact_pressed();
    void _on_delete_fact_pressed();
    void _on_new_fact_pressed();
    void _on_search_text_changed(const String &p_text);
    void _on_category_filter_changed(int p_index);
    void _on_index_refresh_pressed();
    void _on_session_selected(TreeItem *p_item);
    void _on_load_session_pressed();

    // --- Helpers ---
    void _populate_facts_tree();
    void _populate_sessions_tree();
    void _update_index_stats();
};
```

---

## PART 6: EXECUTION LOG & TOOL INSPECTOR

### File: `editor/docks/ai/ai_tools_panel.h`

```cpp
#pragma once
#include "scene/gui/control.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/tree.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"

// ============================================================================
// TOOL EXECUTION LOG & INSPECTOR
// ============================================================================

class AIToolsPanel : public Control {
    GDCLASS(AIToolsPanel, Control);

public:
    AIToolsPanel();

    void set_tool_executor(Ref<class ToolExecutor> p_executor);

    void log_tool_execution(const struct ToolCall &p_call);
    void refresh_execution_log();

protected:
    void _ready() override;
    static void _bind_methods();

private:
    Ref<class ToolExecutor> tool_executor;

    TabContainer  *tab_container = nullptr;

    // --- Execution Log Tab ---
    VBoxContainer     *log_vbox           = nullptr;
    ItemList          *execution_list     = nullptr;  // chronological list of tool calls
    TextEdit          *execution_detail   = nullptr;  // detailed info for selected tool
    Button            *clear_log_button   = nullptr;
    CheckBox          *auto_scroll_checkbox = nullptr;

    // --- Available Tools Tab ---
    Tree              *tools_tree         = nullptr;  // categorized list of available tools
    TextEdit          *tool_documentation = nullptr;  // show tool definition & parameters
    Button            *copy_tool_def_button = nullptr;

    // --- Status Indicators ---
    HBoxContainer     *status_hbox        = nullptr;
    Label             *tool_status_label  = nullptr;  // "Tool: create_script (Running)"
    ProgressBar       *tool_progress      = nullptr;
    Label             *success_count_label = nullptr;  // "✓ 42 successful"
    Label             *error_count_label  = nullptr;   // "✗ 3 failed"

    // --- Signal handlers ---
    void _on_tool_selected(int p_index);
    void _on_execution_selected(int p_index);
    void _on_clear_log_pressed();

    // --- Rendering ---
    void _populate_tools_tree();
    void _show_tool_documentation(const String &p_tool_name);
    void _format_execution_detail(const struct ToolCall &p_call);
};
```

---

## PART 7: STATUS BAR & REAL-TIME INDICATORS

### File: `editor/docks/ai/ai_status_bar.h`

```cpp
#pragma once
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/option_button.h"

// ============================================================================
// REAL-TIME STATUS INDICATOR AT BOTTOM OF DOCK
// ============================================================================

class AIStatusBar : public Control {
    GDCLASS(AIStatusBar, Control);

public:
    enum StatusState {
        STATUS_IDLE = 0,
        STATUS_THINKING = 1,
        STATUS_EXECUTING_TOOL = 2,
        STATUS_INDEXING = 3,
        STATUS_ERROR = 4,
    };

    AIStatusBar();

    void set_status(StatusState p_state, const String &p_message = "");
    void set_progress(float p_value);  // 0-1
    void set_token_count(int p_count);
    void set_context_usage(float p_percentage);  // 0-1

protected:
    void _ready() override;
    void _process(double p_delta) override;
    static void _bind_methods();

private:
    StatusState current_state = STATUS_IDLE;

    Label       *status_label      = nullptr;  // "Ready", "Thinking...", "Error"
    Label       *message_label     = nullptr;  // detailed message
    ProgressBar *progress_bar      = nullptr;  // animated
    Label       *token_label       = nullptr;  // "≈1,250 tokens"
    Label       *context_usage_label = nullptr;  // "Context: 45%"
    Label       *uptime_label      = nullptr;  // session duration
    
    // Animated indicators
    Control     *indicator_dot     = nullptr;  // blinking dot for "thinking"
    float       indicator_pulse_time = 0.0f;

    void _update_status_display();
    void _animate_indicator();
};
```

---

## PART 8: PREMIUM MAIN DOCK

### File: `editor/docks/ai_dock.h`

```cpp
#pragma once
#include "editor/docks/editor_dock.h"
#include "scene/gui/box_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/button.h"

class AIProvider;
class ToolExecutor;
class ContextManager;
class VectorDB;
class AIChatPanel;
class AISettingsPanel;
class AIMemoryPanel;
class AIToolsPanel;
class AIStatusBar;

// ============================================================================
// MAIN AI DOCK — THE ENTRY POINT FOR ALL AI FEATURES
// ============================================================================

class AIDock : public EditorDock {
    GDCLASS(AIDock, EditorDock);

public:
    AIDock();
    ~AIDock();

    void initialize();  // Called after all systems are ready

protected:
    void _ready() override;
    void _process(double p_delta) override;
    void _notification(int p_what) override;
    static void _bind_methods();

private:
    // --- Core AI systems ---
    Ref<AIProvider>     ai_provider;
    Ref<ToolExecutor>   tool_executor;
    Ref<ContextManager> context_manager;
    Ref<VectorDB>       vector_db;

    // --- UI panels ---
    TabContainer        *main_tabs          = nullptr;
    AIChatPanel         *chat_panel         = nullptr;
    AISettingsPanel     *settings_panel     = nullptr;
    AIMemoryPanel       *memory_panel       = nullptr;
    AIToolsPanel        *tools_panel        = nullptr;
    AIStatusBar         *status_bar         = nullptr;

    // --- Top toolbar ---
    HBoxContainer       *toolbar_hbox       = nullptr;
    Button              *new_session_btn    = nullptr;
    Button              *save_session_btn   = nullptr;
    OptionButton        *sessions_dropdown  = nullptr;  // quickly switch sessions
    Button              *clear_history_btn  = nullptr;
    Button              *help_button        = nullptr;
    Button              *settings_button    = nullptr;

    // --- Status & metrics ---
    Label               *session_info_label = nullptr;  // "Session #42: 1,245 messages"
    Label               *provider_label     = nullptr;  // "OpenAI GPT-4"

    // --- Signal handlers ---
    void _on_new_session();
    void _on_save_session();
    void _on_load_session();
    void _on_clear_history();
    void _on_help_pressed();
    void _on_tab_changed(int p_index);

    // --- Setup & initialization ---
    void _setup_ui();
    void _setup_providers();
    void _setup_executor();
    void _connect_signals();
    void _load_last_session();
    void _apply_theme();

    // --- Auto-save ---
    void _auto_save_session();
};
```

---

## PART 9: CORE AI ENGINE ORCHESTRATOR

### File: `modules/ai_engine/core/ai_engine.h`

```cpp
#pragma once
#include "core/object/ref_counted.h"

class AIProvider;
class ToolExecutor;
class ContextManager;
class VectorDB;
class SessionManager;
class MemoryManager;

// ============================================================================
// AI ENGINE SINGLETON — ORCHESTRATES ALL SUBSYSTEMS
// ============================================================================

class AIEngine : public RefCounted {
    GDCLASS(AIEngine, RefCounted);

public:
    static AIEngine *get_singleton();

    // --- Initialization ---
    void initialize();
    void shutdown();

    // --- Access subsystems ---
    Ref<AIProvider> get_provider() const { return provider; }
    Ref<ToolExecutor> get_executor() const { return executor; }
    Ref<ContextManager> get_context_manager() const { return context_mgr; }
    Ref<VectorDB> get_vector_db() const { return vector_db; }
    Ref<SessionManager> get_session_manager() const { return session_mgr; }
    Ref<MemoryManager> get_memory_manager() const { return memory_mgr; }

    // --- Conversation flow ---
    void send_message(const String &p_message);
    void cancel_current_request();
    void handle_tool_call(const String &p_tool_name, const Dictionary &p_args);

    // --- Project analysis ---
    void analyze_project();
    void reindex_codebase();

    // --- Metrics ---
    Dictionary get_metrics() const;

protected:
    static void _bind_methods();

private:
    AIEngine();
    static AIEngine *singleton;

    Ref<AIProvider>     provider;
    Ref<ToolExecutor>   executor;
    Ref<ContextManager> context_mgr;
    Ref<VectorDB>       vector_db;
    Ref<SessionManager> session_mgr;
    Ref<MemoryManager>  memory_mgr;

    bool is_initialized = false;

    // --- Signal handlers for inter-system communication ---
    void _on_provider_response(const String &p_response);
    void _on_tool_call_requested(const String &p_name, const Dictionary &p_args);
    void _on_tool_output(const String &p_output);
};
```

---

## PART 10: ADVANCED PROMPT ENGINEERING

### File: `modules/ai_engine/context/advanced_prompting.h`

```cpp
#pragma once
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"
#include "core/templates/vector.h"

// ============================================================================
// INTELLIGENT PROMPT CONSTRUCTION WITH DYNAMIC CONTEXT PRIORITIZATION
// ============================================================================

class AdvancedPrompting {
public:
    // Build optimized prompt based on:
    // - User message
    // - Current project context
    // - Selected tone/style
    // - Available tools
    // - Recent history
    static String build_system_prompt(
        const String &p_project_name,
        const String &p_godot_version,
        const Vector<String> &p_available_tools,
        const String &p_tone = "Analytical"
    );

    static String build_user_prompt(
        const String &p_user_message,
        const Dictionary &p_context_blocks,  // { "Scene Tree": "...", "Open Scripts": "..." }
        const Vector<String> &p_recent_facts,  // from memory
        const Vector<String> &p_relevant_code_snippets,  // from RAG
        int p_token_budget = 6000
    );

    // Handle slash commands like /plan, /debug, /brainstorm
    static String handle_slash_command(
        const String &p_command,
        const String &p_args,
        const Dictionary &p_context
    );

    // Few-shot examples for different scenarios
    static String get_code_generation_examples();
    static String get_debugging_examples();
    static String get_architecture_examples();

private:
    static String _format_context_for_llm(
        const Dictionary &p_blocks,
        int p_max_tokens
    );
    static String _rank_and_filter_context(
        const Vector<String> &p_blocks,
        int p_max_tokens
    );
};
```

---

## PART 11: UNIFIED CONFIGURATION SYSTEM

### File: `modules/ai_engine/utils/config_manager.h`

```cpp
#pragma once
#include "core/object/ref_counted.h"
#include "core/variant/dictionary.h"

// ============================================================================
// CONFIGURATION MANAGER — CENTRAL HUB FOR ALL SETTINGS
// ============================================================================

class ConfigManager : public RefCounted {
    GDCLASS(ConfigManager, RefCounted);

public:
    static ConfigManager *get_singleton();

    // --- Loading & saving ---
    void load_config(const String &p_path = "");
    void save_config(const String &p_path = "");
    void reset_to_defaults();

    // --- Getting values (with type checking) ---
    Variant get(const String &p_key, const Variant &p_default = Variant());
    String get_string(const String &p_key, const String &p_default = "");
    int get_int(const String &p_key, int p_default = 0);
    float get_float(const String &p_key, float p_default = 0.0f);
    bool get_bool(const String &p_key, bool p_default = false);
    Dictionary get_dict(const String &p_key, const Dictionary &p_default = Dictionary());

    // --- Setting values ---
    void set(const String &p_key, const Variant &p_value);
    void set_string(const String &p_key, const String &p_value);
    void set_int(const String &p_key, int p_value);
    void set_float(const String &p_key, float p_value);
    void set_bool(const String &p_key, bool p_value);

    // --- Profiles (presets) ---
    void save_profile(const String &p_name);
    void load_profile(const String &p_name);
    Array get_profiles() const;
    void delete_profile(const String &p_name);

    // --- Signals ---
    // config_changed(String key, Variant new_value)
    // profile_loaded(String profile_name)

protected:
    static void _bind_methods();

private:
    ConfigManager();
    static ConfigManager *singleton;

    Dictionary config;
    Dictionary profiles;
    String config_path = "user://.ai_engine/config.json";
    String profiles_path = "user://.ai_engine/profiles.json";

    void _load_json_from_disk(const String &p_path, Dictionary &p_out);
    void _save_json_to_disk(const String &p_path, const Dictionary &p_data);
};
```

---

## PART 12: INTEGRATION INTO GODOT

### File: `editor/editor_node.h` (MODIFICATION)

Add to private members:

```cpp
private:
    AIDock *ai_dock = nullptr;
```

### File: `editor/editor_node.cpp` (MODIFICATION)

In `EditorNode::EditorNode()` constructor, after other docks are created:

```cpp
#include "editor/docks/ai_dock.h"
#include "modules/ai_engine/core/ai_engine.h"

// Inside EditorNode::EditorNode() (search for "scene_tree_dock = memnew")

// Initialize AI Engine
AIEngine::get_singleton()->initialize();

// Create AI Dock
ai_dock = memnew(AIDock);
ai_dock->initialize();

// Register with dock manager
EditorDockManager::get_singleton()->add_dock(ai_dock);
add_to_center(ai_dock);  // or appropriate docking method
```

In `EditorNode::_notification()`:

```cpp
case NOTIFICATION_WM_CLOSE_REQUEST:
    AIEngine::get_singleton()->shutdown();
    break;
```

### File: `editor/SCsub` (MODIFICATION)

Add after other dock files:

```python
env_editor.add_source_files(env.editor_sources, "docks/ai_dock.cpp")
env_editor.add_source_files(env.editor_sources, "docks/ai/*.cpp")
```

### File: `editor/register_editor_types.cpp` (MODIFICATION)

```cpp
#include "editor/docks/ai_dock.h"
#include "docks/ai/ai_chat_panel.h"
#include "docks/ai/ai_settings_panel.h"
#include "docks/ai/ai_memory_panel.h"
#include "docks/ai/ai_tools_panel.h"
#include "docks/ai/ai_status_bar.h"
#include "docks/ai/ai_code_editor.h"

void register_editor_types() {
    // ... existing code ...
    
    GDREGISTER_CLASS(AIDock);
    GDREGISTER_CLASS(AIChatPanel);
    GDREGISTER_CLASS(AISettingsPanel);
    GDREGISTER_CLASS(AIMemoryPanel);
    GDREGISTER_CLASS(AIToolsPanel);
    GDREGISTER_CLASS(AIStatusBar);
    GDREGISTER_CLASS(AIDiffViewer);
}
```

### File: `modules/SCsub` (MODIFICATION)

The module system auto-detects `modules/ai_engine/config.py`, so just ensure it exists.

---

## BUILD INSTRUCTIONS

```bash
# Clone Godot 4 source
git clone https://github.com/godotengine/godot.git
cd godot

# Copy module into place
cp -r path/to/modules/ai_engine modules/

# Copy editor changes
cp -r path/to/editor/docks/ai_*.* editor/docks/
cp path/to/editor/ai_dock.* editor/docks/

# Compile (takes 20-40 minutes depending on system)
scons platform=linuxbsd target=editor dev_build=yes -j$(nproc)

# Run custom Godot with AI built-in
./bin/godot.linuxbsd.editor.dev.x86_64
```

---

## FEATURE CHECKLIST

### Phase 1: Core Foundation
- [ ] AI Engine singleton
- [ ] Provider abstraction (OpenAI, Gemini, Local)
- [ ] HTTP request handling with retry logic
- [ ] Session management & persistence
- [ ] Basic chat UI

### Phase 2: Intelligence
- [ ] Context manager (scene tree, scripts, project)
- [ ] Tool executor (34+ tools)
- [ ] Undo/redo integration
- [ ] Diff viewer

### Phase 3: Knowledge Systems
- [ ] Vector database (RAG)
- [ ] Semantic code search
- [ ] Memory management
- [ ] Project indexing

### Phase 4: Premium UX
- [ ] Streaming chat with animations
- [ ] Syntax highlighting in diff viewer
- [ ] Settings profiles & presets
- [ ] Memory browser
- [ ] Execution log
- [ ] Status indicators
- [ ] Accessibility features

### Phase 5: Polish & Deploy
- [ ] Theme integration (light/dark mode)
- [ ] Icon pack
- [ ] Documentation
- [ ] Performance optimization
- [ ] Testing & QA

---

This is the **production-grade AI copilot** that makes Godot developers 10x more productive. Build it, ship it, and watch adoption soar. 🚀
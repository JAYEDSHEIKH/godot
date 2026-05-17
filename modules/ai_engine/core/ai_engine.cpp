#include "ai_engine.h"
#include "core/os/time.h"
#include "utils/config_manager.h"
#include "provider/openai_provider.h"
#include "provider/gemini_provider.h"
#include "executor/tool_executor.h"
#include "context/context_manager.h"
#include "vector_db/vector_db.h"
#include "memory/memory_manager.h"
#include "memory/session_manager.h"

AIEngine *AIEngine::singleton = nullptr;

AIEngine::AIEngine() {
    singleton = this;
}

AIEngine *AIEngine::get_singleton() {
    return singleton;
}

void AIEngine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize"),                  &AIEngine::initialize);
    ClassDB::bind_method(D_METHOD("shutdown"),                    &AIEngine::shutdown);
    ClassDB::bind_method(D_METHOD("send_message", "message"),     &AIEngine::send_message);
    ClassDB::bind_method(D_METHOD("cancel_current_request"),      &AIEngine::cancel_current_request);
    ClassDB::bind_method(D_METHOD("analyze_project"),             &AIEngine::analyze_project);
    ClassDB::bind_method(D_METHOD("reindex_codebase"),            &AIEngine::reindex_codebase);
    ClassDB::bind_method(D_METHOD("get_metrics"),                 &AIEngine::get_metrics);

    // Fix 1 — declare all engine-level signals
    ADD_SIGNAL(MethodInfo("initialized"));
    ADD_SIGNAL(MethodInfo("response_chunk_received",
        PropertyInfo(Variant::STRING, "chunk")));
    ADD_SIGNAL(MethodInfo("tool_output_received",
        PropertyInfo(Variant::STRING, "call_id"),
        PropertyInfo(Variant::STRING, "result")));
    // Fix 3 — tool confirmation signal
    ADD_SIGNAL(MethodInfo("tool_confirmation_required",
        PropertyInfo(Variant::STRING, "tool_name"),
        PropertyInfo(Variant::DICTIONARY, "args")));
}

// Fix 1 — full initialization: creates and wires all subsystems
void AIEngine::initialize() {
    if (is_initialized) return;

    // 1. Config — load first so provider can read api_key etc.
    ConfigManager::get_singleton()->load_config();

    // 2. Context manager
    context_mgr.instantiate();
    context_mgr->refresh_all();

    // 3. Memory
    memory_mgr.instantiate();
    memory_mgr->load_from_disk();

    // 4. Vector DB
    vector_db.instantiate();
    vector_db->load_from_disk();

    // 5. Session manager
    session_mgr.instantiate();

    // 6. Tool executor
    executor.instantiate();

    // 7. Provider — chosen from config
    String prov_name = ConfigManager::get_singleton()->get_string("provider", "openai");
    if (prov_name == "gemini") {
        Ref<GeminiProvider> gp;
        gp.instantiate();
        gp->set_api_key(ConfigManager::get_singleton()->get_string("gemini_api_key", ""));
        gp->set_model(ConfigManager::get_singleton()->get_string("model", "gemini-1.5-pro"));
        provider = gp;
    } else {
        Ref<OpenAIProvider> op;
        op.instantiate();
        op->set_api_key(ConfigManager::get_singleton()->get_string("api_key", ""));
        op->set_model(ConfigManager::get_singleton()->get_string("model", "gpt-4o"));
        provider = op;
    }

    // 8. Wire provider signals → engine callbacks
    provider->connect("response_complete",       callable_mp(this, &AIEngine::_on_provider_response));
    provider->connect("tool_call_requested",     callable_mp(this, &AIEngine::_on_tool_call_requested));
    provider->connect("response_chunk_received", callable_mp(this, &AIEngine::_on_streaming_chunk));

    // 9. Wire tool output back to context/memory
    executor->connect("tool_completed", callable_mp(this, &AIEngine::_on_tool_output_received));

    is_initialized = true;
    emit_signal("initialized");
    print_line("[AIEngine] Initialized — provider: " + provider->get_provider_name());
}

void AIEngine::shutdown() {
    if (!is_initialized) return;
    is_initialized = false;
    provider.unref();
    executor.unref();
    context_mgr.unref();
    vector_db.unref();
    session_mgr.unref();
    memory_mgr.unref();
    print_line("[AIEngine] Shutdown.");
}

// Fix 2 — send_message actually calls the provider with full context
void AIEngine::send_message(const String &p_message) {
    if (!is_initialized) {
        WARN_PRINT("[AIEngine] Not initialized — call initialize() first.");
        return;
    }
    if (!provider.is_valid()) {
        WARN_PRINT("[AIEngine] No provider set.");
        return;
    }

    metrics.total_messages_sent++;

    // Build context from all sources
    context_mgr->refresh_all();
    String system_prompt = context_mgr->build_context_string();

    // Inject memory facts
    if (memory_mgr.is_valid()) {
        Vector<MemoryFact> recent = memory_mgr->get_recent_facts(10);
        if (recent.size() > 0) {
            String mem_block = "\n## Memory Facts\n";
            for (const MemoryFact &f : recent) {
                mem_block += "- [" + f.category + "] " + f.content + "\n";
            }
            system_prompt += mem_block;
        }
    }

    // Build tool definitions from executor
    Array tools;
    if (executor.is_valid()) {
        tools = executor->get_tool_definitions();
    }

    provider->send_request(p_message, system_prompt, tools, /*stream=*/false);
}

void AIEngine::cancel_current_request() {
    if (provider.is_valid()) {
        provider->cancel();
    }
}

// Fix 3 — handle_tool_call now actually executes through the executor
void AIEngine::handle_tool_call(const String &p_tool_name, const Dictionary &p_args) {
    if (!executor.is_valid()) return;
    metrics.total_tool_calls++;

    if (executor->requires_confirmation(p_tool_name)) {
        emit_signal("tool_confirmation_required", p_tool_name, p_args);
        return;
    }
    String call_id = p_tool_name + "_" +
        String::num_int64(Time::get_singleton()->get_ticks_msec());
    executor->execute_async(p_tool_name, p_args, call_id);
}

void AIEngine::analyze_project() {
    if (context_mgr.is_valid()) {
        context_mgr->refresh_all();
    }
}

void AIEngine::reindex_codebase() {
    if (vector_db.is_valid() && context_mgr.is_valid()) {
        context_mgr->refresh_all();
    }
}

Dictionary AIEngine::get_metrics() const {
    Dictionary d;
    d["total_messages_sent"]     = metrics.total_messages_sent;
    d["total_messages_received"] = metrics.total_messages_received;
    d["total_tokens_used"]       = metrics.total_tokens_used;
    d["total_tool_calls"]        = metrics.total_tool_calls;
    d["successful_tool_calls"]   = metrics.successful_tool_calls;
    d["failed_tool_calls"]       = metrics.failed_tool_calls;
    return d;
}

void AIEngine::_on_provider_response(const String &p_response) {
    metrics.total_messages_received++;
    if (memory_mgr.is_valid()) {
        memory_mgr->extract_facts_from_response(p_response, "");
    }
}

void AIEngine::_on_tool_call_requested(const String &p_name, const Dictionary &p_args) {
    handle_tool_call(p_name, p_args);
}

// Fix 1 — streaming chunk forwarded to dock UI via signal
void AIEngine::_on_streaming_chunk(const String &p_chunk) {
    emit_signal("response_chunk_received", p_chunk);
}

// Fix 1 — tool output injected back into context and forwarded
void AIEngine::_on_tool_output_received(const String &p_call_id, const String &p_result) {
    metrics.successful_tool_calls++;
    if (context_mgr.is_valid()) {
        context_mgr->add_block(
            "Last Tool Result (" + p_call_id + ")",
            p_result,
            PRIORITY_HIGH
        );
    }
    emit_signal("tool_output_received", p_call_id, p_result);
}

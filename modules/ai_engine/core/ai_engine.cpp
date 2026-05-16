#include "ai_engine.h"
#include "core/os/time.h"

AIEngine *AIEngine::singleton = nullptr;

AIEngine::AIEngine() {
    singleton = this;
}

AIEngine *AIEngine::get_singleton() {
    if (!singleton) {
        singleton = new AIEngine();
    }
    return singleton;
}

void AIEngine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize"),                 &AIEngine::initialize);
    ClassDB::bind_method(D_METHOD("shutdown"),                   &AIEngine::shutdown);
    ClassDB::bind_method(D_METHOD("send_message", "message"),    &AIEngine::send_message);
    ClassDB::bind_method(D_METHOD("cancel_current_request"),     &AIEngine::cancel_current_request);
    ClassDB::bind_method(D_METHOD("analyze_project"),            &AIEngine::analyze_project);
    ClassDB::bind_method(D_METHOD("reindex_codebase"),           &AIEngine::reindex_codebase);
    ClassDB::bind_method(D_METHOD("get_metrics"),                &AIEngine::get_metrics);
}

void AIEngine::initialize() {
    if (is_initialized) return;
    is_initialized = true;
    print_line("[AIEngine] Initialized.");
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

void AIEngine::send_message(const String &p_message) {
    if (!is_initialized) {
        WARN_PRINT("[AIEngine] Not initialized.");
        return;
    }
    metrics.total_messages_sent++;
    if (provider.is_valid()) {
        // provider->send(p_message, context_mgr);
    }
}

void AIEngine::cancel_current_request() {
    if (provider.is_valid()) {
        // provider->cancel();
    }
}

void AIEngine::handle_tool_call(const String &p_tool_name, const Dictionary &p_args) {
    metrics.total_tool_calls++;
    if (executor.is_valid()) {
        // executor->execute(p_tool_name, p_args);
    }
}

void AIEngine::analyze_project() {
    if (context_mgr.is_valid()) {
        // context_mgr->analyze_full_project();
    }
}

void AIEngine::reindex_codebase() {
    if (vector_db.is_valid()) {
        // vector_db->reindex_all();
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
}

void AIEngine::_on_tool_call_requested(const String &p_name, const Dictionary &p_args) {
    handle_tool_call(p_name, p_args);
}

void AIEngine::_on_tool_output(const String &p_output) {
    metrics.successful_tool_calls++;
}

#pragma once
#include "core/object/ref_counted.h"
#include "ai_types.h"

class AIProvider;
class ToolExecutor;
class ContextManager;
class VectorDB;
class SessionManager;
class MemoryManager;

class AIEngine : public RefCounted {
    GDCLASS(AIEngine, RefCounted);

public:
    static AIEngine *get_singleton();

    void initialize();
    void shutdown();

    Ref<AIProvider>     get_provider()         const { return provider; }
    Ref<ToolExecutor>   get_executor()         const { return executor; }
    Ref<ContextManager> get_context_manager()  const { return context_mgr; }
    Ref<VectorDB>       get_vector_db()        const { return vector_db; }
    Ref<SessionManager> get_session_manager()  const { return session_mgr; }
    Ref<MemoryManager>  get_memory_manager()   const { return memory_mgr; }

    void send_message(const String &p_message);
    void cancel_current_request();
    void handle_tool_call(const String &p_tool_name, const Dictionary &p_args);

    void analyze_project();
    void reindex_codebase();

    Dictionary get_metrics() const;

    AIMetrics &get_metrics_ref() { return metrics; }

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

    bool      is_initialized = false;
    AIMetrics metrics;

    void _on_provider_response(const String &p_response);
    void _on_tool_call_requested(const String &p_name, const Dictionary &p_args);
    void _on_tool_output(const String &p_output);
};

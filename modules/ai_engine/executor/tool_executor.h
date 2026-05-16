#pragma once
#include "core/object/ref_counted.h"
#include "modules/ai_engine/core/ai_types.h"
#include "tool_registry.h"

class ToolExecutor : public RefCounted {
    GDCLASS(ToolExecutor, RefCounted);

public:
    ToolExecutor();

    // Execute a tool by name with given arguments
    String execute(const String &p_tool_name, const Dictionary &p_args);

    // Async execute — emits "tool_completed" signal when done
    void execute_async(const String &p_tool_name, const Dictionary &p_args, const String &p_call_id);

    // Check if a tool requires user confirmation
    bool requires_confirmation(const String &p_tool_name) const;

    // Get all available tools (for system prompt injection)
    Array get_tool_definitions() const;
    Vector<String> get_tool_names() const;

    // Approve/deny a pending tool call
    void approve_pending(const String &p_call_id);
    void deny_pending(const String &p_call_id);

    // Execution log
    Array get_execution_log() const;
    void  clear_execution_log();

    Ref<ToolRegistry> get_registry() const { return registry; }

protected:
    static void _bind_methods();

private:
    Ref<ToolRegistry>       registry;
    Vector<ToolCall>        execution_log;
    HashMap<String, ToolCall> pending_confirmations;

    ToolCall _make_call(const String &name, const Dictionary &args);
    void     _log_result(ToolCall &call, const String &result, bool success);
};

#include "tool_executor.h"
#include "tools/file_tools.h"
#include "tools/node_tools.h"
#include "tools/script_tools.h"
#include "tools/scene_tools.h"
#include "tools/project_tools.h"
#include "tools/git_tools.h"
#include "tools/audit_tools.h"
#include "core/os/time.h"

ToolExecutor::ToolExecutor() {
    registry.instantiate();
    registry->register_tool_set(memnew(FileTools));
    registry->register_tool_set(memnew(NodeTools));
    registry->register_tool_set(memnew(ScriptTools));
    registry->register_tool_set(memnew(SceneTools));
    registry->register_tool_set(memnew(ProjectTools));
    registry->register_tool_set(memnew(GitTools));
    registry->register_tool_set(memnew(AuditTools));
}

void ToolExecutor::_bind_methods() {
    ClassDB::bind_method(D_METHOD("execute", "tool_name", "args"),            &ToolExecutor::execute);
    ClassDB::bind_method(D_METHOD("execute_async", "tool_name", "args", "call_id"), &ToolExecutor::execute_async);
    ClassDB::bind_method(D_METHOD("requires_confirmation", "tool_name"),      &ToolExecutor::requires_confirmation);
    ClassDB::bind_method(D_METHOD("get_tool_definitions"),                    &ToolExecutor::get_tool_definitions);
    ClassDB::bind_method(D_METHOD("get_execution_log"),                       &ToolExecutor::get_execution_log);
    ClassDB::bind_method(D_METHOD("clear_execution_log"),                     &ToolExecutor::clear_execution_log);
    ClassDB::bind_method(D_METHOD("approve_pending", "call_id"),              &ToolExecutor::approve_pending);
    ClassDB::bind_method(D_METHOD("deny_pending", "call_id"),                 &ToolExecutor::deny_pending);
}

ToolCall ToolExecutor::_make_call(const String &name, const Dictionary &args) {
    ToolCall call;
    call.id = String::num_int64(Time::get_singleton()->get_ticks_msec()) + "_" + name;
    call.name = name;
    call.arguments = args;
    call.requested_at_ms = Time::get_singleton()->get_ticks_msec();
    call.status = TOOL_PENDING;
    return call;
}

String ToolExecutor::execute(const String &p_tool_name, const Dictionary &p_args) {
    ToolCall call = _make_call(p_tool_name, p_args);

    if (requires_confirmation(p_tool_name)) {
        call.status = TOOL_REQUIRES_CONFIRMATION;
        call.requires_user_confirmation = true;
        pending_confirmations[call.id] = call;
        return String("[PENDING_CONFIRMATION:") + call.id + "]";
    }

    call.status = TOOL_RUNNING;
    call.started_at_ms = Time::get_singleton()->get_ticks_msec();

    String result;
    bool success = false;
    if (registry.is_valid()) {
        success = registry->execute(p_tool_name, p_args, result);
    } else {
        result = "Tool registry not initialized";
    }

    call.completed_at_ms = Time::get_singleton()->get_ticks_msec();
    _log_result(call, result, success);
    return result;
}

void ToolExecutor::execute_async(const String &p_tool_name, const Dictionary &p_args, const String &p_call_id) {
    String result = execute(p_tool_name, p_args);
    emit_signal("tool_completed", p_call_id, result);
}

bool ToolExecutor::requires_confirmation(const String &p_tool_name) const {
    if (!registry.is_valid()) return false;
    const ToolDefinition *def = registry->get_definition(p_tool_name);
    return def && def->requires_confirmation;
}

Array ToolExecutor::get_tool_definitions() const {
    Array defs;
    if (!registry.is_valid()) return defs;
    for (const ToolDefinition &td : registry->get_all_definitions()) {
        Dictionary d;
        d["name"]        = td.name;
        d["description"] = td.description;
        d["category"]    = td.category;
        Dictionary params_schema;
        params_schema["type"] = "object";
        Dictionary props;
        Array required_arr;
        for (const ToolParameter &p : td.parameters) {
            Dictionary prop;
            prop["type"]        = p.type;
            prop["description"] = p.description;
            if (p.enum_values.size() > 0) prop["enum"] = p.enum_values;
            props[p.name] = prop;
            if (p.required) required_arr.push_back(p.name);
        }
        params_schema["properties"] = props;
        params_schema["required"]   = required_arr;
        Dictionary tool_def;
        tool_def["type"] = "function";
        Dictionary fn_def;
        fn_def["name"]        = td.name;
        fn_def["description"] = td.description;
        fn_def["parameters"]  = params_schema;
        tool_def["function"]  = fn_def;
        defs.push_back(tool_def);
    }
    return defs;
}

Vector<String> ToolExecutor::get_tool_names() const {
    if (!registry.is_valid()) return Vector<String>();
    return registry->get_tool_names();
}

void ToolExecutor::approve_pending(const String &p_call_id) {
    if (!pending_confirmations.has(p_call_id)) return;
    ToolCall &call = pending_confirmations[p_call_id];
    call.status = TOOL_RUNNING;
    call.started_at_ms = Time::get_singleton()->get_ticks_msec();
    String result;
    bool success = registry.is_valid() && registry->execute(call.name, call.arguments, result);
    call.completed_at_ms = Time::get_singleton()->get_ticks_msec();
    _log_result(call, result, success);
    pending_confirmations.erase(p_call_id);
    emit_signal("tool_completed", p_call_id, result);
}

void ToolExecutor::deny_pending(const String &p_call_id) {
    if (!pending_confirmations.has(p_call_id)) return;
    ToolCall &call = pending_confirmations[p_call_id];
    call.status = TOOL_CANCELLED;
    call.error_message = "User denied execution";
    _log_result(call, "Cancelled by user", false);
    pending_confirmations.erase(p_call_id);
    emit_signal("tool_cancelled", p_call_id);
}

void ToolExecutor::_log_result(ToolCall &call, const String &result, bool success) {
    call.result = result;
    call.status = success ? TOOL_SUCCESS : TOOL_FAILED;
    if (!success) call.error_message = result;
    execution_log.push_back(call);
    if (execution_log.size() > 500) execution_log.remove_at(0);
}

Array ToolExecutor::get_execution_log() const {
    Array arr;
    for (const ToolCall &c : execution_log) {
        Dictionary d;
        d["id"]     = c.id;
        d["name"]   = c.name;
        d["status"] = (int)c.status;
        d["result"] = c.result;
        d["error"]  = c.error_message;
        d["duration_ms"] = (int)(c.completed_at_ms - c.started_at_ms);
        arr.push_back(d);
    }
    return arr;
}

void ToolExecutor::clear_execution_log() {
    execution_log.clear();
}

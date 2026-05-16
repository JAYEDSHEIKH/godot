#include "openai_provider.h"
#include "core/io/json.h"
#include "core/os/time.h"

OpenAIProvider::OpenAIProvider() {
    base_url      = "https://api.openai.com/v1";
    current_model = "gpt-4o";
}

void OpenAIProvider::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_default_model", "model"), &OpenAIProvider::set_default_model);
    ClassDB::bind_method(D_METHOD("set_openrouter_mode", "enable"), &OpenAIProvider::set_openrouter_mode);
}

void OpenAIProvider::set_default_model(const String &model) {
    current_model = model;
}

void OpenAIProvider::set_openrouter_mode(bool p_enable) {
    openrouter = p_enable;
    if (p_enable) {
        base_url = "https://openrouter.ai/api/v1";
    }
}

void OpenAIProvider::_build_request_body(Dictionary &r_body, const String &p_prompt,
    const String &p_system, const Array &p_tools, bool p_stream) {
    r_body["model"]       = current_model;
    r_body["temperature"] = temperature;
    r_body["max_tokens"]  = max_tokens;
    r_body["stream"]      = p_stream;

    Array messages;
    if (!p_system.is_empty()) {
        Dictionary sys_msg;
        sys_msg["role"]    = "system";
        sys_msg["content"] = p_system;
        messages.push_back(sys_msg);
    }
    Dictionary user_msg;
    user_msg["role"]    = "user";
    user_msg["content"] = p_prompt;
    messages.push_back(user_msg);
    r_body["messages"] = messages;

    if (p_tools.size() > 0) {
        r_body["tools"] = p_tools;
    }
}

void OpenAIProvider::send_request(
    const String &p_prompt,
    const String &p_system_prompt,
    const Array  &p_tools,
    bool p_stream
) {
    if (api_key.is_empty()) {
        emit_signal("response_error", "API key not set");
        return;
    }
    Dictionary body;
    _build_request_body(body, p_prompt, p_system_prompt, p_tools, p_stream);
    accumulated_response = "";
    is_requesting = true;
    emit_signal("status_changed", "Thinking...");
    // HTTP request is performed asynchronously via HTTPRequest node in Godot
    // The actual implementation requires an HTTPRequest node in the scene tree
    // For now, emit a placeholder response
    emit_signal("response_started");
}

void OpenAIProvider::cancel() {
    is_requesting = false;
    emit_signal("status_changed", "Cancelled");
}

void OpenAIProvider::_parse_sse_chunk(const String &p_chunk) {
    Vector<String> lines = p_chunk.split("\n");
    for (const String &line : lines) {
        if (!line.begins_with("data: ")) continue;
        String data = line.substr(6);
        if (data == "[DONE]") {
            emit_signal("response_complete", accumulated_response);
            is_requesting = false;
            return;
        }
        Variant parsed;
        String err_str;
        int err_line;
        Error err = JSON::parse(data, parsed, err_str, err_line);
        if (err == OK && parsed.get_type() == Variant::DICTIONARY) {
            Dictionary d = parsed;
            Array choices = d.get("choices", Array());
            if (choices.size() > 0) {
                Dictionary choice = choices[0];
                Dictionary delta = choice.get("delta", Dictionary());
                String content = delta.get("content", "");
                if (!content.is_empty()) {
                    accumulated_response += content;
                    emit_signal("response_chunk_received", content);
                }
                if (delta.has("tool_calls")) {
                    _handle_tool_calls(delta["tool_calls"]);
                }
            }
        }
    }
}

void OpenAIProvider::_handle_tool_calls(const Array &p_tool_calls) {
    for (int i = 0; i < p_tool_calls.size(); i++) {
        Dictionary tc = p_tool_calls[i];
        Dictionary fn = tc.get("function", Dictionary());
        String name = fn.get("name", "");
        String args_str = fn.get("arguments", "{}");
        Variant args_v;
        String err_str;
        int err_line;
        if (JSON::parse(args_str, args_v, err_str, err_line) == OK) {
            emit_signal("tool_call_requested", name, args_v);
        }
    }
}

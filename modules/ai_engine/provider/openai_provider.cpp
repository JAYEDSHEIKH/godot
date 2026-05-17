#include "openai_provider.h"
#include "core/io/json.h"
#include "core/os/time.h"

OpenAIProvider::OpenAIProvider() {
    base_url      = "https://api.openai.com/v1";
    current_model = "gpt-4o";
}

void OpenAIProvider::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_default_model",  "model"),  &OpenAIProvider::set_default_model);
    ClassDB::bind_method(D_METHOD("set_openrouter_mode","enable"), &OpenAIProvider::set_openrouter_mode);
}

void OpenAIProvider::set_default_model(const String &model) {
    current_model = model;
}

void OpenAIProvider::set_openrouter_mode(bool p_enable) {
    openrouter = p_enable;
    if (p_enable) {
        base_url = "https://openrouter.ai/api/v1";
    } else {
        base_url = "https://api.openai.com/v1";
    }
}

// Fix 5 — hook up the HTTPRequest node supplied by the dock
void OpenAIProvider::setup_http_request(HTTPRequest *p_req) {
    http_request = p_req;
    if (http_request) {
        http_request->connect("request_completed",
            callable_mp(this, &OpenAIProvider::_on_request_completed));
        http_request->set_use_threads(true);
        connected = true;
    }
}

// Fix 6 — build full conversation history into the request body
void OpenAIProvider::_build_request_body(Dictionary &r_body, const String &p_prompt,
    const String &p_system, const Array &p_tools, bool p_stream) {
    r_body["model"]       = current_model;
    r_body["temperature"] = temperature;
    r_body["max_tokens"]  = max_tokens;
    r_body["stream"]      = false;  // non-streaming until SSE parsing is fully wired

    Array messages;
    if (!p_system.is_empty()) {
        Dictionary sys;
        sys["role"]    = "system";
        sys["content"] = p_system;
        messages.push_back(sys);
    }
    // Inject full history for multi-turn conversation
    for (const Dictionary &msg : conversation_history) {
        messages.push_back(msg);
    }
    // Add the new user turn
    Dictionary user_msg;
    user_msg["role"]    = "user";
    user_msg["content"] = p_prompt;
    messages.push_back(user_msg);
    conversation_history.push_back(user_msg);

    r_body["messages"] = messages;
    if (p_tools.size() > 0) {
        r_body["tools"] = p_tools;
    }
}

// Fix 5 — actually sends the HTTP POST
void OpenAIProvider::send_request(
    const String &p_prompt,
    const String &p_system_prompt,
    const Array  &p_tools,
    bool p_stream
) {
    if (api_key.is_empty()) {
        emit_signal("response_error", "API key not set. Configure it in AI Copilot → Settings.");
        return;
    }
    if (!http_request) {
        emit_signal("response_error",
            "HTTPRequest not initialized — ensure the AI dock is open and setup_http_request() was called.");
        return;
    }
    if (is_requesting) {
        emit_signal("response_error", "Request already in progress. Please wait.");
        return;
    }

    Dictionary body;
    _build_request_body(body, p_prompt, p_system_prompt, p_tools, p_stream);

    PackedStringArray headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back("Authorization: Bearer " + api_key);
    if (openrouter) {
        headers.push_back("HTTP-Referer: https://godot.editor");
        headers.push_back("X-Title: Godot AI Engine");
    }

    String body_str = JSON::stringify(body);
    accumulated_response = "";
    is_requesting = true;
    emit_signal("status_changed", "Thinking...");
    emit_signal("response_started");

    Error err = http_request->request(
        base_url + "/chat/completions",
        headers,
        HTTPClient::METHOD_POST,
        body_str
    );
    if (err != OK) {
        is_requesting = false;
        emit_signal("response_error", "HTTP request failed: " + itos((int)err));
    }
}

void OpenAIProvider::cancel() {
    if (http_request && is_requesting) {
        http_request->cancel_request();
    }
    is_requesting = false;
    emit_signal("status_changed", "Cancelled");
}

// Fix 5 — handle completed HTTP response
void OpenAIProvider::_on_request_completed(
    int p_result, int p_response_code,
    const PackedStringArray &p_headers,
    const PackedByteArray &p_body
) {
    is_requesting = false;
    emit_signal("status_changed", "Ready");

    if (p_result != HTTPRequest::RESULT_SUCCESS) {
        emit_signal("response_error", "Network error: " + itos(p_result));
        return;
    }
    if (p_response_code != 200) {
        String body_str = p_body.get_string_from_utf8();
        String error_msg = "HTTP " + itos(p_response_code);
        Variant parsed;
        String err_str;
        int err_line;
        if (JSON::parse(body_str, parsed, err_str, err_line) == OK &&
                parsed.get_type() == Variant::DICTIONARY) {
            Dictionary d = parsed;
            if (d.has("error")) {
                Dictionary e = d["error"];
                error_msg += ": " + e.get("message", "Unknown error").operator String();
            }
        }
        emit_signal("response_error", error_msg);
        return;
    }

    _parse_full_response(p_body.get_string_from_utf8());
}

// Fix 5 — parse the non-streaming response JSON
void OpenAIProvider::_parse_full_response(const String &p_body) {
    Variant parsed;
    String err_str;
    int err_line;
    if (JSON::parse(p_body, parsed, err_str, err_line) != OK ||
            parsed.get_type() != Variant::DICTIONARY) {
        emit_signal("response_error", "JSON parse error: " + err_str);
        return;
    }
    Dictionary d = parsed;
    Array choices = d.get("choices", Array());
    if (choices.is_empty()) {
        emit_signal("response_error", "No choices in response");
        return;
    }
    Dictionary choice  = choices[0];
    Dictionary message = choice.get("message", Dictionary());
    String content     = message.get("content", "").operator String();

    if (message.has("tool_calls")) {
        _handle_tool_calls(message["tool_calls"]);
    }

    if (!content.is_empty()) {
        // Fix 6 — store assistant turn in history
        Dictionary assistant_msg;
        assistant_msg["role"]    = "assistant";
        assistant_msg["content"] = content;
        conversation_history.push_back(assistant_msg);

        accumulated_response = content;
        emit_signal("response_chunk_received", content);
        emit_signal("response_complete", content);
    }
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
        if (JSON::parse(data, parsed, err_str, err_line) == OK &&
                parsed.get_type() == Variant::DICTIONARY) {
            Dictionary d = parsed;
            Array choices = d.get("choices", Array());
            if (choices.size() > 0) {
                Dictionary choice = choices[0];
                Dictionary delta  = choice.get("delta", Dictionary());
                String chunk_text = delta.get("content", "").operator String();
                if (!chunk_text.is_empty()) {
                    accumulated_response += chunk_text;
                    emit_signal("response_chunk_received", chunk_text);
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
        String name   = fn.get("name", "").operator String();
        String args_str = fn.get("arguments", "{}").operator String();
        Variant args_v;
        String err_str;
        int err_line;
        if (JSON::parse(args_str, args_v, err_str, err_line) == OK) {
            emit_signal("tool_call_requested", name, args_v);
        }
    }
}

#include "gemini_provider.h"
#include "core/io/json.h"

GeminiProvider::GeminiProvider() {
    base_url      = "https://generativelanguage.googleapis.com/v1beta";
    current_model = "gemini-1.5-pro";
}

// Fix 5 — inject HTTPRequest from scene tree
void GeminiProvider::setup_http_request(HTTPRequest *p_req) {
    http_request = p_req;
    if (http_request) {
        http_request->connect("request_completed",
            callable_mp(this, &GeminiProvider::_on_request_completed));
        http_request->set_use_threads(true);
    }
}

void GeminiProvider::send_request(
    const String &p_prompt,
    const String &p_system_prompt,
    const Array  &p_tools,
    bool p_stream
) {
    if (api_key.is_empty()) {
        emit_signal("response_error", "Gemini API key not set.");
        return;
    }
    if (!http_request) {
        emit_signal("response_error", "HTTPRequest not initialized.");
        return;
    }
    if (is_requesting) {
        emit_signal("response_error", "Request already in progress.");
        return;
    }

    // Build Gemini REST body
    Dictionary body;
    Array contents;
    Dictionary user_part;
    Array parts;
    Dictionary part;
    part["text"] = p_prompt;
    parts.push_back(part);
    user_part["role"]  = "user";
    user_part["parts"] = parts;
    contents.push_back(user_part);
    body["contents"] = contents;

    if (!p_system_prompt.is_empty()) {
        Dictionary sys_instruction;
        Array sys_parts;
        Dictionary sys_part;
        sys_part["text"] = p_system_prompt;
        sys_parts.push_back(sys_part);
        sys_instruction["parts"] = sys_parts;
        body["system_instruction"] = sys_instruction;
    }

    Dictionary gen_config;
    gen_config["temperature"] = temperature;
    gen_config["maxOutputTokens"] = max_tokens;
    body["generationConfig"] = gen_config;

    PackedStringArray headers;
    headers.push_back("Content-Type: application/json");

    String url = base_url + "/models/" + current_model + ":generateContent?key=" + api_key;
    accumulated_response = "";
    is_requesting = true;
    emit_signal("status_changed", "Thinking (Gemini)...");
    emit_signal("response_started");

    Error err = http_request->request(url, headers, HTTPClient::METHOD_POST, JSON::stringify(body));
    if (err != OK) {
        is_requesting = false;
        emit_signal("response_error", "HTTP request failed: " + itos((int)err));
    }
}

void GeminiProvider::cancel() {
    if (http_request && is_requesting) {
        http_request->cancel_request();
    }
    is_requesting = false;
    emit_signal("status_changed", "Cancelled");
}

void GeminiProvider::_on_request_completed(
    int p_result, int p_response_code,
    const PackedStringArray &p_headers,
    const PackedByteArray &p_body
) {
    is_requesting = false;
    emit_signal("status_changed", "Ready");

    if (p_result != HTTPRequest::RESULT_SUCCESS || p_response_code != 200) {
        emit_signal("response_error", "Gemini HTTP error: " + itos(p_response_code));
        return;
    }
    _parse_gemini_response(p_body.get_string_from_utf8());
}

void GeminiProvider::_parse_gemini_response(const String &p_response) {
    Variant parsed;
    String err_str;
    int err_line;
    if (JSON::parse(p_response, parsed, err_str, err_line) != OK ||
            parsed.get_type() != Variant::DICTIONARY) {
        emit_signal("response_error", "Gemini JSON parse error: " + err_str);
        return;
    }
    Dictionary d = parsed;
    Array candidates = d.get("candidates", Array());
    if (candidates.is_empty()) {
        emit_signal("response_error", "No candidates in Gemini response");
        return;
    }
    Dictionary cand    = candidates[0];
    Dictionary content = cand.get("content", Dictionary());
    Array parts        = content.get("parts", Array());
    String full_text;
    for (int i = 0; i < parts.size(); i++) {
        Dictionary part_d = parts[i];
        full_text += part_d.get("text", "").operator String();
    }
    if (!full_text.is_empty()) {
        accumulated_response = full_text;
        emit_signal("response_chunk_received", full_text);
        emit_signal("response_complete", full_text);
    } else {
        emit_signal("response_error", "Empty response from Gemini");
    }
}

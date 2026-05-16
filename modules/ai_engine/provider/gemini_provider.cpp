#include "gemini_provider.h"
#include "core/io/json.h"

GeminiProvider::GeminiProvider() {
    base_url      = "https://generativelanguage.googleapis.com/v1beta";
    current_model = "gemini-1.5-pro";
}

void GeminiProvider::send_request(
    const String &p_prompt,
    const String &p_system_prompt,
    const Array  &p_tools,
    bool p_stream
) {
    if (api_key.is_empty()) {
        emit_signal("response_error", "Gemini API key not set");
        return;
    }
    accumulated_response = "";
    is_requesting = true;
    emit_signal("status_changed", "Thinking (Gemini)...");
    emit_signal("response_started");
}

void GeminiProvider::cancel() {
    is_requesting = false;
    emit_signal("status_changed", "Cancelled");
}

void GeminiProvider::_parse_gemini_response(const String &p_response) {
    Variant parsed;
    String err_str;
    int err_line;
    if (JSON::parse(p_response, parsed, err_str, err_line) == OK && parsed.get_type() == Variant::DICTIONARY) {
        Dictionary d = parsed;
        Array candidates = d.get("candidates", Array());
        if (candidates.size() > 0) {
            Dictionary cand = candidates[0];
            Dictionary content = cand.get("content", Dictionary());
            Array parts = content.get("parts", Array());
            String full_text;
            for (int i = 0; i < parts.size(); i++) {
                Dictionary part = parts[i];
                full_text += part.get("text", "").operator String();
            }
            if (!full_text.is_empty()) {
                accumulated_response = full_text;
                emit_signal("response_chunk_received", full_text);
                emit_signal("response_complete", full_text);
            }
        }
    }
}

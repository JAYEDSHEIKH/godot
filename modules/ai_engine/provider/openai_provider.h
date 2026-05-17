#pragma once
#include "ai_provider.h"
#include "scene/main/http_request.h"

class OpenAIProvider : public AIProvider {
    GDCLASS(OpenAIProvider, AIProvider);

public:
    OpenAIProvider();

    void send_request(
        const String &p_prompt,
        const String &p_system_prompt,
        const Array  &p_tools,
        bool p_stream = true
    ) override;

    void cancel() override;
    bool is_connected() const override { return connected; }
    String get_provider_name() const override { return openrouter ? "OpenRouter" : "OpenAI"; }
    String get_model_name()    const override { return current_model; }

    void set_default_model(const String &model);
    void set_openrouter_mode(bool p_enable);

    // Fix 5 — called by dock to inject an HTTPRequest node already in the scene tree
    void setup_http_request(HTTPRequest *p_req) override;

protected:
    static void _bind_methods();

private:
    bool   connected     = false;
    bool   openrouter    = false;
    bool   is_requesting = false;

    HTTPRequest *http_request = nullptr;

    // Fix 6 — full conversation history sent each turn
    Vector<Dictionary> conversation_history;

    void _build_request_body(Dictionary &r_body, const String &p_prompt,
                             const String &p_system, const Array &p_tools, bool p_stream);
    void _parse_sse_chunk(const String &p_chunk);
    void _parse_full_response(const String &p_body);
    void _handle_tool_calls(const Array &p_tool_calls);

    void _on_request_completed(int p_result, int p_response_code,
        const PackedStringArray &p_headers, const PackedByteArray &p_body);

    String accumulated_response;
};

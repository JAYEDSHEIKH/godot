#pragma once
#include "ai_provider.h"
#include "core/io/http_client.h"

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
    String get_provider_name() const override { return "OpenAI"; }
    String get_model_name()    const override { return current_model; }

    void set_default_model(const String &model);

    // OpenRouter support (same API, different base URL)
    void set_openrouter_mode(bool p_enable);

protected:
    static void _bind_methods();

private:
    bool   connected     = false;
    bool   openrouter    = false;
    bool   is_requesting = false;

    void _build_request_body(Dictionary &r_body, const String &p_prompt,
                             const String &p_system, const Array &p_tools, bool p_stream);
    void _parse_sse_chunk(const String &p_chunk);
    void _handle_tool_calls(const Array &p_tool_calls);

    String accumulated_response;
};

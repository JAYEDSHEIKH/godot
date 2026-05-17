#pragma once
#include "ai_provider.h"
#include "scene/main/http_request.h"

class GeminiProvider : public AIProvider {
    GDCLASS(GeminiProvider, AIProvider);

public:
    GeminiProvider();

    void send_request(
        const String &p_prompt,
        const String &p_system_prompt,
        const Array  &p_tools,
        bool p_stream = true
    ) override;

    void cancel() override;
    String get_provider_name() const override { return "Gemini"; }
    String get_model_name()    const override { return current_model; }

    // Fix 5 — inject HTTPRequest from scene tree
    void setup_http_request(HTTPRequest *p_req) override;

protected:
    static void _bind_methods() {}

private:
    bool is_requesting = false;
    HTTPRequest *http_request = nullptr;
    String accumulated_response;

    void _parse_gemini_response(const String &p_response);
    void _on_request_completed(int p_result, int p_response_code,
        const PackedStringArray &p_headers, const PackedByteArray &p_body);
};

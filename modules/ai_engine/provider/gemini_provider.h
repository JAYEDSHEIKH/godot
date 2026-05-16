#pragma once
#include "ai_provider.h"

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

protected:
    static void _bind_methods() {}

private:
    bool is_requesting = false;
    String accumulated_response;
    void _parse_gemini_response(const String &p_response);
};

#pragma once
#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"
#include "modules/ai_engine/core/ai_types.h"

class AIProvider : public RefCounted {
    GDCLASS(AIProvider, RefCounted);

public:
    virtual void send_request(
        const String &p_prompt,
        const String &p_system_prompt,
        const Array  &p_tools,
        bool p_stream = true
    ) {}

    virtual void cancel() {}

    virtual bool is_connected() const { return false; }

    virtual String get_provider_name() const { return "Base"; }
    virtual String get_model_name()    const { return ""; }

    void set_api_key(const String &key)   { api_key = key; }
    void set_base_url(const String &url)  { base_url = url; }
    void set_model(const String &model)   { current_model = model; }
    void set_temperature(float t)         { temperature = t; }
    void set_max_tokens(int m)            { max_tokens = m; }

    String get_api_key()   const { return api_key; }
    String get_base_url()  const { return base_url; }
    float  get_temperature() const { return temperature; }
    int    get_max_tokens()  const { return max_tokens; }

    // Signals emitted by provider implementations
    // "response_started"
    // "response_chunk_received" (chunk: String)
    // "response_complete" (full_response: String)
    // "response_error" (error: String)
    // "status_changed" (status: String)
    // "tool_call_requested" (name: String, args: Dictionary)

protected:
    static void _bind_methods() {}

    String api_key;
    String base_url;
    String current_model;
    float  temperature  = 0.7f;
    int    max_tokens   = 4096;
    bool   streaming    = true;
};

#pragma once
#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"
#include "modules/ai_engine/core/ai_types.h"

class HTTPRequest;

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

    // Called by the dock so the provider can own an HTTPRequest node
    virtual void setup_http_request(HTTPRequest *p_req) {}

    void set_api_key(const String &key)    { api_key = key; }
    void set_base_url(const String &url)   { base_url = url; }
    void set_model(const String &model)    { current_model = model; }
    void set_temperature(float t)          { temperature = t; }
    void set_max_tokens(int m)             { max_tokens = m; }

    String get_api_key()     const { return api_key; }
    String get_base_url()    const { return base_url; }
    float  get_temperature() const { return temperature; }
    int    get_max_tokens()  const { return max_tokens; }

protected:
    static void _bind_methods();

    String api_key;
    String base_url;
    String current_model;
    float  temperature  = 0.7f;
    int    max_tokens   = 4096;
    bool   streaming    = true;
};

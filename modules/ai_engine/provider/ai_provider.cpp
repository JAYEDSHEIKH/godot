#include "ai_provider.h"

void AIProvider::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_api_key",    "key"),   &AIProvider::set_api_key);
    ClassDB::bind_method(D_METHOD("set_base_url",   "url"),   &AIProvider::set_base_url);
    ClassDB::bind_method(D_METHOD("set_model",      "model"), &AIProvider::set_model);
    ClassDB::bind_method(D_METHOD("set_temperature","t"),     &AIProvider::set_temperature);
    ClassDB::bind_method(D_METHOD("set_max_tokens", "m"),     &AIProvider::set_max_tokens);
    ClassDB::bind_method(D_METHOD("cancel"),                  &AIProvider::cancel);
    ClassDB::bind_method(D_METHOD("get_provider_name"),       &AIProvider::get_provider_name);
    ClassDB::bind_method(D_METHOD("get_model_name"),          &AIProvider::get_model_name);

    // Fix 4 — all signals properly declared so emit_signal() works
    ADD_SIGNAL(MethodInfo("response_started"));
    ADD_SIGNAL(MethodInfo("response_chunk_received",
        PropertyInfo(Variant::STRING, "chunk")));
    ADD_SIGNAL(MethodInfo("response_complete",
        PropertyInfo(Variant::STRING, "full_response")));
    ADD_SIGNAL(MethodInfo("response_error",
        PropertyInfo(Variant::STRING, "error")));
    ADD_SIGNAL(MethodInfo("status_changed",
        PropertyInfo(Variant::STRING, "status")));
    ADD_SIGNAL(MethodInfo("tool_call_requested",
        PropertyInfo(Variant::STRING, "name"),
        PropertyInfo(Variant::DICTIONARY, "args")));
    ADD_SIGNAL(MethodInfo("stream_chunk_received",
        PropertyInfo(Variant::STRING, "chunk")));
}

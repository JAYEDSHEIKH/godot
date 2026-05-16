#include "provider_factory.h"
#include "openai_provider.h"
#include "gemini_provider.h"

Ref<AIProvider> ProviderFactory::create(AIProviderType p_type) {
    switch (p_type) {
        case AI_PROVIDER_OPENAI: {
            Ref<OpenAIProvider> p; p.instantiate(); return p;
        }
        case AI_PROVIDER_GEMINI: {
            Ref<GeminiProvider> p; p.instantiate(); return p;
        }
        case AI_PROVIDER_LOCAL:
        default: {
            Ref<OpenAIProvider> p; p.instantiate();
            p->set_base_url("http://localhost:11434/v1");
            return p;
        }
    }
}

Ref<AIProvider> ProviderFactory::create_from_string(const String &p_name) {
    if (p_name == "openai" || p_name == "OpenAI") return create(AI_PROVIDER_OPENAI);
    if (p_name == "gemini" || p_name == "Gemini") return create(AI_PROVIDER_GEMINI);
    if (p_name == "local"  || p_name == "Local")  return create(AI_PROVIDER_LOCAL);
    return create(AI_PROVIDER_OPENAI);
}

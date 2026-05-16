#pragma once
#include "ai_provider.h"
#include "modules/ai_engine/core/ai_types.h"

class ProviderFactory {
public:
    static Ref<AIProvider> create(AIProviderType p_type);
    static Ref<AIProvider> create_from_string(const String &p_name);
};

#include "register_types.h"
#include "core/object/class_db.h"
#include "core/engine.h"
#include "core/config/project_settings.h"

#include "core/ai_engine.h"
#include "provider/ai_provider.h"
#include "provider/openai_provider.h"
#include "provider/gemini_provider.h"
#include "executor/tool_executor.h"
#include "executor/tool_registry.h"
#include "context/context_manager.h"
#include "context/project_indexer.h"
#include "vector_db/vector_db.h"
#include "memory/memory_manager.h"
#include "memory/session_manager.h"
#include "utils/config_manager.h"

static AIEngine *ai_engine_singleton = nullptr;

void initialize_ai_engine_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
        GDREGISTER_CLASS(AIEngine);
        GDREGISTER_CLASS(AIProvider);
        GDREGISTER_CLASS(OpenAIProvider);
        GDREGISTER_CLASS(GeminiProvider);
        GDREGISTER_CLASS(ToolExecutor);
        GDREGISTER_CLASS(ToolRegistry);
        GDREGISTER_CLASS(ContextManager);
        GDREGISTER_CLASS(ProjectIndexer);
        GDREGISTER_CLASS(VectorDB);
        GDREGISTER_CLASS(MemoryManager);
        GDREGISTER_CLASS(SessionManager);
        GDREGISTER_CLASS(ConfigManager);
        return;
    }
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        // Initialize singletons
        ConfigManager::get_singleton()->load_config();
    }
}

void uninitialize_ai_engine_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        if (ai_engine_singleton) {
            ai_engine_singleton->shutdown();
            ai_engine_singleton = nullptr;
        }
    }
}

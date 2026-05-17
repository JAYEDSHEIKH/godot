// Fix 7 — correct initialization levels, singleton ownership, safe shutdown
#include "register_types.h"
#include "core/object/class_db.h"
#include "core/engine.h"

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

static AIEngine      *ai_engine_instance  = nullptr;
static ConfigManager *config_mgr_instance = nullptr;

void initialize_ai_engine_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
        // Register all classes so ClassDB knows about them everywhere
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
        // ConfigManager must be created first — AIEngine::initialize() reads it
        config_mgr_instance = memnew(ConfigManager);
        Engine::get_singleton()->add_singleton(
            Engine::Singleton("ConfigManager", ConfigManager::get_singleton())
        );

        // AIEngine singleton — NOT initialized yet.
        // The editor dock calls AIEngine::get_singleton()->initialize()
        // after injecting the HTTPRequest node into the provider.
        ai_engine_instance = memnew(AIEngine);
        Engine::get_singleton()->add_singleton(
            Engine::Singleton("AIEngine", AIEngine::get_singleton())
        );
        return;
    }
}

void uninitialize_ai_engine_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        if (ai_engine_instance) {
            ai_engine_instance->shutdown();
            Engine::get_singleton()->remove_singleton("AIEngine");
            memdelete(ai_engine_instance);
            ai_engine_instance = nullptr;
        }
        if (config_mgr_instance) {
            Engine::get_singleton()->remove_singleton("ConfigManager");
            memdelete(config_mgr_instance);
            config_mgr_instance = nullptr;
        }
    }
}

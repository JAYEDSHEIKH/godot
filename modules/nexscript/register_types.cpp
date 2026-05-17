#include "register_types.h"
#include "godot/nex_language.h"
#include "godot/nex_loader.h"
#include "godot/nex_saver.h"
#include "godot/nex_script.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"

static NexLanguage                  *nex_language = nullptr;
static Ref<ResourceFormatLoaderNex>  nex_loader;
static Ref<ResourceFormatSaverNex>   nex_saver;

void initialize_nexscript_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

    GDREGISTER_CLASS(NexScript);
    GDREGISTER_CLASS(ResourceFormatLoaderNex);
    GDREGISTER_CLASS(ResourceFormatSaverNex);

    nex_language = memnew(NexLanguage);
    ScriptServer::register_language(nex_language);

    nex_loader.instantiate();
    ResourceLoader::add_resource_format_loader(nex_loader);

    nex_saver.instantiate();
    ResourceSaver::add_resource_format_saver(nex_saver);
}

void uninitialize_nexscript_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

    ScriptServer::unregister_language(nex_language);
    memdelete(nex_language);
    nex_language = nullptr;

    ResourceLoader::remove_resource_format_loader(nex_loader);
    nex_loader.unref();

    ResourceSaver::remove_resource_format_saver(nex_saver);
    nex_saver.unref();
}

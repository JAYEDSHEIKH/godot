#include "nex_loader.h"
#include "nex_script.h"
#include "core/io/file_access.h"

Ref<Resource> ResourceFormatLoaderNex::load(
    const String &p_path, const String &p_original_path,
    Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode
) {
    Error err;
    String source = FileAccess::get_file_as_string(p_path, &err);
    if (err != OK) {
        if (r_error) *r_error = err;
        return Ref<Resource>();
    }

    Ref<NexScript> script;
    script.instantiate();
    script->set_source_code(source);
    script->set_path(p_path);

    Error compile_err = script->compile();
    if (r_error) *r_error = compile_err;

    return script;
}

void ResourceFormatLoaderNex::get_recognized_extensions(List<String> *p_extensions) const {
    p_extensions->push_back("nex");
}

bool ResourceFormatLoaderNex::handles_type(const String &p_type) const {
    return p_type == "Script" || p_type == "NexScript";
}

String ResourceFormatLoaderNex::get_resource_type(const String &p_path) const {
    String ext = p_path.get_extension().to_lower();
    if (ext == "nex") return "NexScript";
    return "";
}

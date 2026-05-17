#include "nex_saver.h"
#include "nex_script.h"
#include "core/io/file_access.h"

Error ResourceFormatSaverNex::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
    Ref<NexScript> script = p_resource;
    ERR_FAIL_COND_V(script.is_null(), ERR_INVALID_PARAMETER);

    String source = script->get_source_code();

    Error err;
    Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE, &err);
    ERR_FAIL_COND_V_MSG(err != OK, err, "Cannot open file for writing: " + p_path);

    f->store_string(source);
    return OK;
}

bool ResourceFormatSaverNex::recognize(const Ref<Resource> &p_resource) const {
    return p_resource->get_class_name() == "NexScript";
}

void ResourceFormatSaverNex::get_recognized_extensions(
    const Ref<Resource> &p_resource, List<String> *p_extensions
) const {
    if (recognize(p_resource)) {
        p_extensions->push_back("nex");
    }
}

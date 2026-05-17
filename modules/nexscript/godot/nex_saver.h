#pragma once
#include "core/io/resource_saver.h"

class ResourceFormatSaverNex : public ResourceFormatSaver {
    GDCLASS(ResourceFormatSaverNex, ResourceFormatSaver);

public:
    Error save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags = 0) override;
    bool recognize(const Ref<Resource> &p_resource) const override;
    void get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const override;

protected:
    static void _bind_methods() {}
};

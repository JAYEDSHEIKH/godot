#pragma once
#include "core/object/ref_counted.h"
#include "core/variant/dictionary.h"

class ConfigManager : public RefCounted {
    GDCLASS(ConfigManager, RefCounted);

    Dictionary config;
    Dictionary profiles;
    String config_path   = "user://.ai_engine/config.json";
    String profiles_path = "user://.ai_engine/profiles.json";

    static ConfigManager *singleton;

public:
    static ConfigManager *get_singleton();

    void load_config(const String &p_path = "");
    void save_config(const String &p_path = "");
    void reset_to_defaults();

    Variant    get(const String &key, const Variant &default_val = Variant());
    String     get_string(const String &key, const String &def = "");
    int        get_int(const String &key, int def = 0);
    float      get_float(const String &key, float def = 0.0f);
    bool       get_bool(const String &key, bool def = false);
    Dictionary get_dict(const String &key, const Dictionary &def = Dictionary());

    void set(const String &key, const Variant &value);
    void set_string(const String &key, const String &value);
    void set_int(const String &key, int value);
    void set_float(const String &key, float value);
    void set_bool(const String &key, bool value);

    void  save_profile(const String &name);
    void  load_profile(const String &name);
    Array get_profiles() const;
    void  delete_profile(const String &name);

protected:
    static void _bind_methods();

private:
    ConfigManager();
    void _load_json(const String &path, Dictionary &out);
    void _save_json(const String &path, const Dictionary &data);
    void _set_defaults();
};

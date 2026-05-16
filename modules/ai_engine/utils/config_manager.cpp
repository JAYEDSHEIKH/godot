#include "config_manager.h"
#include "core/io/file_access.h"
#include "core/io/dir_access.h"
#include "core/io/json.h"

ConfigManager *ConfigManager::singleton = nullptr;

ConfigManager::ConfigManager() {
    singleton = this;
    _set_defaults();
}

ConfigManager *ConfigManager::get_singleton() {
    if (!singleton) singleton = new ConfigManager();
    return singleton;
}

void ConfigManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_config"),                     &ConfigManager::load_config);
    ClassDB::bind_method(D_METHOD("save_config"),                     &ConfigManager::save_config);
    ClassDB::bind_method(D_METHOD("reset_to_defaults"),               &ConfigManager::reset_to_defaults);
    ClassDB::bind_method(D_METHOD("get", "key", "default"),           &ConfigManager::get);
    ClassDB::bind_method(D_METHOD("set", "key", "value"),             &ConfigManager::set);
    ClassDB::bind_method(D_METHOD("get_profiles"),                    &ConfigManager::get_profiles);
    ClassDB::bind_method(D_METHOD("save_profile", "name"),            &ConfigManager::save_profile);
    ClassDB::bind_method(D_METHOD("load_profile", "name"),            &ConfigManager::load_profile);
    ClassDB::bind_method(D_METHOD("delete_profile", "name"),          &ConfigManager::delete_profile);
}

void ConfigManager::_set_defaults() {
    config["provider"]               = "openai";
    config["model"]                  = "gpt-4o";
    config["temperature"]            = 0.7f;
    config["max_tokens"]             = 4096;
    config["streaming"]              = true;
    config["auto_save_sessions"]     = true;
    config["context_token_budget"]   = 6000;
    config["memory_max_facts"]       = 1000;
    config["tone"]                   = "Analytical";
    config["api_key"]                = "";
    config["gemini_api_key"]         = "";
    config["local_endpoint"]         = "http://localhost:11434/v1";
    config["auto_index_on_start"]    = false;
    config["show_thinking_indicator"] = true;
    config["diff_word_level"]        = true;
    config["font_size"]              = 14;
}

void ConfigManager::load_config(const String &p_path) {
    String path = p_path.is_empty() ? config_path : p_path;
    _load_json(path, config);
}

void ConfigManager::save_config(const String &p_path) {
    String path = p_path.is_empty() ? config_path : p_path;
    DirAccess::make_dir_recursive_absolute("user://.ai_engine");
    _save_json(path, config);
}

void ConfigManager::reset_to_defaults() {
    config.clear();
    _set_defaults();
}

void ConfigManager::_load_json(const String &path, Dictionary &out) {
    Error err;
    String content = FileAccess::get_file_as_string(path, &err);
    if (err != OK) return;
    Variant parsed;
    String err_str;
    int err_line;
    if (JSON::parse(content, parsed, err_str, err_line) == OK && parsed.get_type() == Variant::DICTIONARY) {
        Dictionary loaded = parsed;
        Array keys = loaded.keys();
        for (int i = 0; i < keys.size(); i++) out[keys[i]] = loaded[keys[i]];
    }
}

void ConfigManager::_save_json(const String &path, const Dictionary &data) {
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (f.is_valid()) f->store_string(JSON::stringify(data, "  "));
}

Variant    ConfigManager::get(const String &key, const Variant &def)   { return config.get(key, def); }
String     ConfigManager::get_string(const String &key, const String &def) { return config.get(key, def).operator String(); }
int        ConfigManager::get_int(const String &key, int def)          { return config.get(key, def).operator int(); }
float      ConfigManager::get_float(const String &key, float def)      { return config.get(key, (double)def).operator double(); }
bool       ConfigManager::get_bool(const String &key, bool def)        { return config.get(key, def).operator bool(); }
Dictionary ConfigManager::get_dict(const String &key, const Dictionary &def) {
    Variant v = config.get(key, def);
    return v.get_type() == Variant::DICTIONARY ? v.operator Dictionary() : def;
}
void ConfigManager::set(const String &key, const Variant &value)       { config[key] = value; emit_signal("config_changed", key, value); }
void ConfigManager::set_string(const String &key, const String &v)     { set(key, v); }
void ConfigManager::set_int(const String &key, int v)                  { set(key, v); }
void ConfigManager::set_float(const String &key, float v)              { set(key, (double)v); }
void ConfigManager::set_bool(const String &key, bool v)                { set(key, v); }

void ConfigManager::save_profile(const String &name) {
    profiles[name] = config.duplicate();
    _save_json(profiles_path, profiles);
}

void ConfigManager::load_profile(const String &name) {
    if (!profiles.has(name)) return;
    Dictionary p = profiles[name];
    Array keys = p.keys();
    for (int i = 0; i < keys.size(); i++) config[keys[i]] = p[keys[i]];
    emit_signal("profile_loaded", name);
}

Array ConfigManager::get_profiles() const {
    return profiles.keys();
}

void ConfigManager::delete_profile(const String &name) {
    profiles.erase(name);
    _save_json(profiles_path, profiles);
}

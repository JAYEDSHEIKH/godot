#pragma once
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"
#include "core/templates/hash_map.h"

// Lightweight in-memory key-value store for session state
class MemoryStore {
    HashMap<String, Variant> store;
    int max_entries = 10000;

public:
    void set(const String &key, const Variant &value);
    Variant get(const String &key, const Variant &default_val = Variant()) const;
    bool has(const String &key) const { return store.has(key); }
    void remove(const String &key);
    void clear();
    int size() const { return store.size(); }
    Array keys() const;
    Dictionary as_dict() const;
};

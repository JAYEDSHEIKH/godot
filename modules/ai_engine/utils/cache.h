#pragma once
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/variant/variant.h"

struct CacheEntry {
    Variant  value;
    int64_t  created_ms;
    int64_t  ttl_ms;
    int      access_count;
};

class AICache {
    HashMap<String, CacheEntry> store;
    int   max_entries = 500;
    int   default_ttl = 300000; // 5 minutes

public:
    void    set(const String &key, const Variant &value, int ttl_ms = -1);
    bool    get(const String &key, Variant &r_value);
    bool    has(const String &key) const;
    void    remove(const String &key);
    void    clear();
    void    evict_expired();
    int     size() const { return store.size(); }
};

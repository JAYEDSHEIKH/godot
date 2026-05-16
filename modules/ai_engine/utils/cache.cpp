#include "cache.h"
#include "core/os/time.h"

void AICache::set(const String &key, const Variant &value, int ttl_ms) {
    evict_expired();
    if ((int)store.size() >= max_entries && !store.has(key)) {
        // Remove oldest entry
        int64_t oldest = INT64_MAX;
        String oldest_key;
        for (const auto &pair : store) {
            if (pair.value.created_ms < oldest) { oldest = pair.value.created_ms; oldest_key = pair.key; }
        }
        if (!oldest_key.is_empty()) store.erase(oldest_key);
    }
    CacheEntry e;
    e.value        = value;
    e.created_ms   = Time::get_singleton()->get_ticks_msec();
    e.ttl_ms       = (ttl_ms > 0) ? ttl_ms : default_ttl;
    e.access_count = 0;
    store[key] = e;
}

bool AICache::get(const String &key, Variant &r_value) {
    if (!store.has(key)) return false;
    CacheEntry &e = store[key];
    int64_t now = Time::get_singleton()->get_ticks_msec();
    if (now - e.created_ms > e.ttl_ms) { store.erase(key); return false; }
    e.access_count++;
    r_value = e.value;
    return true;
}

bool AICache::has(const String &key) const {
    if (!store.has(key)) return false;
    const CacheEntry &e = store[key];
    int64_t now = Time::get_singleton()->get_ticks_msec();
    return (now - e.created_ms) <= e.ttl_ms;
}

void AICache::remove(const String &key) { store.erase(key); }
void AICache::clear() { store.clear(); }

void AICache::evict_expired() {
    int64_t now = Time::get_singleton()->get_ticks_msec();
    Vector<String> to_remove;
    for (const auto &pair : store) {
        if (now - pair.value.created_ms > pair.value.ttl_ms) to_remove.push_back(pair.key);
    }
    for (const String &k : to_remove) store.erase(k);
}

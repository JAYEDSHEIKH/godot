#include "memory_store.h"

void MemoryStore::set(const String &key, const Variant &value) {
    if ((int)store.size() >= max_entries && !store.has(key)) {
        // Remove first entry
        for (auto it = store.begin(); it != store.end(); ++it) {
            store.erase(it->key);
            break;
        }
    }
    store[key] = value;
}

Variant MemoryStore::get(const String &key, const Variant &default_val) const {
    if (store.has(key)) return store[key];
    return default_val;
}

void MemoryStore::remove(const String &key) {
    store.erase(key);
}

void MemoryStore::clear() {
    store.clear();
}

Array MemoryStore::keys() const {
    Array arr;
    for (const auto &pair : store) arr.push_back(pair.key);
    return arr;
}

Dictionary MemoryStore::as_dict() const {
    Dictionary d;
    for (const auto &pair : store) d[pair.key] = pair.value;
    return d;
}

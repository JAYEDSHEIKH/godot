#include "memory_manager.h"
#include "core/os/time.h"
#include "core/io/json.h"
#include "core/io/file_access.h"

void MemoryManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_fact", "category", "content"),      &MemoryManager::add_fact);
    ClassDB::bind_method(D_METHOD("remove_fact", "id"),                     &MemoryManager::remove_fact);
    ClassDB::bind_method(D_METHOD("clear_all"),                             &MemoryManager::clear_all);
    ClassDB::bind_method(D_METHOD("get_fact_count"),                        &MemoryManager::get_fact_count);
    ClassDB::bind_method(D_METHOD("save_to_disk"),                          &MemoryManager::save_to_disk);
    ClassDB::bind_method(D_METHOD("load_from_disk"),                        &MemoryManager::load_from_disk);
}

String MemoryManager::_generate_id() const {
    return String::num_int64(Time::get_singleton()->get_ticks_msec());
}

void MemoryManager::add_fact(const String &category, const String &content, int confidence) {
    // Check for duplicates
    for (const MemoryFact &f : facts) {
        if (f.category == category && f.content == content) return;
    }
    MemoryFact fact;
    fact.id            = _generate_id();
    fact.category      = category;
    fact.content       = content;
    fact.confidence    = confidence;
    fact.created_at_ms = Time::get_singleton()->get_ticks_msec();
    fact.access_count  = 0;
    facts.push_back(fact);
    _compact_if_needed();
    emit_signal("fact_added", fact.id, category);
}

void MemoryManager::remove_fact(const String &id) {
    for (int i = facts.size() - 1; i >= 0; i--) {
        if (facts[i].id == id) { facts.remove_at(i); return; }
    }
}

void MemoryManager::update_fact(const String &id, const String &new_content) {
    for (int i = 0; i < facts.size(); i++) {
        if (facts[i].id == id) {
            facts.write[i].content = new_content;
            return;
        }
    }
}

Vector<MemoryFact> MemoryManager::query_facts(const String &category, int limit) {
    Vector<MemoryFact> result;
    for (int i = facts.size() - 1; i >= 0 && result.size() < limit; i--) {
        if (facts[i].category == category) result.push_back(facts[i]);
    }
    return result;
}

Vector<MemoryFact> MemoryManager::search_facts(const String &query, int limit) {
    Vector<MemoryFact> result;
    String q = query.to_lower();
    for (int i = facts.size() - 1; i >= 0 && result.size() < limit; i--) {
        if (facts[i].content.to_lower().contains(q)) result.push_back(facts[i]);
    }
    return result;
}

Vector<MemoryFact> MemoryManager::get_recent_facts(int limit) {
    Vector<MemoryFact> result;
    for (int i = facts.size() - 1; i >= 0 && result.size() < limit; i--) {
        result.push_back(facts[i]);
    }
    return result;
}

void MemoryManager::save_to_disk() {
    Array arr;
    for (const MemoryFact &f : facts) {
        Dictionary d;
        d["id"]           = f.id;
        d["category"]     = f.category;
        d["content"]      = f.content;
        d["confidence"]   = f.confidence;
        d["created_at_ms"]= f.created_at_ms;
        arr.push_back(d);
    }
    Ref<FileAccess> file = FileAccess::open(storage_path, FileAccess::WRITE);
    if (file.is_valid()) {
        file->store_string(JSON::stringify(arr, "  "));
    }
}

bool MemoryManager::load_from_disk() {
    Error err;
    String content = FileAccess::get_file_as_string(storage_path, &err);
    if (err != OK) return false;
    Variant parsed;
    String err_str;
    int err_line;
    if (JSON::parse(content, parsed, err_str, err_line) != OK) return false;
    if (parsed.get_type() != Variant::ARRAY) return false;
    Array arr = parsed;
    facts.clear();
    for (int i = 0; i < arr.size(); i++) {
        Dictionary d = arr[i];
        MemoryFact f;
        f.id            = d.get("id", "").operator String();
        f.category      = d.get("category", "").operator String();
        f.content       = d.get("content", "").operator String();
        f.confidence    = d.get("confidence", 80).operator int();
        f.created_at_ms = d.get("created_at_ms", 0).operator int64_t();
        facts.push_back(f);
    }
    return true;
}

void MemoryManager::clear_all() {
    facts.clear();
}

void MemoryManager::clear_category(const String &category) {
    for (int i = facts.size() - 1; i >= 0; i--) {
        if (facts[i].category == category) facts.remove_at(i);
    }
}

void MemoryManager::extract_facts_from_response(const String &ai_response, const String &user_message) {
    // Simple heuristic fact extraction
    // In production: use a summarization prompt to extract key facts
    if (user_message.to_lower().contains("my name is")) {
        int idx = user_message.to_lower().find("my name is");
        String name = user_message.substr(idx + 11).strip_edges();
        if (!name.is_empty()) add_fact("developer", "Developer's name: " + name.split(" ")[0]);
    }
    if (user_message.to_lower().contains("making a") || user_message.to_lower().contains("building a")) {
        add_fact("project_type", "Project description noted from conversation");
    }
}

void MemoryManager::_compact_if_needed() {
    if (facts.size() <= max_facts) return;
    // Remove lowest confidence facts
    while (facts.size() > max_facts) {
        int worst_idx = 0;
        for (int i = 1; i < facts.size(); i++) {
            if (facts[i].confidence < facts[worst_idx].confidence) worst_idx = i;
        }
        facts.remove_at(worst_idx);
    }
}

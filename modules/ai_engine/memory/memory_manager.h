#pragma once
#include "core/object/ref_counted.h"
#include "modules/ai_engine/core/ai_types.h"

class MemoryManager : public RefCounted {
    GDCLASS(MemoryManager, RefCounted);

    Vector<MemoryFact> facts;
    String             storage_path = "user://.ai_engine/memory.json";
    int                max_facts    = 1000;

public:
    void add_fact(const String &category, const String &content, int confidence = 80);
    void remove_fact(const String &id);
    void update_fact(const String &id, const String &new_content);

    Vector<MemoryFact> query_facts(const String &category, int limit = 10);
    Vector<MemoryFact> search_facts(const String &query, int limit = 10);
    Vector<MemoryFact> get_recent_facts(int limit = 5);
    Vector<MemoryFact> get_all_facts() const { return facts; }

    void save_to_disk();
    bool load_from_disk();
    void clear_all();
    void clear_category(const String &category);

    int  get_fact_count() const { return facts.size(); }

    // Extract and store facts from an AI conversation
    void extract_facts_from_response(const String &ai_response, const String &user_message);

protected:
    static void _bind_methods();

private:
    String _generate_id() const;
    void   _compact_if_needed();
};

#pragma once
#include "core/object/ref_counted.h"
#include "modules/ai_engine/core/ai_types.h"

class VectorDB : public RefCounted {
    GDCLASS(VectorDB, RefCounted);

    Vector<CodeChunk> chunks;
    String            db_path = "user://.ai_engine/vector_db.bin";
    bool              is_dirty = false;

public:
    void insert(const CodeChunk &chunk);
    void insert_batch(const Vector<CodeChunk> &batch);

    void remove_by_file(const String &p_file_path);
    void remove_by_id(const String &p_chunk_id);

    Vector<SearchResult> search(const Vector<float> &p_embedding, int p_limit = 10);
    Vector<SearchResult> keyword_search(const String &p_query, int p_limit = 20);
    Vector<SearchResult> hybrid_search(const Vector<float> &p_embedding, const String &p_query, int p_limit = 10);

    void save_to_disk();
    bool load_from_disk();
    void clear();

    int get_chunk_count() const { return chunks.size(); }

    int get_file_count() const;
    Array get_indexed_files() const;

protected:
    static void _bind_methods();

private:
    float _cosine_similarity(const Vector<float> &a, const Vector<float> &b) const;
};

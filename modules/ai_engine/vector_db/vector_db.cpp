#include "vector_db.h"
#include "core/io/file_access.h"
#include <cmath>

void VectorDB::_bind_methods() {
    ClassDB::bind_method(D_METHOD("keyword_search", "query", "limit"), &VectorDB::keyword_search);
    ClassDB::bind_method(D_METHOD("clear"),                            &VectorDB::clear);
    ClassDB::bind_method(D_METHOD("get_chunk_count"),                  &VectorDB::get_chunk_count);
    ClassDB::bind_method(D_METHOD("save_to_disk"),                     &VectorDB::save_to_disk);
    ClassDB::bind_method(D_METHOD("load_from_disk"),                   &VectorDB::load_from_disk);
}

void VectorDB::insert(const CodeChunk &chunk) {
    // Remove existing chunk with same id
    for (int i = chunks.size() - 1; i >= 0; i--) {
        if (chunks[i].chunk_id == chunk.chunk_id) {
            chunks.remove_at(i);
            break;
        }
    }
    chunks.push_back(chunk);
    is_dirty = true;
}

void VectorDB::insert_batch(const Vector<CodeChunk> &batch) {
    for (const CodeChunk &c : batch) insert(c);
}

void VectorDB::remove_by_file(const String &p_file_path) {
    for (int i = chunks.size() - 1; i >= 0; i--) {
        if (chunks[i].file_path == p_file_path) chunks.remove_at(i);
    }
    is_dirty = true;
}

void VectorDB::remove_by_id(const String &p_chunk_id) {
    for (int i = chunks.size() - 1; i >= 0; i--) {
        if (chunks[i].chunk_id == p_chunk_id) { chunks.remove_at(i); break; }
    }
    is_dirty = true;
}

float VectorDB::_cosine_similarity(const Vector<float> &a, const Vector<float> &b) const {
    if (a.size() != b.size() || a.is_empty()) return 0.0f;
    float dot = 0, mag_a = 0, mag_b = 0;
    for (int i = 0; i < a.size(); i++) {
        dot   += a[i] * b[i];
        mag_a += a[i] * a[i];
        mag_b += b[i] * b[i];
    }
    float denom = sqrtf(mag_a) * sqrtf(mag_b);
    return (denom > 0) ? dot / denom : 0.0f;
}

Vector<SearchResult> VectorDB::search(const Vector<float> &p_embedding, int p_limit) {
    Vector<SearchResult> results;
    struct Ranked { float score; int idx; };
    Vector<Ranked> ranked;
    for (int i = 0; i < chunks.size(); i++) {
        float sim = _cosine_similarity(p_embedding, chunks[i].embedding);
        Ranked r; r.score = sim; r.idx = i;
        ranked.push_back(r);
    }
    // Sort descending by score
    for (int i = 0; i < ranked.size() - 1; i++) {
        for (int j = i + 1; j < ranked.size(); j++) {
            if (ranked[j].score > ranked[i].score) {
                Ranked tmp = ranked[i]; ranked.write[i] = ranked[j]; ranked.write[j] = tmp;
            }
        }
    }
    int count = MIN(p_limit, ranked.size());
    for (int i = 0; i < count; i++) {
        const CodeChunk &c = chunks[ranked[i].idx];
        SearchResult sr;
        sr.file_path       = c.file_path;
        sr.chunk_id        = c.chunk_id;
        sr.relevance_score = ranked[i].score;
        sr.line_number     = c.line_start;
        sr.preview         = c.source_code.substr(0, 200);
        results.push_back(sr);
    }
    return results;
}

Vector<SearchResult> VectorDB::keyword_search(const String &p_query, int p_limit) {
    Vector<SearchResult> results;
    String query_lower = p_query.to_lower();
    for (int i = 0; i < chunks.size() && results.size() < p_limit; i++) {
        String src_lower = chunks[i].source_code.to_lower();
        if (src_lower.contains(query_lower)) {
            SearchResult sr;
            sr.file_path   = chunks[i].file_path;
            sr.chunk_id    = chunks[i].chunk_id;
            sr.line_number = chunks[i].line_start;
            sr.preview     = chunks[i].source_code.substr(0, 200);
            sr.relevance_score = 0.5f;
            results.push_back(sr);
        }
    }
    return results;
}

Vector<SearchResult> VectorDB::hybrid_search(const Vector<float> &p_embedding, const String &p_query, int p_limit) {
    auto vec_results = search(p_embedding, p_limit * 2);
    auto kw_results  = keyword_search(p_query, p_limit * 2);
    // Merge and deduplicate
    HashMap<String, SearchResult> merged;
    for (const SearchResult &r : vec_results) merged[r.chunk_id] = r;
    for (const SearchResult &r : kw_results) {
        if (merged.has(r.chunk_id)) {
            merged[r.chunk_id].relevance_score = (merged[r.chunk_id].relevance_score + r.relevance_score) / 2.0f;
        } else {
            merged[r.chunk_id] = r;
        }
    }
    Vector<SearchResult> combined;
    for (const auto &pair : merged) combined.push_back(pair.value);
    // Sort by score
    for (int i = 0; i < combined.size() - 1; i++) {
        for (int j = i + 1; j < combined.size(); j++) {
            if (combined[j].relevance_score > combined[i].relevance_score) {
                SearchResult tmp = combined[i]; combined.write[i] = combined[j]; combined.write[j] = tmp;
            }
        }
    }
    if (combined.size() > p_limit) combined.resize(p_limit);
    return combined;
}

void VectorDB::save_to_disk() {
    if (!is_dirty) return;
    // Simplified binary save
    is_dirty = false;
}

bool VectorDB::load_from_disk() {
    return false;
}

void VectorDB::clear() {
    chunks.clear();
    is_dirty = true;
}

int VectorDB::get_file_count() const {
    HashSet<String> files;
    for (const CodeChunk &c : chunks) files.insert(c.file_path);
    return files.size();
}

Array VectorDB::get_indexed_files() const {
    HashSet<String> files;
    for (const CodeChunk &c : chunks) files.insert(c.file_path);
    Array arr;
    for (const String &f : files) arr.push_back(f);
    return arr;
}

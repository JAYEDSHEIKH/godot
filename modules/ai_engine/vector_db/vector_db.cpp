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
    return (denom > 0.0f) ? dot / denom : 0.0f;
}

Vector<SearchResult> VectorDB::search(const Vector<float> &p_embedding, int p_limit) {
    Vector<SearchResult> results;
    if (p_embedding.is_empty()) {
        // No real embedding — fall back to returning first N chunks with low score
        int count = MIN(p_limit, chunks.size());
        for (int i = 0; i < count; i++) {
            SearchResult sr;
            sr.file_path       = chunks[i].file_path;
            sr.chunk_id        = chunks[i].chunk_id;
            sr.relevance_score = 0.1f;
            sr.line_number     = chunks[i].line_start;
            sr.preview         = chunks[i].source_code.substr(0, 200);
            results.push_back(sr);
        }
        return results;
    }
    struct Ranked { float score; int idx; };
    Vector<Ranked> ranked;
    for (int i = 0; i < chunks.size(); i++) {
        float sim = _cosine_similarity(p_embedding, chunks[i].embedding);
        Ranked r; r.score = sim; r.idx = i;
        ranked.push_back(r);
    }
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

// Fix 17 — proper multi-term keyword scoring with sorted results
Vector<SearchResult> VectorDB::keyword_search(const String &p_query, int p_limit) {
    Vector<SearchResult> results;
    if (p_query.is_empty()) return results;

    String q = p_query.to_lower();
    Vector<String> terms = q.split(" ");
    // Remove empty tokens
    for (int i = terms.size() - 1; i >= 0; i--) {
        if (terms[i].is_empty()) terms.remove_at(i);
    }
    if (terms.is_empty()) return results;

    for (const CodeChunk &chunk : chunks) {
        String haystack = chunk.source_code.to_lower();
        // Also search in the file path
        String haystack_full = haystack + " " + chunk.file_path.to_lower();

        int matched_terms = 0;
        Array keyword_matches;
        for (const String &term : terms) {
            if (haystack_full.contains(term)) {
                matched_terms++;
                keyword_matches.push_back(term);
            }
        }
        if (matched_terms == 0) continue;

        SearchResult r;
        r.file_path       = chunk.file_path;
        r.chunk_id        = chunk.chunk_id;
        r.relevance_score = (float)matched_terms / (float)terms.size();
        r.preview         = chunk.source_code.substr(0, 300);
        r.line_number     = chunk.line_start;
        r.keyword_matches = keyword_matches;
        results.push_back(r);
    }

    // Sort descending by relevance score
    for (int i = 0; i < results.size() - 1; i++) {
        for (int j = i + 1; j < results.size(); j++) {
            if (results[j].relevance_score > results[i].relevance_score) {
                SearchResult tmp = results[i];
                results.write[i] = results[j];
                results.write[j] = tmp;
            }
        }
    }

    if (results.size() > p_limit) results.resize(p_limit);
    return results;
}

// Fix 17 — hybrid falls back to keyword_search until embeddings are available
Vector<SearchResult> VectorDB::hybrid_search(
    const Vector<float> &p_embedding, const String &p_query, int p_limit
) {
    if (p_embedding.is_empty()) {
        // No embedding generation yet — use keyword only
        return keyword_search(p_query, p_limit);
    }

    auto vec_results = search(p_embedding, p_limit * 2);
    auto kw_results  = keyword_search(p_query,  p_limit * 2);

    HashMap<String, SearchResult> merged;
    for (const SearchResult &r : vec_results) merged[r.chunk_id] = r;
    for (const SearchResult &r : kw_results) {
        if (merged.has(r.chunk_id)) {
            merged[r.chunk_id].relevance_score =
                (merged[r.chunk_id].relevance_score + r.relevance_score) / 2.0f;
            // Merge keyword_matches
            for (int i = 0; i < r.keyword_matches.size(); i++) {
                merged[r.chunk_id].keyword_matches.push_back(r.keyword_matches[i]);
            }
        } else {
            merged[r.chunk_id] = r;
        }
    }

    Vector<SearchResult> combined;
    for (const auto &kv : merged) combined.push_back(kv.value);

    for (int i = 0; i < combined.size() - 1; i++) {
        for (int j = i + 1; j < combined.size(); j++) {
            if (combined[j].relevance_score > combined[i].relevance_score) {
                SearchResult tmp = combined[i];
                combined.write[i] = combined[j];
                combined.write[j] = tmp;
            }
        }
    }

    if (combined.size() > p_limit) combined.resize(p_limit);
    return combined;
}

void VectorDB::save_to_disk() {
    if (!is_dirty) return;
    is_dirty = false;
    // Full binary serialization is complex; placeholder for now
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

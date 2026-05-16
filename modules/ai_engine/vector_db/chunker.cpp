#include "chunker.h"
#include "embeddings.h"
#include "core/crypto/crypto.h"

Vector<CodeChunk> Chunker::chunk_source_file(
    const String &p_file_path,
    const String &p_source,
    int p_chunk_lines,
    int p_overlap_lines
) {
    Vector<CodeChunk> chunks;
    Vector<String> lines = p_source.split("\n");
    int total_lines = lines.size();

    for (int start = 0; start < total_lines; start += (p_chunk_lines - p_overlap_lines)) {
        int end = MIN(start + p_chunk_lines, total_lines);
        String chunk_src;
        for (int i = start; i < end; i++) {
            chunk_src += lines[i] + "\n";
        }
        CodeChunk c;
        c.file_path   = p_file_path;
        c.chunk_id    = p_file_path + ":" + itos(start);
        c.line_start  = start + 1;
        c.line_end    = end;
        c.source_code = chunk_src;
        c.embedding   = Embeddings::generate_hash_embedding(chunk_src);
        chunks.push_back(c);
        if (end >= total_lines) break;
    }
    return chunks;
}

Vector<CodeChunk> Chunker::chunk_by_functions(
    const String &p_file_path,
    const String &p_source,
    const String &p_language
) {
    Vector<CodeChunk> chunks;
    Vector<String> lines = p_source.split("\n");
    int total = lines.size();

    Vector<int> func_starts;
    for (int i = 0; i < total; i++) {
        String line = lines[i].strip_edges();
        if ((p_language == "gdscript" && line.begins_with("func ")) ||
            (p_language == "nex" && line.begins_with("fn "))) {
            func_starts.push_back(i);
        }
    }

    if (func_starts.is_empty()) {
        // No functions — chunk by lines
        return chunk_source_file(p_file_path, p_source);
    }

    // Add sentinel
    func_starts.push_back(total);

    for (int fi = 0; fi < func_starts.size() - 1; fi++) {
        int start = func_starts[fi];
        int end   = func_starts[fi + 1];
        String chunk_src;
        for (int i = start; i < end; i++) chunk_src += lines[i] + "\n";

        CodeChunk c;
        c.file_path   = p_file_path;
        c.chunk_id    = p_file_path + ":func:" + itos(start);
        c.line_start  = start + 1;
        c.line_end    = end;
        c.source_code = chunk_src;
        c.embedding   = Embeddings::generate_hash_embedding(chunk_src);
        chunks.push_back(c);
    }
    return chunks;
}

#pragma once
#include "modules/ai_engine/core/ai_types.h"

class Chunker {
public:
    // Split source code into overlapping chunks for indexing
    static Vector<CodeChunk> chunk_source_file(
        const String &p_file_path,
        const String &p_source,
        int p_chunk_lines = 50,
        int p_overlap_lines = 10
    );

    // Chunk by function boundaries (smarter, for GDScript/NEX)
    static Vector<CodeChunk> chunk_by_functions(
        const String &p_file_path,
        const String &p_source,
        const String &p_language = "gdscript"
    );
};

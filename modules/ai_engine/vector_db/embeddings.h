#pragma once
#include "core/templates/vector.h"
#include "core/string/ustring.h"

class Embeddings {
public:
    // Generate a simple hash-based embedding (256 dimensions, float)
    // For production: call an embedding API (OpenAI text-embedding-3-small)
    static Vector<float> generate_hash_embedding(const String &p_text, int p_dims = 256);

    // Generate embedding via API call (async)
    static Vector<float> generate_api_embedding(const String &p_text, const String &p_api_key);

    static int embedding_dims() { return 256; }
};

#include "embeddings.h"
#include <cmath>

Vector<float> Embeddings::generate_hash_embedding(const String &p_text, int p_dims) {
    Vector<float> embedding;
    embedding.resize(p_dims);
    for (int i = 0; i < p_dims; i++) embedding.write[i] = 0.0f;

    // Simple TF-like embedding using character n-grams
    String text = p_text.to_lower();
    int len = text.length();
    for (int i = 0; i < len; i++) {
        // Unigram
        int idx = ((int)text[i] * 31 + i) % p_dims;
        if (idx < 0) idx += p_dims;
        embedding.write[idx] += 1.0f;

        // Bigram
        if (i + 1 < len) {
            int idx2 = ((int)text[i] * 131 + (int)text[i + 1] * 17) % p_dims;
            if (idx2 < 0) idx2 += p_dims;
            embedding.write[idx2] += 0.5f;
        }
    }

    // L2 normalize
    float mag = 0.0f;
    for (int i = 0; i < p_dims; i++) mag += embedding[i] * embedding[i];
    mag = sqrtf(mag);
    if (mag > 0.0f) {
        for (int i = 0; i < p_dims; i++) embedding.write[i] /= mag;
    }
    return embedding;
}

Vector<float> Embeddings::generate_api_embedding(const String &p_text, const String &p_api_key) {
    // API embedding would require async HTTP call
    // Fallback to hash-based embedding
    return generate_hash_embedding(p_text);
}

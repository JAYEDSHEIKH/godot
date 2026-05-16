#include "similarity.h"
#include <cmath>

float Similarity::cosine(const Vector<float> &a, const Vector<float> &b) {
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

float Similarity::dot_product(const Vector<float> &a, const Vector<float> &b) {
    if (a.size() != b.size()) return 0.0f;
    float dot = 0;
    for (int i = 0; i < a.size(); i++) dot += a[i] * b[i];
    return dot;
}

float Similarity::euclidean_distance(const Vector<float> &a, const Vector<float> &b) {
    if (a.size() != b.size()) return 1e9f;
    float sum = 0;
    for (int i = 0; i < a.size(); i++) {
        float d = a[i] - b[i];
        sum += d * d;
    }
    return sqrtf(sum);
}

float Similarity::bm25_score(const String &query, const String &document, float k1, float b) {
    Vector<String> query_terms = query.to_lower().split(" ");
    String doc_lower = document.to_lower();
    int dl = doc_lower.split(" ").size();
    float avgdl = 100.0f; // approximate
    float score = 0.0f;
    for (const String &term : query_terms) {
        int tf = 0;
        int pos = 0;
        while ((pos = doc_lower.find(term, pos)) != -1) { tf++; pos += term.length(); }
        if (tf == 0) continue;
        float idf = logf(2.0f); // simplified IDF
        float num = tf * (k1 + 1.0f);
        float den = tf + k1 * (1.0f - b + b * (float)dl / avgdl);
        score += idf * num / den;
    }
    return score;
}

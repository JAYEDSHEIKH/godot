#pragma once
#include "core/templates/vector.h"

class Similarity {
public:
    static float cosine(const Vector<float> &a, const Vector<float> &b);
    static float dot_product(const Vector<float> &a, const Vector<float> &b);
    static float euclidean_distance(const Vector<float> &a, const Vector<float> &b);
    static float bm25_score(const String &query, const String &document,
                            float k1 = 1.5f, float b = 0.75f);
};

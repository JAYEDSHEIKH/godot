#pragma once
#include "core/templates/hash_map.h"
#include "core/string/ustring.h"

class RateLimiter {
    struct Bucket {
        int     tokens;
        int     max_tokens;
        int     refill_per_second;
        int64_t last_refill_ms;
    };
    HashMap<String, Bucket> buckets;

public:
    void   configure(const String &key, int max_tokens, int refill_per_sec);
    bool   try_consume(const String &key, int tokens = 1);
    void   reset(const String &key);
    float  get_fill_ratio(const String &key) const;

private:
    void   _refill(Bucket &b) const;
};

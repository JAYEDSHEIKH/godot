#include "rate_limiter.h"
#include "core/os/time.h"

void RateLimiter::configure(const String &key, int max_tokens, int refill_per_sec) {
    Bucket b;
    b.tokens            = max_tokens;
    b.max_tokens        = max_tokens;
    b.refill_per_second = refill_per_sec;
    b.last_refill_ms    = Time::get_singleton()->get_ticks_msec();
    buckets[key] = b;
}

void RateLimiter::_refill(Bucket &b) const {
    int64_t now = Time::get_singleton()->get_ticks_msec();
    float elapsed_sec = (float)(now - b.last_refill_ms) / 1000.0f;
    int new_tokens = (int)(elapsed_sec * b.refill_per_second);
    if (new_tokens > 0) {
        b.tokens = MIN(b.tokens + new_tokens, b.max_tokens);
        b.last_refill_ms = now;
    }
}

bool RateLimiter::try_consume(const String &key, int tokens) {
    if (!buckets.has(key)) return true; // no limit configured
    Bucket &b = buckets[key];
    _refill(b);
    if (b.tokens >= tokens) { b.tokens -= tokens; return true; }
    return false;
}

void RateLimiter::reset(const String &key) {
    if (buckets.has(key)) buckets[key].tokens = buckets[key].max_tokens;
}

float RateLimiter::get_fill_ratio(const String &key) const {
    if (!buckets.has(key)) return 1.0f;
    const Bucket &b = buckets[key];
    return (float)b.tokens / (float)b.max_tokens;
}

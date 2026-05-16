#include "metrics.h"

void AIMetricsTracker::increment(const String &name, int64_t amount) {
    if (!counters.has(name)) { Counter c; counters[name] = c; }
    counters[name].value += amount;
}

void AIMetricsTracker::record_duration(const String &name, float duration_ms) {
    if (!timers.has(name)) { Timer t; timers[name] = t; }
    timers[name].total += duration_ms;
    timers[name].count++;
}

int64_t AIMetricsTracker::get_counter(const String &name) const {
    if (!counters.has(name)) return 0;
    return counters[name].value;
}

float AIMetricsTracker::get_avg_duration(const String &name) const {
    if (!timers.has(name) || timers[name].count == 0) return 0.0f;
    return timers[name].total / timers[name].count;
}

void AIMetricsTracker::reset() {
    counters.clear();
    timers.clear();
}

Dictionary AIMetricsTracker::to_dict() const {
    Dictionary d;
    for (const auto &pair : counters) d[pair.key] = pair.value.value;
    for (const auto &pair : timers) {
        if (pair.value.count > 0) d[pair.key + "_avg_ms"] = pair.value.total / pair.value.count;
    }
    return d;
}

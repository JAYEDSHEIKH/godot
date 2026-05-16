#pragma once
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/variant/dictionary.h"

class AIMetricsTracker {
    struct Counter { int64_t value = 0; };
    struct Timer   { float   total = 0; int count = 0; };

    HashMap<String, Counter> counters;
    HashMap<String, Timer>   timers;

public:
    void increment(const String &name, int64_t amount = 1);
    void record_duration(const String &name, float duration_ms);
    int64_t get_counter(const String &name) const;
    float   get_avg_duration(const String &name) const;
    void    reset();
    Dictionary to_dict() const;
};

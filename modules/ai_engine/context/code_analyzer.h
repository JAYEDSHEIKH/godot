#pragma once
#include "core/string/ustring.h"
#include "core/templates/vector.h"

struct FunctionSignature {
    String name;
    String return_type;
    Vector<String> params;
    int line;
};

class CodeAnalyzer {
public:
    static Vector<FunctionSignature> extract_functions(const String &p_source, const String &p_language);
    static Vector<String> extract_signal_names(const String &p_source);
    static Vector<String> extract_exports(const String &p_source);
    static String summarize_script(const String &p_source);
    static int count_tokens_approximate(const String &p_source);
};

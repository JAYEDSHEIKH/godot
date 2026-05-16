#include "code_analyzer.h"

Vector<FunctionSignature> CodeAnalyzer::extract_functions(const String &p_source, const String &p_language) {
    Vector<FunctionSignature> funcs;
    Vector<String> lines = p_source.split("\n");
    for (int i = 0; i < lines.size(); i++) {
        String line = lines[i].strip_edges();
        if (p_language == "gdscript" || p_language == "") {
            if (line.begins_with("func ")) {
                FunctionSignature f;
                int paren = line.find("(");
                f.name = (paren >= 0) ? line.substr(5, paren - 5).strip_edges() : line.substr(5);
                f.line = i + 1;
                funcs.push_back(f);
            }
        } else if (p_language == "nex") {
            if (line.begins_with("fn ")) {
                FunctionSignature f;
                int paren = line.find("(");
                int brace = line.find("{");
                int end = paren >= 0 ? paren : (brace >= 0 ? brace : line.length());
                f.name = line.substr(3, end - 3).strip_edges();
                f.line = i + 1;
                funcs.push_back(f);
            }
        }
    }
    return funcs;
}

Vector<String> CodeAnalyzer::extract_signal_names(const String &p_source) {
    Vector<String> signals;
    Vector<String> lines = p_source.split("\n");
    for (const String &line : lines) {
        String stripped = line.strip_edges();
        if (stripped.begins_with("signal ")) {
            int paren = stripped.find("(");
            String name = (paren >= 0) ? stripped.substr(7, paren - 7).strip_edges() : stripped.substr(7).strip_edges();
            signals.push_back(name);
        }
    }
    return signals;
}

Vector<String> CodeAnalyzer::extract_exports(const String &p_source) {
    Vector<String> exports;
    Vector<String> lines = p_source.split("\n");
    for (const String &line : lines) {
        String stripped = line.strip_edges();
        if (stripped.begins_with("@export") || stripped.begins_with("@export_var") ||
            (stripped.begins_with("var ") && stripped.find("@export") >= 0)) {
            exports.push_back(stripped);
        }
    }
    return exports;
}

String CodeAnalyzer::summarize_script(const String &p_source) {
    auto funcs = extract_functions(p_source, "gdscript");
    auto sigs = extract_signal_names(p_source);
    String summary = vformat("Functions: %d, Signals: %d, Lines: %d",
        funcs.size(), sigs.size(), p_source.split("\n").size());
    return summary;
}

int CodeAnalyzer::count_tokens_approximate(const String &p_source) {
    return p_source.length() / 4 + 1;
}

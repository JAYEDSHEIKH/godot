#include "nex_language.h"
#include "nex_script.h"
#include "modules/nexscript/frontend/nex_tokenizer.h"
#include "modules/nexscript/frontend/nex_parser.h"
#include "modules/nexscript/frontend/nex_analyzer.h"
#include "core/io/file_access.h"

NexLanguage *NexLanguage::singleton = nullptr;

NexLanguage::NexLanguage() {
    singleton = this;
}

NexLanguage::~NexLanguage() {
    singleton = nullptr;
}

void NexLanguage::init() {
}

void NexLanguage::finish() {
}

void NexLanguage::get_recognized_extensions(List<String> *p_extensions) const {
    p_extensions->push_back("nex");
}

void NexLanguage::get_reserved_words(List<String> *p_words) const {
    static const char *words[] = {
        "fn", "ret", "var", "const",
        "if", "elif", "else",
        "for", "while", "loop", "break", "continue",
        "in", "by",
        "match",
        "struct", "impl", "enum",
        "use", "pub",
        "async", "await",
        "and", "or", "not",
        "some", "none", "ok", "err",
        "true", "false",
        "extends", "class_name", "signal", "emit",
        "autoload", "as", "is",
        "int", "int8", "int16", "int32", "int64",
        "float", "float32", "float64",
        "bool", "str", "char", "void",
        "Option", "Result",
        "Vector2", "Vector3", "Color", "Rect2", "Transform2D",
        nullptr
    };
    for (int i = 0; words[i]; i++) {
        p_words->push_back(words[i]);
    }
}

void NexLanguage::get_comment_delimiters(List<String> *p_delimiters) const {
    p_delimiters->push_back("//");
    p_delimiters->push_back("/* */");
}

void NexLanguage::get_string_delimiters(List<String> *p_delimiters) const {
    p_delimiters->push_back("\" \"");
}

// Fix 9a — Global constants (needed for singletons / autoloads)
void NexLanguage::add_global_constant(const StringName &p_variable, const Variant &p_value) {
    global_constants[p_variable] = p_value;
}

void NexLanguage::add_named_global_constant(const StringName &p_name, const Variant &p_value) {
    named_global_constants[p_name] = p_value;
}

void NexLanguage::remove_named_global_constant(const StringName &p_name) {
    named_global_constants.erase(p_name);
}

const Variant *NexLanguage::get_global_constant(const StringName &p_name) const {
    if (named_global_constants.has(p_name)) return &named_global_constants[p_name];
    if (global_constants.has(p_name))       return &global_constants[p_name];
    return nullptr;
}

// Fix 9b — get_global_class_name (needed for class_name declarations)
void NexLanguage::get_global_class_name(const String &p_path, String *r_name, String *r_base_type) const {
    Error err;
    String source = FileAccess::get_file_as_string(p_path, &err);
    if (err != OK) return;

    NexTokenizer tk;
    tk.set_source(source);
    Vector<NexToken> tokens = tk.tokenize();

    for (int i = 0; i + 1 < tokens.size(); i++) {
        if (tokens[i].kind == NexTokenKind::IDENT && tokens[i].value == "class_name") {
            if (r_name && i + 1 < tokens.size()) {
                *r_name = tokens[i + 1].value;
            }
        }
        if (tokens[i].kind == NexTokenKind::KW_EXTENDS && i + 1 < tokens.size()) {
            if (r_base_type) {
                *r_base_type = tokens[i + 1].value;
            }
        }
    }
}

// Fix 10 — Corrected make_template with valid NEX syntax (no semicolons)
Ref<Script> NexLanguage::make_template(
    const String &p_template,
    const String &p_class_name,
    const String &p_base_class_name
) const {
    Ref<NexScript> script;
    script.instantiate();

    String base = p_base_class_name.is_empty() ? "Node" : p_base_class_name;
    String src;
    src += "extends " + base + "\n";
    src += "\n";
    src += "fn _ready {\n";
    src += "\t\n";
    src += "}\n";
    src += "\n";
    src += "fn _process(delta: float) {\n";
    src += "\t\n";
    src += "}\n";

    script->set_source_code(src);
    return script;
}

bool NexLanguage::validate(
    const String &p_script,
    const String &p_path,
    List<String> *r_functions,
    List<ScriptLanguage::ScriptError> *r_errors,
    List<ScriptLanguage::Warning> *r_warnings,
    HashSet<int> *r_safe_lines
) const {
    NexTokenizer tokenizer;
    tokenizer.set_source(p_script);
    Vector<NexToken> tokens = tokenizer.tokenize();

    NexParser parser;
    NexNode *ast = parser.parse(tokens);

    bool valid = true;
    for (const String &err : parser.get_errors()) {
        ScriptLanguage::ScriptError se;
        se.path = p_path;
        se.line = 0;
        se.message = err;
        if (r_errors) r_errors->push_back(se);
        valid = false;
    }

    if (ast && valid) {
        NexAnalyzer analyzer;
        bool ok = analyzer.analyze(ast);
        if (!ok) {
            for (const String &err : analyzer.get_errors()) {
                ScriptLanguage::ScriptError se;
                se.path = p_path;
                se.line = 0;
                se.message = err;
                if (r_errors) r_errors->push_back(se);
            }
            valid = false;
        }
        if (r_functions) {
            for (NexNode *child : ast->children) {
                if (child && child->kind == NexNode::FN_DEF) {
                    r_functions->push_back(child->str_val);
                }
            }
        }
    }

    if (ast) delete ast;
    return valid;
}

Script *NexLanguage::create_script() const {
    return memnew(NexScript);
}

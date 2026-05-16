#include "nex_script.h"
#include "nex_instance.h"
#include "nex_language.h"
#include "modules/nexscript/frontend/nex_tokenizer.h"
#include "modules/nexscript/frontend/nex_parser.h"
#include "modules/nexscript/frontend/nex_analyzer.h"
#include "modules/nexscript/optimizer/nex_optimizer.h"
#include "modules/nexscript/ir/nex_ir_gen.h"

void NexScript::_bind_methods() {}

NexScript::~NexScript() {
    for (auto &pair : functions) {
        delete pair.value;
    }
}

Error NexScript::compile() {
    for (auto &pair : functions) delete pair.value;
    functions.clear();
    exports.clear();
    signals_list.clear();
    is_compiled = false;

    NexTokenizer tokenizer;
    tokenizer.set_source(source_code);
    Vector<NexToken> tokens = tokenizer.tokenize();

    NexParser parser;
    NexNode *ast = parser.parse(tokens);
    if (!ast) return ERR_PARSE_ERROR;

    if (!parser.get_errors().is_empty()) {
        delete ast;
        return ERR_PARSE_ERROR;
    }

    NexAnalyzer analyzer;
    if (!analyzer.analyze(ast)) {
        delete ast;
        return ERR_COMPILATION_FAILED;
    }

    NexOptimizer optimizer;
    optimizer.run(ast);

    structs = analyzer.get_structs();

    NexIRGen ir_gen;
    ir_gen.set_structs(structs);

    // Collect exports and signals from AST
    for (NexNode *child : ast->children) {
        if (!child) continue;

        if (child->kind == NexNode::EXTENDS_STMT) {
            base_class = StringName(child->str_val);
        } else if (child->kind == NexNode::FN_DEF) {
            NexFunction *fn = ir_gen.generate_function(child);
            if (fn) functions[fn->name] = fn;
        } else if (child->kind == NexNode::SIGNAL_DEF) {
            SignalDecl sd;
            sd.name = StringName(child->str_val);
            MethodInfo mi;
            mi.name = sd.name;
            for (NexNode *p : child->children) {
                PropertyInfo pi;
                pi.name = p->str_val;
                mi.arguments.push_back(pi);
            }
            sd.method_info = mi;
            signals_list.push_back(sd);
        } else if (child->kind == NexNode::VAR_DECL) {
            if (!child->annotation_name.is_empty() && child->annotation_name.begins_with("export")) {
                ExportVar ev;
                ev.name = StringName(child->str_val);
                ev.type = child->type_annotation;
                if (!child->children.is_empty()) {
                    NexNode *val = child->children[0];
                    if (val->kind == NexNode::INT_LITERAL) ev.default_value = Variant(val->int_val);
                    else if (val->kind == NexNode::FLOAT_LITERAL) ev.default_value = Variant(val->float_val);
                    else if (val->kind == NexNode::STR_LITERAL) ev.default_value = Variant(val->str_val);
                    else if (val->kind == NexNode::BOOL_LITERAL) ev.default_value = Variant(val->bool_val);
                }
                PropertyInfo pi;
                pi.name = ev.name;
                switch (ev.type.base) {
                    case NexBaseType::INT64:   pi.type = Variant::INT;   break;
                    case NexBaseType::FLOAT64: pi.type = Variant::FLOAT; break;
                    case NexBaseType::STR:     pi.type = Variant::STRING; break;
                    case NexBaseType::BOOL:    pi.type = Variant::BOOL;  break;
                    default: pi.type = Variant::NIL; break;
                }
                ev.property_info = pi;
                exports.push_back(ev);
            }
        }
    }

    delete ast;
    is_compiled = true;
    return OK;
}

bool NexScript::can_instantiate() const {
    return is_compiled && !base_class.is_empty();
}

ScriptInstance *NexScript::instance_create(Object *p_this) {
    if (!is_compiled) {
        if (compile() != OK) return nullptr;
    }
    NexScriptInstance *instance = memnew(NexScriptInstance(p_this, Ref<NexScript>(this)));
    return instance;
}

void NexScript::get_script_property_list(List<PropertyInfo> *p_list) const {
    for (const ExportVar &ev : exports) {
        p_list->push_back(ev.property_info);
    }
}

void NexScript::get_script_signal_list(List<MethodInfo> *p_list) const {
    for (const SignalDecl &sd : signals_list) {
        p_list->push_back(sd.method_info);
    }
}

void NexScript::get_script_method_list(List<MethodInfo> *p_list) const {
    for (const auto &pair : functions) {
        MethodInfo mi;
        mi.name = pair.key;
        p_list->push_back(mi);
    }
}

bool NexScript::has_method(const StringName &p_method) const {
    return functions.has(p_method);
}

MethodInfo NexScript::get_method_info(const StringName &p_method) const {
    MethodInfo mi;
    if (functions.has(p_method)) mi.name = p_method;
    return mi;
}

Error NexScript::reload(bool p_keep_state) {
    return compile();
}

ScriptLanguage *NexScript::get_language() const {
    return NexLanguage::get_singleton();
}

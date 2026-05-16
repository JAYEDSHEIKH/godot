#pragma once
#include "nex_type.h"
#include "core/templates/vector.h"
#include "core/variant/variant.h"

struct NexNode {
    enum Kind {
        PROGRAM,
        EXTENDS_STMT,
        USE_STMT,
        AUTOLOAD_STMT,
        SIGNAL_DEF,
        FN_DEF,
        STRUCT_DEF,
        IMPL_BLOCK,
        ENUM_DEF,
        CONST_DEF,
        VAR_DECL,
        ASSIGN_STMT,
        ASSIGN_OP_STMT,
        EMIT_STMT,
        RETURN_STMT,
        IF_STMT,
        FOR_RANGE_STMT,
        FOR_EACH_STMT,
        WHILE_STMT,
        LOOP_STMT,
        BREAK_STMT,
        CONTINUE_STMT,
        MATCH_STMT,
        MATCH_ARM,
        BLOCK,
        CALL_EXPR,
        METHOD_CALL_EXPR,
        INDEX_EXPR,
        FIELD_EXPR,
        BINARY_EXPR,
        UNARY_EXPR,
        RANGE_EXPR,
        INLINE_IF_EXPR,
        CLOSURE_EXPR,
        AWAIT_EXPR,
        QUESTION_EXPR,
        AS_CAST_EXPR,
        IS_CHECK_EXPR,
        TUPLE_EXPR,
        ARRAY_LITERAL,
        MAP_LITERAL,
        STRUCT_LITERAL,
        OPTION_SOME_EXPR,
        OPTION_NONE_EXPR,
        RESULT_OK_EXPR,
        RESULT_ERR_EXPR,
        DOLLAR_PATH_EXPR,
        IDENTIFIER,
        INT_LITERAL,
        FLOAT_LITERAL,
        STR_LITERAL,
        BOOL_LITERAL,
        CHAR_LITERAL,
        ANNOTATION,
        FN_PARAM,
        STRUCT_FIELD,
        ENUM_VARIANT,
    };

    Kind              kind;
    NexType           resolved_type;
    int               line = 0;

    Vector<NexNode *> children;
    String            str_val;
    int64_t           int_val    = 0;
    double            float_val  = 0.0;
    bool              bool_val   = false;
    bool              is_mutable = false;
    bool              is_async   = false;
    bool              is_pub     = false;
    String            op;
    String            annotation_name;
    Vector<Variant>   annotation_args;
    NexType           type_annotation;

    NexNode() : kind(PROGRAM), line(0) {}
    NexNode(Kind k, int ln) : kind(k), line(ln) {}

    ~NexNode() {
        for (NexNode *child : children) {
            if (child) delete child;
        }
    }
};

#pragma once
#include "modules/nexscript/frontend/nex_type.h"
#include "core/string/string_name.h"
#include "core/templates/vector.h"

enum class NexOp : uint16_t {
    LOAD_INT_CONST,
    LOAD_FLOAT_CONST,
    LOAD_BOOL_CONST,
    LOAD_STR_CONST,
    LOAD_NULL,

    LOAD_LOCAL,
    STORE_LOCAL,

    LOAD_FIELD_INT,
    LOAD_FIELD_FLOAT,
    LOAD_FIELD_VEC2,
    LOAD_FIELD_VEC3,
    LOAD_FIELD_STR,
    LOAD_FIELD_OBJ,
    STORE_FIELD_INT,
    STORE_FIELD_FLOAT,
    STORE_FIELD_VEC2,
    STORE_FIELD_OBJ,

    LOAD_INDEX,
    STORE_INDEX,

    INT_ADD, INT_SUB, INT_MUL, INT_DIV, INT_MOD, INT_POW, INT_NEG,
    INT_EQ, INT_NEQ, INT_LT, INT_GT, INT_LTE, INT_GTE,

    FLOAT_ADD, FLOAT_SUB, FLOAT_MUL, FLOAT_DIV, FLOAT_POW, FLOAT_NEG,
    FLOAT_EQ, FLOAT_NEQ, FLOAT_LT, FLOAT_GT, FLOAT_LTE, FLOAT_GTE,

    INT_TO_FLOAT,
    FLOAT_TO_INT,

    BOOL_AND, BOOL_OR, BOOL_NOT,

    STR_CONCAT, STR_EQ, STR_NEQ,

    VEC2_ADD, VEC2_SUB, VEC2_MUL_SCALAR, VEC2_DIV_SCALAR,
    VEC2_DOT, VEC2_LENGTH, VEC2_NORMALIZED,

    JUMP,
    JUMP_IF_FALSE,
    JUMP_IF_TRUE,
    JUMP_TABLE,
    RETURN,
    RETURN_VOID,

    CALL_DIRECT,
    CALL_FN_PTR,
    CALL_GODOT_METHOD,
    CALL_BUILTIN,

    STRUCT_ALLOC,
    STRUCT_COPY,

    ARRAY_NEW,
    ARRAY_PUSH, ARRAY_POP,
    ARRAY_LEN,
    ARRAY_GET_INT,
    ARRAY_GET_FLOAT,
    ARRAY_GET_OBJ,

    OPTION_SOME,
    OPTION_NONE,
    OPTION_IS_SOME,
    OPTION_UNWRAP,
    OPTION_UNWRAP_OR,

    RESULT_OK,
    RESULT_ERR,
    RESULT_IS_OK,
    RESULT_UNWRAP_OK,
    RESULT_UNWRAP_ERR,
    RESULT_PROPAGATE,

    GET_NODE,
    EMIT_SIGNAL,
    AWAIT_SIGNAL,
    TO_VARIANT,
    FROM_VARIANT,

    NOP,
    END,
};

struct NexIRInstr {
    NexOp    op        = NexOp::NOP;
    int      dst       = -1;
    int      src_a     = -1;
    int      src_b     = -1;
    int64_t  imm_int   = 0;
    double   imm_float = 0.0;
    uint32_t imm_str   = 0;
    int      label     = -1;
    int      field_offset = 0;
    StringName call_name;
    void    *fn_ptr    = nullptr;
    Vector<int> jump_table;
    NexType  slot_type;
};

struct NexFunction {
    StringName          name;
    Vector<NexIRInstr>  instructions;
    int                 slot_count   = 0;
    Vector<NexType>     slot_types;
    Vector<String>      string_pool;
    int                 param_count  = 0;
    NexType             return_type;
    bool                is_async     = false;
    bool                is_inline    = false;
};

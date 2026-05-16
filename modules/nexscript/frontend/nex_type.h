#pragma once
#include "core/string/string_name.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"

enum class NexBaseType {
    UNKNOWN,
    VOID,
    BOOL,
    INT8, INT16, INT32, INT64,
    FLOAT32, FLOAT64,
    STR,
    CHAR,
    VECTOR2, VECTOR3, VECTOR4,
    COLOR, RECT2, TRANSFORM2D, QUATERNION,
    STRUCT,
    ENUM,
    ARRAY,
    MAP,
    TUPLE,
    OPTION,
    RESULT,
    FN,
    OBJECT,
    VARIANT,
};

struct NexType {
    NexBaseType base = NexBaseType::UNKNOWN;

    StringName name;

    NexType *elem_type   = nullptr;
    NexType *key_type    = nullptr;
    NexType *value_type  = nullptr;

    Vector<NexType> tuple_types;

    NexType *option_inner = nullptr;

    NexType *result_ok  = nullptr;
    NexType *result_err = nullptr;

    Vector<NexType> fn_params;
    NexType *fn_ret = nullptr;

    bool is_mutable = false;

    int size_bytes() const {
        switch (base) {
            case NexBaseType::BOOL:    return 1;
            case NexBaseType::INT8:    return 1;
            case NexBaseType::INT16:   return 2;
            case NexBaseType::INT32:   return 4;
            case NexBaseType::INT64:   return 8;
            case NexBaseType::FLOAT32: return 4;
            case NexBaseType::FLOAT64: return 8;
            case NexBaseType::CHAR:    return 4;
            case NexBaseType::VECTOR2: return 8;
            case NexBaseType::VECTOR3: return 12;
            case NexBaseType::VECTOR4: return 16;
            case NexBaseType::COLOR:   return 16;
            case NexBaseType::RECT2:   return 16;
            default: return 8;
        }
    }

    bool is_integer() const {
        return base == NexBaseType::INT8  || base == NexBaseType::INT16 ||
               base == NexBaseType::INT32 || base == NexBaseType::INT64;
    }

    bool is_float() const {
        return base == NexBaseType::FLOAT32 || base == NexBaseType::FLOAT64;
    }

    bool is_numeric() const { return is_integer() || is_float(); }

    bool is_primitive() const {
        return is_integer() || is_float() || base == NexBaseType::BOOL ||
               base == NexBaseType::CHAR;
    }

    bool is_godot_value_type() const {
        return base == NexBaseType::VECTOR2   || base == NexBaseType::VECTOR3   ||
               base == NexBaseType::VECTOR4   || base == NexBaseType::COLOR     ||
               base == NexBaseType::RECT2     || base == NexBaseType::TRANSFORM2D ||
               base == NexBaseType::QUATERNION;
    }

    bool is_reference_type() const {
        return base == NexBaseType::OBJECT;
    }

    bool operator==(const NexType &o) const {
        return base == o.base && name == o.name;
    }

    String to_string() const {
        switch (base) {
            case NexBaseType::VOID:    return "void";
            case NexBaseType::BOOL:    return "bool";
            case NexBaseType::INT8:    return "int8";
            case NexBaseType::INT16:   return "int16";
            case NexBaseType::INT32:   return "int32";
            case NexBaseType::INT64:   return "int";
            case NexBaseType::FLOAT32: return "float32";
            case NexBaseType::FLOAT64: return "float";
            case NexBaseType::STR:     return "str";
            case NexBaseType::CHAR:    return "char";
            case NexBaseType::VECTOR2: return "Vector2";
            case NexBaseType::VECTOR3: return "Vector3";
            case NexBaseType::COLOR:   return "Color";
            case NexBaseType::STRUCT:  return String(name);
            case NexBaseType::ENUM:    return String(name);
            case NexBaseType::OBJECT:  return String(name);
            default: return "unknown";
        }
    }

    static NexType make_int()   { NexType t; t.base = NexBaseType::INT64;   return t; }
    static NexType make_float() { NexType t; t.base = NexBaseType::FLOAT64; return t; }
    static NexType make_bool()  { NexType t; t.base = NexBaseType::BOOL;    return t; }
    static NexType make_str()   { NexType t; t.base = NexBaseType::STR;     return t; }
    static NexType make_void()  { NexType t; t.base = NexBaseType::VOID;    return t; }
    static NexType make_char()  { NexType t; t.base = NexBaseType::CHAR;    return t; }
    static NexType make_vec2()  { NexType t; t.base = NexBaseType::VECTOR2; return t; }
    static NexType make_vec3()  { NexType t; t.base = NexBaseType::VECTOR3; return t; }
    static NexType make_color() { NexType t; t.base = NexBaseType::COLOR;   return t; }
    static NexType make_variant(){ NexType t; t.base = NexBaseType::VARIANT; return t; }

    static NexType make_option(NexType inner) {
        NexType t;
        t.base = NexBaseType::OPTION;
        t.option_inner = new NexType(inner);
        return t;
    }

    static NexType make_result(NexType ok, NexType err) {
        NexType t;
        t.base = NexBaseType::RESULT;
        t.result_ok  = new NexType(ok);
        t.result_err = new NexType(err);
        return t;
    }

    static NexType make_array(NexType elem) {
        NexType t;
        t.base = NexBaseType::ARRAY;
        t.elem_type = new NexType(elem);
        return t;
    }

    static NexType make_struct(const StringName &n) {
        NexType t;
        t.base = NexBaseType::STRUCT;
        t.name = n;
        return t;
    }

    static NexType make_object(const StringName &n) {
        NexType t;
        t.base = NexBaseType::OBJECT;
        t.name = n;
        return t;
    }
};

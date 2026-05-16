#pragma once
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/math/color.h"
#include "core/variant/variant.h"
#include "modules/nexscript/frontend/nex_type.h"

enum class SlotKind : uint8_t {
    EMPTY,
    BOOL,
    INT8, INT16, INT32, INT64,
    FLOAT32, FLOAT64,
    STR,
    CHAR,
    VECTOR2,
    VECTOR3,
    COLOR,
    OBJECT,
    STRUCT_PTR,
    VARIANT,
};

union SlotData {
    bool     b;
    int8_t   i8;
    int16_t  i16;
    int32_t  i32;
    int64_t  i64;
    float    f32;
    double   f64;
    uint32_t ch;
    Vector2  v2;
    Vector3  v3;
    Color    col;
    void    *ptr;
};

struct TypedSlot {
    SlotKind kind = SlotKind::EMPTY;
    SlotData data = {};

    inline int64_t  as_int()    const { return data.i64; }
    inline double   as_float()  const { return data.f64; }
    inline bool     as_bool()   const { return data.b; }
    inline Vector2  as_vec2()   const { return data.v2; }
    inline Vector3  as_vec3()   const { return data.v3; }
    inline Color    as_color()  const { return data.col; }
    inline String  *as_str()    const { return (String *)data.ptr; }
    inline Object  *as_object() const { return (Object *)data.ptr; }
    inline void    *as_struct() const { return data.ptr; }

    inline void set_int(int64_t v)   { kind = SlotKind::INT64;   data.i64 = v; }
    inline void set_float(double v)  { kind = SlotKind::FLOAT64; data.f64 = v; }
    inline void set_bool(bool v)     { kind = SlotKind::BOOL;    data.b   = v; }
    inline void set_vec2(Vector2 v)  { kind = SlotKind::VECTOR2; data.v2  = v; }
    inline void set_vec3(Vector3 v)  { kind = SlotKind::VECTOR3; data.v3  = v; }
    inline void set_color(Color v)   { kind = SlotKind::COLOR;   data.col = v; }
    inline void set_char(uint32_t v) { kind = SlotKind::CHAR;    data.ch  = v; }
    inline void set_str(String *s)   { kind = SlotKind::STR;     data.ptr = s; }
    inline void set_object(Object *o){ kind = SlotKind::OBJECT;  data.ptr = o; }
    inline void set_struct(void *p)  { kind = SlotKind::STRUCT_PTR; data.ptr = p; }

    Variant to_variant() const {
        switch (kind) {
            case SlotKind::BOOL:    return Variant(data.b);
            case SlotKind::INT8:    return Variant((int64_t)data.i8);
            case SlotKind::INT16:   return Variant((int64_t)data.i16);
            case SlotKind::INT32:   return Variant((int64_t)data.i32);
            case SlotKind::INT64:   return Variant(data.i64);
            case SlotKind::FLOAT32: return Variant((double)data.f32);
            case SlotKind::FLOAT64: return Variant(data.f64);
            case SlotKind::STR:     return data.ptr ? Variant(*((String *)data.ptr)) : Variant(String());
            case SlotKind::CHAR:    return Variant((int64_t)data.ch);
            case SlotKind::VECTOR2: return Variant(data.v2);
            case SlotKind::VECTOR3: return Variant(data.v3);
            case SlotKind::COLOR:   return Variant(data.col);
            case SlotKind::OBJECT:  return data.ptr ? Variant((Object *)data.ptr) : Variant();
            default:                return Variant();
        }
    }

    void from_variant(const Variant &v, const NexType &expected_type) {
        switch (expected_type.base) {
            case NexBaseType::BOOL:    set_bool((bool)v);               break;
            case NexBaseType::INT8:
            case NexBaseType::INT16:
            case NexBaseType::INT32:
            case NexBaseType::INT64:   set_int((int64_t)v);             break;
            case NexBaseType::FLOAT32:
            case NexBaseType::FLOAT64: set_float((double)v);            break;
            case NexBaseType::STR: {
                String *s = new String((String)v);
                set_str(s);
            } break;
            case NexBaseType::VECTOR2: set_vec2((Vector2)v);            break;
            case NexBaseType::VECTOR3: set_vec3((Vector3)v);            break;
            case NexBaseType::COLOR:   set_color((Color)v);             break;
            case NexBaseType::OBJECT:  set_object((Object *)v);         break;
            default: kind = SlotKind::VARIANT; break;
        }
    }
};

struct TypedStack {
    TypedSlot *slots = nullptr;
    int        count = 0;

    void allocate(int slot_count) {
        slots = new TypedSlot[slot_count];
        count = slot_count;
    }

    void free_slots() {
        if (slots) {
            delete[] slots;
            slots = nullptr;
            count = 0;
        }
    }

    inline TypedSlot &operator[](int i) { return slots[i]; }
    inline const TypedSlot &operator[](int i) const { return slots[i]; }
};

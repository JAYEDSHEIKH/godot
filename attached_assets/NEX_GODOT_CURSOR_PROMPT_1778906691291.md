# NEX Language — Godot Module Build Prompt
> Paste this into Cursor or Windsurf with the Godot source tree open.
> Goal: implement NEX as a faster-than-GDScript scripting module for Godot 4.

---

## MISSION

Build the NEX scripting language as a Godot module in `modules/nexscript/`.

NEX is NOT a GDScript clone with different syntax. It is a **statically-typed,
typed-IR compiled language** whose VM operates on typed slots, not Variants.
This is what makes it faster than GDScript.

**Performance target:** NEX numeric code must run at minimum 2x faster than
equivalent GDScript. Struct field access must be a direct byte offset, not a
HashMap lookup. Integer arithmetic must emit i64 CPU instructions, not
Variant::operator+.

---

## ARCHITECTURE OVERVIEW

```
.nex source file
      │
      ▼
 [1] Tokenizer          → Token stream
      │
      ▼
 [2] Parser             → Untyped AST
      │
      ▼
 [3] Type Analyzer      → Typed AST (every node has a resolved NexType)
      │
      ▼
 [4] Optimizer          → Optimized Typed AST
      │   • constant folding
      │   • dead code elimination
      │   • inline expansion
      │   • immutability propagation
      ▼
 [5] IR Generator       → NexIR (typed intermediate representation)
      │
      ▼
 [6] Typed VM           → Executes NexIR using typed slots, not Variants
```

The key difference from GDScript:

- GDScript VM: `Vector<Variant> stack` — every value boxed, tagged, dispatched
- NEX Typed VM: `TypedSlotArray stack` — each slot is a typed union, operations
  directly read the right field without tag checking

---

## STEP 1 — FOLDER STRUCTURE

Create all files before implementing any:

```
modules/nexscript/
├── config.py
├── SCsub
├── register_types.h
├── register_types.cpp
│
├── frontend/
│   ├── nex_token.h             ← Token types and Token struct
│   ├── nex_tokenizer.h
│   ├── nex_tokenizer.cpp       ← Converts source text to Token stream
│   ├── nex_ast.h               ← All AST node types
│   ├── nex_parser.h
│   ├── nex_parser.cpp          ← Builds untyped AST from tokens
│   ├── nex_type.h              ← NexType system
│   ├── nex_analyzer.h
│   └── nex_analyzer.cpp        ← Type inference, mutability, validation
│
├── optimizer/
│   ├── nex_optimizer.h
│   └── nex_optimizer.cpp       ← Runs optimization passes on typed AST
│
├── ir/
│   ├── nex_ir.h                ← NexIR instruction set
│   └── nex_ir_gen.cpp          ← Generates NexIR from typed+optimized AST
│
├── vm/
│   ├── nex_typed_slot.h        ← TypedSlot union — the core of the fast VM
│   ├── nex_vm.h
│   └── nex_vm.cpp              ← Executes NexIR using TypedSlot stack
│
└── godot/
    ├── nex_language.h
    ├── nex_language.cpp        ← ScriptLanguage implementation
    ├── nex_script.h
    ├── nex_script.cpp          ← Script implementation
    ├── nex_instance.h
    ├── nex_instance.cpp        ← ScriptInstance implementation
    ├── nex_loader.h
    └── nex_loader.cpp          ← ResourceFormatLoader for .nex files
```

---

## STEP 2 — BUILD FILES

**config.py:**
```python
def can_build(env, platform):
    return True

def configure(env):
    pass
```

**SCsub:**
```python
Import("env")
Import("env_modules")

env_nex = env_modules.Clone()

env_nex.add_source_files(env.modules_sources, "*.cpp")
env_nex.add_source_files(env.modules_sources, "frontend/*.cpp")
env_nex.add_source_files(env.modules_sources, "optimizer/*.cpp")
env_nex.add_source_files(env.modules_sources, "ir/*.cpp")
env_nex.add_source_files(env.modules_sources, "vm/*.cpp")
env_nex.add_source_files(env.modules_sources, "godot/*.cpp")
```

---

## STEP 3 — TYPE SYSTEM (nex_type.h)

This is the foundation of the entire compiler. Every AST node, IR instruction,
and VM slot is tagged with a `NexType`. Implement this first.

```cpp
// nex_type.h
#pragma once
#include "core/string/string_name.h"

enum class NexBaseType {
    UNKNOWN,
    VOID,
    BOOL,
    INT8, INT16, INT32, INT64,
    FLOAT32, FLOAT64,
    STR,
    CHAR,
    // Godot value types (stored by value in typed slots)
    VECTOR2, VECTOR3, VECTOR4,
    COLOR, RECT2, TRANSFORM2D, QUATERNION,
    // Composite
    STRUCT,
    ENUM,
    ARRAY,          // [T]
    MAP,            // map[K, V]
    TUPLE,          // (T, U, ...)
    OPTION,         // Option<T>
    RESULT,         // Result<T, E>
    FN,             // fn(A, B) -> R
    OBJECT,         // Godot Object* (reference type)
    VARIANT,        // fallback for truly dynamic values
};

struct NexType {
    NexBaseType base = NexBaseType::UNKNOWN;

    // For STRUCT, ENUM: the type name
    StringName name;

    // For ARRAY: element type
    NexType *elem_type = nullptr;

    // For MAP: key and value types
    NexType *key_type   = nullptr;
    NexType *value_type = nullptr;

    // For TUPLE: element types
    Vector<NexType> tuple_types;

    // For OPTION: inner type
    NexType *option_inner = nullptr;

    // For RESULT: ok type and err type
    NexType *result_ok  = nullptr;
    NexType *result_err = nullptr;

    // For FN: param types and return type
    Vector<NexType> fn_params;
    NexType *fn_ret = nullptr;

    bool is_mutable = false;    // tracks var vs immutable binding

    // Size in bytes for slot allocation
    int size_bytes() const;

    // Helpers
    bool is_integer() const;
    bool is_float() const;
    bool is_numeric() const;
    bool is_primitive() const;   // fits in a single typed slot without indirection
    bool is_godot_value_type() const;
    bool is_reference_type() const;   // Object* — needs refcounting
    bool operator==(const NexType &o) const;
    String to_string() const;

    // Factory methods
    static NexType make_int()    { NexType t; t.base = NexBaseType::INT64;   return t; }
    static NexType make_float()  { NexType t; t.base = NexBaseType::FLOAT64; return t; }
    static NexType make_bool()   { NexType t; t.base = NexBaseType::BOOL;    return t; }
    static NexType make_str()    { NexType t; t.base = NexBaseType::STR;     return t; }
    static NexType make_void()   { NexType t; t.base = NexBaseType::VOID;    return t; }
    static NexType make_option(NexType inner);
    static NexType make_result(NexType ok, NexType err);
    static NexType make_array(NexType elem);
    static NexType make_struct(const StringName &name);
};
```

---

## STEP 4 — TYPED SLOT (nex_typed_slot.h)

This is the core of the fast VM. Every stack slot is a `TypedSlot`. The VM
never reads the wrong field — the IR instructions encode which field to access.

```cpp
// nex_typed_slot.h
#pragma once
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/math/color.h"
#include "core/variant/variant.h"

// A TypedSlot holds exactly one value.
// The kind field is set once at slot allocation time and never changes.
// VM instructions use the kind to decide which union field to read/write.
// This eliminates all runtime type tag checking for typed operations.

enum class SlotKind : uint8_t {
    EMPTY,
    BOOL,
    INT8, INT16, INT32, INT64,
    FLOAT32, FLOAT64,
    STR,        // String* (Godot String, reference semantics)
    CHAR,       // uint32_t codepoint
    VECTOR2,    // 2x float
    VECTOR3,    // 3x float
    COLOR,      // 4x float
    OBJECT,     // Object* (Godot reference-counted object)
    STRUCT_PTR, // void* to heap-allocated struct
    VARIANT,    // Variant (fallback for dynamic values — avoid if possible)
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
    void    *ptr;       // for STR (String*), OBJECT (Object*), STRUCT_PTR
};

struct TypedSlot {
    SlotKind kind = SlotKind::EMPTY;
    SlotData data = {};

    // Fast typed accessors — inline, no branch when kind is known
    inline int64_t  as_int()    const { return data.i64; }
    inline double   as_float()  const { return data.f64; }
    inline bool     as_bool()   const { return data.b; }
    inline Vector2  as_vec2()   const { return data.v2; }
    inline Vector3  as_vec3()   const { return data.v3; }
    inline Color    as_color()  const { return data.col; }
    inline String  *as_str()    const { return (String *)data.ptr; }
    inline Object  *as_object() const { return (Object *)data.ptr; }
    inline void    *as_struct() const { return data.ptr; }

    // Setters
    inline void set_int(int64_t v)   { kind = SlotKind::INT64;   data.i64 = v; }
    inline void set_float(double v)  { kind = SlotKind::FLOAT64; data.f64 = v; }
    inline void set_bool(bool v)     { kind = SlotKind::BOOL;    data.b   = v; }
    inline void set_vec2(Vector2 v)  { kind = SlotKind::VECTOR2; data.v2  = v; }
    inline void set_vec3(Vector3 v)  { kind = SlotKind::VECTOR3; data.v3  = v; }

    // Convert to/from Godot Variant (used only for interop with Godot API)
    Variant  to_variant() const;
    void     from_variant(const Variant &v, const NexType &expected_type);
};

// The typed stack — allocated once per call frame
// Size is known at compile time from IR slot count
struct TypedStack {
    TypedSlot *slots = nullptr;
    int        count = 0;

    void allocate(int slot_count);
    void free_slots();

    inline TypedSlot &operator[](int i) { return slots[i]; }
    inline const TypedSlot &operator[](int i) const { return slots[i]; }
};
```

---

## STEP 5 — TYPED IR (nex_ir.h)

The IR is the bridge between the compiler and the VM. Every instruction is typed —
it knows exactly what slot kinds it operates on. The VM never needs to decide at
runtime.

```cpp
// nex_ir.h
#pragma once
#include "nex_type.h"

enum class NexOp : uint16_t {
    // ── Loads ──────────────────────────────────────────────────────────
    LOAD_INT_CONST,       // dst_slot = immediate int64
    LOAD_FLOAT_CONST,     // dst_slot = immediate double
    LOAD_BOOL_CONST,      // dst_slot = immediate bool
    LOAD_STR_CONST,       // dst_slot = string pool index
    LOAD_NULL,            // dst_slot = none/null

    LOAD_LOCAL,           // dst_slot = local_slot[src]
    STORE_LOCAL,          // local_slot[dst] = src_slot

    LOAD_FIELD_INT,       // dst_slot = struct_ptr->field_offset  (i64)
    LOAD_FIELD_FLOAT,     // dst_slot = struct_ptr->field_offset  (f64)
    LOAD_FIELD_VEC2,      // dst_slot = struct_ptr->field_offset  (Vector2)
    LOAD_FIELD_OBJ,       // dst_slot = struct_ptr->field_offset  (Object*)
    STORE_FIELD_INT,
    STORE_FIELD_FLOAT,
    STORE_FIELD_VEC2,
    STORE_FIELD_OBJ,

    LOAD_INDEX,           // array element access
    STORE_INDEX,

    // ── Integer Arithmetic ─────────────────────────────────────────────
    // All i64 — no Variant, no tag check
    INT_ADD,              // dst = a + b
    INT_SUB,
    INT_MUL,
    INT_DIV,
    INT_MOD,
    INT_POW,
    INT_NEG,              // dst = -a
    INT_EQ, INT_NEQ,
    INT_LT, INT_GT, INT_LTE, INT_GTE,

    // ── Float Arithmetic ───────────────────────────────────────────────
    // All f64 — no Variant, no tag check
    FLOAT_ADD,
    FLOAT_SUB,
    FLOAT_MUL,
    FLOAT_DIV,
    FLOAT_POW,
    FLOAT_NEG,
    FLOAT_EQ, FLOAT_NEQ,
    FLOAT_LT, FLOAT_GT, FLOAT_LTE, FLOAT_GTE,

    // ── Int ↔ Float conversions ────────────────────────────────────────
    INT_TO_FLOAT,
    FLOAT_TO_INT,

    // ── Boolean ────────────────────────────────────────────────────────
    BOOL_AND, BOOL_OR, BOOL_NOT,

    // ── String ─────────────────────────────────────────────────────────
    STR_CONCAT,
    STR_EQ, STR_NEQ,

    // ── Vector2 math ───────────────────────────────────────────────────
    VEC2_ADD, VEC2_SUB, VEC2_MUL_SCALAR, VEC2_DIV_SCALAR,
    VEC2_DOT, VEC2_LENGTH, VEC2_NORMALIZED,

    // ── Control flow ───────────────────────────────────────────────────
    JUMP,                 // unconditional jump to label
    JUMP_IF_FALSE,        // jump if bool slot is false
    JUMP_IF_TRUE,
    JUMP_TABLE,           // jump table for match on integer (O(1) dispatch)
    RETURN,               // return slot value
    RETURN_VOID,

    // ── Calls ──────────────────────────────────────────────────────────
    CALL_DIRECT,          // call NexFunction* directly (known at compile time)
    CALL_FN_PTR,          // call function pointer from slot
    CALL_GODOT_METHOD,    // call Godot object method via ClassDB (interop)
    CALL_BUILTIN,         // call built-in function (print, etc.)

    // ── Struct ─────────────────────────────────────────────────────────
    STRUCT_ALLOC,         // allocate struct on stack, store ptr in slot
    STRUCT_COPY,          // copy struct data

    // ── Arrays ─────────────────────────────────────────────────────────
    ARRAY_NEW,
    ARRAY_PUSH, ARRAY_POP,
    ARRAY_LEN,
    ARRAY_GET_INT,        // typed array element get (i64)
    ARRAY_GET_FLOAT,      // typed array element get (f64)
    ARRAY_GET_OBJ,

    // ── Option ─────────────────────────────────────────────────────────
    OPTION_SOME,          // wrap value → Option<T>::some
    OPTION_NONE,          // produce Option<T>::none
    OPTION_IS_SOME,       // bool: is option some?
    OPTION_UNWRAP,        // get inner value (panics if none)
    OPTION_UNWRAP_OR,     // get inner or default

    // ── Result ─────────────────────────────────────────────────────────
    RESULT_OK,
    RESULT_ERR,
    RESULT_IS_OK,
    RESULT_UNWRAP_OK,
    RESULT_UNWRAP_ERR,
    RESULT_PROPAGATE,     // ? operator: if Err, return it; 2 instructions

    // ── Godot interop ──────────────────────────────────────────────────
    GET_NODE,             // $NodePath sugar
    EMIT_SIGNAL,          // emit signal_name(args...)
    AWAIT_SIGNAL,         // suspend coroutine, resume on signal
    TO_VARIANT,           // convert TypedSlot → Variant (interop only)
    FROM_VARIANT,         // convert Variant → TypedSlot (interop only)

    NOP,
    END,
};

// One IR instruction
struct NexIRInstr {
    NexOp    op;
    int      dst;          // destination slot index (-1 if none)
    int      src_a;        // first source slot (-1 if unused)
    int      src_b;        // second source slot (-1 if unused)
    int64_t  imm_int;      // immediate integer constant
    double   imm_float;    // immediate float constant
    uint32_t imm_str;      // string pool index
    int      label;        // jump target label index
    int      field_offset; // byte offset for struct field access
    StringName call_name;  // function or signal name for calls/signals
    void    *fn_ptr;       // direct function pointer for CALL_DIRECT
    Vector<int> jump_table; // for JUMP_TABLE
    NexType  slot_type;    // type of the slot being operated on
};

// A compiled NEX function — the unit the VM executes
struct NexFunction {
    StringName          name;
    Vector<NexIRInstr>  instructions;
    int                 slot_count;     // total typed slots needed
    Vector<NexType>     slot_types;     // type of each slot
    Vector<String>      string_pool;    // interned string constants
    int                 param_count;
    NexType             return_type;
    bool                is_async;
};
```

---

## STEP 6 — TOKENIZER (frontend/nex_tokenizer.h + .cpp)

```cpp
enum class NexTokenKind {
    // Literals
    LIT_INT, LIT_FLOAT, LIT_STRING, LIT_BOOL, LIT_CHAR,

    // Keywords
    KW_FN, KW_RET, KW_VAR, KW_CONST,
    KW_IF, KW_ELIF, KW_ELSE,
    KW_FOR, KW_WHILE, KW_LOOP, KW_BREAK, KW_CONTINUE,
    KW_IN, KW_BY,
    KW_MATCH,
    KW_STRUCT, KW_IMPL, KW_ENUM,
    KW_USE, KW_PUB,
    KW_ASYNC, KW_AWAIT,
    KW_AND, KW_OR, KW_NOT,
    KW_SOME, KW_NONE, KW_OK, KW_ERR,
    KW_TRUE, KW_FALSE,
    KW_EXTENDS,    // Godot
    KW_SIGNAL,     // Godot
    KW_EMIT,       // Godot
    KW_AUTOLOAD,   // Godot
    KW_AS,         // type cast
    KW_IS,         // type check
    KW_INLINE,     // @inline annotation (also parsed as annotation)

    // Symbols
    SYM_WALRUS,        // :=
    SYM_COLON,         // :
    SYM_SEMICOLON,     // ;
    SYM_COMMA,         // ,
    SYM_DOT,           // .
    SYM_DOTDOT,        // ..
    SYM_DOTDOTEQ,      // ..=
    SYM_ARROW,         // ->
    SYM_FAT_ARROW,     // -> (used in match arms)
    SYM_ASSIGN,        // =
    SYM_LPAREN, SYM_RPAREN,
    SYM_LBRACE, SYM_RBRACE,
    SYM_LBRACKET, SYM_RBRACKET,
    SYM_QUESTION,      // ?
    SYM_AT,            // @
    SYM_DOLLAR,        // $
    SYM_HASH,          // #

    // Operators
    OP_PLUS, OP_MINUS, OP_STAR, OP_SLASH, OP_PERCENT,
    OP_STARSTAR,       // **
    OP_EQ, OP_NEQ,
    OP_LT, OP_GT, OP_LTE, OP_GTE,
    OP_PLUS_EQ, OP_MINUS_EQ, OP_STAR_EQ, OP_SLASH_EQ, OP_PERCENT_EQ,

    IDENT,
    EOF_TOKEN,
    ERROR,
};

struct NexToken {
    NexTokenKind kind;
    String       value;
    int          line;
    int          col;
};

class NexTokenizer {
    String  source;
    int     pos = 0, line = 1, col = 1;

public:
    void              set_source(const String &src);
    Vector<NexToken>  tokenize();

private:
    char     peek(int offset = 0) const;
    char     advance();
    void     skip_whitespace_and_comments();
    NexToken read_string();        // handles {interpolation} markers
    NexToken read_number();        // int or float
    NexToken read_ident_or_kw();
    NexToken read_symbol();
};
```

**Key tokenizer rules:**
- `//` single-line comment, `/* */` block comment
- `:=` walrus (one token, not two)
- `..` and `..=` range tokens (not dot-dot)
- `**` power (not star-star)
- `{name}` inside strings — mark as interpolation zones in the string token
- `$NodePath` — tokenize `$` separately, then the path as IDENT

---

## STEP 7 — AST (frontend/nex_ast.h)

```cpp
struct NexNode {
    enum Kind {
        PROGRAM,
        EXTENDS_STMT,
        USE_STMT,
        AUTOLOAD_STMT,
        SIGNAL_DEF,        // signal name(params);
        FN_DEF,
        STRUCT_DEF,
        IMPL_BLOCK,
        ENUM_DEF,
        CONST_DEF,
        VAR_DECL,          // var x := val  or  x := val
        ASSIGN_STMT,       // x = val
        ASSIGN_OP_STMT,    // x += val
        EMIT_STMT,         // emit signal(args)
        RETURN_STMT,
        IF_STMT,
        FOR_RANGE_STMT,
        FOR_EACH_STMT,
        WHILE_STMT,
        LOOP_STMT,
        BREAK_STMT,
        CONTINUE_STMT,
        MATCH_STMT,
        BLOCK,
        // Expressions
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
        QUESTION_EXPR,     // expr?
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
        DOLLAR_PATH_EXPR,  // $Sprite2D
        IDENTIFIER,
        INT_LITERAL,
        FLOAT_LITERAL,
        STR_LITERAL,
        BOOL_LITERAL,
        CHAR_LITERAL,
        ANNOTATION,        // @export, @inline etc.
    };

    Kind           kind;
    NexType        resolved_type;   // filled in by analyzer
    int            line;

    // Children — meaning depends on kind
    Vector<NexNode *> children;
    String            str_val;
    int64_t           int_val;
    double            float_val;
    bool              bool_val;
    bool              is_mutable;   // for VAR_DECL: was 'var' used?
    String            annotation_name;
    Vector<Variant>   annotation_args;
};
```

---

## STEP 8 — PARSER (frontend/nex_parser.h + .cpp)

Build a recursive descent parser. Consume `NexToken` stream, produce `NexNode *` AST.

```cpp
class NexParser {
    Vector<NexToken> tokens;
    int              pos = 0;

public:
    NexNode *parse(const Vector<NexToken> &p_tokens);

private:
    NexToken &peek(int offset = 0);
    NexToken  consume();
    NexToken  expect(NexTokenKind kind, const String &msg);
    bool      check(NexTokenKind kind) const;
    bool      match_tok(NexTokenKind kind);

    NexNode *parse_program();
    NexNode *parse_top_level();
    NexNode *parse_extends();
    NexNode *parse_use();
    NexNode *parse_fn(bool is_async = false);
    NexNode *parse_struct();
    NexNode *parse_impl();
    NexNode *parse_enum();
    NexNode *parse_signal();
    NexNode *parse_annotation();    // @export, @inline, @resource, etc.

    NexNode *parse_block();
    NexNode *parse_stmt();
    NexNode *parse_var_decl(bool force_mutable = false);
    NexNode *parse_const_decl();
    NexNode *parse_if();
    NexNode *parse_for();
    NexNode *parse_while();
    NexNode *parse_loop();
    NexNode *parse_match();
    NexNode *parse_match_arm();
    NexNode *parse_emit();
    NexNode *parse_return();

    NexNode *parse_expr();
    NexNode *parse_inline_if();
    NexNode *parse_or_expr();
    NexNode *parse_and_expr();
    NexNode *parse_comparison();
    NexNode *parse_addition();
    NexNode *parse_multiplication();
    NexNode *parse_power();
    NexNode *parse_unary();
    NexNode *parse_postfix();         // calls, indexing, field access, ?, as
    NexNode *parse_primary();

    NexType  parse_type_annotation();
};
```

**Critical parsing rules:**
- `x := val` — check if next token after ident is `:=` → VAR_DECL (immutable)
- `var x := val` — `var` keyword → VAR_DECL (mutable)
- `const X := val` — constant
- `match` arms use `->` not `:` — parse as `pattern -> expr_or_block`
- `for i in 0..10` — after `in` parse range expr first
- `for i in 0..20 by 2` — detect `by` keyword after range
- `for i, item in arr` — comma before `in` means index + value form
- `@inline fn name ...` — annotation attaches to the next fn_def
- `async fn name ...` — async flag set on FN_DEF node
- `$NodePath` — `$` token followed by IDENT → DOLLAR_PATH_EXPR node

---

## STEP 9 — TYPE ANALYZER (frontend/nex_analyzer.h + .cpp)

Walk the AST bottom-up. Assign `resolved_type` to every node. Report errors.

```cpp
class NexAnalyzer {
    struct Scope {
        HashMap<StringName, NexType> bindings;
        HashMap<StringName, bool>    mutability;   // true = mutable
        Scope *parent = nullptr;
    };

    struct StructInfo {
        StringName name;
        struct Field { StringName name; NexType type; int byte_offset; };
        Vector<Field> fields;
        int total_size;
        HashMap<StringName, NexFunction *> methods;
    };

    struct SignalInfo {
        StringName name;
        Vector<NexType> param_types;
    };

    HashMap<StringName, StructInfo>  structs;
    HashMap<StringName, NexType>     enums;
    HashMap<StringName, SignalInfo>  signals;
    HashMap<StringName, NexType>     globals;
    Scope                           *current_scope = nullptr;
    NexType                          current_fn_return;
    StringName                       base_class;
    List<String>                     errors;

public:
    bool   analyze(NexNode *root);
    List<String> get_errors() const { return errors; }

private:
    // Scope management
    void push_scope();
    void pop_scope();
    void declare(const StringName &name, const NexType &type, bool mutable_flag);
    const NexType *lookup(const StringName &name) const;
    bool  is_mutable(const StringName &name) const;

    // Node analysis
    void     analyze_node(NexNode *node);
    NexType  analyze_expr(NexNode *node);
    NexType  analyze_binary(NexNode *node);
    NexType  analyze_call(NexNode *node);
    NexType  analyze_field_access(NexNode *node);

    // Critical checks
    void check_immutability(NexNode *assign_node);
    // If left side is not mutable, emit error — this is how NEX prevents
    // reassigning immutable bindings with ZERO runtime cost

    void check_type_match(const NexType &expected, const NexType &got, int line);
    void check_result_handled(NexNode *call_node);
    // If a function returns Result<T,E> and the caller ignores it — warn

    void resolve_struct_offsets(StructInfo &info);
    // Compute byte_offset for each field — used directly by IR field instructions

    NexType godot_class_method_return_type(const StringName &cls, const StringName &method);
    // Query ClassDB for Godot class method return types
};
```

**Key immutability implementation:**
```cpp
void NexAnalyzer::check_immutability(NexNode *assign_node) {
    // assign_node is ASSIGN_STMT or ASSIGN_OP_STMT
    NexNode *lhs = assign_node->children[0];

    if (lhs->kind == NexNode::IDENTIFIER) {
        StringName name = lhs->str_val;
        if (!is_mutable(name)) {
            errors.push_back(vformat(
                "Line %d: Cannot assign to '%s' — it is immutable. Use 'var %s := ...' to declare it mutable.",
                assign_node->line, name, name
            ));
        }
    }
    // Struct fields are mutable if the struct binding is var
    // Array elements are mutable if the array binding is var
}
```

---

## STEP 10 — OPTIMIZER (optimizer/nex_optimizer.h + .cpp)

Run passes on the typed AST before IR generation. Each pass is independent.

```cpp
class NexOptimizer {
public:
    void run(NexNode *root);

private:
    // Pass 1: Constant Folding
    // Replace compile-time-computable expressions with their result
    // Examples:
    //   2 ** 8              → INT_LITERAL(256)
    //   MAX_HEALTH * 2      → INT_LITERAL(200)   (if MAX_HEALTH is const 100)
    //   true and false      → BOOL_LITERAL(false)
    //   "hello " + "world"  → STR_LITERAL("hello world")
    NexNode *fold_constants(NexNode *node);

    // Pass 2: Dead Code Elimination
    // Remove branches that can never execute
    // Examples:
    //   if false { ... }           → remove entirely
    //   if true { A } else { B }   → keep only A
    //   match const_val { ... }    → keep only the matching arm
    void eliminate_dead_code(NexNode *block);

    // Pass 3: Immutability Propagation
    // Replace reads of immutable bindings with their literal value
    // This enables further constant folding in pass 1
    // Example:
    //   const SPEED := 200.0;
    //   x := SPEED * dt;
    //   → x := 200.0 * dt;   (SPEED reference replaced with 200.0)
    void propagate_immutables(NexNode *block, HashMap<StringName, NexNode *> &constants);

    // Pass 4: Inline Expansion
    // Replace calls to @inline functions with their body
    // Also auto-inline functions with ≤ 8 AST expression nodes
    // After inlining, re-run passes 1 and 3 on the expanded body
    void expand_inline_calls(NexNode *block);
    int  count_expr_nodes(NexNode *fn_body);

    // Pass 5: Loop Invariant Code Motion
    // Move expressions that don't change inside a loop to before the loop
    // Example:
    //   for i in 0..100 {
    //       x := heavy_compute() * 2;   // heavy_compute has no side effects
    //   }
    //   → moved_x := heavy_compute() * 2;
    //     for i in 0..100 { x := moved_x; }
    void hoist_loop_invariants(NexNode *for_node);
};
```

---

## STEP 11 — IR GENERATOR (ir/nex_ir_gen.cpp)

Convert the optimized typed AST into `NexFunction` IR.

```cpp
class NexIRGen {
    NexFunction     *current_fn  = nullptr;
    int              next_slot   = 0;
    HashMap<StringName, int> local_slots;    // name → slot index
    HashMap<StringName, StructInfo *> structs;

public:
    NexFunction *generate_function(NexNode *fn_def_node);

private:
    int  alloc_slot(const NexType &type);    // assign next available slot
    void emit(NexIRInstr instr);

    int  gen_expr(NexNode *node);            // returns dst slot index
    void gen_stmt(NexNode *node);

    // Typed arithmetic — select correct opcode based on type
    NexOp arith_op(const String &op, const NexType &type);
    // INT_ADD for int+int, FLOAT_ADD for float+float, etc.

    // Field access — uses byte offset from StructInfo, never a name lookup
    int  gen_field_access(NexNode *field_node);
    // Emits LOAD_FIELD_INT / LOAD_FIELD_FLOAT / etc. based on field type

    // Range loops → pure integer counter
    void gen_for_range(NexNode *for_node);
    // Emits: LOAD_INT_CONST(start), loop header, INT_LT, JUMP_IF_FALSE(end),
    // body, INT_ADD(step), JUMP(loop_header), label(end)
    // No iterator object. No allocation. Identical to C for loop.

    // Match → jump table for integer types
    void gen_match(NexNode *match_node);
    // If matching on int/enum: emit JUMP_TABLE with arm labels
    // O(1) dispatch regardless of arm count

    // Result ? propagation
    void gen_question(NexNode *q_node);
    // Emits: RESULT_IS_OK, JUMP_IF_FALSE(return_err), RESULT_UNWRAP_OK
    // Two instructions for the happy path
};
```

---

## STEP 12 — TYPED VM (vm/nex_vm.h + .cpp)

The VM executes `NexFunction` IR. It uses `TypedStack` — never `Vector<Variant>`.

```cpp
class NexVM {
public:
    Variant execute(
        NexFunction        *p_fn,
        NexScriptInstance  *p_instance,
        const Variant     **p_args,
        int                 p_arg_count,
        Callable::CallError &r_error
    );

private:
    // The dispatch loop — use computed goto on GCC/Clang for maximum speed
    // Falls back to switch on MSVC
    void dispatch(NexFunction *fn, TypedStack &stack, NexScriptInstance *instance);
};
```

**VM implementation — the critical inner loop:**

```cpp
void NexVM::dispatch(NexFunction *fn, TypedStack &stack, NexScriptInstance *instance) {
    const NexIRInstr *ip = fn->instructions.ptr();

    // Use computed goto for maximum dispatch speed on GCC/Clang
    // This eliminates the switch overhead — each handler jumps directly
    // to the next handler via a function pointer table
    #define DISPATCH() goto *dispatch_table[ip->op]; ++ip;

    static void *dispatch_table[] = {
        &&op_LOAD_INT_CONST,
        &&op_INT_ADD,
        // ... all opcodes
    };

    DISPATCH();

    op_LOAD_INT_CONST:
        stack[ip->dst].set_int(ip->imm_int);
        DISPATCH();

    op_INT_ADD:
        // No tag check. No Variant dispatch. One CPU ADD instruction.
        stack[ip->dst].set_int(stack[ip->src_a].as_int() + stack[ip->src_b].as_int());
        DISPATCH();

    op_FLOAT_ADD:
        stack[ip->dst].set_float(stack[ip->src_a].as_float() + stack[ip->src_b].as_float());
        DISPATCH();

    op_LOAD_FIELD_INT:
        // struct_ptr->field_offset — compile-time byte offset, no hashmap
        {
            char *base = (char *)stack[ip->src_a].as_struct();
            stack[ip->dst].set_int(*(int64_t *)(base + ip->field_offset));
        }
        DISPATCH();

    op_JUMP_TABLE:
        // O(1) match dispatch for integer/enum values
        {
            int64_t val = stack[ip->src_a].as_int();
            int arm = (val >= 0 && val < ip->jump_table.size())
                    ? ip->jump_table[val]
                    : ip->label;    // default arm
            ip = fn->instructions.ptr() + arm;
        }
        DISPATCH();

    op_RESULT_PROPAGATE:
        // ? operator — if Result is Err, return it immediately
        // Two instructions: tag read + conditional jump
        if (!stack[ip->src_a].as_bool()) {    // as_bool() = is_ok flag
            // copy err value to return slot and return
            stack[0] = stack[ip->src_b];      // err value
            return;
        }
        DISPATCH();

    op_TO_VARIANT:
        // Only used at Godot API call boundaries — avoid in hot paths
        stack[ip->dst].from_variant(
            stack[ip->dst].to_variant(), fn->slot_types[ip->dst]
        );
        DISPATCH();

    // ... all other opcodes
}
```

**Why computed goto is important:**
A switch statement generates one compare+jump per case per iteration.
Computed goto jumps directly to the handler — zero overhead dispatch.
This alone gives ~30% speedup on tight numeric loops.

---

## STEP 13 — GODOT INTEGRATION (godot/)

### nex_language.cpp — ScriptLanguage

```cpp
class NexLanguage : public ScriptLanguage {
    static NexLanguage *singleton;

public:
    static NexLanguage *get_singleton() { return singleton; }

    String get_name()      const override { return "NEX"; }
    String get_type()      const override { return "NexScript"; }
    String get_extension() const override { return "nex"; }

    void init()   override;
    void finish() override;

    // Editor integration
    void get_reserved_words(List<String> *p_words) const override;
    // Must include all NEX keywords: fn ret var const if elif else for
    // while loop break continue match struct impl enum use pub async
    // await in by and or not some none ok err extends signal emit as is

    void get_comment_delimiters(List<String> *p_delimiters) const override;
    // Add: "//" and "/* */"

    void get_string_delimiters(List<String> *p_delimiters) const override;
    // Add: "\" \""

    Ref<Script> make_template(
        const String &p_template,
        const String &p_class_name,
        const String &p_base_class_name
    ) const override;
    // Default template:
    // extends {base_class_name};\n\nfn _ready {\n\t\n}\n

    bool validate(
        const String &p_script, const String &p_path,
        List<String> *r_functions,
        List<ScriptLanguage::ScriptError> *r_errors,
        List<ScriptLanguage::Warning> *r_warnings,
        HashSet<int> *r_safe_lines
    ) const override;
    // Run tokenizer + parser + analyzer ONLY (no IR gen, no VM)
    // Return errors with line numbers for editor squiggles
    // This runs on every keypress — must be fast
};
```

### nex_script.cpp — Script

```cpp
class NexScript : public Script {
    String     source_code;
    String     file_path;
    bool       is_compiled = false;
    StringName base_class;

    // Compiled functions — indexed by name
    HashMap<StringName, NexFunction *> functions;

    // Struct layouts from this script
    HashMap<StringName, NexAnalyzer::StructInfo> structs;

    // Exported variables (for Inspector)
    struct ExportVar {
        StringName  name;
        NexType     type;
        Variant     default_value;
        PropertyInfo property_info;
    };
    Vector<ExportVar> exports;

    // Declared signals
    struct SignalDecl {
        StringName name;
        MethodInfo method_info;
    };
    Vector<SignalDecl> signals;

public:
    Error compile();
    // Run: Tokenizer → Parser → Analyzer → Optimizer → IRGen
    // Store resulting NexFunctions in 'functions' map

    // Script interface
    bool        can_instantiate() const override;
    StringName  get_instance_base_type() const override { return base_class; }
    ScriptInstance *instance_create(Object *p_this) override;

    void get_script_property_list(List<PropertyInfo> *p_list) const override;
    // Returns exports — what appears in the Godot Inspector

    void get_script_signal_list(List<MethodInfo> *p_list) const override;
    // Returns declared signals — what appears in Node panel

    Error reload(bool p_keep_state) override;
    // Re-run compile(), preserve instance state if p_keep_state
};
```

### nex_instance.cpp — ScriptInstance

```cpp
class NexScriptInstance : public ScriptInstance {
    Object        *owner  = nullptr;
    Ref<NexScript> script;

    // Runtime member variables — one TypedSlot per declared 'var' or @export
    HashMap<StringName, TypedSlot> member_slots;

public:
    NexScriptInstance(Object *p_owner, const Ref<NexScript> &p_script);

    bool set(const StringName &p_name, const Variant &p_value) override;
    bool get(const StringName &p_name, Variant &r_ret) const override;
    // Convert between TypedSlot and Variant for Godot Inspector/serialization

    void call(
        const StringName &p_method,
        const Variant **p_args, int p_argcount,
        Variant &r_ret, Callable::CallError &r_error
    ) override;
    // Look up NexFunction by name, execute via NexVM

    void notification(int p_notification, bool p_reversed) override;
    // Map Godot notifications to NEX lifecycle functions:
    //   NOTIFICATION_READY          → _ready
    //   NOTIFICATION_PROCESS        → _process(delta)
    //   NOTIFICATION_PHYSICS_PROCESS → _physics_process(delta)
    //   NOTIFICATION_EXIT_TREE      → _exit_tree

    Ref<Script> get_script() const override { return script; }
    ScriptLanguage *get_language() override { return NexLanguage::get_singleton(); }
};
```

---

## STEP 14 — REGISTER TYPES

```cpp
// register_types.cpp
#include "godot/nex_language.h"
#include "godot/nex_loader.h"

static NexLanguage                  *nex_language = nullptr;
static Ref<ResourceFormatLoaderNex>  nex_loader;

void initialize_nexscript_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

    nex_language = memnew(NexLanguage);
    ScriptServer::register_language(nex_language);

    nex_loader.instantiate();
    ResourceLoader::add_resource_format_loader(nex_loader);
}

void uninitialize_nexscript_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

    ScriptServer::unregister_language(nex_language);
    memdelete(nex_language);

    ResourceLoader::remove_resource_format_loader(nex_loader);
    nex_loader.unref();
}
```

---

## STEP 15 — BUILD AND VERIFY

```bash
# Build with module
scons platform=linuxbsd target=editor dev_build=yes

# Module auto-detected from modules/nexscript/config.py
```

### Verification Checklist

**Basic:**
- [ ] Engine starts without crash or assertion
- [ ] `NexLanguage` appears in ScriptServer language list
- [ ] `.nex` extension appears in "New Script" dialog
- [ ] Script template renders: `extends {base};\n\nfn _ready { }`
- [ ] Editor shows syntax highlighting for NEX keywords

**Compiler:**
- [ ] Immutable reassignment caught at compile time with clear error message
- [ ] Type mismatch caught at compile time
- [ ] `const` values absent from output IR (folded)
- [ ] `@inline` functions absent from call sites (inlined)

**Performance — run these benchmarks:**

```nex
// Benchmark 1: Integer loop — should match C speed
fn bench_int_loop {
    var sum := 0;
    for i in 0..10000000 {
        sum += i;
    }
    ret sum;
}

// Benchmark 2: Struct field access — should be 1 load instruction
struct Vec { x: float, y: float }
fn bench_struct_access(v: Vec) -> float {
    ret v.x + v.y;    // 2 loads + 1 add — verify in IR dump
}

// Benchmark 3: Match dispatch — should be O(1) jump table
fn bench_match(code: int) -> int {
    match code {
        0 -> ret 10,
        1 -> ret 20,
        2 -> ret 30,
        3 -> ret 40,
        _ -> ret 0,
    }
}
```

Compare each benchmark against equivalent GDScript:
- `bench_int_loop`: NEX should be ≥ 2x faster
- `bench_struct_access`: NEX should be ≥ 3x faster (no HashMap)
- `bench_match`: NEX O(1), GDScript O(n) — gap grows with arm count

**Godot integration:**
- [ ] `extends Node2D;` accepted, script attaches correctly
- [ ] `fn _ready` executes when node enters scene tree
- [ ] `fn _process(dt: float)` called every frame, dt is correct
- [ ] `@export` variables appear in Godot Inspector
- [ ] Signal declared with `signal` appears in Node panel
- [ ] `emit signal_name(args)` fires connected handlers
- [ ] `$NodePath` resolves correctly
- [ ] `await signal` suspends and resumes correctly
- [ ] Hot reload works — edit `.nex`, save, changes apply without restart

---

## REFERENCE: GDSCRIPT VS NEX VM COMPARISON

Study these files but do NOT copy them directly. Use them to understand Godot's
scripting interface, then implement NEX's faster version:

| NEX file | GDScript reference | Key difference |
|---|---|---|
| nex_typed_slot.h | (no equivalent) | TypedSlot replaces Variant for all hot-path operations |
| nex_ir.h | gdscript_function.h | IR instructions are typed — each encodes its operand types |
| nex_vm.cpp | gdscript_vm.cpp | Uses TypedStack + computed goto, not Variant stack + switch |
| nex_analyzer.cpp | gdscript_analyzer.cpp | Tracks mutability per binding — immutability is compile-time |
| nex_optimizer.cpp | (no equivalent) | GDScript has no optimization pass — NEX does constant folding etc. |
| nex_ir_gen.cpp | gdscript_compiler.cpp | Emits typed IR ops, not Variant bytecode |
| nex_language.cpp | gdscript.cpp | Same ScriptLanguage interface |
| nex_script.cpp | gdscript.cpp | Same Script interface, holds NexFunctions instead of GDScriptFunctions |
| nex_instance.cpp | gdscript.cpp | Same ScriptInstance interface, uses TypedSlot for member vars |

---

## IMPORTANT RULES FOR AI ASSISTANT

1. Every file in the folder structure must be implemented. No stubs.

2. The VM stack is `TypedStack`, never `Vector<Variant>`. If you find yourself
   writing `Variant stack`, stop and use `TypedStack` instead.

3. Use computed goto (`goto *table[op]`) in the VM dispatch loop on GCC/Clang.
   Fall back to switch only for MSVC. This is not optional — it is a significant
   performance gain.

4. Struct field access must use `ip->field_offset` (byte offset computed at
   compile time by the analyzer). Never look up a field by name at runtime.

5. Integer match must emit `JUMP_TABLE` IR instruction, never a chain of
   `JUMP_IF_FALSE` instructions. Jump table = O(1). Chain = O(n).

6. The `?` operator (Result propagation) must emit exactly 2 IR instructions:
   `RESULT_IS_OK` check + `JUMP_IF_FALSE`. Not 5. Not 10. Two.

7. Immutability is a compile-time check only. The VM has zero immutability logic.
   Violation caught by analyzer → compile error. Nothing in the VM enforces it.

8. `TO_VARIANT` and `FROM_VARIANT` instructions are interop instructions only —
   used at Godot API call boundaries. They must never appear inside a tight loop
   in normal NEX code.

9. NEX syntax differs from GDScript. Always remember:
   - `fn` not `func`
   - `ret` not `return`
   - `:=` for declaration, `=` for assignment only
   - `var` required for mutable bindings (default is immutable)
   - `elif` not `else if`
   - `and` / `or` / `not` — not `&&` / `||` / `!`
   - `match arm ->` not `match arm:`
   - `0..10` range (not `range(0, 10)`)
   - `struct` + `impl` — not classes
   - `signal` / `emit` are keywords
   - `extends` ends with semicolon
   - `@inline` annotation on functions
   - `$NodePath` for node access

10. Run the benchmark suite in Step 15 before declaring the implementation done.
    If NEX is not faster than GDScript on the numeric benchmarks, the VM
    implementation is wrong.

---

*Build the whole thing. Benchmark it. Make it faster than GDScript.*
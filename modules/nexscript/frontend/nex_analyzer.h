#pragma once
#include "nex_ast.h"
#include "core/templates/hash_map.h"
#include "core/templates/list.h"

class NexAnalyzer {
public:
    struct FieldInfo {
        StringName name;
        NexType    type;
        int        byte_offset = 0;
    };

    struct StructInfo {
        StringName         name;
        Vector<FieldInfo>  fields;
        int                total_size = 0;
        HashMap<StringName, int> field_index;
    };

    struct SignalInfo {
        StringName       name;
        Vector<NexType>  param_types;
    };

    struct FnInfo {
        StringName      name;
        NexType         return_type;
        Vector<NexType> param_types;
        bool            is_async = false;
    };

private:
    struct Scope {
        HashMap<StringName, NexType> bindings;
        HashMap<StringName, bool>    mutability;
        Scope *parent = nullptr;
    };

    HashMap<StringName, StructInfo>  structs;
    HashMap<StringName, NexType>     enums;
    HashMap<StringName, SignalInfo>  signals;
    HashMap<StringName, FnInfo>      functions;
    HashMap<StringName, NexType>     globals;
    Scope                           *current_scope = nullptr;
    NexType                          current_fn_return;
    StringName                       base_class;
    List<String>                     errors;
    bool                             in_loop = false;

public:
    bool         analyze(NexNode *root);
    List<String> get_errors() const { return errors; }

    const HashMap<StringName, StructInfo> &get_structs() const { return structs; }

private:
    void push_scope();
    void pop_scope();
    void declare(const StringName &name, const NexType &type, bool mutable_flag);
    const NexType *lookup(const StringName &name) const;
    bool           is_mutable_binding(const StringName &name) const;

    void    analyze_node(NexNode *node);
    NexType analyze_expr(NexNode *node);
    NexType analyze_binary(NexNode *node);
    NexType analyze_call(NexNode *node);
    NexType analyze_method_call(NexNode *node);
    NexType analyze_field_access(NexNode *node);

    void check_immutability(NexNode *assign_node);
    void check_type_match(const NexType &expected, const NexType &got, int line);

    void resolve_struct_offsets(StructInfo &info);
    void collect_top_level(NexNode *root);

    NexType godot_type_for_name(const StringName &name);
};

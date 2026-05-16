#pragma once
#include "nex_ir.h"
#include "modules/nexscript/frontend/nex_ast.h"
#include "modules/nexscript/frontend/nex_analyzer.h"
#include "core/templates/hash_map.h"

class NexIRGen {
    NexFunction *current_fn = nullptr;
    int          next_slot  = 0;
    int          next_label = 0;
    HashMap<StringName, int>  local_slots;
    HashMap<StringName, NexAnalyzer::StructInfo> structs;
    HashMap<StringName, int64_t> const_ints;
    HashMap<StringName, double>  const_floats;
    HashMap<StringName, String>  const_strs;

public:
    void set_structs(const HashMap<StringName, NexAnalyzer::StructInfo> &s) { structs = s; }

    NexFunction *generate_function(NexNode *fn_def_node);
    NexFunction *generate_script_body(NexNode *program_node);

private:
    int  alloc_slot(const NexType &type);
    int  new_label();
    void patch_label(int label_idx, int target_instr);
    void emit(NexIRInstr instr);
    uint32_t intern_string(const String &s);

    int  gen_expr(NexNode *node);
    void gen_stmt(NexNode *node);
    void gen_block(NexNode *block_node);

    NexOp arith_op(const String &op, const NexType &type);
    NexOp cmp_op(const String &op, const NexType &type);

    void gen_field_access(NexNode *field_node, int dst_slot);
    void gen_for_range(NexNode *for_node);
    void gen_for_each(NexNode *for_node);
    void gen_match(NexNode *match_node);
    void gen_question(NexNode *q_node, int dst_slot);
    void gen_if(NexNode *if_node);
    void gen_while(NexNode *while_node);
    void gen_loop(NexNode *loop_node);
};

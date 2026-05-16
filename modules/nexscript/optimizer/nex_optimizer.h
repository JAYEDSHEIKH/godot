#pragma once
#include "modules/nexscript/frontend/nex_ast.h"
#include "core/templates/hash_map.h"

class NexOptimizer {
public:
    void run(NexNode *root);

private:
    NexNode *fold_constants(NexNode *node);
    void     eliminate_dead_code(NexNode *block);
    void     propagate_immutables(NexNode *block, HashMap<StringName, NexNode *> &constants);
    void     expand_inline_calls(NexNode *block);
    void     hoist_loop_invariants(NexNode *for_node);
    int      count_expr_nodes(NexNode *fn_body);
    bool     is_pure(NexNode *node) const;
    NexNode *clone_node(NexNode *node) const;
};

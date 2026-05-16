#include "nex_optimizer.h"
#include <cmath>

void NexOptimizer::run(NexNode *root) {
    if (!root) return;
    HashMap<StringName, NexNode *> constants;
    propagate_immutables(root, constants);
    for (NexNode *child : root->children) {
        fold_constants(child);
        if (child && child->kind == NexNode::FN_DEF) {
            for (NexNode *c : child->children) {
                if (c && c->kind == NexNode::BLOCK) {
                    eliminate_dead_code(c);
                }
            }
        }
    }
}

NexNode *NexOptimizer::clone_node(NexNode *node) const {
    if (!node) return nullptr;
    NexNode *copy = new NexNode(node->kind, node->line);
    copy->str_val = node->str_val;
    copy->int_val = node->int_val;
    copy->float_val = node->float_val;
    copy->bool_val = node->bool_val;
    copy->op = node->op;
    copy->resolved_type = node->resolved_type;
    copy->type_annotation = node->type_annotation;
    for (NexNode *child : node->children) {
        copy->children.push_back(clone_node(child));
    }
    return copy;
}

bool NexOptimizer::is_pure(NexNode *node) const {
    if (!node) return true;
    switch (node->kind) {
        case NexNode::INT_LITERAL:
        case NexNode::FLOAT_LITERAL:
        case NexNode::STR_LITERAL:
        case NexNode::BOOL_LITERAL:
        case NexNode::CHAR_LITERAL:
        case NexNode::IDENTIFIER:
            return true;
        case NexNode::BINARY_EXPR:
        case NexNode::UNARY_EXPR:
        case NexNode::FIELD_EXPR:
            for (NexNode *c : node->children) { if (!is_pure(c)) return false; }
            return true;
        default:
            return false;
    }
}

NexNode *NexOptimizer::fold_constants(NexNode *node) {
    if (!node) return nullptr;

    // Recursively fold children first
    for (int i = 0; i < node->children.size(); i++) {
        NexNode *folded = fold_constants(node->children[i]);
        if (folded && folded != node->children[i]) {
            delete node->children.write[i];
            node->children.write[i] = folded;
        }
    }

    if (node->kind == NexNode::BINARY_EXPR && node->children.size() == 2) {
        NexNode *lhs = node->children[0];
        NexNode *rhs = node->children[1];
        const String &op = node->op;

        if (lhs->kind == NexNode::INT_LITERAL && rhs->kind == NexNode::INT_LITERAL) {
            int64_t a = lhs->int_val, b = rhs->int_val;
            NexNode *result = new NexNode(NexNode::INT_LITERAL, node->line);
            bool folded = true;
            if (op == "+")  result->int_val = a + b;
            else if (op == "-") result->int_val = a - b;
            else if (op == "*") result->int_val = a * b;
            else if (op == "/" && b != 0) { result->kind = NexNode::FLOAT_LITERAL; result->float_val = (double)a / b; }
            else if (op == "%") result->int_val = b != 0 ? a % b : 0;
            else if (op == "**") { result->int_val = (int64_t)pow((double)a, (double)b); }
            else { folded = false; delete result; }
            if (folded) {
                result->resolved_type = (result->kind == NexNode::FLOAT_LITERAL) ? NexType::make_float() : NexType::make_int();
                return result;
            }
        }
        if (lhs->kind == NexNode::FLOAT_LITERAL && rhs->kind == NexNode::FLOAT_LITERAL) {
            double a = lhs->float_val, b = rhs->float_val;
            NexNode *result = new NexNode(NexNode::FLOAT_LITERAL, node->line);
            bool folded = true;
            if (op == "+")  result->float_val = a + b;
            else if (op == "-") result->float_val = a - b;
            else if (op == "*") result->float_val = a * b;
            else if (op == "/" && b != 0.0) result->float_val = a / b;
            else if (op == "**") result->float_val = pow(a, b);
            else { folded = false; delete result; }
            if (folded) { result->resolved_type = NexType::make_float(); return result; }
        }
        if (lhs->kind == NexNode::BOOL_LITERAL && rhs->kind == NexNode::BOOL_LITERAL) {
            NexNode *result = new NexNode(NexNode::BOOL_LITERAL, node->line);
            if (op == "and") result->bool_val = lhs->bool_val && rhs->bool_val;
            else if (op == "or") result->bool_val = lhs->bool_val || rhs->bool_val;
            else { delete result; return node; }
            result->resolved_type = NexType::make_bool();
            return result;
        }
        if (lhs->kind == NexNode::STR_LITERAL && rhs->kind == NexNode::STR_LITERAL && op == "+") {
            NexNode *result = new NexNode(NexNode::STR_LITERAL, node->line);
            result->str_val = lhs->str_val + rhs->str_val;
            result->resolved_type = NexType::make_str();
            return result;
        }
    }

    if (node->kind == NexNode::UNARY_EXPR && node->children.size() == 1) {
        NexNode *operand = node->children[0];
        if (node->op == "not" && operand->kind == NexNode::BOOL_LITERAL) {
            NexNode *r = new NexNode(NexNode::BOOL_LITERAL, node->line);
            r->bool_val = !operand->bool_val;
            r->resolved_type = NexType::make_bool();
            return r;
        }
        if (node->op == "-" && operand->kind == NexNode::INT_LITERAL) {
            NexNode *r = new NexNode(NexNode::INT_LITERAL, node->line);
            r->int_val = -operand->int_val;
            r->resolved_type = NexType::make_int();
            return r;
        }
        if (node->op == "-" && operand->kind == NexNode::FLOAT_LITERAL) {
            NexNode *r = new NexNode(NexNode::FLOAT_LITERAL, node->line);
            r->float_val = -operand->float_val;
            r->resolved_type = NexType::make_float();
            return r;
        }
    }
    return node;
}

void NexOptimizer::eliminate_dead_code(NexNode *block) {
    if (!block) return;
    Vector<NexNode *> kept;
    for (NexNode *child : block->children) {
        if (!child) continue;
        if (child->kind == NexNode::IF_STMT && !child->children.is_empty()) {
            NexNode *cond = child->children[0];
            if (cond->kind == NexNode::BOOL_LITERAL) {
                if (cond->bool_val && child->children.size() > 1) {
                    // Always true — keep true branch
                    NexNode *true_block = child->children[1];
                    for (NexNode *s : true_block->children) kept.push_back(s);
                    true_block->children.clear();
                    delete child;
                } else if (!cond->bool_val && child->children.size() > 2) {
                    // Always false — keep else branch
                    NexNode *else_block = child->children[child->children.size() - 1];
                    if (else_block && else_block->kind == NexNode::BLOCK) {
                        for (NexNode *s : else_block->children) kept.push_back(s);
                        else_block->children.clear();
                    }
                    delete child;
                } else {
                    kept.push_back(child);
                }
                continue;
            }
        }
        kept.push_back(child);
    }
    block->children = kept;
    for (NexNode *child : block->children) {
        if (child && child->kind == NexNode::BLOCK) eliminate_dead_code(child);
    }
}

void NexOptimizer::propagate_immutables(NexNode *block, HashMap<StringName, NexNode *> &constants) {
    if (!block) return;
    for (NexNode *child : block->children) {
        if (!child) continue;
        if ((child->kind == NexNode::VAR_DECL || child->kind == NexNode::CONST_DEF) &&
            !child->is_mutable && !child->children.is_empty()) {
            NexNode *val = child->children[0];
            if (val->kind == NexNode::INT_LITERAL || val->kind == NexNode::FLOAT_LITERAL ||
                val->kind == NexNode::STR_LITERAL  || val->kind == NexNode::BOOL_LITERAL) {
                constants[StringName(child->str_val)] = val;
            }
        }
        // Replace identifier references to known constants
        if (child->kind == NexNode::IDENTIFIER && constants.has(StringName(child->str_val))) {
            NexNode *replacement = clone_node(constants[StringName(child->str_val)]);
            child->kind = replacement->kind;
            child->int_val = replacement->int_val;
            child->float_val = replacement->float_val;
            child->str_val = replacement->str_val;
            child->bool_val = replacement->bool_val;
            delete replacement;
        }
        propagate_immutables(child, constants);
    }
}

void NexOptimizer::expand_inline_calls(NexNode *block) {
    // Basic inline expansion — mark small functions for inlining
    // Full inline expansion requires function body substitution
    // which is handled at IR gen level
}

int NexOptimizer::count_expr_nodes(NexNode *fn_body) {
    if (!fn_body) return 0;
    int count = 1;
    for (NexNode *c : fn_body->children) count += count_expr_nodes(c);
    return count;
}

void NexOptimizer::hoist_loop_invariants(NexNode *for_node) {
    // Loop invariant code motion — future optimization pass
}

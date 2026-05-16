#pragma once
#include "modules/ai_engine/executor/tool_registry.h"

class AuditTools : public ToolSet {
public:
    void register_tools(ToolRegistry *r) override;
    bool execute(const String &p_name, const Dictionary &p_args, String &r_result) override;
};

#pragma once
#include "modules/ai_engine/executor/tool_registry.h"

class ScriptTools : public ToolSet {
public:
    void register_tools(ToolRegistry *r) override;
    bool execute(const String &p_name, const Dictionary &p_args, String &r_result) override;

private:
    String _create_script(const Dictionary &args);
    String _attach_script(const Dictionary &args);
    String _run_code(const Dictionary &args);
    String _explain_script(const Dictionary &args);
    String _refactor_function(const Dictionary &args);
    String _add_function(const Dictionary &args);
    String _get_open_scripts(const Dictionary &args);
    String _format_script(const Dictionary &args);
    String _check_syntax(const Dictionary &args);
};

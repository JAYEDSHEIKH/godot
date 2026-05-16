#pragma once
#include "modules/ai_engine/executor/tool_registry.h"

class FileTools : public ToolSet {
public:
    void register_tools(ToolRegistry *r) override;
    bool execute(const String &p_name, const Dictionary &p_args, String &r_result) override;

private:
    String _read_file(const Dictionary &args);
    String _write_file(const Dictionary &args);
    String _create_file(const Dictionary &args);
    String _delete_file(const Dictionary &args);
    String _list_directory(const Dictionary &args);
    String _search_in_files(const Dictionary &args);
    String _replace_in_file(const Dictionary &args);
    String _get_file_info(const Dictionary &args);
    String _copy_file(const Dictionary &args);
    String _move_file(const Dictionary &args);
};

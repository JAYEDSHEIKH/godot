#pragma once
#include "core/object/ref_counted.h"
#include "core/string/ustring.h"

class ProjectIndexer : public RefCounted {
    GDCLASS(ProjectIndexer, RefCounted);

    bool is_indexing  = false;
    int  files_total  = 0;
    int  files_done   = 0;
    String project_path;

public:
    void start_indexing(const String &p_root_path);
    void stop_indexing();
    bool get_is_indexing() const { return is_indexing; }
    float get_progress()   const { return files_total > 0 ? (float)files_done / files_total : 0.0f; }
    int get_files_indexed() const { return files_done; }

protected:
    static void _bind_methods();

private:
    void _index_directory(const String &path);
};

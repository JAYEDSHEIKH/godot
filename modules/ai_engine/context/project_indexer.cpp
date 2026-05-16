#include "project_indexer.h"
#include "core/io/dir_access.h"

void ProjectIndexer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_indexing", "root_path"), &ProjectIndexer::start_indexing);
    ClassDB::bind_method(D_METHOD("stop_indexing"),               &ProjectIndexer::stop_indexing);
    ClassDB::bind_method(D_METHOD("get_is_indexing"),             &ProjectIndexer::get_is_indexing);
    ClassDB::bind_method(D_METHOD("get_progress"),                &ProjectIndexer::get_progress);
    ClassDB::bind_method(D_METHOD("get_files_indexed"),           &ProjectIndexer::get_files_indexed);
}

void ProjectIndexer::start_indexing(const String &p_root_path) {
    project_path = p_root_path;
    is_indexing  = true;
    files_total  = 0;
    files_done   = 0;
    _index_directory(p_root_path);
    is_indexing = false;
    emit_signal("indexing_complete", files_done);
}

void ProjectIndexer::stop_indexing() {
    is_indexing = false;
}

void ProjectIndexer::_index_directory(const String &path) {
    if (!is_indexing) return;
    Ref<DirAccess> dir = DirAccess::open(path);
    if (!dir.is_valid()) return;
    dir->list_dir_begin();
    String item = dir->get_next();
    while (!item.is_empty()) {
        if (item == "." || item == "..") { item = dir->get_next(); continue; }
        String full = path.path_join(item);
        if (dir->current_is_dir()) {
            _index_directory(full);
        } else {
            String ext = item.get_extension();
            if (ext == "gd" || ext == "nex" || ext == "tscn" || ext == "tres" || ext == "gdshader") {
                files_done++;
                emit_signal("file_indexed", full);
            }
        }
        item = dir->get_next();
    }
    dir->list_dir_end();
}

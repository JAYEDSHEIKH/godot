#include "session_manager.h"
#include "core/os/time.h"
#include "core/io/json.h"
#include "core/io/file_access.h"
#include "core/io/dir_access.h"

void SessionManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("new_session"),           &SessionManager::new_session);
    ClassDB::bind_method(D_METHOD("save_session"),          &SessionManager::save_session);
    ClassDB::bind_method(D_METHOD("clear_history"),         &SessionManager::clear_history);
    ClassDB::bind_method(D_METHOD("get_message_count"),     &SessionManager::get_message_count);
    ClassDB::bind_method(D_METHOD("get_total_tokens"),      &SessionManager::get_total_tokens);
    ClassDB::bind_method(D_METHOD("list_sessions"),         &SessionManager::list_sessions);
    ClassDB::bind_method(D_METHOD("build_api_messages"),    &SessionManager::build_api_messages);
}

String SessionManager::_generate_session_id() const {
    return String::num_int64(Time::get_singleton()->get_ticks_msec());
}

String SessionManager::_get_session_path(const String &id) const {
    return sessions_dir + id + ".json";
}

void SessionManager::new_session(const String &p_project_name) {
    current_session.session_id         = _generate_session_id();
    current_session.created_at_ms      = Time::get_singleton()->get_ticks_msec();
    current_session.last_accessed_at_ms = current_session.created_at_ms;
    current_session.project_name       = p_project_name;
    current_session.message_count      = 0;
    current_session.total_tokens_used  = 0;
    current_messages.clear();
}

void SessionManager::load_session(const String &p_session_id) {
    Error err;
    String content = FileAccess::get_file_as_string(_get_session_path(p_session_id), &err);
    if (err != OK) return;
    Variant parsed;
    String err_str;
    int err_line;
    if (JSON::parse(content, parsed, err_str, err_line) != OK) return;
    Dictionary d = parsed;
    current_session.session_id    = d.get("session_id", "").operator String();
    current_session.project_name  = d.get("project_name", "").operator String();
    current_messages.clear();
    Array msgs = d.get("messages", Array());
    for (int i = 0; i < msgs.size(); i++) {
        Dictionary m = msgs[i];
        AIMessage msg;
        msg.id          = m.get("id", "").operator String();
        msg.role        = m.get("role", "user").operator String();
        msg.content     = m.get("content", "").operator String();
        msg.tokens_used = m.get("tokens", 0).operator int();
        current_messages.push_back(msg);
    }
}

void SessionManager::save_session() {
    DirAccess::make_dir_recursive_absolute(sessions_dir);
    Dictionary d;
    d["session_id"]   = current_session.session_id;
    d["project_name"] = current_session.project_name;
    d["created_at_ms"]= current_session.created_at_ms;
    Array msgs;
    for (const AIMessage &m : current_messages) {
        Dictionary md;
        md["id"]      = m.id;
        md["role"]    = m.role;
        md["content"] = m.content;
        md["tokens"]  = m.tokens_used;
        msgs.push_back(md);
    }
    d["messages"] = msgs;
    Ref<FileAccess> f = FileAccess::open(_get_session_path(current_session.session_id), FileAccess::WRITE);
    if (f.is_valid()) f->store_string(JSON::stringify(d, "  "));
}

void SessionManager::delete_session(const String &p_session_id) {
    DirAccess::remove_absolute(_get_session_path(p_session_id));
}

void SessionManager::add_message(const String &role, const String &content, int tokens) {
    AIMessage msg;
    msg.id            = _generate_session_id() + "_" + role;
    msg.role          = role;
    msg.content       = content;
    msg.tokens_used   = tokens > 0 ? tokens : content.length() / 4;
    msg.timestamp_ms  = Time::get_singleton()->get_ticks_msec();
    current_messages.push_back(msg);
    current_session.message_count++;
    current_session.total_tokens_used += msg.tokens_used;
    current_session.last_accessed_at_ms = msg.timestamp_ms;
    if (current_messages.size() > max_history) {
        current_messages.remove_at(0);
    }
    emit_signal("message_added", msg.role, msg.content);
}

void SessionManager::clear_history() {
    current_messages.clear();
    current_session.message_count = 0;
}

Array SessionManager::list_sessions() const {
    Array sessions;
    Ref<DirAccess> dir = DirAccess::open(sessions_dir);
    if (!dir.is_valid()) return sessions;
    dir->list_dir_begin();
    String item = dir->get_next();
    while (!item.is_empty()) {
        if (item.ends_with(".json")) {
            sessions.push_back(sessions_dir + item);
        }
        item = dir->get_next();
    }
    dir->list_dir_end();
    return sessions;
}

Array SessionManager::build_api_messages(int p_max_messages) const {
    Array msgs;
    int start = MAX(0, current_messages.size() - p_max_messages);
    for (int i = start; i < current_messages.size(); i++) {
        Dictionary d;
        d["role"]    = current_messages[i].role;
        d["content"] = current_messages[i].content;
        msgs.push_back(d);
    }
    return msgs;
}

#pragma once
#include "core/object/ref_counted.h"
#include "modules/ai_engine/core/ai_types.h"

class SessionManager : public RefCounted {
    GDCLASS(SessionManager, RefCounted);

    SessionMetadata     current_session;
    Vector<AIMessage>   current_messages;
    String              sessions_dir = "user://.ai_engine/sessions/";
    int                 max_history  = 200;

public:
    void new_session(const String &p_project_name = "");
    void load_session(const String &p_session_id);
    void save_session();
    void delete_session(const String &p_session_id);

    void add_message(const String &role, const String &content, int tokens = 0);
    void clear_history();

    const Vector<AIMessage>  &get_messages() const { return current_messages; }
    const SessionMetadata    &get_current_session() const { return current_session; }
    Array                     list_sessions() const;

    // Build a messages array for the API call
    Array build_api_messages(int p_max_messages = 20) const;

    int get_message_count() const { return current_messages.size(); }
    int get_total_tokens()  const { return current_session.total_tokens_used; }

protected:
    static void _bind_methods();

private:
    String _get_session_path(const String &id) const;
    String _generate_session_id() const;
};

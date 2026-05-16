#pragma once
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"
#include "core/variant/array.h"
#include "core/templates/vector.h"

enum AIProviderType {
    AI_PROVIDER_OPENAI = 0,
    AI_PROVIDER_GEMINI = 1,
    AI_PROVIDER_LOCAL  = 2,
};

enum AIModelFamily {
    MODEL_FAMILY_GPT         = 0,
    MODEL_FAMILY_CLAUDE      = 1,
    MODEL_FAMILY_GEMINI      = 2,
    MODEL_FAMILY_OPEN_SOURCE = 3,
};

enum ToolExecutionStatus {
    TOOL_PENDING              = 0,
    TOOL_RUNNING              = 1,
    TOOL_SUCCESS              = 2,
    TOOL_FAILED               = 3,
    TOOL_CANCELLED            = 4,
    TOOL_REQUIRES_CONFIRMATION = 5,
};

enum ContextSectionPriority {
    PRIORITY_CRITICAL = 0,
    PRIORITY_HIGH     = 1,
    PRIORITY_MEDIUM   = 2,
    PRIORITY_LOW      = 3,
};

struct AIMessage {
    String   id;
    String   role;
    String   content;
    int64_t  timestamp_ms    = 0;
    int      tokens_used     = 0;
    int      input_tokens    = 0;
    int      output_tokens   = 0;
    Dictionary metadata;
    bool     is_streaming    = false;
    float    confidence      = 1.0f;
    Array    citations;
};

struct ToolParameter {
    String  name;
    String  type;
    String  description;
    bool    required      = true;
    Variant default_value;
    Array   enum_values;
    Dictionary validation;
};

struct ToolDefinition {
    String             name;
    String             description;
    String             category;
    bool               requires_confirmation = false;
    bool               is_dangerous          = false;
    Vector<ToolParameter> parameters;
    String             return_type;
    int                priority = 0;
};

struct ToolCall {
    String   id;
    String   name;
    Dictionary arguments;
    int64_t  requested_at_ms  = 0;
    int64_t  started_at_ms    = 0;
    int64_t  completed_at_ms  = 0;
    ToolExecutionStatus status = TOOL_PENDING;
    String   result;
    String   error_message;
    bool     requires_user_confirmation = false;
};

struct ContextBlock {
    String                section_name;
    String                content;
    ContextSectionPriority priority;
    int                   tokens           = 0;
    int64_t               generated_at_ms  = 0;
    bool                  is_stale         = false;
};

struct CodeChunk {
    String         file_path;
    String         chunk_id;
    int            line_start    = 0;
    int            line_end      = 0;
    String         source_code;
    Vector<float>  embedding;
    String         embedding_model;
    int64_t        indexed_at_ms = 0;
    String         content_hash;
};

struct SearchResult {
    String  file_path;
    String  chunk_id;
    float   relevance_score = 0.0f;
    String  preview;
    int     line_number     = 0;
    Array   keyword_matches;
};

struct SessionMetadata {
    String     session_id;
    int64_t    created_at_ms      = 0;
    int64_t    last_accessed_at_ms = 0;
    String     project_name;
    String     project_path;
    int        message_count       = 0;
    int        total_tokens_used   = 0;
    Dictionary tags;
};

struct MemoryFact {
    String  id;
    String  category;
    String  content;
    Array   related_files;
    int     confidence      = 100;
    int64_t created_at_ms   = 0;
    int     access_count    = 0;
};

struct AIMetrics {
    int   total_messages_sent      = 0;
    int   total_messages_received  = 0;
    int   total_tokens_used        = 0;
    int   total_tool_calls         = 0;
    int   successful_tool_calls    = 0;
    int   failed_tool_calls        = 0;
    float average_response_time_ms = 0.0f;
    float average_token_time_ms    = 0.0f;
    Dictionary provider_usage;
};

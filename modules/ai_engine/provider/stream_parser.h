#pragma once
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"

class StreamParser {
public:
    // Parse a server-sent events (SSE) chunk and extract content
    static String parse_sse_content(const String &p_raw_chunk);

    // Parse OpenAI streaming JSON delta
    static String parse_openai_delta(const String &p_json);

    // Parse Gemini streaming response
    static String parse_gemini_chunk(const String &p_json);

    // Check if stream is done (SSE "[DONE]" token)
    static bool is_stream_done(const String &p_chunk);

    // Extract tool calls from an OpenAI chunk
    static Array extract_tool_calls(const String &p_json);
};

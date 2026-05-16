#include "stream_parser.h"
#include "core/io/json.h"

String StreamParser::parse_sse_content(const String &p_raw_chunk) {
    String result;
    Vector<String> lines = p_raw_chunk.split("\n");
    for (const String &line : lines) {
        if (line.begins_with("data: ")) {
            String data = line.substr(6).strip_edges();
            if (data != "[DONE]") {
                result += parse_openai_delta(data);
            }
        }
    }
    return result;
}

String StreamParser::parse_openai_delta(const String &p_json) {
    Variant parsed;
    String err_str;
    int err_line;
    if (JSON::parse(p_json, parsed, err_str, err_line) == OK) {
        if (parsed.get_type() == Variant::DICTIONARY) {
            Dictionary d = parsed;
            Array choices = d.get("choices", Array());
            if (choices.size() > 0) {
                Dictionary choice = choices[0];
                Dictionary delta = choice.get("delta", Dictionary());
                return delta.get("content", "").operator String();
            }
        }
    }
    return "";
}

String StreamParser::parse_gemini_chunk(const String &p_json) {
    Variant parsed;
    String err_str;
    int err_line;
    if (JSON::parse(p_json, parsed, err_str, err_line) == OK) {
        if (parsed.get_type() == Variant::DICTIONARY) {
            Dictionary d = parsed;
            Array candidates = d.get("candidates", Array());
            if (candidates.size() > 0) {
                Dictionary cand = candidates[0];
                Dictionary content = cand.get("content", Dictionary());
                Array parts = content.get("parts", Array());
                String text;
                for (int i = 0; i < parts.size(); i++) {
                    Dictionary part = parts[i];
                    text += part.get("text", "").operator String();
                }
                return text;
            }
        }
    }
    return "";
}

bool StreamParser::is_stream_done(const String &p_chunk) {
    return p_chunk.contains("[DONE]");
}

Array StreamParser::extract_tool_calls(const String &p_json) {
    Variant parsed;
    String err_str;
    int err_line;
    Array result;
    if (JSON::parse(p_json, parsed, err_str, err_line) == OK) {
        if (parsed.get_type() == Variant::DICTIONARY) {
            Dictionary d = parsed;
            Array choices = d.get("choices", Array());
            if (choices.size() > 0) {
                Dictionary choice = choices[0];
                Dictionary delta = choice.get("delta", Dictionary());
                if (delta.has("tool_calls")) return delta["tool_calls"];
            }
        }
    }
    return result;
}

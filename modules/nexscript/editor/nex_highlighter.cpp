#include "nex_highlighter.h"
#ifdef TOOLS_ENABLED
#include "scene/gui/syntax_highlighter.h"

PackedStringArray NexSyntaxHighlighter::_get_supported_languages() const {
    PackedStringArray langs;
    langs.push_back("NEX");
    return langs;
}

Dictionary NexSyntaxHighlighter::_get_line_syntax_highlighting_impl(int p_line) {
    // Minimum viable: return empty dict so the editor does not crash.
    // A full implementation should instantiate a CodeHighlighter, configure
    // keyword/number/string/comment colors from NexLanguage::get_reserved_words(),
    // and delegate to it — see gdscript_highlighter.cpp for the exact pattern.
    return Dictionary();
}
#endif // TOOLS_ENABLED

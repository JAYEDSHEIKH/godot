#pragma once
#ifdef TOOLS_ENABLED
#include "editor/plugins/script_editor_plugin.h"
#include "scene/gui/text_edit.h"

class NexSyntaxHighlighter : public EditorSyntaxHighlighter {
    GDCLASS(NexSyntaxHighlighter, EditorSyntaxHighlighter);

public:
    Dictionary _get_line_syntax_highlighting_impl(int p_line) override;

    String _get_name() const override { return "NEX"; }
    PackedStringArray _get_supported_languages() const override;

protected:
    static void _bind_methods() {}
};
#endif // TOOLS_ENABLED

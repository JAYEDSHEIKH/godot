#pragma once
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"

class SceneAnalyzer {
public:
    static String get_scene_tree_json();
    static String get_node_info(const String &p_path);
    static Dictionary analyze_node_types();
    static int count_nodes();
};

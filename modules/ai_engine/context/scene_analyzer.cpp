#include "scene_analyzer.h"

String SceneAnalyzer::get_scene_tree_json() {
    return "{ \"root\": null }";
}

String SceneAnalyzer::get_node_info(const String &p_path) {
    return "{ \"path\": \"" + p_path + "\", \"type\": \"Node\" }";
}

Dictionary SceneAnalyzer::analyze_node_types() {
    return Dictionary();
}

int SceneAnalyzer::count_nodes() {
    return 0;
}

# Fix 12 — register doc classes so `scons doc` picks them up
def can_build(env, platform):
    return True

def configure(env):
    pass

def get_doc_classes():
    return [
        "AIEngine",
        "AIProvider",
        "ToolExecutor",
        "VectorDB",
        "MemoryManager",
        "ContextManager",
        "SessionManager",
        "ConfigManager",
    ]

def get_doc_path():
    return "doc_classes"

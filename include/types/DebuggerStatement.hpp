#pragma once

#include <node.hpp>
#include <defs.h>


struct DebuggerStatement : Node {
    DebuggerStatement(Session* session);

    void render(SitixWriter* out, Object* scope, bool dereference);

    void pTree(int tabLevel);
};
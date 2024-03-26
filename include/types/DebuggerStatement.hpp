#pragma once

#include <node.hpp>
#include <defs.h>


struct DebuggerStatement : Node {
    DebuggerStatement(Session* session);

    virtual void render(SitixWriter* out, Object* scope, bool dereference);
};
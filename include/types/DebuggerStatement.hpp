#pragma once

#include <node.hpp>


struct DebuggerStatement : Node {
    virtual void render(SitixWriter* out, Object* scope, bool dereference);
};
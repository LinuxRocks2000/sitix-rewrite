#pragma once
#include <node.hpp>
#include <string>
#include <defs.h>


struct Dereference : Node { // dereference and render an Object (the [^] operator)
    std::string name;

    Dereference(Session* session);

    void render(SitixWriter* out, Object* scope, bool dereference);
};
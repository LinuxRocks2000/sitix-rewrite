#pragma once

#include <node.hpp>
#include <string>
#include <sitixwriter.hpp>
#include <defs.h>


struct Copier : Node {
    std::string target;
    std::string object;

    Copier(Session* session);

    void render(SitixWriter* out, Object* scope, bool dereference);
};
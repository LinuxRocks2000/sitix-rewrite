#pragma once

#include <node.hpp>
#include <string>
#include <sitixwriter.hpp>


struct Copier : Node {
    std::string target;
    std::string object;

    void render(SitixWriter* out, Object* scope, bool dereference);
};
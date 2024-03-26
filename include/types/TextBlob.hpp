#pragma once

#include <string>
#include <node.hpp>
#include <sitixwriter.hpp>
#include <defs.h>


struct TextBlob : Node { // designed for smaller, heap-allocated text bits that it frees (not suitable for memory maps)
    std::string data;

    TextBlob(Session*);

    void render(SitixWriter* out, Object* scope, bool dereference);

    void pTree(int tabLevel = 0);
};
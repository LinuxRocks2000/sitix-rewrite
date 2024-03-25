#pragma once

#include <string>
#include <node.hpp>
#include <sitixwriter.hpp>
struct Object;


struct TextBlob : Node { // designed for smaller, heap-allocated text bits that it frees (not suitable for memory maps)
    std::string data;

    TextBlob();

    void render(SitixWriter* out, Object* scope, bool dereference);

    void pTree(int tabLevel = 0);
};
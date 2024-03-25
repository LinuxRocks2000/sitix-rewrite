#pragma once

#include <node.hpp>
#include <mapview.hpp>
#include <sitixwriter.hpp>
struct Object; // forward-dec


struct PlainText : Node {
    MapView data;

    PlainText(MapView d);

    void render(SitixWriter* stream, Object* scope, bool dereference);

    void pTree(int tabLevel = 0);
};
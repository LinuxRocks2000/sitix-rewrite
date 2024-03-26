#pragma once

#include <node.hpp>
#include <mapview.hpp>
#include <sitixwriter.hpp>
#include <defs.h>


struct PlainText : Node {
    MapView data;

    PlainText(Session*, MapView d);

    void render(SitixWriter* stream, Object* scope, bool dereference);

    void pTree(int tabLevel = 0);
};
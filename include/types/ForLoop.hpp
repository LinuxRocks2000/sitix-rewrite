#pragma once
#include <string>
#include <node.hpp>
#include <defs.h>
#include <mapview.hpp>


struct ForLoop : Node {
    std::string goal; // the name of the object we're going to iterate over
    std::string iteratorName; // the name of the object we're going to create as an iterator when this loop is rendered
    Object* internalObject; // the object we're going to render at every point in the loop

    ~ForLoop();

    ForLoop(MapView tagData, MapView& map, FileFlags* flags);

    void attachToParent(Object* thing);

    void render(SitixWriter* out, Object* scope, bool dereference);

    virtual void pTree(int tabLevel = 0);
};
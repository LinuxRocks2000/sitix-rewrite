#pragma once
#include <node.hpp>
#include <defs.h>


struct RedirectorStatement : Node {
    Object* object;
    MapView evalsCommand;

    ~RedirectorStatement();

    RedirectorStatement(MapView& map, MapView command, FileFlags* flags);

    void attachToParent(Object* p);

    void render(SitixWriter*, Object* scope, bool dereference);
};
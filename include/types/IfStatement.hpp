#pragma once
#include <node.hpp>
#include <defs.h>
#include <mapview.hpp>
#include <fileflags.h>
#include <sitixwriter.hpp>


struct IfStatement : Node {
    Object* mainObject;
    Object* elseObject = NULL;

    MapView evalsCommand;

    IfStatement(Session*, MapView& map, MapView command, FileFlags *flags);

    void attachToParent(Object* p);

    ~IfStatement();

    void render(SitixWriter* out, Object* scope, bool dereference);
};
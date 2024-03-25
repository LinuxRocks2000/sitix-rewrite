#pragma once
#include <fileflags.h>
#include <sitixwriter.hpp>

struct Object; // forward-dec


struct Node { // superclass
    virtual ~Node();

    Object* parent = NULL; // everything has an Object parent, except the root Object (which will have a NULL parent)

    FileFlags fileflags;
    enum Type {
        OTHER = 1,
        PLAINTEXT = 3,
        TEXTBLOB = 5,
        OBJECT = 7
    } type = OTHER; // it's necessary to specificate from Node to plaintext, object, etc in some cases.

    virtual void render(SitixWriter* stream, Object* scope, bool dereference = false) = 0; // true virtual function
    // scope is a SECONDARY scope. If a lookup fails in the primary scope (the parent), scope lookup will be performed in the secondary scope.
    // dereference causes forced rendering on objects (objects don't render by default unless dereferenced with [^name])

    virtual void debugPrint(); // optional

    virtual void pTree(int tabLevel = 0);

    virtual void attachToParent(Object* parent);
};
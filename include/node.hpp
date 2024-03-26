#pragma once
#include <fileflags.h>
#include <sitixwriter.hpp>

struct Object; // forward-declarations
struct Session;

struct Node { // superclass
    virtual ~Node();

    Object* parent = NULL; // everything has an Object parent, except the root Object (which will have a NULL parent)
    Session* sitix;

    Node(Session* session) {
        sitix = session;
    }

    FileFlags fileflags;
    enum Type {
        OTHER,
        PLAINTEXT,
        TEXTBLOB,
        OBJECT
    } type = OTHER; // it's occasionally necessary to promote from Node to PlainText, Object, etc.

    virtual void render(SitixWriter* stream, Object* scope, bool dereference = false) = 0; // true virtual function
    // scope is a SECONDARY scope. If a lookup fails in the primary scope (the parent), scope lookup will be performed in the secondary scope.
    // dereference causes forced rendering on objects (objects don't render by default unless dereferenced with [^name])

    virtual void debugPrint(); // optional

    virtual void pTree(int tabLevel = 0);

    virtual void attachToParent(Object* parent);
};
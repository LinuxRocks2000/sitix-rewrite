#pragma once

#include <node.hpp>
#include <vector>
#include <string>
#include <sitixwriter.hpp>


struct Object : Node { // Sitix objects contain a list of *nodes*, which can be enumerated (like for array reference), named (for variables), operations, or pure text.
    // Sitix objects are only converted to text when it's absolutely necessary; e.g. when the root object is rendered. Scoping is KISS - when an object resolve
    // is requested, we walk down the chain (towards the root) until we hit an object containing the right name, then walk up to match nested variables.
    // Variable dereferences are considered operations and will not be executed until the relevant section is rendered; it is important to make sure your
    // Sitix code is aware of the scope caveats at all times.
    std::vector<Node*> children;
    bool isTemplate = false;
    bool isFile = false;
    uint32_t highestEnumerated = 0; // the highest enumerated value in this object

    Object* ghost = NULL; // if this is not-null, the current object is a "ghost" of that object - everything resolves into the ghost.

    bool virile = true; // does it call replace()?

    int rCount = 1;

    Object();

    void pushedOut();

    std::string name; // a union would save some bytes of space but would cause annoying crap with the std::string name.
    uint32_t number;

    enum NamingScheme {
        Named, // it has a real name, which is contained in std::string name
        Enumerated, // it has a number assigned by the parent
        Virtual // it's contained inside a logical operation or something similar
    } namingScheme = Virtual;

    ~Object();

    void render(SitixWriter* out, Object* scope, bool dereference);

    void addChild(Node* child);

    void dropObject(Object* object);

    Object* lookup(std::string& lname, Object* nope = NULL);

    Object* childSearchUp(const char* name);

    bool ptrEquals(Object* thing);

    void pTree(int tabLevel = 0);

    void ptrswap(Object* current, Object* repl);

    Object* deghost();

    Object* walkToFile();

    bool replace(std::string& name, Object* obj);

    void debugPrint();

    Object* nonvRoot();
};


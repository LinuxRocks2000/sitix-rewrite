// "type" DECLARATIONS for Evals
#pragma once
#include <evals/core.hpp>
#include <evals/ops.hpp>
#include <defs.h>


struct ErrorObject : EvalsObject {
    ErrorObject();

    bool equals(EvalsObject*);

    std::string toString();

    bool truthyness();

    EvalsObject* copy();
};


struct StringObject : EvalsObject {
    std::string content;

    StringObject(std::string data);

    bool equals(EvalsObject* thing);

    std::string toString();

    bool truthyness();

    EvalsObject* copy();
};


struct NumberObject : EvalsObject {
    double content;

    NumberObject(double data);

    bool equals(EvalsObject* thing);

    std::string toString();

    bool truthyness();

    EvalsObject* copy();
};


struct BooleanObject : EvalsObject {
    bool content;

    BooleanObject(bool value);

    bool equals(EvalsObject* thing);

    std::string toString();

    bool truthyness();

    EvalsObject* copy();
};


struct SitixVariableObject : EvalsObject {
    Object* content;
    Object* scope;

    SitixVariableObject(Object* obj, Object* sc);

    bool equals(EvalsObject* thing);

    std::string toString();

    bool truthyness();

    EvalsObject* copy();
};


struct EvalsFunction : EvalsObject {
    ~EvalsFunction();

    std::vector<EvalsOperation*> program;

    EvalsFunction(MapView&, Object*, Object*); // construct this function from a mapview. it will consume the mapview up till either the end or a closing ")".

    EvalsFunction(EvalsFunction&) = default; // copy-constructor

    bool equals(EvalsObject* thing);

    bool truthyness();

    std::string toString();
    
    EvalsObject* copy();

    void exec(EvalsStackType);
};

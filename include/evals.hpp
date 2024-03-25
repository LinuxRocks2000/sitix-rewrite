// Evals is a simple, turing-incomplete evaluated language designed for string processing. It directly integrates with and depends on the Sitix scope model.
// Evals is primarily designed for string manipulation and value comparison.
// It is not fancy; the syntax is redolent of Tyler++.
#include <vector>
#include <string>
#include <math.h>
#include <sitixwriter.hpp>
#include <mapview.hpp>
#include <types/Object.hpp>
#include <defs.h>
#include <util.hpp>


struct EvalsObject {
    enum Type {
        Number,
        String,
        Boolean,
        Error,
        SitixVariable
    } type;

    virtual bool equals(EvalsObject* obj) = 0;

    virtual std::string toString() = 0;

    virtual bool truthyness() = 0;

    virtual EvalsObject* copy() = 0;
};


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


struct EvalsSession {
    Object* parent;
    Object* scope;

    EvalsObject* render(MapView data);
};


struct EvalsBlob : Node {
    MapView data;

    EvalsBlob(MapView d);

    void render(SitixWriter* out, Object* scope, bool dereference);
};

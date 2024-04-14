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
    enum Type : int {
        Number        = 1,
        String        = 2,
        Boolean       = 4,
        Error         = 8,
        SitixVariable = 16
    } type; // bitbangable

    virtual bool equals(EvalsObject* obj) = 0;

    virtual std::string toString() = 0;

    virtual bool truthyness() = 0;

    virtual EvalsObject* copy() = 0;
};


typedef std::vector<EvalsObject*>& EvalsStackType;


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


struct EvalsOperation {
    virtual ~EvalsOperation(){}

    EvalsObject* atop(std::vector<EvalsObject*>& stack, EvalsObject::Type type = (EvalsObject::Type)0xFFFFFFFF, int searchDepth = 1);

    virtual void run(std::vector<EvalsObject*>& stack) = 0;

    void parseBinary(EvalsStackType, EvalsObject::Type t1 = (EvalsObject::Type)0xFFFFFFFF, EvalsObject::Type t2 = (EvalsObject::Type)0xFFFFFFFF);

    virtual void binary(EvalsStackType, EvalsObject* one, EvalsObject* two) {};
};


struct EvalsFunction : EvalsOperation {
    ~EvalsFunction();

    std::vector<EvalsOperation*> program;

    EvalsFunction(MapView&, Object*, Object*); // construct this function from a mapview. it will consume the mapview up till either the end or a closing ).

    void run(std::vector<EvalsObject*>&);
};


struct StackPush : EvalsOperation {
    EvalsObject* data;

    StackPush(EvalsObject*);

    void run(std::vector<EvalsObject*>&);
};


struct EqualityCheck : EvalsOperation {
    void run(std::vector<EvalsObject*>&);

    void binary(EvalsStackType, EvalsObject* one, EvalsObject* two);
};


struct NotOperation : EvalsOperation {
    void run(std::vector<EvalsObject*>&);
};


struct ConcatOperation : EvalsOperation {
    void run(std::vector<EvalsObject*>&);

    void binary(EvalsStackType, EvalsObject* one, EvalsObject* two);
};


struct CopyOperation : EvalsOperation {
    void run(std::vector<EvalsObject*>&);
};


struct FilenameifyOperation : EvalsOperation {
    void run(EvalsStackType);
};


struct TrimOperation : EvalsOperation {
    void run(EvalsStackType);
};


struct Counter : EvalsOperation {
    int off;
    int t;

    Counter(int, int);

    void run(std::vector<EvalsObject*>&);

    void binary(EvalsStackType, EvalsObject* one, EvalsObject* two);
};


struct Slicer : EvalsOperation {
    enum { // given the string "hello, world" with the comma set as a breakpoint,
        LeftLow,    // would yield "hello"
        LeftHigh,   // would yield "hello,"
        RightLow,   // would yield " world"
        RightHigh,  // would yield ", world"
    } mode;

    template<typename Enum>
    Slicer(Enum m);

    void run(std::vector<EvalsObject*>&);

    void binary(EvalsStackType, EvalsObject* one, EvalsObject* two);
};



struct EvalsSession {
    Object* parent;
    Object* scope;

    EvalsObject* render(MapView data);
};


struct EvalsBlob : Node {
    MapView data;

    EvalsBlob(Session*, MapView d);

    void render(SitixWriter* out, Object* scope, bool dereference);
};

// operation DECLARATIONS for evals
#pragma once
#include <evals/core.hpp>
#include <mapview.hpp>
#include <defs.h>


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
    enum Type { // given the string "hello, world" with the comma set as a breakpoint,
        LeftLow,    // would yield "hello"
        LeftHigh,   // would yield "hello,"
        RightLow,   // would yield " world"
        RightHigh,  // would yield ", world"
    } mode;

    Slicer(Type m);

    void run(std::vector<EvalsObject*>&);

    void binary(EvalsStackType, EvalsObject* one, EvalsObject* two);
};
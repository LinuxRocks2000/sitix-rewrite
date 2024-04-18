// operation DECLARATIONS for evals
#pragma once
#include <evals/core.hpp>
#include <mapview.hpp>
#include <defs.h>


struct StackPush : EvalsOperation {
    EvalsObject* data;

    StackPush(EvalsObject*);

    void run(EvalsStackType);
};


struct EqualityCheck : EvalsOperation {
    void run(EvalsStackType);

    void binary(EvalsStackType, EvalsObject* one, EvalsObject* two);
};


struct NotOperation : EvalsOperation {
    void run(EvalsStackType);
};


struct ConcatOperation : EvalsOperation {
    void run(EvalsStackType);

    void binary(EvalsStackType, EvalsObject* one, EvalsObject* two);
};


struct CopyOperation : EvalsOperation {
    void run(EvalsStackType);
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

    void run(EvalsStackType);

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

    void run(EvalsStackType);

    void binary(EvalsStackType, EvalsObject* one, EvalsObject* two);
};


struct CallOperation : EvalsOperation {
    void run(EvalsStackType);
};


struct SwapOperation : EvalsOperation {
    void run(EvalsStackType);

    void binary(EvalsStackType, EvalsObject*, EvalsObject*);
};


struct DeleteOperation : EvalsOperation {
    void run(EvalsStackType);
};
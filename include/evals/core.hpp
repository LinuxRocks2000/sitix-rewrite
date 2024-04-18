// core superclasses for Evals
#pragma once
#include <vector>
#include <string>

struct EvalsObject { // Core base class for all Evals objects
    enum Type : int {
        Number        = 1,
        String        = 2,
        Boolean       = 4,
        Error         = 8,
        SitixVariable = 16,
        Function      = 32
    } type; // bitbangable

    virtual bool equals(EvalsObject* obj) = 0;

    virtual std::string toString() = 0;

    virtual bool truthyness() = 0;

    virtual EvalsObject* copy() = 0;
};


typedef std::vector<EvalsObject*>& EvalsStackType;


struct EvalsOperation { // Core base class for Evals operations
    virtual ~EvalsOperation(){}

    EvalsObject* atop(EvalsStackType stack, EvalsObject::Type type = (EvalsObject::Type)0xFFFFFFFF, int searchDepth = 1);

    virtual void run(EvalsStackType stack) = 0;

    void parseBinary(EvalsStackType, bool preserve = false, EvalsObject::Type t1 = (EvalsObject::Type)0xFFFFFFFF, EvalsObject::Type t2 = (EvalsObject::Type)0xFFFFFFFF);

    virtual void binary(EvalsStackType, EvalsObject* one, EvalsObject* two) {};
};


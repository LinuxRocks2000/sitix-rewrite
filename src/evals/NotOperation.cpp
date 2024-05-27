#include <evals/evals.hpp>
#ifdef INLINE_MODE_EVALS


void NotOperation::run(EvalsStackType stack) {
    EvalsObject* thing = atop(stack);
    stack.push_back(new BooleanObject(thing == NULL ? true : !thing -> truthyness()));
    delete thing;
}

#endif
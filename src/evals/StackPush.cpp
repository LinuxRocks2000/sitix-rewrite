#include <evals/evals.hpp>
#ifdef INLINE_MODE_EVALS


StackPush::StackPush(EvalsObject* obj) : data(obj){}

void StackPush::run(EvalsStackType stack) {
    stack.push_back(data);
}

#endif
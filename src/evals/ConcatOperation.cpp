#include <evals/evals.hpp>
#ifdef INLINE_MODE_EVALS


void ConcatOperation::run(EvalsStackType stack) {
    parseBinary(stack);
}

void ConcatOperation::binary(EvalsStackType stack, EvalsObject* one, EvalsObject* two) {
    stack.push_back(new StringObject(two -> toString() + one -> toString()));
}

#endif
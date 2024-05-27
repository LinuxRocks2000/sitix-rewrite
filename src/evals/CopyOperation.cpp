#include <evals/evals.hpp>
#ifdef INLINE_MODE_EVALS


void CopyOperation::run(EvalsStackType stack) {
    stack.push_back(stack[stack.size() - 1] -> copy());
}

#endif
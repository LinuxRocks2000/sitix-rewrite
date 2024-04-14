#include <evals/evals.hpp>


void CopyOperation::run(EvalsStackType stack) {
    stack.push_back(stack[stack.size() - 1] -> copy());
}
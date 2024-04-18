#include <evals/ops.hpp>


void SwapOperation::run(EvalsStackType stack) {
    parseBinary(stack, true);
}


void SwapOperation::binary(EvalsStackType stack, EvalsObject* one, EvalsObject* two) {
    stack.push_back(one); // two was the "deeper" one, so now it's being swapped with the "shallower" one
    stack.push_back(two);
}
#include <evals/evals.hpp>


void EqualityCheck::run(EvalsStackType stack) {
    parseBinary(stack);
}

void EqualityCheck::binary(EvalsStackType stack, EvalsObject* one, EvalsObject* two) {
    if (one == NULL || two == NULL) {
        if (one != NULL) { delete one; }
        if (two != NULL) { delete two; }
        stack.push_back(new ErrorObject);
        printf(ERROR "Bad Evals program!\n\tNot enough data on Evals stack to check equality!\n");
        return;
    }
    stack.push_back(new BooleanObject(one -> equals(two) || two -> equals(one)));
}
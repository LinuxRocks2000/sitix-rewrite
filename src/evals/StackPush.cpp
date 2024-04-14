#include <evals/evals.hpp>


StackPush::StackPush(EvalsObject* obj) : data(obj){}

void StackPush::run(EvalsStackType stack) {
    stack.push_back(data);
}
#include <evals/ops.hpp>
#include <evals/types.hpp>


void CallOperation::run(EvalsStackType stack) {
    EvalsFunction* func = (EvalsFunction*)atop(stack, EvalsObject::Type::Function, 1);
    if (func == NULL) {
        printf(ERROR "No callable Evals function on stack!\n\tThis may be a stack alignment issue.\n");
    }
    func -> exec(stack);
}
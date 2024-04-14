// Evals is a simple, turing-incomplete evaluated language designed for string processing. It directly integrates with and depends on the Sitix scope model.
// Evals is primarily designed for string manipulation and value comparison.
// It is not fancy; the syntax is redolent of Tyler++.
#include <evals/evals.hpp>
#include <types/PlainText.hpp>


EvalsObject* EvalsOperation::atop(EvalsStackType stack, EvalsObject::Type type, int searchDepth) { // search down from the top of the stack for an item, and pop it out.
    // If it encounters an object where (object -> type & type), it will return that object.
    // It will not search further than searchDepth.
    // If no match is found, it will return NULL.
    for (size_t i = 0; i < searchDepth && i < stack.size(); i ++) {
        if (stack[stack.size() - i - 1] -> type & type) {
            EvalsObject* obj = stack[stack.size() - i - 1];
            stack[stack.size() - i - 1] = stack[stack.size() - 1]; // swapout
            stack.pop_back();
            return obj;
        }
    }
    return NULL;
}

void EvalsOperation::parseBinary(EvalsStackType stack, EvalsObject::Type t1, EvalsObject::Type t2) {
    EvalsObject* one = atop(stack, t1, 2);
    EvalsObject* two = atop(stack, t2, 1);
    if (one == NULL || two == NULL) {
        if (one != NULL) { delete one; }
        if (two != NULL) { delete two; }
        stack.push_back(new ErrorObject);
        printf(ERROR "Bad Evals program!\n\tNot enough data on stack to perform a binary operation!\n\tThis can also be caused by incorrect datatypes on the stack.\n");
        return;
    }
    binary(stack, one, two);
    delete one;
    delete two;
}


EvalsObject* EvalsSession::render(MapView data) { // all Evals commands produce an EvalsObject, so we can be sure that we'll be returning an EvalsObject
    std::vector<EvalsObject*> stack;
    EvalsFunction func(data, parent, scope);
    func.run(stack);
    if (stack.size() != 1) {
        printf(ERROR "Bad Evals program!\n");
        printf("\tstack size is %d, must be 1\n", stack.size());
        for (EvalsObject* o : stack) {
            delete o;
        }
        return new ErrorObject();
    }
    return stack[0];
}



EvalsBlob::EvalsBlob(Session* session, MapView d) : Node(session), data(d) {};

void EvalsBlob::render(SitixWriter* out, Object* scope, bool dereference) {
    EvalsSession session{parent, scope};
    EvalsObject* result = session.render(data);
    out -> write(result -> toString());
    delete result;
}

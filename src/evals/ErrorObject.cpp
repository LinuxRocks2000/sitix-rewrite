#include <evals/types.hpp>


ErrorObject::ErrorObject() {
    type = Type::Error;
}

bool ErrorObject::equals(EvalsObject*) {
    return false;
}

std::string ErrorObject::toString() {
    return "[ ERROR ]";
}

bool ErrorObject::truthyness() {
    return false;
}

EvalsObject* ErrorObject::copy() {
    return new ErrorObject;
}

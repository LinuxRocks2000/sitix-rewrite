#include <evals/types.hpp>


BooleanObject::BooleanObject(bool value) {
    content = value;
    type = Type::Boolean;
}

bool BooleanObject::equals(EvalsObject* thing) {
    if (thing -> type != type) {
        return false;
    }
    return ((BooleanObject*)thing) -> content == content;
}

std::string BooleanObject::toString() {
    return content ? "true" : "false";
}

bool BooleanObject::truthyness() {
    return content;
}

EvalsObject* BooleanObject::copy() {
    return new BooleanObject(content);
}
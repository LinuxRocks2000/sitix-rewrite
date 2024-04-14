#include <evals/types.hpp>


NumberObject::NumberObject(double data) {
    content = data;
    type = Type::Number;
}

bool NumberObject::equals(EvalsObject* thing) {
    if (thing -> type != type) {
        return false;
    }
    return ((NumberObject*)thing) -> content == content;
}

std::string NumberObject::toString() {
    return std::to_string(content);
}

bool NumberObject::truthyness() {
    return content != 0; // 0 is falsey, everything else is truthy
}

EvalsObject* NumberObject::copy() {
    return new NumberObject(content);
}
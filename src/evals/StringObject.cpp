#include <evals/types.hpp>


StringObject::StringObject(std::string data) {
    content = data;
    type = Type::String;
}

bool StringObject::equals(EvalsObject* thing) {
    return thing -> toString() == content;
}

std::string StringObject::toString() {
    return content;
}

bool StringObject::truthyness() { // empty strings are falsey, but otherwise strings are always truthy
    return content.size() > 0;
}

EvalsObject* StringObject::copy() {
    return new StringObject(content);
}
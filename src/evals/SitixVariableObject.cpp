#include <evals/types.hpp>
#include <types/Object.hpp>


SitixVariableObject::SitixVariableObject(Object* obj, Object* sc) {
    scope = sc;
    content = obj == NULL ? NULL : obj -> deghost();
    type = Type::SitixVariable;
}

bool SitixVariableObject::equals(EvalsObject* thing) {
    if (content == NULL) {
        return false;
    }
    if (thing -> type == Type::SitixVariable && ((SitixVariableObject*)thing) -> content != NULL) {
        auto frend = ((SitixVariableObject*)thing) -> content -> deghost();
        if (frend == content) { // if they point to the same data, they're the same variable
            return true;
        }
        if (thing -> toString() != toString()) { // bad solution but eh
            return false;
        }
        return true;
    }
    return false;
}

std::string SitixVariableObject::toString() {
    if (content == NULL) {
        return "";
    }
    StringWriteOutput out;
    SitixWriter writer(out);
    content -> render(&writer, scope, true);
    return out.content.c_str();
}

bool SitixVariableObject::truthyness() {
    return content != NULL;
}

EvalsObject* SitixVariableObject::copy() {
    return new SitixVariableObject(content, scope);
}

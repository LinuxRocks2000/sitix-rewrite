#include <types/Dereference.hpp>
#include <types/Object.hpp>


void Dereference::render(SitixWriter* out, Object* scope, bool dereference) {
    Object* found = parent -> lookup(name);
    if (found == NULL) {
        found = scope -> lookup(name);
    }
    if (found == NULL) {
        printf(ERROR "Couldn't find %s! The output \033[1mwill\033[0m be malformed.\n", name.c_str());
        return;
    }
    if (found -> isFile) { // dereferencing file roots copies over all their objects to you immediately
        for (Node* thing : found -> children) {
            if (thing -> type == Node::Type::OBJECT) {
                Object* o = (Object*)thing;
                if (!o -> virile) {
                    continue;
                }
                if (!scope -> replace(o -> name, o)) {
                    o -> rCount ++;
                    scope -> addChild(o);
                }
            }
        }
    }
    found -> render(out, parent, true);
}
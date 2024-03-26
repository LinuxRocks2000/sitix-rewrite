#include <defs.h>
#include <types/Copier.hpp>
#include <types/Object.hpp>


void Copier::render(SitixWriter* out, Object* scope, bool dereference) {
    Object* t = parent -> lookup(target);
    if (t == NULL) {
        t = scope -> lookup(target);
    }
    if (t == NULL) {
        printf(ERROR "Couldn't find %s for a copy operation. The output will be malformed.\n", target);
    }
    Object* o = parent -> lookup(object);
    if (o == NULL) {
        o = scope -> lookup(object);
    }
    if (o == NULL) {
        printf(ERROR "Couldn't find %s for a copy operation. The output will be malformed.\n", object);
    }
    o -> rCount ++;
    t -> ghost = o;
}

Copier::Copier(Session* session) : Node(session){}
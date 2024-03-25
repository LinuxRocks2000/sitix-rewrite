#include <types/IfStatement.hpp>
#include <evals.hpp>
#include <types/Object.hpp>


IfStatement::IfStatement(MapView& map, MapView command, FileFlags *flags) : evalsCommand(command) {
    mainObject = new Object;
    fileflags = *flags;
    if (fillObject(map, mainObject, flags) == FILLOBJ_EXIT_ELSE) {
        elseObject = new Object;
        fillObject(map, elseObject, flags);
        elseObject -> fileflags = *flags;
    }
    mainObject -> fileflags = *flags;
}

void IfStatement::attachToParent(Object* p) { 
    if (elseObject != NULL) {
        elseObject -> parent = p;
    }
    mainObject -> parent = p;
}

IfStatement::~IfStatement() {
    mainObject -> pushedOut();
    if (elseObject != NULL) {
        elseObject -> pushedOut();
    }
}

void IfStatement::render(SitixWriter* out, Object* scope, bool dereference) {
    EvalsSession session { parent, scope };
    EvalsObject* cond = session.render(evalsCommand);
    if (cond -> truthyness()) {
        mainObject -> render(out, scope, true);
    }
    else if (elseObject != NULL) {
        elseObject -> render(out, scope, true);
    }
    free(cond);
}
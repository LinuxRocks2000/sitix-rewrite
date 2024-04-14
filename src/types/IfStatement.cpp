#include <types/IfStatement.hpp>
#include <evals/evals.hpp>
#include <types/Object.hpp>


IfStatement::IfStatement(Session* session, MapView& map, MapView command, FileFlags *flags) : Node(session), evalsCommand(command) {
    mainObject = new Object(session);
    fileflags = *flags;
    if (fillObject(map, mainObject, flags, session) == FILLOBJ_EXIT_ELSE) {
        elseObject = new Object(session);
        fillObject(map, elseObject, flags, session);
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
    delete mainObject;
    if (elseObject != NULL) {
        delete elseObject;
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
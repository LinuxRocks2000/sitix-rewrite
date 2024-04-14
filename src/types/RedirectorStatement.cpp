#include <types/RedirectorStatement.hpp>
#include <types/Object.hpp>
#include <evals/evals.hpp>
#include <fcntl.h>
#include <util.hpp>
#include <unistd.h>
#include <session.hpp>


RedirectorStatement::~RedirectorStatement() {
    delete object;
}

RedirectorStatement::RedirectorStatement(Session* session, MapView& map, MapView command, FileFlags* flags) : Node(session), evalsCommand(command) {
    object = new Object(session);
    object -> fileflags = *flags;
    fileflags = *flags;
    fillObject(map, object, flags, session);
}

void RedirectorStatement::attachToParent(Object* p) {
    object -> parent = p;
}

void RedirectorStatement::render(SitixWriter*, Object* scope, bool dereference) {
    EvalsSession session { parent, scope };
    EvalsObject* result = session.render(evalsCommand);
    FileWriteOutput file = sitix -> create(result -> toString());
    delete result;
    SitixWriter writer(file);
    object -> render(&writer, scope, true);
}
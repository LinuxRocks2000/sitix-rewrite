#include <types/RedirectorStatement.hpp>
#include <types/Object.hpp>
#include <evals.hpp>
#include <fcntl.h>
#include <util.hpp>
#include <unistd.h>


RedirectorStatement::~RedirectorStatement() {
    object -> pushedOut();
}

RedirectorStatement::RedirectorStatement(MapView& map, MapView command, FileFlags* flags) : evalsCommand(command) {
    object = new Object;
    object -> fileflags = *flags;
    fileflags = *flags;
    fillObject(map, object, flags);
}

void RedirectorStatement::attachToParent(Object* p) {
    object -> parent = p;
}

void RedirectorStatement::render(SitixWriter*, Object* scope, bool dereference) {
    EvalsSession session { parent, scope };
    EvalsObject* result = session.render(evalsCommand);
    std::string filename = transmuted("", (std::string)outputDir, result -> toString());
    delete result;
    mkdirR(filename);
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        printf(ERROR "Couldn't create %s for a redirect!\n", filename.c_str());
        perror("\topen");
    }
    FileWriteOutput file(fd);
    SitixWriter writer(file);
    object -> render(&writer, scope, true);
    close(fd);
}
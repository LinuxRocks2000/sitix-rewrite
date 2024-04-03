// Session is a class that contains several FileMans, root configuration data, and any other global properties.
// Sessions should not be mutated except at the start by the main function. Every Node should contain a pointer to a Session.
#include <defs.h>
#include <string>
#include <fileman.hpp>
#include <treewatcher.hpp>


struct Session {
    std::vector<Object*> config;
    FileMan input;
    FileMan output;
    TreeWatcher watcher;

    Session(std::string inDir, std::string outDir);

    Object* configLookup(std::string name);

    // Session redirects a lot of the functions in input and output:

    FileMan::PathState checkPath(std::string path);

    std::string transmuted(std::string path);

    FileWriteOutput create(std::string path);

    MapView open(std::string path);

    std::string toOutput(std::string path);
};
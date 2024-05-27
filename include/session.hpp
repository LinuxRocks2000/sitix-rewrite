// Session is a class that contains several FileMans, root configuration data, and any other global properties.
// Sessions should not be mutated except at the start by the main function. Every Node should contain a pointer to a Session.
#include <defs.h>
#include <string>
#include <fileman.hpp>
#include <treewatcher.hpp>
#ifdef INLINE_MODE_LUAJIT
#include <luajit-2.1/lua.hpp> // TODO: fix this somehow
#endif
#include <mutex>


struct Session {
    std::mutex m_mutex;
    std::vector<Object*> config;
    FileMan input;
    FileMan output;
    TreeWatcher watcher;
    bool watchdog;
    bool usesDynamo = false; // do we use Sitix Dynamo (a lil' single-threaded HTTP server designed to replace PHP)?
    #ifdef INLINE_MODE_LUAJIT
    lua_State* lua;
    #endif

    Session(std::string, std::string, bool);

    Object* configLookup(std::string name);

    // Session redirects a lot of the functions in input and output:

    FileMan::PathState checkPath(std::string path);

    std::string transmuted(std::string path);

    FileWriteOutput create(std::string path);

    FileWriteOutput fcreate(int fd);

    MapView open(std::string path);

    MapView fopen(int fd);

    std::string toOutput(std::string path);

    void lock(); // forwards to m_mutex

    void unlock(); // ditto
};

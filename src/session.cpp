#include <session.hpp>
#include <types/Object.hpp>

#ifdef INLINE_MODE_LUAJIT
#include <sitixwriter.hpp>

static int sitix_lookup(lua_State* L) {
    std::string name = (std::string)lua_tostring(L, -1);
    lua_getglobal(L, "_sitix_scope");
    lua_getglobal(L, "_sitix_parent");
    Object* parent = (Object*)lua_touserdata(L, -1);
    Object* scope = (Object*)lua_touserdata(L, -1);
    Object* thing = parent -> lookup(name);
    if (thing == NULL) {
        thing = scope -> lookup(name);
    }
    if (thing == NULL) {
        return 0;
    }
    StringWriteOutput out;
    SitixWriter writer(out);
    thing -> render(&writer, scope, true);
    lua_pushstring(L, out.content.c_str());
    return 1;
}
#endif

Session::Session(std::string inDir, std::string outDir, bool isWatchdog) : input(inDir), output(outDir), watchdog{isWatchdog} {
    #ifdef INLINE_MODE_LUAJIT
    lua = lua_open();
    luaL_openlibs(lua);
    lua_createtable(lua, 0, 0); // "sitix" table
    lua_createtable(lua, 0, 1); // sitix metatable
    lua_pushcfunction(lua, sitix_lookup); // push the index function
    lua_setfield(lua, -2, "__index"); // attach the index function to the metatable
    lua_setmetatable(lua, -2); // set the metatable as the metatable of the sitix table
    lua_setglobal(lua, "sitix"); // set the table to a global variable named sitix
    #endif
}

Object* Session::configLookup(std::string name) {
    for (Object* o : config) {
        if (o -> name == name) {
            return o;
        }
    }
    return NULL;
}

FileMan::PathState Session::checkPath(std::string path) {
    return input.checkPath(path);
}

std::string Session::transmuted(std::string path) {
    return input.transmuted(path);
}

FileWriteOutput Session::create(std::string path) {
    return output.create(path);
}

FileWriteOutput Session::fcreate(int fd) {
    return FileWriteOutput(fd);
}

MapView Session::open(std::string path) {
    return input.open(path);
}

MapView Session::fopen(int fd) {
    return MapView(fd);
}

std::string Session::toOutput(std::string path) {
    return input.arcTransmuted(path);
}

void Session::lock() {
    m_mutex.lock();
}

void Session::unlock() {
    m_mutex.unlock();
}
#pragma once
#include <vector>

#define INFO      "\033[32m[   INFO   ]\033[0m "
#define ERROR   "\033[1;31m[   ERROR  ]\033[0m "
#define WARNING   "\033[33m[  WARNING ]\033[0m "
#define WATCHDOG  "\033[34m[ WATCHDOG ]\033[0m "
#define DYNAMO    "\033[35m[  DYNAMO  ]\033[0m "

#define FILLOBJ_EXIT_EOF  0
#define FILLOBJ_EXIT_ELSE 1
#define FILLOBJ_EXIT_END  2

#define INLINE_MODE_EVALS // use my Evals stack-based language for all inline stuff. To switch to LuaJIT mode, comment this out, and uncomment the following line.
//#define INLINE_MODE_LUAJIT // use LuaJIT (better, but slightly harder to set up) for inline stuff

// the legacy Evals api will be used no matter what; this simply changes the runtime that actually does the processing.


struct Object; // forward-declarations for everything. this is useful because it means we don't have to import those files, decreasing the dependency web
struct FileFlags; // (which makes builds faster)
struct MapView; // ideally this file will be changed far less frequently than the other headers
struct FileWriteOutput;
struct StringWriteOutput;
struct SitixWriter;
struct Node;
class Session;


Object string2object(const char* data, size_t length, Session*);
int fillObject(MapView&, Object*, FileFlags*, Session*);

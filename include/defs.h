#pragma once
#include <vector>

#define INFO     "\033[32m[  INFO   ]\033[0m "
#define ERROR  "\033[1;31m[  ERROR  ]\033[0m "
#define WARNING  "\033[33m[ WARNING ]\033[0m "

#define FILLOBJ_EXIT_EOF  0
#define FILLOBJ_EXIT_ELSE 1
#define FILLOBJ_EXIT_END  2


struct Object; // forward-declarations
struct FileFlags;
struct MapView;

extern const char* outputDir; // global variables are gross but some of the c standard library higher-order functions I'm using don't support argument passing
extern const char* siteDir;
extern std::vector<Object*> config;


Object string2object(const char* data, size_t length);
int fillObject(MapView& m, Object*, FileFlags*);
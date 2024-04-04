#pragma once
#include <string>
#include <cstring>
#include <cstdint>
#include <sys/stat.h>
#include <defs.h>
#include <ftw.h>

char* strdupn(const char* thing, size_t length);

std::string strip(std::string thing, char trigger);

void mkdirR(std::string filename);

std::string escapeString(std::string thing, char toEscape);

char* truncatn(const char* thing, size_t n, char stop);

bool isNumber(const char* data);

uint32_t toNumber(const char* data, size_t about);

template <typename N>
N min(N one, N two);

int endcmp(const char* one, const char* two);

char* transmuted(const char* from, const char* to, const char* path);

std::string transmuted(std::string from, std::string to, std::string path);

bool isWhitespace(char thing);

int iterRemove(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf);

void rmrf(const char* path);

std::string trim2dir(std::string file); // strip off a filename from a path
// if the path ends in /, it will not be changed
// the output will always be formatted for quick appending: if it is not fully stripped to an empty string, the last character will be a /
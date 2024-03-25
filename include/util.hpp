#pragma once
#include <string>
#include <cstring>
#include <cstdint>
#include <sys/stat.h>
#include <defs.h>

char* strdupn(const char* thing, size_t length);

char* strip(const char* thing, char trigger);

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
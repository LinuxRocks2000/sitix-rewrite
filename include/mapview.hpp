// "view" a memory map
// provides reference counted unmapping, fancy buffer-ey functions, view slicing, etc
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>


class MapView {
    char* map;
    size_t length; // authoritative length of the WHOLE MEMORY MAP
    size_t start; // starting position of this MapView's slice of the memory map
    size_t end; // ending position of this MapView's slice of the memory map
    int* rCount; // counts references to the underlying memory map
    int fd; // file descriptor of the map (useful for statf)

    void init(int, char* mm, size_t size);
public:
    MapView(int, char* mm, size_t size);

    MapView(int);

    MapView(std::string filename);

    bool isValid();

    MapView(const MapView& m);

    char operator[](int64_t n);

    void operator++(int);

    char operator++();

    void operator+=(size_t n);

    int64_t len();

    bool operator==(MapView& other);

    ~MapView();

    MapView slice(size_t from, size_t len);

    std::string toString(); // COPIES! TRY TO AVOID IT!

    bool cmp(const char* cmp, size_t at = 0);

    MapView operator+(size_t shift);

    const char* cbuf(); // get the "underlying c buffer"
    // since this is a MapView, the c buffer will be inside a memory map

    MapView consume(char until, bool escapeState = false, bool doesEscape = true); // consume bytes until one of them is until (allows escaping by default)

    void trim(); // tosses whitespace towards the `start`.

    char popFront();

    bool needsReload(); // check if the file on disc has changed in a way that would require a remap.
};
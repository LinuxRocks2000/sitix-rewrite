// "view" a memory map
// provides reference counted unmapping, fancy buffer-ey functions, view slicing, etc

#include <mapview.hpp>
#include <fcntl.h>
#include <defs.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>


void MapView::init(int file, char* mm, size_t size) {
    map = mm;
    length = size;
    start = 0;
    end = length;
    fd = file;
}

MapView::MapView(int file, char* mm, size_t size) {
    rCount = new int(1);
    init(file, mm, size);
}

MapView::MapView(std::string filename) {
    rCount = new int(1);
    map = NULL;
    int file = open(filename.c_str(), O_RDONLY);
    fd = file; // so when the destructor calls it gets closed properly
    if (file == -1) {
        printf(ERROR "Can't open %s for memory mapping!\n", filename.c_str());
        perror("\topen");
        return;
    }
    struct stat sb;
    if (stat(filename.c_str(), &sb)) {
        printf(ERROR "Can't load %s for memory mapping!\n", filename.c_str());
        perror("\tstat");
        return;
    }
    if (sb.st_size == 0) {
        printf(WARNING "%s has zero size and will not be rendered.\n\tIf you absolutely need an empty file there, consider writing a script to `touch` it in after Sitix builds.\n", filename.c_str());
        return;
    }
    map = (char*)mmap(0, sb.st_size, PROT_READ, MAP_SHARED, file, 0);
    if (map == MAP_FAILED) {
        map = NULL;
        printf(ERROR "Can't load %s for memory mapping!\n", filename.c_str());
        perror("\tmmap");
        return;
    }
    init(file, map, sb.st_size);
}

MapView::MapView(int file) {
    rCount = new int(1);
    map = NULL;
    fd = file;
    struct stat sb;
    if (fstat(file, &sb)) {
        printf(ERROR "Can't check file descriptor for memory mapping!\n");
        perror("\tfstat");
        return;
    }
    if (sb.st_size == 0) {
        printf(WARNING "File referenced by file descriptor has 0 size and will not be memory mapped.\n");
        return;
    }
    map = (char*)mmap(0, sb.st_size, PROT_READ, MAP_SHARED, file, 0);
    if (map == MAP_FAILED) {
        map = NULL;
        printf(ERROR "Can't memory map from file descriptor.\n");
        perror("\tmmap");
        return;
    }
    init(file, map, sb.st_size);
}

MapView::MapView(const MapView& m) {
    *this = m;
    (*rCount) ++;
}

bool MapView::isValid() {
    return map != NULL;
}

char MapView::operator[](int64_t n) {
    if (len() == 0) {
        return EOF;
    }
    else {
        while (n < 0) { // TODO: Make this more efficient with clever math (modulo will NOT work)
            n += len();
        }
        return map[start + n];
    }
}

void MapView::operator++(int) {
    start ++;
}

char MapView::operator++() {
    start ++;
    return map[start - 1];
}

void MapView::operator+=(size_t n) {
    start += n;
}

int64_t MapView::len() {
    return end - start;
}

bool MapView::operator==(MapView& other) {
    if (other.len() != len()) { // if their lengths are different, they can't possibly be equal
        return false;
    }
    for (size_t i = 0; i < len(); i ++) {
        if ((*this)[i] != other[i]) {
            return false;
        }
    }
    return true;
}

MapView::~MapView() {
    (*rCount) --;
    if (*rCount == 0) {
        free(rCount);
        if (map != NULL) {
            munmap(map, length);
        }
        if (fd != -1) {
            close(fd);
        }
    }
}

MapView MapView::slice(size_t from, size_t len) {
    MapView ret(*this);
    ret.start = start + from;
    ret.end = start + from + len;
    return ret;
}

std::string MapView::toString() { // COPIES! TRY TO AVOID IT!
    return std::string(map + start, end - start);
}

bool MapView::cmp(const char* cmp, size_t at) {
    size_t cmpLen = strlen(cmp);
    if (end - at - start < cmpLen) {
        cmpLen = end - at;
    }
    for (size_t i = 0; i < cmpLen; i ++) {
        if ((*this)[i + at] != cmp[i]) {
            return false;
        }
    }
    return true;
}

MapView MapView::operator+(size_t shift) {
    MapView ret(*this);
    ret.start += shift;
    return ret;
}

const char* MapView::cbuf() {
    return (map + start);
}

MapView MapView::consume(char until, bool escapeState, bool doesEscape) { // consume bytes until one of them is until (allows escaping by default)
    // return the consumed bytes as a child MapView
    // escapeState allows the caller to determine if the first byte is considered to be escaped or not (useful if there's a "master" escape count)
    MapView ret = *this;
    while (len() > 0) {
        if (map[start] == '\\' && doesEscape && !escapeState) {
            escapeState = true;
            start ++;
            continue;
        }
        else if (map[start] == until && !escapeState) {
            break;
        }
        escapeState = false;
        start ++;
    }
    ret.end = start;
    return ret;
}

void MapView::trim() { // tosses whitespace towards the `start`.
    //start ++; // strip off the first byte
    while (map[start] == ' ' || map[start] == '\n' || map[start] == '\t') {start ++;} // continue to strip off bytes until they're not whitespace
    //start --; // add back the snipped-off byte.
}

char MapView::popFront() {
    end --;
    return map[end];
}

bool MapView::needsReload() {
    struct stat sb;
    if (fstat(fd, &sb) == 0) {
        return sb.st_size != len(); // if the size has changed, the map needs to reload!
    }
    else {
        return true;
    }
}
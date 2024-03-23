// "view" a memory map
// provides reference counted unmapping, fancy buffer-ey functions, view slicing, etc
#pragma once


class MapView {
    char* map;
    size_t length; // authoritative length of the WHOLE MEMORY MAP
    size_t start; // starting position of this MapView's slice of the memory map
    size_t end; // ending position of this MapView's slice of the memory map
    int* rCount; // counts references to the underlying memory map

    void init(char* mm, size_t size) {
        map = mm;
        rCount = new int(1);
        length = size;
        start = 0;
        end = length;
    }
public:
    MapView(char* mm, size_t size) {
        init(mm, size);
    }

    MapView(const char* filename) {
        map = NULL;
        int file = open(filename, O_RDONLY);
        if (file == -1) {
            printf(ERROR "Can't open %s for memory mapping!\n", filename);
            perror("\topen");
            return;
        }
        struct stat sb;
        if (stat(filename, &sb)) {
            printf(ERROR "Can't load %s for memory mapping!\n", filename);
            perror("\tstat");
            return;
        }
        if (sb.st_size == 0) {
            printf(WARNING "%s has zero size and will not be rendered.\n\tIf you absolutely need an empty file there, consider writing a script to `touch` it in after Sitix builds.\n", filename);
            return;
        }
        map = (char*)mmap(0, sb.st_size, PROT_READ, MAP_SHARED, file, 0);
        if (map == MAP_FAILED) {
            map = NULL;
            printf(ERROR "Can't load %s for memory mapping!\n", filename);
            perror("\tmmap");
            return;
        }
        close(file);
        init(map, sb.st_size);
    }

    bool isValid() {
        return map != NULL;
    }

    MapView(const MapView& m) {
        *this = m;
        (*rCount) ++;
    }

    inline char operator[](int64_t n) {
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

    inline void operator++(int) {
        start ++;
    }

    inline char operator++() {
        start ++;
        return map[start - 1];
    }

    inline void operator+=(size_t n) {
        start += n;
    }

    inline int64_t len() {
        return end - start;
    }

    inline bool operator==(MapView& other) {
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

    ~MapView() {
        (*rCount) --;
        if (*rCount == 0) {
            free(rCount);
            munmap(map, length);
        }
    }

    MapView slice(size_t from, size_t len) {
        MapView ret(*this);
        ret.start = start + from;
        ret.end = start + from + len;
        return ret;
    }

    std::string toString() { // COPIES! TRY TO AVOID IT!
        return std::string(map + start, end - start);
    }

    bool cmp(const char* cmp, size_t at = 0) {
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

    inline MapView operator+(size_t shift) {
        MapView ret(*this);
        ret.start += shift;
        return ret;
    }

    inline const char* cbuf() {
        return (map + start);
    }

    MapView consume(char until, bool escapeState = false, bool doesEscape = true) { // consume bytes until one of them is until (allows escaping by default)
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

    void trim() { // tosses whitespace towards the `start`.
        //start ++; // strip off the first byte
        while (map[start] == ' ' || map[start] == '\n' || map[start] == '\t') {start ++;} // continue to strip off bytes until they're not whitespace
        //start --; // add back the snipped-off byte.
    }

    char popFront() {
        end --;
        return map[end];
    }
};
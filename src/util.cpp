#include <util.hpp>
// definitions for util functions

char* strdupn(const char* thing, size_t length) { // copy length bytes of a string to a new, NULL-terminated C string
    char* ret = (char*)malloc(length + 1);
    ret[length] = 0;
    for (size_t i = 0; i < length; i ++) {
        ret[i] = thing[i];
    }
    return ret;
}


char* strip(const char* thing, char trigger) { // clears all instances of a symbol out of a string
    // this allocates memory! That means you have to free the memory!
    size_t trigCount = 0;
    size_t size = strlen(thing);
    for (size_t i = 0; i < size; i ++) {
        if (thing[i] == '\\') {
            trigCount ++;
        }
    }
    char* ret = (char*)malloc(size - trigCount + 1);
    ret[size - trigCount] = 0;
    size_t truePos = 0;
    size_t i = 0;
    while (i < size - trigCount) {
        if (thing[truePos] == trigger) {
            truePos ++;
            continue;
        }
        ret[i] = thing[truePos];
        i ++;
        truePos ++;
    }
    return ret;
}


void mkdirR(std::string filename) { // recursively create the directories before a file
    // expects syntax like directory/directory/directory/file or directory/directory/directory/. directory/directory/directory is not supported.
    struct stat sb;
    size_t blobend = 0;
    while (blobend < filename.size()) {
        if (filename[blobend] == '/') {
            std::string dirname = filename.substr(0, blobend);
            if (stat(dirname.c_str(), &sb) == -1) {
                if (mkdir(dirname.c_str(), 0) != 0) {
                    printf(ERROR "Couldn't create %s!\n", dirname.c_str());
                    perror("\tmkdir");
                }
                chmod(dirname.c_str(), 0755);
            }
            else if (!S_ISDIR(sb.st_mode)) {
                printf(ERROR "%s exists and is not a directory. Aborting recursive mkdir operation.\n", dirname.c_str());
                return;
            }
        }
        blobend++;
    }
}


std::string escapeString(std::string thing, char toEscape) {
    size_t escapeC = 0;
    for (size_t i = 0; i < thing.size(); i ++) {
        if (thing[i] == toEscape) {
            escapeC ++;
        }
    }
    std::string ret;// = (char*)malloc(size + escapeC + 1);
    ret.reserve(thing.size() + escapeC);
    size_t i = 0;
    while (i < thing.size() + escapeC) {
        if (thing[i] == toEscape) {
            ret += '\\';
        }
        ret += thing[i];
        i ++;
    }
    return ret;
}


char* truncatn(const char* thing, size_t n, char stop) { // backwards-truncate the end of a string of known length to the last `stop` character
    // (so truncatn("hello, world", 12, ',') == " world")
    // this ALLOCATES MEMORY! That means you have to free the memory!
    size_t tailLen;
    for (tailLen = 0; tailLen < n; tailLen ++) {
        if (thing[n - tailLen - 1] == stop) {
            break;
        }
    }
    char* ret = (char*)malloc(tailLen + 1);
    ret[tailLen] = 0;
    memcpy(ret, thing + (n - tailLen), tailLen);
    return ret;
}


bool isNumber(const char* data) {
    size_t s = strlen(data);
    for (size_t i = 0; i < s; i ++) {
        if (data[i] < '0' || data[i] > '9') {
            if (data[i] != '-') {
                return false;
            }
        }
    }
    return true;
}


uint32_t toNumber(const char* data, size_t about) {
    int32_t ret = 0;
    size_t s = strlen(data);
    for (size_t i = 0; i < s; i ++) {
        ret *= 10;
        ret += data[i] - '0';
    }
    if (data[0] == '-') {
        ret *= -1;
    }
    while (ret < 0) {
        ret += about;
    }
    while (ret > about) {
        ret -= about;
    }
    return ret;
}


template <typename N>
N min(N one, N two) {
    if (one < two) {
        return one;
    }
    return two;
}


int endcmp(const char* one, const char* two) { // check how many bytes are shared at the ends of two strings
    size_t oneLen = strlen(one);
    size_t twoLen = strlen(two);
    int i;
    for (i = 0; i < min(oneLen, twoLen); i ++) {
        if (one[oneLen - i - 1] != two[twoLen - i - 1]) {
            return i;
        }
    }
    return i;
}


char* transmuted(const char* from, const char* to, const char* path) { // transmute a pathname.
    // calling transmuted("/dev", "/opt", "/dev/null") would return "/opt/null", for instance.
    if (path[0] == '.') {
        path = (const char*)(((char*)path) + 1);
    }
    size_t fromLen = strlen(from);
    if (from[fromLen - 1] == '/') {
        fromLen --;
    }
    size_t toLen = strlen(to);
    if (to[toLen - 1] == '/') {
        toLen --;
    }
    size_t pathLen = strlen(path);
    if (path[fromLen] == '/') {
        pathLen --;
        path ++;
    }
    size_t retLen = pathLen - fromLen + toLen;
    bool needsSep = false;
    if ((to[toLen - 1] != '/') && (path[0] != '/') && (toLen > 0)) {
        needsSep = true;
        retLen ++;
    }
    char* ret = (char*)malloc(retLen + 1);
    ret[retLen] = 0; // NULL-termination!
    memcpy(ret, to, toLen);
    if (needsSep) {
        ret[toLen] = '/';
        toLen += 1;
    }
    memcpy(ret + toLen, path + fromLen, pathLen - fromLen);
    return ret;
}

std::string transmuted(std::string from, std::string to, std::string path) {
    char* r = transmuted(from.c_str(), to.c_str(), path.c_str());
    std::string ret = r;
    free(r);
    return ret;
}

bool isWhitespace(char thing) {
    return thing == ' ' || thing == '\t' || thing == '\n' || thing == '\r';
}
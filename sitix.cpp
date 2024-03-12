/* Sitix, by Tyler Clarke
    An efficient templating engine written in C++, meant to reduce the pain of web development.
    Sitix templating is optional; files that desire Sitix templating should include [!] as the first three bytes. Otherwise, sitix will simply copy them.
    Files may also use [?] as the first three bytes; this informs the Sitix engine that the file should not be rendered (most templates will use this).
    Sitix works with a simple scope structure. Variables are set with [=name content] OR [=name-]content[/]. Variables are accessed with [^name].
    Variables can contain other variable accesses! This allows functions of a sort. For instance,
    
    [=link-]
        <a href="[^url]">[^linktext]</a>
    [/]
    [=url https://example.com][=linktext Example Dot Com][^link]

    Variables can be nested:

    [=config-]
        [=title Hello, World]
        [=baseurl https://helloworld.com/]
    [/]

    [^config.title]

    Variables can be reassigned:

    [=config-]
        [=title Hello, ]
        [=baseurl https://helloworld.com/]
    [/]
    [^config.title][=config.title World][^config.title]
    
    Which would output "Hello, World".

    So the rule is that variable operations occur on usage, rather than on assignment. This is obviously useful for templating.

    [#filename] includes a file. It doesn't just copy/paste; that file will be processed separately, but the scope at the inclusion point
    will be treated as the root scope for the include.
    
    [^filename] is a magic variable attached to every variable referencing the name of the file in which it was *created*. This is useful, you'll see why.

    Sitix doesn't have much of a type system. It does, however, have a way to do array-like operations: numerically assigned properties. These are done with
    [=+-][/] (or [=+ content]). [loop]s work on these, ignoring other nested variables. Loops specify a numerically indexed variable to operate on, and
    the name of the iterator variable. For instance,

    [=array-]
        [=+ Hello]
        [=+ , ]
        [=+ World]
    [/]
    [l array i]
        [^i]
    [/]

    would output "Hello, World".

    Arrays can be directly indexed with regular access syntax, like [^array.0], [^array.1], etc. Like in Python, [^array.-1] is the last element,
    [^array.-2] is the second to last, etc. This is rarely desirable. The major useful trait of arrays is that they are project-aware! Child
    directories are treated as variables on the global scope, numerically indexing in every file they contain (and name-indexing their subdirectories),
    so you can use this for things like generating blog links:

    [l posts post]
        <h1>[^post.title]</h1>
        <p>
            [^post.blurb]
        </p>
        <a href="[^post.filename]">Go To [^post.title]!</a>
    [/]

    The parent directory is referenced with the "magic" [^parentdir].

    Conditional branching is pretty important. At the moment the only valid conditional branch is equality, since other operations are ill-formed; equality checking
    just checks if raw scope (the list of text nodes and operations) is the same:

    [=variable Data]
    [=variable2 Data]
    [i equal variable v2]
        Hello, World
    [/]

    Or 

    [=variable Data]
    [=variable2 OtherData]
    [i nequal variable v2]
        Hello, World
    [/]

    Else is also supported -
    
    [=variable Data]
    [=variable2 OtherData]
    [i equal variable v2]
        They're the same.
    [e]
        They're different.
    [/]

    Configuration can be done at command-line with -c and checked with [i config]. A very real usage is like so:

    [=name Test Site]
    [i config production]
        [=name Production Site]
    [/]

    Where sitix -c production would cause that if gate to resolve.

    Escaping is done with backslashes. Because of the nature of the escape-processing mechanism, you can sanely escape whole tags with just one backslash, like

    \[=test Test]\[^test]

    would yield "[=test Test][^test]" rather than "Test".
    Backslashes themselves can be escaped with backslashes.

    NOTE: Escaping inside inline-expressions IS UNDEFINED! It might work, it might crash the program, it might do nothing. If you need to escape text,
    just waste like five bytes on a full expanded-expression.

    The [@] operator manages fileflags. These are specific to the current file and are boolean; [@on <flag>] turns a flag on and [@off <flag>] turns a flag off.
    These are file-wide and do NOT respect render-time constraints; they are applied immediately at parse-time. They are file-wide ONLY; if you want the same
    flags in two files, you must specify them in both files, even if one is including the other. The only currently available flag is minify, enable it with
    [@on minify] to reduce unnecessary whitespace in your Sitix files.
*/

// TODO: memory management. Set up sane destructors and USE THEM!
// At the moment the program leaks a LOT of memory. like, a LOT.


#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <stdlib.h>
#include <ftw.h>
#include <sys/types.h>
#include <dirent.h>


#define INFO     "\033[32m[  INFO   ]\033[0m "
#define ERROR  "\033[1;31m[  ERROR  ]\033[0m "
#define WARNING  "\033[33m[ WARNING ]\033[0m "


const char* outputDir = "output"; // global variables are gross but some of the c standard library higher-order functions I'm using don't support argument passing
const char* siteDir = "";
std::vector<const char*> config;
bool wasElse = false; // This is a very bad solution. Need to make something better.


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


char* escapeString(const char* thing, char toEscape) {
    size_t escapeC = 0;
    size_t size = strlen(thing);
    for (size_t i = 0; i < size; i ++) {
        if (thing[i] == toEscape) {
            escapeC ++;
        }
    }
    char* ret = (char*)malloc(size + escapeC + 1);
    ret[size + escapeC] = 0;
    size_t i = 0;
    size_t point = 0;
    while (i < size + escapeC) {
        if (thing[i] == toEscape) {
            ret[i] = '\\';
            point ++;
            ret[i + point] = thing[i];
        }
        else {
            ret[i + point] = thing[i];
        }
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
            return false;
        }
    }
    return true;
}


uint32_t toNumber(const char* data) {
    uint32_t ret = 0;
    size_t s = strlen(data);
    for (size_t i = 0; i < s; i ++) {
        ret *= 10;
        ret += data[i] - '0';
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
    size_t toLen = strlen(to);
    size_t pathLen = strlen(path);
    size_t retLen = pathLen - fromLen + toLen;
    bool needsSep = false;
    if ((to[toLen - 1] != '/') && (path[0] != '/')) {
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


struct Object; // forward-dec


struct FileFlags {
    bool minify = false;
};


struct Node { // superclass
    virtual ~Node() {

    }
    Object* parent = NULL; // everything has an Object parent, except the root Object (which will have a NULL parent)

    FileFlags fileflags;
    enum Type {
        OTHER = 1,
        PLAINTEXT = 3,
        TEXTBLOB = 5,
        OBJECT = 7
    } type = OTHER; // it's necessary to specificate from Node to plaintext, object, etc in some cases.

    virtual void render(int fd, Object* scope, bool dereference = false) = 0; // true virtual function
    // scope is a SECONDARY scope. If a lookup fails in the primary scope (the parent), scope lookup will be performed in the secondary scope.
    // dereference causes forced rendering on objects (objects don't render by default unless dereferenced with [^name])

    virtual void debugPrint() {}; // optional

    virtual void pTree(int tabLevel = 0) { // replacing debugPrint because it's much more usefulicious
        for (int x = 0; x < tabLevel; x ++) {printf("\t");}
        printf("Generic Node with type %d ", type);
        if (parent != NULL) {
            printf("with parent (%d)\n", parent);
        }
        else {
            printf("without parent\n");
        }
    }
};


struct PlainText : Node {
    const char* data = NULL;
    size_t size = 0;

    PlainText() {
        type = PLAINTEXT;
    }

    void render(int fd, Object* scope, bool dereference) {
        if (fileflags.minify) { // TODO: buffering to make this more fasterly
            bool hadSpace = false;
            for (size_t i = 0; i < size; i ++) {
                if (data[i] == ' ' || data[i] == '\t' || data[i] == '\n') {
                    if (!hadSpace) {
                        hadSpace = true;
                        if (i != 0) { // don't write the first whitespaces in a file, that's dumb
                            write(fd, " ", 1);
                        }
                    }
                    continue;
                }
                else {
                    hadSpace = false;
                }
                write(fd, &data[i], 1);
            }
        }
        else {
            write(fd, data, size);
        }
    }

    virtual void pTree(int tabLevel = 0) { // replacing debugPrint because it's much more usefulicious
        for (int x = 0; x < tabLevel; x ++) {printf("\t");}
        printf("Text content\n");
    }
};


struct TextBlob : Node { // designed for smaller, heap-allocated text bits that it frees (not suitable for memory maps)
    char* data = NULL;
    TextBlob() {
        type = TEXTBLOB;
    }

    ~TextBlob() {
        free(data);
    }

    void render(int fd, Object* scope, bool dereference) {
        size_t size = strlen(data);
        if (fileflags.minify) { // TODO: buffering to make this more fasterly
            bool hadSpace = false;
            for (size_t i = 0; i < size; i ++) {
                if (data[i] == ' ' || data[i] == '\t' || data[i] == '\n') {
                    if (!hadSpace) {
                        hadSpace = true;
                        if (i != 0) { // don't write the first whitespaces in a file, that's dumb
                            write(fd, " ", 1);
                        }
                    }
                    continue;
                }
                else {
                    hadSpace = false;
                }
                write(fd, &data[i], 1);
            }
        }
        else {
            write(fd, data, size);
        }
    }

    virtual void pTree(int tabLevel = 0) { // replacing debugPrint because it's much more usefulicious
        for (int x = 0; x < tabLevel; x ++) {printf("\t");}
        printf("Text content\n");
    }
};


Object string2object(const char* data, size_t length); // forward-dec
size_t fillObject(const char*, size_t, Object*, FileFlags*);


struct Object : Node { // Sitix objects contain a list of *nodes*, which can be enumerated (like for array reference), named (for variables), operations, or pure text.
    // Sitix objects are only converted to text when it's absolutely necessary; e.g. when the root object is rendered. Scoping is KISS - when an object resolve
    // is requested, we walk down the chain (towards the root) until we hit an object containing the right name, then walk up to match nested variables.
    // Variable dereferences are considered operations and will not be executed until the relevant section is rendered; it is important to make sure your
    // Sitix code is aware of the scope caveats at all times.
    std::vector<Node*> children;
    bool isTemplate = false;
    bool isFile = false;
    uint32_t highestEnumerated = 0; // the highest enumerated value in this object

    Object* ghost = NULL; // if this is not-null, the current object is a "ghost" of that object - everything resolves into the ghost.

    int rCount = 1;

    Object() {
        type = OBJECT;
    }

    void pushedOut() {
        rCount --;
        if (rCount < 0) {
            printf(ERROR "Bad reference count. The program will probably crash.\n");
        }
        if (rCount != 0) {
            return;
        }
        if (ghost != NULL) {
            ghost -> pushedOut();
        }
        if (namingScheme == NamingScheme::Named) {
            if (name != NULL) {
                free(name);
                name = NULL;
            }
        }
        for (Node* child : children) {
            if (child -> type == Node::Type::OBJECT) {
                Object* o = (Object*)child;
                o -> pushedOut();
            }
            else {
                delete child;
            }
        }
        delete this;
    }

    union {
        char* name;
        uint32_t number;
    };
    enum NamingScheme {
        Named, // it has a real name, which is contained in char* name (which will be a proper C string)
        Enumerated, // it has a number assigned by the parent
        Virtual // it's contained inside a logical operation or something similar
    } namingScheme = Virtual;

    ~Object() {/*
        for (size_t i = 0; i < children.size(); i ++) {
            Node* child = children[i];
            if (child == NULL) {
                printf(ERROR "Null child found. This may cause strange errors.");
            }
            else {
                if (child -> type == Node::Type::OBJECT) {
                    Object* thing = (Object*)child;
                    if (*(thing -> rCount) > 0) {
                        *(thing -> rCount) --;
                    }
                    else {
                        delete child;
                    }
                }
                else {
                    delete child;
                }
            }
        }
        if (namingScheme == NamingScheme::Named) {
            free(name);
        }
        if (rCount != NULL) {
            free(rCount);
            rCount = NULL;
        }*/
    }

    void render(int fd, Object* scope, bool dereference) { // objects are just delegation agents, they don't contribute anything to the final text.
        if (ghost != NULL) {
            ghost -> render(fd, scope, dereference);
            return;
        }
        if (namingScheme == NamingScheme::Named) { // when objects are rendered, they replace the other objects of the same name on the scope tree.
            if (parent != NULL && !dereference) { // replace operations are ALWAYS on the parent, we can't intrude on someone else's scoped
                parent -> replace(name, this);
            }
        }
        if (!dereference) {
            return;
        }
        for (size_t i = 0; i < children.size(); i ++) {
            children[i] -> render(fd, scope);
        }
    }

    void addChild(Node* child) {
        child -> parent = this;
        children.push_back(child);
    }

    void dropObject(Object* object) {
        for (size_t i = 0; i < children.size(); i ++) {
            if (children[i] == object) {
                children.erase(children.begin() + i);
            }
        }
    }

    Object* lookup(const char* name, Object* nope = NULL) { // lookup variant that works on NAMES
        // returning NULL means no suitable object was found here or at any point down in the tree
        // if `nope` is non-null, it will be used as a discriminant (it will not be returned)
        // note that copied objects will be returned; `nope` uses pointer-comparison only.
        if (ghost != NULL) {
            return ghost -> lookup(name, nope);
        }
        size_t nameLength = strlen(name);
        size_t rootSegLen;
        for (rootSegLen = 0; rootSegLen < nameLength; rootSegLen ++) {
            if (name[rootSegLen] == '.' && name[rootSegLen - 1] != '\\') {
                break;
            }
        }
        char* rootBit = strdupn(name, rootSegLen);
        char* root = strip(rootBit, '\\');
        free(rootBit);
        for (Node* node : children) {
            if (node -> type == Node::Type::OBJECT) {
                Object* candidate = (Object*)node;
                if (candidate == nope && parent != NULL) { // if we ARE the root, nope stops being meaningful; this is because the nope system exists
                    // to allow objects inside a lower scope to overwrite objects in their parent scope (allowing structures like the if config to set global variables)
                    // however, if the scope it's looking up on *is* the global, there's no reason to try hopping up another scope, and we don't want to reload
                    // data from disc (such as, loaded files and directories will be "rendered" again when the parser hits that point)
                    break; // do NOT continue, because we need to propagate the illusion that there can only be one instance of an object per scope.
                    // This is necessary because of situations like [=test One][^test][=test Two][^test], because "nope"ing (inside a replace) when the first one is
                    // rendered would skip the first one and set the second one - which would mean the second one renders incorrectly. So instead, we
                    // want to jump out to the next-highest scope when we find an object that is correct, but noped.
                }
                if (candidate -> namingScheme == Object::NamingScheme::Named && strcmp(candidate -> name, root) == 0) {
                    free(root);
                    if (rootSegLen == nameLength) {
                        return candidate;
                    }
                    else {
                        return candidate -> childSearchUp(name + rootSegLen + 1); // the +1 is to consume the '.'
                    }
                }
            }
        }
        if (parent == NULL) { // if we ARE the parent
            // directory unpacks and file unpacks are on the root scope, see
            struct stat sb;
            char* directoryName = transmuted("", siteDir, root);
            if (stat(directoryName, &sb) == 0) {
                if (S_ISDIR(sb.st_mode)) { // if `root` is the name of an accessible directory
                    Object* dirObject = new Object;
                    DIR* directory = opendir(directoryName);
                    struct dirent* entry;
                    while ((entry = readdir(directory)) != NULL) { // LEAKS MEMORY!
                        if (entry -> d_name[0] == '.') { // . and ..
                            continue;
                        }
                        char* transmuteNamep1 = transmuted("", root, entry -> d_name);
                        char* transmuteName = escapeString(transmuteNamep1, '.');
                        free(transmuteNamep1);
                        Object* enumerated = new Object;
                        Object* file = lookup(transmuteName); // guaranteed not to fail
                        if (file == NULL) {
                            printf(ERROR "Unpacking lookup for %s in directory-to-array unpacking FAILED! The output will be malformed!\n", transmuteName);
                            continue;
                        }
                        file -> rCount ++; // bump the ref count, see the Object definition above for information on this
                        enumerated -> ghost = file;
                        enumerated -> namingScheme = Object::NamingScheme::Enumerated; // change the structure of the copied object to be an enumerated entry
                        enumerated -> number = dirObject -> highestEnumerated;
                        dirObject -> highestEnumerated ++;
                        dirObject -> addChild(enumerated); // look up the file and add it to this directory object
                        // the whole scheme is, like, *whoa*
                        // when I realized how much simpler this could be (no includes, everything is lazy-loaded, etc)
                        // I was, like,
                        // *whoa*
                        // I imagine this is what doing marijuana feels like
                        // 'cause, yk, it's all connected, *maaaaan*
                    }
                    closedir(directory);
                    dirObject -> namingScheme = Object::NamingScheme::Named;
                    ::free(directoryName);
                    dirObject -> name = root;
                    addChild(dirObject);// DON'T free root because it was passed into the dirObject
                    if (rootSegLen == nameLength) {
                        return dirObject;
                    }
                    else {
                        return dirObject -> childSearchUp(name + rootSegLen + 1);
                    }
                }
                else if (S_ISREG(sb.st_mode)) {
                    // construct the "filename" object inside loaded files
                    // TODO: add a truncated filename object inside the loaded file, which would contain "mod1.html" rather than
                    // "templates/modules/mod1.html", for instance.
                    TextBlob* fNameContent = new TextBlob;
                    fNameContent -> data = strdup(root);
                    Object* fNameObj = new Object;
                    fNameObj -> namingScheme = Object::NamingScheme::Named;
                    fNameObj -> name = strdup("filename");
                    fNameObj -> addChild(fNameContent);

                    // memory map and load the actual file
                    int file = open(directoryName, O_RDONLY);
                    if (file == -1) {
                        printf(ERROR "Couldn't open %s (%s) for file-to-object unpacking.\n", root, directoryName);
                        free(directoryName);
                        free(root);
                        return NULL;
                    }
                    struct stat data;
                    if (stat(directoryName, &data)) {
                        printf(ERROR "Can't grab file information for %s.\n", root);
                        perror("\tstat");
                        free(directoryName);
                        free(root);
                        return NULL;
                    }
                    char* map = (char*)mmap(0, data.st_size, PROT_READ, MAP_SHARED, file, 0);
                    close(file);
                    if (map == MAP_FAILED) {
                        printf(ERROR "Can't memory map %s for directory-to-array inflation.\n", root);
                        perror("\tmmap");
                        free(directoryName);
                        free(root);
                        return NULL;
                    }

                    // put together the actual file object, store it on global, and return it
                    Object* fileObj = new Object;
                    fileObj -> isFile = true;
                    fileObj -> addChild(fNameObj); // add the filename to the object
                    fileObj -> namingScheme = Object::NamingScheme::Named;
                    fileObj -> name = directoryName; // reference name of the object, so it can be quickly looked up later without another slow cold-load
                    if (strncmp(map, "[?]", 3) == 0 || strncmp(map, "[!]", 3) == 0) {
                        FileFlags flags;
                        fillObject(map + 3, data.st_size - 3, fileObj, &flags);
                    }
                    else {
                        PlainText* content = new PlainText;
                        content -> data = map;
                        content -> size = data.st_size;
                        fileObj -> addChild(content);
                    }
                    addChild(fileObj); // since we're the global scope, we should add the file to us.
                    // the goal is to create an illusion that the entire directory structure is a cohesive part of the object tree
                    // and then sorta just load files when they ask us to
                    free(root);
                    return fileObj;
                }
            }
            free(directoryName);
        }
        else {
            ::free(root);
            return parent -> lookup(name, nope);
        }
        ::free(root);
        return NULL;
    }

    Object* childSearchUp(const char* name) { // name is expected to be a . separated 
        if (ghost != NULL) {
            return ghost -> childSearchUp(name);
        }
        size_t nameLen = strlen(name);
        size_t segLen;
        for (segLen = 0; segLen < nameLen; segLen ++) {
            if (name[segLen] == '.' && name[segLen - 1] != '\\') {
                break;
            }
        }
        char* mSeg = (char*)malloc(segLen + 1);
        mSeg[segLen] = 0;
        memcpy(mSeg, name, segLen);
        for (Node* child : children) {
            if (child -> type == Node::Type::OBJECT) {
                Object* candidate = (Object*)child;
                if (isNumber(mSeg)) {
                    if (candidate -> namingScheme == Object::NamingScheme::Enumerated && toNumber(mSeg) == candidate -> number) {
                        ::free(mSeg);
                        if (segLen == nameLen) {
                            return candidate;
                        }
                        else {
                            return candidate -> childSearchUp(name + segLen + 1);
                        }
                    }
                }
                else if (candidate -> namingScheme == Object::NamingScheme::Named && strcmp(candidate -> name, mSeg) == 0) {
                    ::free(mSeg);
                    if (segLen == nameLen) {
                        return candidate;
                    }
                    else {
                        return candidate -> childSearchUp(name + segLen + 1); // the +1 is to consume the '.'
                    }
                }
            }
        }
        ::free(mSeg);
        return NULL;
    }

    void pTree(int tabLevel = 0) {
        for (int x = 0; x < tabLevel; x ++) {printf("\t");}
        printf("Object ");
        if (parent == NULL) {
            printf("without parent (root) ");
        }
        else {
            printf("with parent (%d) ", parent);
        }
        if (namingScheme == NamingScheme::Named) {
            printf("named %s ", name);
        }
        else if (namingScheme == NamingScheme::Enumerated) {
            printf("#%d ", number);
        }
        else {
            printf("unnamed ");
        }
        printf("(%d)\n", this);
        for (Node* child : children) {
            child -> pTree(tabLevel + 1);
        }
    }

    void ptrswap(Object* current, Object* repl) {
        if (ghost != NULL) {
            ghost -> ptrswap(current, repl);
            return;
        }
        for (size_t i = 0; i < children.size(); i ++) {
            if (children[i] == current) {
                children[i] = repl;
            }
        }
    }

    bool replace(const char* name, Object* obj) {
        if (ghost != NULL) {
            return ghost -> replace(name, obj);
        }
        // returns true if the object was replaced, and false if it wasn't
        Object* o = lookup(name, obj); // enable excluded-lookup
        if (o == obj) { // if the object that would be replaced is us, we don't want to go through with it.
            return false;
        }
        if (o != NULL) {
            obj -> rCount ++;
            obj -> parent = o -> parent;
            o -> parent -> ptrswap(o, obj);
            o -> pushedOut();
            return true;
        }
        return false;
    }

    void debugPrint() {
        printf("Object ");
        if (namingScheme == NamingScheme::Named) {
            printf("named %s", name);
        }
        printf("\n");
        if (ghost != NULL) {
            printf("I'm ghosting!\n");
        }
        printf("my content: \n\n");
        render(0, NULL, true);
        printf("\n\n");
    }

    Object* nonvRoot() { // walk up the parent tree until we reach the first non-virtual object
        if (parent == NULL) { // we're the rootin' tootin' rootiest root in these here hills
            return this;
        }
        if (namingScheme == NamingScheme::Virtual) {
            return parent -> nonvRoot();
        }
        else {
            return this;
        }
    }
};


struct DebuggerStatement : Node {
    virtual void render(int fd, Object* scope, bool dereference) {
        printf("==== DEBUGGER BREAK ====\n");
        printf("++++     PARENT     ++++\n");
        parent -> pTree(1);
        printf("++++      SCOPE     ++++\n");
        scope -> pTree(1);
        printf("====  DEBUGGER OVER ====\n");
    }
};


struct ForLoop : Node {
    char* goal; // the name of the object we're going to iterate over
    char* iteratorName; // the name of the object we're going to create as an iterator when this loop is rendered
    Object* internalObject; // the object we're going to render at every point in the loop

    ~ForLoop() {
        delete internalObject; // it was virtual. it can't possibly have been referenced. so let's just get rid of it.
        free(goal); // they will not be needed *ominous grin*
        free(iteratorName);
    }

    void render(int fd, Object* scope, bool dereference) { // the memory management here is truly horrendous.
        Object* array = scope -> lookup(goal);
        if (array == NULL) {
            array = parent -> lookup(goal);
        }
        if (array == NULL) {
            printf(ERROR "Array lookup for %s failed. The output will be malformed.\n", goal);
            return;
        }
        Object iterator;
        iterator.namingScheme = Object::NamingScheme::Named;
        iterator.name = iteratorName;
        internalObject -> addChild(&iterator);
        for (size_t i = 0; i < array -> children.size(); i ++) {
            if (array -> children[i] -> type == Node::Type::OBJECT) {
                Object* object = (Object*)(array -> children[i]);
                if (object -> namingScheme == Object::NamingScheme::Enumerated) { // ForLoop only checks over enumerated things
                    iterator.ghost = object;
                    internalObject -> render(fd, scope, true); // force dereference the internal anonymous object
                }
            }
        }
        internalObject -> dropObject(&iterator);
    }

    virtual void pTree(int tabLevel = 0) { // replacing debugPrint because it's much more usefulicious
        for (int x = 0; x < tabLevel; x ++) {printf("\t");}
        printf("For loop over %s with iterator named %s\n", goal, iteratorName);
        internalObject -> pTree(tabLevel + 1);
    }
};


struct IfStatement : Node {
    enum Mode {
        Config,
        Equality,
        Existence
    } mode;
    union {
        char* configName;
        struct { // for equality mode
            char* oneName;
            char* twoName;
        };
        char* exName;
    };

    Object* mainObject;
    Object* elseObject;
    bool hasElse = false;

    ~IfStatement() {
        delete mainObject; // can't have been referenced, 'twas virtual
        if (hasElse) {
            delete elseObject; // ditto
        }
        switch (mode) {
            case Config:
                free(configName);
                break;
            case Equality:
                free(oneName);
                free(twoName);
                break;
            case Existence:
                free(exName);
                break;
        };
    }

    void render(int fd, Object* scope, bool dereference) {
        bool is = false;
        if (mode == Mode::Config) {
            for (const char* configItem : config) {
                if (strcmp(configItem, configName) == 0) {
                    is = true;
                    break;
                }
            }
        }
        else if (mode == Mode::Equality) {
            Object* one = scope -> lookup(oneName);
            if (one == NULL) {
                one = parent -> lookup(oneName);
            }
            if (one == NULL) {
                printf(ERROR "Can't find %s for an if statement. The output will be malformed.\n", oneName);
                return;
            }
            Object* two = scope -> lookup(twoName);
            if (two == NULL) {
                two = parent -> lookup(twoName);
            }
            if (two == NULL) {
                printf(ERROR "Can't find %s for an if statement. The output will be malformed.\n", twoName);
                return;
            }
            if (one == two) { // if the pointers are the same
                is = true;
            }
            else if (one -> children.size() == two -> children.size()) {
                bool was = true;
                for (size_t i = 0; i < one -> children.size(); i ++) {
                    if (one -> children[i] -> type != two -> children[i] -> type) {
                        was = false;
                        break;
                    }
                    if (one -> children[i] -> type == Node::Type::PLAINTEXT) {
                        PlainText* tOne = (PlainText*)(one -> children[i]);
                        PlainText* tTwo = (PlainText*)(two -> children[i]);
                        if (tOne -> size != tTwo -> size) {
                            was = false;
                            break;
                        }
                        if (strncmp(tOne -> data, tTwo -> data, tOne -> size) != 0) {
                            was = false;
                            break;
                        }
                    }
                    if (one -> children[i] -> type == Node::Type::TEXTBLOB) {
                        TextBlob* tOne = (TextBlob*)(one -> children[i]);
                        TextBlob* tTwo = (TextBlob*)(two -> children[i]);
                        if (strcmp(tOne -> data, tTwo -> data) != 0) {
                            was = false;
                            break;
                        }
                    }
                }
                if (was) {
                    is = true;
                }
            }
        }
        else if (mode == Mode::Existence) {
            Object* o = scope -> lookup(exName);
            if (o == NULL) {
                o = parent -> lookup(exName);
            }
            if (o != NULL) {
                is = true;
            }
        }
        if (is) {
            mainObject -> render(fd, scope, true);
        }
        else if (hasElse) {
            elseObject -> render(fd, scope, true);
        }
    }
};


struct Dereference : Node { // dereference and render an Object (the [^] operator)
    char* name;

    ~Dereference() {
        free(name);
    }

    void render(int fd, Object* scope, bool dereference) {
        Object* found = scope -> lookup(name);
        if (found == NULL && parent != NULL) {
            found = parent -> lookup(name);
        }
        if (found == NULL) {
            printf(ERROR "Couldn't find %s! The output \033[1mwill\033[0m be malformed.\n", name);
            return;
        }
        if (found -> isFile) { // dereferencing file roots copies over all their objects to you immediately
            for (Node* thing : found -> children) {
                if (thing -> type == Node::Type::OBJECT) {
                    Object* o = (Object*)thing;
                    if (o -> namingScheme == Object::NamingScheme::Named) {
                        if (strcmp(o -> name, "filename") == 0) {
                            continue; // skip filenames, of course (we want them to be "pure" to the file they're a part of)
                        }
                    }
                    if (!scope -> replace(o -> name, o)) {
                        o -> rCount ++;
                        scope -> addChild(o);
                    }
                }
            }
        }
        found -> render(fd, parent, true);
    }
};


size_t fillObject(const char* from, size_t length, Object* container, FileFlags* fileflags) { // designed to recurse
    // if length == 0, just run until we hit a closing tag (and then consume the closing tag, for nesting)
    // returns the amount it consumed
    size_t i = 0;
    bool escape = false;
    while (i < length || length == 0) { // TODO: backslash escaping (both in tag data and in plaintext data)
        if (from[i] == '\\' && !escape) {
            escape = true;
            i ++;
        }
        if (from[i] == '[' && !escape) {
            i ++;
            size_t tagStart = i;
            while (from[i] != ']' && (i < length || length == 0)) {
                i ++;
            }
            char tagOp = from[tagStart];
            size_t tagDataSize = i - tagStart - 1; // the -1 is because we consumed the first byte, the operation code
            const char* tagData = from + tagStart + 1; // the +1 is for the same reason as above
            while (tagData[0] == ' ') {tagData ++;tagDataSize --;} // trim whitespace from the start
            // god I love C
            // note: whitespace *after* the tag data is considered part of the content, but not whitespace *before*.
            if (tagOp == '=') {
                Object* obj = new Object; // we just created an object with [=]
                const char* objName = tagData;
                size_t objNameLength = 0;
                while (tagData[objNameLength] != ' ' && tagData[objNameLength] != '-') {
                    objNameLength ++;
                }
                if (objNameLength == 1 && objName[0] == '+') { // it's enumerated, an array.
                    obj -> namingScheme = Object::NamingScheme::Enumerated;
                    obj -> number = container -> highestEnumerated;
                    container -> highestEnumerated ++;
                }
                else {
                    obj -> namingScheme = Object::NamingScheme::Named;
                    obj -> name = truncatn(objName, objNameLength, '.');
                }
                if (tagData[tagDataSize - 1] == '-') {
                    i += fillObject(from + i + 1, 0, obj, fileflags);
                }
                else {
                    PlainText* text = new PlainText;
                    text -> size = tagDataSize - objNameLength - 1; // the -1 and +1 are because of the space
                    text -> data = tagData + objNameLength + 1;
                    obj -> addChild(text);
                }
                obj -> fileflags = *fileflags;
                container -> addChild(obj);
            }
            else if (tagOp == 'f') {
                Object* loopObject = new Object;
                loopObject -> parent = container;
                ForLoop* loop = new ForLoop;
                loop -> internalObject = loopObject;
                const char* goal = tagData;
                size_t goalLength;
                for (goalLength = 0; tagData[goalLength] != ' '; goalLength ++);
                loop -> goal = strdupn(goal, goalLength);

                const char* iteratorName = tagData + goalLength + 1; // consume the goal section and the whitespace
                size_t iteratorLen = tagDataSize - goalLength - 1;
                loop -> iteratorName = strdupn(iteratorName, iteratorLen);
                i += fillObject(from + i + 1, 0, loopObject, fileflags);
                loop -> fileflags = *fileflags;
                container -> addChild(loop);
            }
            else if (tagOp == 'i') {
                IfStatement* ifs = new IfStatement;
                const char* ifCommand = tagData;
                size_t ifCommandLength;
                for (ifCommandLength = 0; ifCommand[ifCommandLength] != ' '; ifCommandLength ++); // TODO: syntax verification
                // (this could potentially trigger a catastrophic buffer overflow)
                if (strncmp(ifCommand, "config", ifCommandLength) == 0) {
                    const char* configName = tagData + ifCommandLength + 1; // the +1 consumes the space
                    size_t configNameLen = tagDataSize - ifCommandLength - 1;
                    ifs -> mode = IfStatement::Mode::Config;
                    ifs -> configName = strdupn(configName, configNameLen);
                }
                else if (strncmp(ifCommand, "equals", ifCommandLength) == 0) {
                    const char* name1 = tagData + ifCommandLength + 1; // the +1 consumes the space
                    size_t name1len;
                    for (name1len = 0; name1[name1len] != ' '; name1len ++);
                    const char* name2 = name1 + name1len + 1; // the +1 consumes the space... of DOOM
                    size_t name2len = tagDataSize - ifCommandLength - name1len - 2; // the -2 ignores some spaces... of DOOM
                    ifs -> mode = IfStatement::Mode::Equality;
                    ifs -> oneName = strdupn(name1, name1len);
                    ifs -> twoName = strdupn(name2, name2len);
                }
                else if (strncmp(ifCommand, "exists", ifCommandLength) == 0) {
                    const char* exName = tagData + ifCommandLength + 1;
                    size_t nameLen = tagDataSize - ifCommandLength - 1;
                    ifs -> mode = IfStatement::Mode::Existence;
                    ifs -> exName = strdupn(exName, nameLen);
                }
                Object* ifObj = new Object;
                ifObj -> parent = container;
                i += fillObject(from + i + 1, 0, ifObj, fileflags);
                ifs -> mainObject = ifObj;
                if (wasElse) {
                    ifs -> hasElse = true;
                    Object* elseObj = new Object;
                    elseObj -> parent = container;
                    i += fillObject(from + i, 0, elseObj, fileflags);
                    ifs -> elseObject = elseObj;
                    wasElse = false;
                }
                ifs -> fileflags = *fileflags;
                container -> addChild(ifs);
            }
            else if (tagOp == 'e' && length == 0) { // same behavior as /, but it also signals to whoever's calling that it terminated-on-else
                while (from[i] != ']') {i ++;} // consume at least the closing "]", and anything before it too
                wasElse = true;
                break;
            }
            else if (tagOp == 'd') {
                DebuggerStatement* debugger = new DebuggerStatement;
                container -> addChild(debugger);
            }
            else if (tagOp == '^') {
                Dereference* d = new Dereference;
                d -> name = strdupn(tagData, tagDataSize);
                d -> fileflags = *fileflags;
                container -> addChild(d);
            }
            else if (tagOp == '/' && length == 0) { // if we hit a closing tag while length == 0, it's because there's a closing tag no subroutine consumed.
                // if it wasn't already consumed, it's either INVALID or OURS! Assume the latter and break.
                while (from[i] != ']') {i ++;} // consume at least the closing "]", and anything before it too
                break; 
            }
            else if (tagOp == '#') { // update: include will be kept because of the auto-escaping feature, which is nice.
                //printf(WARNING "The functionality of [#] has been reviewed and it may be deprecated in the near future.\n\tPlease see the Noteboard (https://swaous.asuscomm.com/sitix/pages/noteboard.html) for March 10th, 2024 for more information.\n");
                Dereference* d = new Dereference;
                char* unescaped = strdupn(tagData, tagDataSize);
                d -> name = escapeString(unescaped, '.');
                free(unescaped);
                d -> fileflags = *fileflags;
                container -> addChild(d);
            }
            else if (tagOp == '@') {
                const char* tagRequest = tagData;
                size_t tagRequestSize;
                for (tagRequestSize = 0; tagRequest[tagRequestSize] != ' '; tagRequestSize ++);
                const char* tagTarget = tagData + tagRequestSize + 1;
                size_t tagTargetSize = tagDataSize - tagRequestSize - 1;
                if (strncmp(tagRequest, "on", tagRequestSize) == 0) {
                    if (strncmp(tagTarget, "minify", tagTargetSize) == 0) {
                        fileflags -> minify = true;
                    }
                }
                else if (strncmp(tagRequest, "off", tagRequestSize) == 0) {
                    if (strncmp(tagTarget, "minify", tagTargetSize) == 0) {
                        fileflags -> minify = false;
                    }
                }
            }
            else {
                printf(WARNING "Unrecognized tag operation %c! Parsing will continue, but the result may be malformed.\n", tagOp);
            }
        }
        else if (from[i] == ']' && !escape) {
            printf(INFO "Unmatched closing bracket detected! This is probably not important; there are several minor interpreter bugs that can cause this without actually breaking anything.");
        }
        else {
            size_t plainStart = i;
            while ((from[i] != '[' || (from[i - 1] == '\\' && from[i - 2] != '\\')) && (i < length || length == 0)) {
                if (from[i] == '\\') {
                    if (from[i + 1] == '\\') {
                        i ++;
                    }
                    break;
                }
                i ++;
            }
            PlainText* text = new PlainText;
            text -> size = i - plainStart;
            i --;
            text -> data = from + plainStart;
            text -> fileflags = *fileflags;
            container -> addChild(text);
        }
        escape = false;
        i ++;
    }
    return i + 1; // the +1 consumes whatever byte we closed on
}


Object* string2object(const char* data, size_t length, FileFlags* flags) {
    Object* ret = new Object;
    ret -> fileflags = *flags;
    if (strncmp(data, "[!]", 3) == 0) { // it's a valid Sitix file
        fillObject(data + 3, length - 3, ret, flags);
    }
    else if (strncmp(data, "[?]", 3) == 0) {
        fillObject(data + 3, length - 3, ret, flags);
        ret -> isTemplate = true;
    }
    else {
        PlainText* text = new PlainText;
        text -> fileflags = *flags;
        text -> data = data;
        text -> size = length;
        ret -> addChild(text);
    }
    return ret;
}


void renderFile(const char* in, const char* out) {
    FileFlags fileflags;
    printf(INFO "Rendering %s to %s.\n", in, out);
    struct stat buf;
    stat(in, &buf);
    size_t size = buf.st_size;
    if (size == 0) {
        printf(WARNING "%s has zero size! This file will not be rendered.\n\tIf you absolutely need an empty file there, consider writing a custom script that creates it after invoking Sitix.\n", in);
        return;
    }
    int input = open(in, O_RDONLY);
    if (input == -1) {
        printf(ERROR "Couldn't open %s! This file will not be rendered.\n", in);
        return;
    }
    char* inMap = (char*)mmap(0, size, PROT_READ, MAP_SHARED, input, 0); // it can handle obscenely large files because of map_shared
    if (inMap == MAP_FAILED) {
        close(input); // housekeeping
        printf(ERROR "Couldn't memory map %s! This file will not be rendered.\n", in);
        return;
    }

    Object* file = string2object(inMap, size, &fileflags);
    if (file -> isTemplate) {
        printf(INFO "%s is marked [?], will not be rendered.\n", in);
        printf("\t If this file should be rendered, replace [?] with [!] in the header.\n");
    }
    else {
        int output = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0666); // rw for anyone, it's not a protected file (this may lead to security vulnerabilities in the future)
        // TODO: inherit permissions from the parent directory by some intelligent strategy so Sitix doesn't accidentally give attackers access to secure information
        // contained in a statically generated website.
        if (output == -1) {
            close(input); // housekeeping
            printf(ERROR "Couldn't open output file %s (for %s). This file will not be rendered.\n", out, in);
            return;
        }
        //file -> lookup("tuba");
        file -> render(output, file, true);
        close(output);
    }
    file -> pushedOut();
    
    close(input);
    munmap(inMap, size);
}


int iterRemove(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    if (remove(fpath)) {
        printf(WARNING "Encountered a non-fatal error while removing %s\n", fpath);
    }
    return 0;
}

void rmrf(const char* path) {
    nftw(path, iterRemove, 64, FTW_DEPTH);
}


int renderTree(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    char* thing = transmuted(siteDir, outputDir, fpath);
    if (typeflag == FTW_D) {
        mkdir(thing, 0);
        chmod(thing, 0755);
    }
    else {
        renderFile(fpath, thing);
    }
    free(thing);
    return FTW_CONTINUE;
}

int main(int argc, char** argv) {
    printf("\033[1mSitix v1.0 by Tyler Clarke\033[0m\n");
    bool hasSpecificSitedir = false;
    for (int i = 1; i < argc; i ++) {
        if (strcmp(argv[i], "-o") == 0) {
            i ++;
            outputDir = argv[i];
        }
        else if (strcmp(argv[i], "-c") == 0) {
            i ++;
            config.push_back(argv[i]);
        }
        else if (!hasSpecificSitedir) {
            hasSpecificSitedir = true;
            siteDir = argv[i];
        }
        else {
            printf(ERROR "Unexpected argument %s\n", argv[i]);
        }
    }
    printf(INFO "Cleaning output directory\n");
    rmrf(outputDir);
    mkdir(outputDir, 0);
    chmod(outputDir, 0755);
    printf(INFO "Output directory clean.\n");
    printf(INFO "Rendering project '%s' to '%s'.\n", siteDir, outputDir);
    nftw(siteDir, renderTree, 64, 0);
    printf("\033[1;33mComplete!\033[0m\n");
    return 0;
}
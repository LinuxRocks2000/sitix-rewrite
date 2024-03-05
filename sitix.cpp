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
    [i variable == v2]
        Hello, World
    [/]

    Or 

    [=variable Data]
    [=variable2 OtherData]
    [i variable != v2]
        Hello, World
    [/]

    Else is also supported -
    
    [=variable Data]
    [=variable2 OtherData]
    [i variable == v2]
        They're the same.
    [e]
        They're different.
    [/]

    Escaping is done with backslashes.
*/


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


#define INFO     "\033[32m[  INFO   ]\033[0m "
#define ERROR  "\033[1;31m[  ERROR  ]\033[0m "
#define WARNING  "\033[33m[ WARNING ]\033[0m "


const char* outputDir = "output"; // global variables are gross but, eh, screw you
const char* siteDir = "";


char* strdupn(const char* thing, size_t length) { // copy length bytes of a string to a new, NULL-terminated C string
    char* ret = (char*)malloc(length + 1);
    ret[length] = 0;
    for (size_t i = 0; i < length; i ++) {
        ret[i] = thing[i];
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


struct Node { // superclass
    Object* parent = NULL; // everything has an Object parent, except the root Object (which will have a NULL parent)
    bool isObject = false; // it's necessary to specificate from Node to Object in some cases

    virtual void render(int fd, Object* scope, bool dereference = false) = 0; // true virtual function
    // scope is a SECONDARY scope. If a lookup fails in the primary scope (the parent), scope lookup will be performed in the secondary scope.
    // dereference causes forced rendering on objects (objects don't render by default unless dereferenced with [^name])

    virtual void free() {}; // optional

    virtual void debugPrint() {}; // optional
};


struct PlainText : Node {
    const char* data = NULL;
    size_t size = 0;

    void render(int fd, Object* scope, bool dereference) {
        if (size == 1 && (data[0] == ' ' || data[0] == '\n')) {
            return;
        }
        write(fd, data, size);
    }
};


struct Object : Node { // Sitix objects contain a list of *nodes*, which can be enumerated (like for array reference), named (for variables), operations, or pure text.
    // Sitix objects are only converted to text when it's absolutely necessary; e.g. when the root object is rendered. Scoping is KISS - when an object resolve
    // is requested, we walk down the chain (towards the root) until we hit an object containing the right name, then walk up to match nested variables.
    // Variable dereferences are considered operations and will not be executed until the relevant section is rendered; it is important to make sure your
    // Sitix code is aware of the scope caveats at all times.
    std::vector<Node*> children;
    bool isTemplate = false;

    Object() {
        isObject = true;
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

    void render(int fd, Object* scope, bool dereference) { // objects are just delegation agents, they don't contribute anything to the final text.
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

    void free() { // destructors are cringe
        for (Node* child : children) {
            child -> free();
            delete child;
        }
        if (namingScheme == NamingScheme::Named) {
            ::free(name);
        }
        children.clear();
    }

    Object* lookup(const char* name) { // lookup variant that works on NAMES
        // returning NULL means no suitable object was found here or at any point down in the tree
        // TODO: nested object lookup
        for (Node* node : children) {
            if (node -> isObject) {
                Object* candidate = (Object*)node;
                if (candidate -> namingScheme == Object::NamingScheme::Named && strcmp(candidate -> name, name) == 0) {
                    return candidate;
                }
            }
        }
        return NULL;
    }

    Object* lookup(uint32_t index) { // indexed lookup (for arrays)
        // TODO: implement
        return NULL;
    }

    bool replace(Object* obj) { // LEAKS MEMORY BY FAILING TO FREE REPLACED OBJECTS!
        bool didReplace = false;
        for (size_t i = 0; i < children.size(); i ++) {
            if (children[i] -> isObject) {
                Object* candidate = (Object*)(children[i]);
                if (candidate -> namingScheme == Object::NamingScheme::Named && strcmp(candidate -> name, obj -> name) == 0) {
                    children[i] = obj;
                    obj -> parent = this;
                    didReplace = true;
                }
            }
        }
        if (!didReplace) {
            if (parent != NULL) {
                didReplace = parent -> replace(obj);
            }
        }
        return didReplace;
    }

    void debugPrint() {
        printf("Object ");
        if (namingScheme == NamingScheme::Named) {
            printf("named %s", name);
        }
        printf("\n");
        printf("my content: \n\n");
        render(0, NULL, true);
        printf("\n\n");
    }
};


Object string2object(const char* data, size_t length); // forward-dec


struct Include : Node { // adds a file (useful for dynamic templating)
    char* fname;

    void render(int fd, Object* scope, bool dereference) {
        char* transmuteName = transmuted("", siteDir, fname);
        int file = open(transmuteName, O_RDONLY);
        if (file == -1) {
            printf(ERROR "Couldn't open %s for inclusion.\n", transmuteName);
            return;
        }
        struct stat data;
        if (stat(transmuteName, &data)) {
            printf(ERROR "Can't grab file information for %s.\n", transmuteName);
            perror("\tstat");
        }
        char* map = (char*)mmap(0, data.st_size, PROT_READ, MAP_SHARED, file, 0);
        if (map == MAP_FAILED) {
            printf(ERROR "Can't memory map %s for inclusion in another file! The output \033[1mwill\033[0m be malformed.\n", transmuteName);
            printf("\tIf %s is a template file, the file it is templating will most likely be left empty.\n", transmuteName);
            perror("\tmmap");
            return;
        }
        Object* object = new Object;
        *object = string2object(map, data.st_size);
        for (Node* n : object -> children) { // INEFFICIENT method of unpacking the loaded file into the parent context.
            n -> parent = parent;
            parent -> addChild(n);
        }
    }

    void free() {
        ::free(fname);
    }
};


struct Dereference : Node { // dereference and render an Object (the [^] operator)
    char* name;

    void render(int fd, Object* scope, bool dereference) {
        Object* found = parent -> lookup(name);
        if (found == NULL) {
            found = scope -> lookup(name);
        }
        if (found == NULL) {
            printf(ERROR "Couldn't find %s! The output \033[1mwill\033[0m be malformed.\n", name);
            return;
        }
        found -> render(fd, scope, true);
    }

    void free() {
        ::free(name);
    }
};


struct Setter : Node {
    Object* object;

    void render(int fd, Object* scope, bool dereference) { // doesn't write anything, just swaps in the new object
        bool didReplace = false;
        didReplace |= scope -> replace(object);
        if (parent != NULL) {
            didReplace |= parent -> replace(object);
        }
        if (!didReplace) { // if we can't find the object we're supposed to replace, just shove it in the highest applicable point on the tree
            parent -> addChild(object);
        }
    }

    void free() {
        // no free tasks for this guy, it only has data borrowed from other peopl
    }
};


size_t fillObject(const char* from, size_t length, Object* container) { // designed to recurse
    // if length == 0, just run until we hit a closing tag (and then consume the closing tag, for nesting)
    // returns the amount it consumed
    size_t i = 0;
    while (i < length || length == 0) { // TODO: backslash escaping (both in tag data and in plaintext data)
        if (from[i] == '[') {
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
                obj -> namingScheme = Object::NamingScheme::Named;
                obj -> name = strdupn(objName, objNameLength);
                if (tagData[tagDataSize - 1] == '-') {
                    i += fillObject(from + i + 1, 0, obj);
                }
                else {
                    PlainText* text = new PlainText;
                    text -> size = tagDataSize - objNameLength - 1; // the -1 and +1 are because of the space
                    text -> data = tagData + objNameLength + 1;
                    obj -> addChild(text);
                }
                Object* collision = container -> lookup(obj -> name);
                if (collision != NULL) { // this is setting a value rather than declaring a new variable, so we have to instead put in a Setter
                    Setter* setter = new Setter;
                    setter -> object = obj;
                    container -> addChild(setter);
                }
                else {
                    container -> addChild(obj);
                }
            }
            else if (tagOp == '^') {
                Dereference* d = new Dereference;
                d -> name = strdupn(tagData, tagDataSize);
                container -> addChild(d);
            }
            else if (tagOp == '/' && length == 0) { // if we hit a closing tag while length == 0, it's because there's a closing tag no subroutine consumed.
                // if it wasn't already consumed, it's either INVALID or OURS! Assume the latter and break.
                while (from[i] != ']') {i ++;} // consume at least the closing "]", and anything before it too
                break; 
            }
            else if (tagOp == '#') { // include a file. Include actually does three things:
            // 1. At render-time, load the file as a memory map and parse it to an object tree
            // 2. Push the loaded file into the object tree at the appropriate position in the object tree (sibling to the actual Include operation)
            // 3. Render the object in forced dereference mode
                Include* i = new Include;
                i -> fname = strdupn(tagData, tagDataSize);
                container -> addChild(i);
            }
            else {
                printf(WARNING "Unrecognized tag operation %c! Parsing will continue, but the result may be malformed.\n", tagOp);
            }
        }
        else if (from[i] == ']') {
            printf("Something has gone terribly wrong\n");
        }
        else {
            size_t plainStart = i;
            while (from[i] != '[' && (i < length || length == 0)) {
                i ++;
            }
            PlainText* text = new PlainText;
            text -> size = i - plainStart;
            i --;
            text -> data = from + plainStart;
            container -> addChild(text);
        }
        i ++;
    }
    return i + 1; // the +1 consumes whatever byte we closed on
}


Object string2object(const char* data, size_t length) {
    Object ret;
    if (strncmp(data, "[!]", 3) == 0) { // it's a valid Sitix file
        fillObject(data + 3, length - 3, &ret);
    }
    else if (strncmp(data, "[?]", 3) == 0) {
        fillObject(data + 3, length - 3, &ret);
        ret.isTemplate = true;
    }
    else {
        PlainText* text = new PlainText;
        text -> data = data;
        text -> size = length;
        ret.addChild(text);
    }
    return ret;
}


void renderFile(const char* in, const char* out) {
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

    Object file = string2object(inMap, size);
    if (file.isTemplate) {
        printf(INFO "%s is a template file, will not be rendered.\n", in);
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
        file.render(output, &file, true);
        close(output);
    }
    file.free();
    
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


int renderRecursive(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    char* thing = transmuted(siteDir, outputDir, fpath);
    if (typeflag == FTW_D) {
        mkdir(thing, 0700);
    }
    else {
        renderFile(fpath, thing);
    }
    free(thing);
    return FTW_CONTINUE;
}

int main(int argc, char** argv) {
    printf("\033[1mSitix v0.1 by Tyler Clarke\033[0m\n");
    bool hasSpecificSitedir = false;
    for (int i = 1; i < argc; i ++) {
        if (strcmp(argv[i], "-o") == 0) {
            i ++;
            outputDir = argv[i];
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
    mkdir(outputDir, 0700);
    printf(INFO "Output directory clean.\n");
    printf(INFO "Rendering project '%s' to '%s'.\n", siteDir, outputDir);
    nftw(siteDir, renderRecursive, 64, 0);
    printf("\033[1;33mComplete!\033[0m\n");
    return 0;
}
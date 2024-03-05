/* Sitix, by Tyler Clarke
    An efficient templating engine written in C++, meant to reduce the pain of web development.
    Sitix templating is optional; files that desire Sitix templating should include [!] as the first three bytes. Otherwise, sitix will simply copy them.
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


char* strdupn(const char* thing, size_t length) { // copy length bytes of a string to a new, NULL-terminated C string
    char* ret = (char*)malloc(length + 1);
    ret[length] = 0;
    for (size_t i = 0; i < length; i ++) {
        ret[i] = thing[i];
    }
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

    void replace(Object* obj) { // LEAKS MEMORY!
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
                parent -> replace(obj);
            }
        }
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
        int file = open(fname, O_RDONLY);
        if (file == -1) {
            printf("Couldn't open %s for inclusion.\n", fname);
            return;
        }
        struct stat data;
        stat(fname, &data);
        char* map = (char*)mmap(0, data.st_size, PROT_READ, MAP_SHARED, file, 0);
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
            printf("Couldn't find %s!\n", name);
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

    void render(int fd, Object* scope, bool dereference) {
        scope -> replace(object); // doesn't write anything, just swaps in the new object
        if (parent != NULL) {
            parent -> replace(object);
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
            printf("tagop is %c\n", tagOp);
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
                printf("WARNING: Unrecognized tag operation %c! Parsing will continue, but the result may be malformed.\n", tagOp);
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
    else {
        PlainText* text = new PlainText;
        text -> data = data;
        text -> size = length;
        ret.addChild(text);
    }
    return ret;
}


void renderFile(const char* in, const char* out) {
    printf("INFO: Rendering %s to %s.\n", in, out);
    struct stat buf;
    stat(in, &buf);
    size_t size = buf.st_size;
    int input = open(in, O_RDONLY);
    if (input == -1) {
        printf("ERROR: Couldn't open %s! This file will not be rendered.\n", in);
        return;
    }
    int output = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0666); // rw for anyone, it's not a protected file (this may lead to security vulnerabilities in the future)
    // TODO: inherit permissions from the parent directory by some intelligent strategy so Sitix doesn't accidentally give attackers access to secure information
    // contained in a statically generated website.
    if (output == -1) {
        close(input); // housekeeping
        printf("ERROR: Couldn't open output file %s (for %s). This file will not be rendered.\n", out, in);
        return;
    }
    char* inMap = (char*)mmap(0, size, PROT_READ, MAP_SHARED, input, 0); // it can handle obscenely large files because of map_shared
    if (inMap == MAP_FAILED) {
        close(input); // housekeeping
        close(output);
        printf("ERROR: Couldn't memory map %s! This file will not be rendered.\n", in);
        return;
    }

    Object file = string2object(inMap, size);
    file.render(output, &file, true);
    file.free();
    
    close(output);
    close(input);
    munmap(inMap, size);
}

int main(int argc, char** argv) {
    renderFile("test/index.html", "testOut.html");
}
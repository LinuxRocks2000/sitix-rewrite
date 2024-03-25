#include <types/Object.hpp>
#include <defs.h>
#include <util.hpp>
#include <dirent.h>
#include <types/TextBlob.hpp>
#include <types/PlainText.hpp>


Object::Object() {
    type = OBJECT;
}

void Object::pushedOut() {
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

Object::~Object() {/*
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

void Object::render(SitixWriter* out, Object* scope, bool dereference) { // objects are just delegation agents, they don't contribute anything to the final text.
    if (ghost != NULL) {
        ghost -> render(out, scope, dereference);
        return;
    }
    if (namingScheme == NamingScheme::Named) { // when objects are rendered, they replace the other objects of the same name on the scope tree.
        if (parent != NULL && !dereference && virile) { // replace operations are ALWAYS on the parent, we can't intrude on someone else's scoped
            parent -> replace(name, this);
        }
    }
    if (!dereference) {
        return;
    }
    for (size_t i = 0; i < children.size(); i ++) {
        children[i] -> render(out, scope);
    }
}

void Object::addChild(Node* child) {
    child -> parent = this;
    children.push_back(child);
    child -> attachToParent(this);
}

void Object::dropObject(Object* object) {
    for (size_t i = 0; i < children.size(); i ++) {
        if (children[i] == object) {
            children.erase(children.begin() + i);
        }
    }
}

Object* Object::lookup(std::string& lname, Object* nope) { // lookup variant that works on NAMES
    // returning NULL means no suitable object was found here or at any point down in the tree
    // if `nope` is non-null, it will be used as a discriminant (it will not be returned)
    // note that copied objects will be returned; `nope` uses pointer-comparison only.
    if (ghost != NULL) {
        return ghost -> lookup(lname, nope);
    }
    size_t rootSegLen;
    for (rootSegLen = 0; rootSegLen < lname.size(); rootSegLen ++) {
        if (lname[rootSegLen] == '.' && lname[rootSegLen - 1] != '\\') {
            break;
        }
    }
    char* rootBit = strdupn(lname.c_str(), rootSegLen);
    char* root = strip(rootBit, '\\');
    free(rootBit);
    if (strcmp(root, "__this__") == 0) {
        free(root);
        if (rootSegLen == lname.size()) {
            return this;
        }
        else {
            return this -> childSearchUp(lname.c_str() + rootSegLen + 1);
        }
    }
    if (strcmp(root, "__file__") == 0) {
        free(root);
        Object* w = walkToFile();
        if (rootSegLen == lname.size()) {
            return w;
        }
        else {
            return w -> childSearchUp(lname.c_str() + rootSegLen + 1);
        }
    }
    if (isFile && (namingScheme == NamingScheme::Named) && (strcmp(name.c_str(), root) == 0)) { // IF we're a file (or root), AND we have a name, AND the name matches, return us.
        free(root); // this allows for things like comparing, say, tuba/rhubarb.stx with __file__
        if (rootSegLen == lname.size()) {
            return this;
        }
        else {
            return childSearchUp(lname.c_str() + rootSegLen + 1);
        }
    }
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
            if (candidate -> namingScheme == Object::NamingScheme::Named && strcmp(candidate -> name.c_str(), root) == 0) {
                free(root);
                if (rootSegLen == lname.size()) {
                    return candidate;
                }
                else {
                    return candidate -> childSearchUp(lname.c_str() + rootSegLen + 1); // the +1 is to consume the '.'
                }
            }
        }
    }
    if (parent == NULL) { // if we ARE the parent
        // config searches, directory unpacks and file unpacks are on the root scope, see
        // check config
        for (Object* cnf : config) {
            if (cnf -> name == lname) {
                return cnf;
            }
        }
        // unpack a directory/file
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
                    std::string transmuteName = escapeString(transmuteNamep1, '.');
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
                if (rootSegLen == lname.size()) {
                    return dirObject;
                }
                else {
                    return dirObject -> childSearchUp(lname.c_str() + rootSegLen + 1);
                }
            }
            else if (S_ISREG(sb.st_mode)) {
                // construct the "filename" object inside loaded files
                // TODO: add a truncated filename object inside the loaded file, which would contain "mod1.html" rather than
                // "templates/modules/mod1.html", for instance.
                TextBlob* fNameContent = new TextBlob;
                fNameContent -> data = root;
                Object* fNameObj = new Object;
                fNameObj -> virile = false;
                fNameObj -> namingScheme = Object::NamingScheme::Named;
                fNameObj -> name = "filename";
                fNameObj -> addChild(fNameContent);

                MapView map(directoryName);
                if (!map.isValid()) {
                    printf(ERROR "Invalid map!\n");
                    return NULL;
                }

                // put together the actual file object, store it on global, and return it
                Object* fileObj = new Object;
                fileObj -> isFile = true;
                fileObj -> addChild(fNameObj); // add the filename to the object
                fileObj -> namingScheme = Object::NamingScheme::Named;
                fileObj -> name = strdup(root); // reference name of the object, so it can be quickly looked up later without another slow cold-load
                if (map.cmp("[?]") || map.cmp("[!]")) {
                    FileFlags flags;
                    map += 3;
                    fillObject(map, fileObj, &flags);
                    fNameContent -> fileflags = flags;
                    fNameObj -> fileflags = flags;
                }
                else {
                    PlainText* content = new PlainText(map);
                    content -> fileflags.sitix = false;
                    fileObj -> addChild(content);
                }
                addChild(fileObj); // since we're the global scope, we should add the file to us.
                // the goal is to create an illusion that the entire directory structure is a cohesive part of the object tree
                // and then sorta just load files when they ask us to
                free(directoryName);
                free(root);
                return fileObj;
            }
        }
        free(directoryName);
    }
    else {
        ::free(root);
        return parent -> lookup(lname, nope);
    }
    ::free(root);
    return NULL;
}

Object* Object::childSearchUp(const char* name, Object* context) { // name is expected to be a . separated 
    if (context == NULL) {
        context = parent;
    }
    if (ghost != NULL) {
        return ghost -> childSearchUp(name, parent);
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
                if (candidate -> namingScheme == Object::NamingScheme::Enumerated && toNumber(mSeg, highestEnumerated) == candidate -> number) {
                    ::free(mSeg);
                    if (segLen == nameLen) {
                        return candidate;
                    }
                    else {
                        return candidate -> childSearchUp(name + segLen + 1);
                    }
                }
            }
            else if (strcmp(mSeg, "__before__") == 0) {
                Object* last = NULL;
                for (Node* n : context -> children) {
                    if (n -> type == Node::Type::OBJECT) {
                        Object* candidate = (Object*)n;
                        if (candidate -> ptrEquals(this)) {
                            break;
                        }
                        else if (candidate -> namingScheme == NamingScheme::Enumerated) {
                            last = candidate;
                        }
                    }
                }
                if (segLen == nameLen) { // if we're the last requested entry
                    return last;
                }
                else if (last != NULL) {
                    return last -> childSearchUp(name + segLen + 1);
                }
                else {
                    return NULL;
                }
            }
            else if (strcmp(mSeg, "__after__") == 0) {
                Object* next = NULL;
                bool goin = false;
                for (Node* n : context -> children) {
                    if (n -> type == Node::Type::OBJECT) {
                        Object* candidate = (Object*)n;
                        if (candidate -> ptrEquals(this)) {
                            goin = true;
                        }
                        else if (goin && candidate -> namingScheme == NamingScheme::Enumerated) {
                            next = candidate;
                            break;
                        }
                    }
                }
                if (segLen == nameLen) { // if we're the last requested entry
                    return next;
                }
                else if (next != NULL) {
                    return next -> childSearchUp(name + segLen + 1);
                }
                else {
                    return NULL;
                }
            }
            else if (candidate -> namingScheme == Object::NamingScheme::Named && strcmp(candidate -> name.c_str(), mSeg) == 0) {
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

bool Object::ptrEquals(Object* thing) {
    if (ghost != NULL) {
        return ghost -> ptrEquals(thing);
    }
    return this == thing;
}

void Object::pTree(int tabLevel) {
    for (int x = 0; x < tabLevel; x ++) {printf("\t");}
    printf("Object ");
    if (parent == NULL) {
        printf("without parent (root) ");
    }
    else {
        printf("with parent (%d) ", parent);
    }
    if (namingScheme == NamingScheme::Named) {
        printf("named %s ", name.c_str());
    }
    else if (namingScheme == NamingScheme::Enumerated) {
        printf("#%d ", number);
    }
    else {
        printf("unnamed ");
    }
    printf("(%d)", this);
    if (ghost != NULL) {
        printf(" ghosting %d\n", ghost);
    }
    else {
        printf("\n");
    }
    for (Node* child : children) {
        child -> pTree(tabLevel + 1);
    }
}

void Object::ptrswap(Object* current, Object* repl) {
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

Object* Object::deghost() { // walk across the ghost tree, grabbing the actual object
    if (ghost == NULL) {
        return this;
    }
    return ghost -> deghost();
}

Object* Object::walkToFile() { // walk up the object tree, until we hit something that's a file
    if (isFile) { // if we're definitely a file, return us
        return this;
    }
    if (parent != NULL) { // if we have a parent and are not a file, ask the parent!
        return parent -> walkToFile();
    }
    printf(WARNING "File walk failed! This means the tree is misconfigured. The output may be malformed.\n");
    return this; // if we don't have a parent and aren't a file, return us anyways and provide an warning.
}

bool Object::replace(std::string& name, Object* obj) {
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

void Object::debugPrint() {
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

Object* Object::nonvRoot() { // walk up the parent tree until we reach the first non-virtual object
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

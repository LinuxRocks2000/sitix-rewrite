#include <types/ForLoop.hpp>
#include <types/Object.hpp>


ForLoop::~ForLoop() {
    if (internalObject == NULL) {
        printf("DOUBLE FREE\n");
    }
    internalObject -> pushedOut();
    internalObject = NULL;
}

ForLoop::ForLoop(Session* session, MapView tagData, MapView& map, FileFlags* flags) : Node(session) {
    internalObject = new Object(session);
    tagData.trim();
    goal = tagData.consume(' ').toString();
    tagData.trim();
    fileflags = *flags;
    iteratorName = tagData.toString(); // whatever's left is the name of the iterator
    fillObject(map, internalObject, flags, session);
}

void ForLoop::attachToParent(Object* thing) {
    internalObject -> parent = thing;
}

void ForLoop::render(SitixWriter* out, Object* scope, bool dereference) { // the memory management here is truly horrendous.
    Object* array = scope -> lookup(goal);
    if (array == NULL) {
        array = parent -> lookup(goal);
    }
    if (array == NULL) {
        printf(ERROR "Array lookup for %s failed. The output will be malformed.\n", goal);
        return;
    }
    Object iterator(sitix);
    iterator.namingScheme = Object::NamingScheme::Named;
    iterator.name = iteratorName;
    internalObject -> addChild(&iterator);
    for (size_t i = 0; i < array -> children.size(); i ++) {
        if (array -> children[i] -> type == Node::Type::OBJECT) {
            Object* object = (Object*)(array -> children[i]);
            if (object -> namingScheme == Object::NamingScheme::Enumerated) { // ForLoop only checks over enumerated things
                iterator.ghost = object;
                internalObject -> render(out, scope, true); // force dereference the internal anonymous object
            }
        }
    }
    internalObject -> dropObject(&iterator);
}

void ForLoop::pTree(int tabLevel) { // replacing debugPrint because it's much more usefulicious
    for (int x = 0; x < tabLevel; x ++) {printf("\t");}
    printf("For loop over %s with iterator named %s\n", goal.c_str(), iteratorName.c_str());
    internalObject -> pTree(tabLevel + 1);
}
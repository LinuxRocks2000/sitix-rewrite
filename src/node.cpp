#include <node.hpp>


void Node::pTree(int tabLevel) { // replacing debugPrint because it's much more usefulicious
    for (int x = 0; x < tabLevel; x ++) {printf("\t");}
    printf("Generic Node with type %d ", type);
    if (parent != NULL) {
        printf("with parent (%d)\n", parent);
    }
    else {
        printf("without parent\n");
    }
}

void Node::attachToParent(Object* parent) {

}

void Node::debugPrint() {

}

Node::~Node() {

}
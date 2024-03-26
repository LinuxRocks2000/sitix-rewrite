#include <types/PlainText.hpp>


PlainText::PlainText(Session* session, MapView d) : Node(session), data(d) {
    type = PLAINTEXT;
}

void PlainText::render(SitixWriter* stream, Object* scope, bool dereference) {
    stream -> setFlags(fileflags);
    stream -> write(data);
}

void PlainText::pTree(int tabLevel) { // replacing debugPrint because it's much more usefulicious
    for (int x = 0; x < tabLevel; x ++) {printf("\t");}
    printf("Text content\n");
}
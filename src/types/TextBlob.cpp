#include <types/TextBlob.hpp>


TextBlob::TextBlob(Session* session) : Node(session) {
    type = TEXTBLOB;
}

void TextBlob::render(SitixWriter* out, Object* scope, bool dereference) {
    out -> setFlags(fileflags);
    out -> write(data);
}

void TextBlob::pTree(int tabLevel) { // replacing debugPrint because it's much more usefulicious
    for (int x = 0; x < tabLevel; x ++) {printf("\t");}
    printf("Text content %d\n", this);
}
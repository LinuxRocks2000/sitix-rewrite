#include <types/DebuggerStatement.hpp>
#include <types/Object.hpp>


void DebuggerStatement::render(SitixWriter* out, Object* scope, bool dereference) {
    printf("==== DEBUGGER BREAK ====\n");
    printf("++++     PARENT     ++++\n");
    parent -> pTree(1);
    printf("++++      SCOPE     ++++\n");
    scope -> pTree(1);
    printf("====  DEBUGGER OVER ====\n");
}

DebuggerStatement::DebuggerStatement(Session* session) : Node(session) {}

void DebuggerStatement::pTree(int tabLevel) {
    for (int i = 0; i < tabLevel; i ++) {printf("\t");}
    printf("CALL TO DEBUGGER\n");
}
// Evals is a simple, turing-incomplete evaluated language designed for string processing. It directly integrates with and depends on the Sitix scope model.
// Evals is primarily designed for string manipulation and value comparison.
// It is not fancy; the syntax is redolent of Tyler++.
#include <evals/evals.hpp>
#include <types/PlainText.hpp>
#ifdef INLINE_MODE_LUAJIT
#include <luajit-2.1/lua.hpp> // TODO: fix this somehow
#include <cstring>
#include <session.hpp>
#include <types/Object.hpp>
#endif

#ifdef INLINE_MODE_EVALS
EvalsObject* EvalsOperation::atop(EvalsStackType stack, EvalsObject::Type type, int searchDepth) { // search down from the top of the stack for an item, and pop it out.
    // If it encounters an object where (object -> type & type), it will return that object.
    // It will not search further than searchDepth.
    // If no match is found, it will return NULL.
    for (size_t i = 0; i < searchDepth && i < stack.size(); i ++) {
        if (stack[stack.size() - i - 1] -> type & type) {
            EvalsObject* obj = stack[stack.size() - i - 1];
            stack[stack.size() - i - 1] = stack[stack.size() - 1]; // swapout
            stack.pop_back();
            return obj;
        }
    }
    return NULL;
}

void EvalsOperation::parseBinary(EvalsStackType stack, bool preserve, EvalsObject::Type t1, EvalsObject::Type t2) {
    EvalsObject* one = atop(stack, t1, 2);
    EvalsObject* two = atop(stack, t2, 1);
    if (one == NULL || two == NULL) {
        if (one != NULL) { delete one; }
        if (two != NULL) { delete two; }
        stack.push_back(new ErrorObject);
        printf(ERROR "Bad Evals program!\n\tNot enough data on stack to perform a binary operation!\n\tThis can also be caused by incorrect datatypes on the stack.\n");
        return;
    }
    binary(stack, one, two);
    if (!preserve) {
        delete one;
        delete two;
    }
}
#endif

EvalsObject* EvalsSession::render(MapView data, Session* sitix) { // all Evals commands produce an EvalsObject, so we can be sure that we'll be returning an EvalsObject
#ifdef INLINE_MODE_EVALS
    std::vector<EvalsObject*> stack;
    EvalsFunction func(data, parent, scope);
    func.exec(stack);
    if (stack.size() != 1) {
        printf(ERROR "Bad Evals program!\n");
        printf("\tstack size is %d, must be 1\n", stack.size());
        for (EvalsObject* o : stack) {
            delete o;
        }
        return new ErrorObject();
    }
    return stack[0];
#endif

#ifdef INLINE_MODE_LUAJIT
    // TODO: process the lua programs in constructors (requires a rework of how EvalsSession behaves) and invoke them in render
    // this speeds things up and allows using lua in exciting new ways (like defining sitix variables as lua functions)

    // setup: loading the chunk to lua
    std::string toLua = "return " + data.toString() + ";";
    if (luaL_loadbuffer(sitix -> lua, toLua.c_str(), toLua.size(), "Evals blob") == LUA_ERRSYNTAX) {
        printf(ERROR "Syntax error in a Lua embedded blob!\n");
        // todo: actually print out the lua error
        return new ErrorObject();
    }

    // housekeeping: loading the useful objects to scope
    lua_pushlightuserdata(sitix -> lua, parent);
    lua_setglobal(sitix -> lua, "_sitix_parent");
    lua_pushlightuserdata(sitix -> lua, scope);
    lua_setglobal(sitix -> lua, "_sitix_scope");

    // calling: actually call the sitix chunk, and return the result as an Evals object
    lua_pcall(sitix -> lua, 0, 1, 0);
    int top = lua_gettop(sitix -> lua);
    switch (lua_type(sitix -> lua, top)) {
        case LUA_TNUMBER:
            return new NumberObject(lua_tonumber(sitix -> lua, top));
            break;
        case LUA_TSTRING:
            return new StringObject(std::string{lua_tostring(sitix -> lua, top)});
            break;
        case LUA_TBOOLEAN:
            return new BooleanObject(lua_toboolean(sitix -> lua, top));
            break;
        default:
            return new BooleanObject(false);
            break;
    }
#endif
}

EvalsBlob::EvalsBlob(Session* session, MapView d) : Node(session), data(d) {}

void EvalsBlob::render(SitixWriter* out, Object* scope, bool dereference) {
    EvalsSession session{parent, scope};
    EvalsObject* result = session.render(data, sitix);
    out -> write(result -> toString());
    delete result;
}
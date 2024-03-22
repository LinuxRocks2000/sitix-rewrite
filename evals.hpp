// Evals is a simple, turing-incomplete evaluated language designed for string processing. It directly integrates with and depends on the Sitix scope model.
// Evals is primarily designed for string manipulation and value comparison.
// It is not fancy; the syntax is redolent of Tyler++.
#include <vector>
#include <string>
#include <math.h>
#include "sitixwriter.hpp"


struct EvalsObject {
    enum Type {
        Number,
        String,
        Boolean,
        Error,
        SitixVariable
    } type;

    virtual bool equals(EvalsObject* obj) = 0;

    virtual std::string toString() = 0;

    virtual bool truthyness() { // things are not "truthy" by default
        return false;
    }
};


struct ErrorObject : EvalsObject {
    ErrorObject() {
        type = Type::Error;
    }

    bool equals(EvalsObject*) {
        return false;
    }

    std::string toString() {
        return "[ ERROR ]";
    }
};


struct StringObject : EvalsObject {
    std::string content;

    StringObject(std::string data) {
        content = data;
        type = Type::String;
    }

    bool equals(EvalsObject* thing) {
        return thing -> toString() == content;
    }

    std::string toString() {
        return content;
    }

    bool truthyness() { // empty strings are falsey, but otherwise strings are always truthy
        return content.size() > 0;
    }
};


struct NumberObject : EvalsObject {
    double content;

    NumberObject(double data) {
        content = data;
        type = Type::Number;
    }

    bool equals(EvalsObject* thing) {
        if (thing -> type != type) {
            return false;
        }
        return ((NumberObject*)thing) -> content == content;
    }

    std::string toString() {
        return std::to_string(content);
    }

    bool truthyness() {
        return content != 0; // 0 is falsey, everything else is truthy
    }
};


struct BooleanObject : EvalsObject {
    bool content;

    BooleanObject(bool value) {
        content = value;
        type = Type::Boolean;
    }

    bool equals(EvalsObject* thing) {
        if (thing -> type != type) {
            return false;
        }
        return ((BooleanObject*)thing) -> content == content;
    }

    std::string toString() {
        return content ? "true" : "false";
    }

    bool truthyness() {
        return content;
    }
};


struct SitixVariableObject : EvalsObject {
    Object* content;
    Object* scope;

    SitixVariableObject(Object* obj, Object* sc) {
        scope = sc;
        content = obj -> deghost();
        type = Type::SitixVariable;
    }

    bool equals(EvalsObject* thing) {
        if (content == NULL) {
            return false;
        }
        if (thing -> type == Type::SitixVariable) {
            auto frend = ((SitixVariableObject*)thing) -> content -> deghost();
            if (frend == content) { // if they point to the same data, they're the same variable
                return true;
            }
            if (frend -> children.size() != content -> children.size()) {
                return false;
            }
            for (size_t i = 0; i < frend -> children.size(); i ++) {
                if (content -> children[i] -> type != frend -> children[i] -> type) {
                    return false;
                }
                if (content -> children[i] -> type == Node::Type::PLAINTEXT) {
                    auto t1 = (PlainText*)(content -> children)[i];
                    auto t2 = (PlainText*)(frend -> children)[i];
                    if (t1 -> data != t2 -> data) {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }

    std::string toString() {
        StringWriteOutput out;
        SitixWriter writer(out);
        content -> render(&writer, scope, true);
        return out.content.c_str();
    }

    bool truthy() {
        return content != NULL;
    }
};


struct EvalsSession {
    Object* parent;
    Object* scope;

    EvalsObject* render(MapView& data) { // all Evals commands produce an EvalsObject, so we can be sure that we'll be returning an EvalsObject
        std::vector<EvalsObject*> stack;
        while (data.len() > 0) {
            data.trim();
            if (data[0] == '"') {
                data ++;
                stack.push_back(new StringObject(data.consume('"').toString()));
                data ++; // actually consume the " (.consume only consumes *up till* the target)
            }
            else if (data[0] >= '0' && data[0] <= '9') {
                double num = 0.0;
                int point = 0;
                while (true) {
                    if (data[0] >= '0' && data[0] <= '9') {
                        num *= 10;
                        num += data[0] - '0';
                        if (point > 0) {
                            point ++;
                        }
                    }
                    else if (data[0] == '.') {
                        point = 1;
                    }
                    else {
                        break;
                    }
                    data ++;
                }
                num /= pow(10, point - 1);
                stack.push_back(new NumberObject(num));
            }
            else {
                // it's a variable or symbolic instruction that we have to resolve. consume until the next space.
                auto symbol = data.consume(' ').toString();
                if (symbol == "equals") { // if there are at least two items at the top of the stack,
                    // 1. pop them out
                    // 2. check if either of them reports equality with the other
                    // 3. if either of them do, add a BooleanObject set to the result to the stack
                    if (stack.size() >= 2) {
                        EvalsObject* o1 = stack[stack.size() - 1];
                        stack.pop_back();
                        EvalsObject* o2 = stack[stack.size() - 1];
                        stack.pop_back();
                        stack.push_back(new BooleanObject(o1 -> equals(o2) || o2 -> equals(o1)));
                    }
                    else {
                        printf(ERROR "Not enough data on stack to check equality!\n");
                    }
                }
                else { // if it's not a symbol, we're going to assume it's a Sitix variable
                    Object* o = parent -> lookup(symbol);
                    if (o != NULL) {
                        o = scope -> lookup(symbol);
                    }
                    stack.push_back(new SitixVariableObject(o, scope));
                }
            }
        }
        if (stack.size() != 1) {
            printf(ERROR "Bad Evals syntax!\n");
            return new ErrorObject();
        }
        return stack[0];
    }
};


struct EvalsBlob : Node {
    MapView data;

    EvalsBlob(MapView d) : data(d) {};

    void render(SitixWriter* out, Object* scope, bool dereference) {
        EvalsSession session{parent, scope};
        EvalsObject* result = session.render(data);
        out -> write(result -> toString());
        delete result;
    }
};

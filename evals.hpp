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

    virtual bool truthyness() = 0;

    virtual EvalsObject* copy() = 0;
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

    bool truthyness() {
        return false;
    }

    EvalsObject* copy() {
        return new ErrorObject;
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

    EvalsObject* copy() {
        return new StringObject(content);
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

    EvalsObject* copy() {
        return new NumberObject(content);
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

    EvalsObject* copy() {
        return new BooleanObject(content);
    }
};


struct SitixVariableObject : EvalsObject {
    Object* content;
    Object* scope;

    SitixVariableObject(Object* obj, Object* sc) {
        scope = sc;
        content = obj == NULL ? NULL : obj -> deghost();
        type = Type::SitixVariable;
    }

    bool equals(EvalsObject* thing) {
        if (content == NULL) {
            return false;
        }
        if (thing -> type == Type::SitixVariable && ((SitixVariableObject*)thing) -> content != NULL) {
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
        if (content == NULL) {
            return "";
        }
        StringWriteOutput out;
        SitixWriter writer(out);
        content -> render(&writer, scope, true);
        return out.content.c_str();
    }

    bool truthyness() {
        return content != NULL;
    }

    EvalsObject* copy() {
        return new SitixVariableObject(content, scope);
    }
};


struct EvalsSession {
    Object* parent;
    Object* scope;

    EvalsObject* render(MapView data) { // all Evals commands produce an EvalsObject, so we can be sure that we'll be returning an EvalsObject
        std::vector<EvalsObject*> stack;
        while (data.len() > 0) {
            data.trim();
            if (data[0] == '"') {
                data ++;
                auto string = data.consume('"').toString();
                stack.push_back(new StringObject(string));
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
                        delete o1;
                        delete o2;
                    }
                    else {
                        printf(ERROR "Not enough data on stack to check equality!\n");
                    }
                }
                else if (symbol == "not") {
                    EvalsObject* o = stack[stack.size() - 1];
                    stack.pop_back();
                    stack.push_back(new BooleanObject(!(o -> truthyness())));
                    delete o;
                }
                else if (symbol == "concat") {
                    EvalsObject* o1 = stack[stack.size() - 1];
                    stack.pop_back();
                    EvalsObject* o2 = stack[stack.size() - 1];
                    stack.pop_back();
                    stack.push_back(new StringObject(o2 -> toString() + o1 -> toString()));
                    delete o1;
                    delete o2;
                }
                else if (symbol == "strip_fname") { // cut the directories and extension off a filename on the stack. thus it'll go from
                    // `/root/home/bourgeoisie/test.cpp` to `test` or `/home/test.a.out` to `test.a`.
                    // this is a convenient minified version of `copy "." count_back slice_left copy "/" count_back slice_right`.
                    EvalsObject* fname = stack[stack.size() - 1];
                    stack.pop_back();
                    std::string name = fname -> toString();
                    delete fname;
                    size_t lastSlash = 0;
                    for (lastSlash = name.size() - 1; lastSlash >= 0; lastSlash --) {
                        if (name[lastSlash] == '/') {
                            break;
                        }
                    }
                    size_t lastDot = 0;
                    for (lastDot = name.size() - 1; lastDot >= 0; lastDot --) {
                        if (name[lastDot] == '.') {
                            break;
                        }
                    }
                    stack.push_back(new StringObject(name.substr(lastSlash + 1, lastDot - lastSlash - 1)));
                }
                else if (symbol == "copy") {
                    stack.push_back(stack[stack.size() - 1] -> copy());
                }
                else if (symbol == "count_back") { // count from the back of the larger of the top two strings on stack to the highest index that contains the smaller
                // string *in front of* it, and push that number to the stack.
                    EvalsObject* o1 = stack[stack.size() - 1];
                    stack.pop_back();
                    EvalsObject* o2 = stack[stack.size() - 1];
                    stack.pop_back();
                    std::string s1 = o1 -> toString();
                    std::string s2 = o2 -> toString();
                    delete o1;
                    delete o2;
                    std::string smaller = s1.size() < s2.size() ? s1 : s2;
                    std::string larger = s1.size() < s2.size() ? s2 : s1;
                    size_t i;
                    for (i = larger.size() - 1; i >= smaller.size(); i --) {
                        bool matches = true;
                        for (size_t j = 0; j < smaller.size(); j ++) {
                            if (larger[i - j] != smaller[smaller.size() - j - 1]) {
                                matches = false;
                                break;
                            }
                        }
                        if (matches) {
                            break;
                        }
                    }
                    stack.push_back(new NumberObject(i));
                }
                else if (symbol == "slice_left") { // slice a string LEFT at a number. Cuts off that index.
                    // If you're slicing "hello" at index 3, for instance, you get "hel".
                    if (stack.size() >= 2) {
                        EvalsObject* o1 = stack[stack.size() - 1];
                        stack.pop_back();
                        EvalsObject* o2 = stack[stack.size() - 1];
                        stack.pop_back();
                        double number;
                        EvalsObject* string;
                        if (o1 -> type == EvalsObject::Type::Number) {
                            number = ((NumberObject*)o1) -> content;
                            string = o2;
                        }
                        else if (o2 -> type == EvalsObject::Type::Number) {
                            number = ((NumberObject*)o2) -> content;
                            string = o1;
                        }
                        else {
                            printf(ERROR "Can't slice left: insufficient information on stack\n");
                            break;
                        }
                        std::string s = string -> toString();
                        stack.push_back(new StringObject(s.substr(0, (size_t)number)));
                        delete o1;
                        delete o2;
                    }
                    else {
                        printf(ERROR "Can't slice left: insufficient information on stack\n");
                        break;
                    }
                }
                else if (symbol == "slice_right") { // slice a string RIGHT at a number. Cuts off that index.
                    // if you're slicing "helloworld" right at index 3, you get "oworld".
                    if (stack.size() >= 2) {
                        EvalsObject* o1 = stack[stack.size() - 1];
                        stack.pop_back();
                        EvalsObject* o2 = stack[stack.size() - 1];
                        stack.pop_back();
                        double number;
                        EvalsObject* string;
                        if (o1 -> type == EvalsObject::Type::Number) {
                            number = ((NumberObject*)o1) -> content;
                            string = o2;
                        }
                        else if (o2 -> type == EvalsObject::Type::Number) {
                            number = ((NumberObject*)o2) -> content;
                            string = o1;
                        }
                        else {
                            printf(ERROR "Can't slice left: insufficient information on stack\n");
                            break;
                        }
                        std::string s = string -> toString();
                        stack.push_back(new StringObject(s.substr((size_t)number + 1, s.size())));
                        delete o1;
                        delete o2;
                    }
                    else {
                        printf(ERROR "Can't slice left: insufficient information on stack\n");
                        break;
                    }
                }
                else if (symbol == "filenameify") {
                    // converts a string to a valid filename
                    EvalsObject* string = stack[stack.size() - 1];
                    stack.pop_back();
                    std::string s = string -> toString();
                    delete string;
                    for (size_t i = 0; i < s.size(); i ++) {
                        if (s[i] >= 'A' && s[i] <= 'Z') {
                            s[i] -= 'A';
                            s[i] += 'a';
                        }
                        else if (s[i] < 'a' || s[i] > 'z') {
                            if (s[i] != '.' && s[i] != '_' && (s[i] < '0' || s[i] > '9')) {
                                s[i] = '-';
                                if (i > 0 && s[i - 1] == '-') {
                                    for (size_t j = i; j < s.size() - 1; j ++) {
                                        s[j] = s[j + 1];
                                    }
                                    s.resize(s.size() - 1);
                                    i --;
                                }
                            }
                        }
                    }
                    stack.push_back(new StringObject(s));
                }
                else if (symbol == "true") {
                    stack.push_back(new BooleanObject(true));
                }
                else if (symbol == "false") {
                    stack.push_back(new BooleanObject(false));
                }
                else { // if it's not a symbol, we're going to assume it's a Sitix variable
                    Object* o = parent -> lookup(symbol);
                    if (o == NULL) {
                        o = scope -> lookup(symbol);
                    }
                    stack.push_back(new SitixVariableObject(o, scope));
                }
            }
        }
        if (stack.size() != 1) {
            printf(ERROR "Bad Evals syntax!\n");
            printf("\tstack size is %d\n", stack.size());
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

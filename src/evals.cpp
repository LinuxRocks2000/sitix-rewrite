// Evals is a simple, turing-incomplete evaluated language designed for string processing. It directly integrates with and depends on the Sitix scope model.
// Evals is primarily designed for string manipulation and value comparison.
// It is not fancy; the syntax is redolent of Tyler++.
#include <evals.hpp>
#include <types/PlainText.hpp>


ErrorObject::ErrorObject() {
    type = Type::Error;
}

bool ErrorObject::equals(EvalsObject*) {
    return false;
}

std::string ErrorObject::toString() {
    return "[ ERROR ]";
}

bool ErrorObject::truthyness() {
    return false;
}

EvalsObject* ErrorObject::copy() {
    return new ErrorObject;
}


StringObject::StringObject(std::string data) {
    content = data;
    type = Type::String;
}

bool StringObject::equals(EvalsObject* thing) {
    return thing -> toString() == content;
}

std::string StringObject::toString() {
    return content;
}

bool StringObject::truthyness() { // empty strings are falsey, but otherwise strings are always truthy
    return content.size() > 0;
}

EvalsObject* StringObject::copy() {
    return new StringObject(content);
}

NumberObject::NumberObject(double data) {
    content = data;
    type = Type::Number;
}

bool NumberObject::equals(EvalsObject* thing) {
    if (thing -> type != type) {
        return false;
    }
    return ((NumberObject*)thing) -> content == content;
}

std::string NumberObject::toString() {
    return std::to_string(content);
}

bool NumberObject::truthyness() {
    return content != 0; // 0 is falsey, everything else is truthy
}

EvalsObject* NumberObject::copy() {
    return new NumberObject(content);
}


BooleanObject::BooleanObject(bool value) {
    content = value;
    type = Type::Boolean;
}

bool BooleanObject::equals(EvalsObject* thing) {
    if (thing -> type != type) {
        return false;
    }
    return ((BooleanObject*)thing) -> content == content;
}

std::string BooleanObject::toString() {
    return content ? "true" : "false";
}

bool BooleanObject::truthyness() {
    return content;
}

EvalsObject* BooleanObject::copy() {
    return new BooleanObject(content);
}


SitixVariableObject::SitixVariableObject(Object* obj, Object* sc) {
    scope = sc;
    content = obj == NULL ? NULL : obj -> deghost();
    type = Type::SitixVariable;
}

bool SitixVariableObject::equals(EvalsObject* thing) {
    if (content == NULL) {
        return false;
    }
    if (thing -> type == Type::SitixVariable && ((SitixVariableObject*)thing) -> content != NULL) {
        auto frend = ((SitixVariableObject*)thing) -> content -> deghost();
        if (frend == content) { // if they point to the same data, they're the same variable
            return true;
        }
        if (thing -> toString() != toString()) { // bad solution but eh
            return false;
        }
        /*
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
        }*/
        return true;
    }
    return false;
}

std::string SitixVariableObject::toString() {
    if (content == NULL) {
        return "";
    }
    StringWriteOutput out;
    SitixWriter writer(out);
    content -> render(&writer, scope, true);
    return out.content.c_str();
}

bool SitixVariableObject::truthyness() {
    return content != NULL;
}

EvalsObject* SitixVariableObject::copy() {
    return new SitixVariableObject(content, scope);
}

void EqualityCheck::run(EvalsStackType stack) {
    parseBinary(stack);
}

void EqualityCheck::binary(EvalsStackType stack, EvalsObject* one, EvalsObject* two) {
    if (one == NULL || two == NULL) {
        if (one != NULL) { delete one; }
        if (two != NULL) { delete two; }
        stack.push_back(new ErrorObject);
        printf(ERROR "Bad Evals program!\n\tNot enough data on Evals stack to check equality!\n");
        return;
    }
    stack.push_back(new BooleanObject(one -> equals(two) || two -> equals(one)));
}

void NotOperation::run(EvalsStackType stack) {
    EvalsObject* thing = atop(stack);
    stack.push_back(new BooleanObject(thing == NULL ? true : !thing -> truthyness()));
    delete thing;
}

void ConcatOperation::run(EvalsStackType stack) {
    parseBinary(stack);
}

void ConcatOperation::binary(EvalsStackType stack, EvalsObject* one, EvalsObject* two) {
    stack.push_back(new StringObject(two -> toString() + one -> toString()));
}

template<typename Enum>
Slicer::Slicer(Enum m) : mode(m) {}

void Slicer::run(EvalsStackType stack) {
    parseBinary(stack, EvalsObject::Type::Number);
}

void Slicer::binary(EvalsStackType stack, EvalsObject* one, EvalsObject* two) { // `one` is guaranteed to be a number, because of the filter applied in the parseBinary() call
    double num = ((NumberObject*)one) -> content;
    std::string st = two -> toString();
    if (mode == LeftLow) {
        stack.push_back(new StringObject(st.substr(0, (size_t)(num))));
    }
    else if (mode == LeftHigh) {
        stack.push_back(new StringObject(st.substr(0, (size_t)(num) + 1)));
    }
    else if (mode == RightLow) {
        stack.push_back(new StringObject(st.substr((size_t)(num) + 1, st.size())));
    }
    else if (mode == RightHigh) {
        stack.push_back(new StringObject(st.substr((size_t)(num), st.size())));
    }
}

Counter::Counter(int startOffset, int tick) : off(startOffset), t(tick) {}

void Counter::run(EvalsStackType stack) {
    parseBinary(stack);
}

void Counter::binary(EvalsStackType stack, EvalsObject* o1, EvalsObject* o2) {
    std::string o1s = o1 -> toString();
    std::string o2s = o2 -> toString();
    std::string& smaller = (o1s.size() < o2s.size() ? o1s : o2s);
    std::string& larger = (o1s.size() < o2s.size() ? o2s : o1s);
    size_t i;
    for (i = coterminal(off, larger.size() + 1); i >= smaller.size() && i <= larger.size(); i += t) {
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

void CopyOperation::run(EvalsStackType stack) {
    stack.push_back(stack[stack.size() - 1] -> copy());
}

void FilenameifyOperation::run(EvalsStackType stack) {
    // converts a string to a valid filename
    EvalsObject* string = atop(stack);
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

void TrimOperation::run(EvalsStackType stack) {
    EvalsObject* string = atop(stack);
    std::string s = string -> toString();
    delete string;
    size_t trimStart, trimEnd;
    for (trimStart = 0; trimStart < s.size(); trimStart ++) {
        if (!isWhitespace(s[trimStart])) {
            break;
        }
    }
    for (trimEnd = s.size() - 1; trimEnd >= 0; trimEnd --) {
        if (!isWhitespace(s[trimEnd])) {
            break;
        }
    }
    stack.push_back(new StringObject(s.substr(trimStart, trimEnd - trimStart + 1)));
}

EvalsFunction::EvalsFunction(MapView& m, Object* parent, Object* scope) {
    while (m.len() > 0) {
        m.trim();
        if (m[0] == ')') {
            m++;
            break;
        }
        else if (m[0] == '(') {
            m++;
            program.push_back(new EvalsFunction(m, parent, scope));
        }
        else if (m[0] == '"') {
            m ++;
            auto string = m.consume('"').toString();
            program.push_back(new StackPush(new StringObject(string)));
            m ++; // actually consume the " (.consume only consumes *up till* the target)
        }
        else if (m[0] >= '0' && m[0] <= '9') {
            double num = 0.0;
            int point = 0;
            while (true) {
                if (m[0] >= '0' && m[0] <= '9') {
                    num *= 10;
                    num += m[0] - '0';
                    if (point > 0) {
                        point ++;
                    }
                }
                else if (m[0] == '.') {
                    point = 1;
                }
                else {
                    break;
                }
                m ++;
            }
            if (point) {
                num /= pow(10, point - 1);
            }
            program.push_back(new StackPush(new NumberObject(num)));
        }
        else {
            auto symbol = m.consume(' ').toString();
            if (symbol == "false") {
                program.push_back(new StackPush(new BooleanObject(false)));
            }
            else if (symbol == "true") {
                program.push_back(new StackPush(new BooleanObject(true)));
            }
            else if (symbol == "equals") {
                program.push_back(new EqualityCheck());
            }
            else if (symbol == "not") {
                program.push_back(new NotOperation());
            }
            else if (symbol == "concat") {
                program.push_back(new ConcatOperation());
            }
            else if (symbol == "strip_fname") {
                program.push_back(new CopyOperation());
                program.push_back(new StackPush(new StringObject(".")));
                program.push_back(new Counter(-1, -1));
                program.push_back(new Slicer(Slicer::LeftLow));
                program.push_back(new CopyOperation());
                program.push_back(new StackPush(new StringObject("/")));
                program.push_back(new Counter(-1, -1));
                program.push_back(new Slicer(Slicer::RightLow));
                // this is a convenient minified version of `copy "." count_back slice_left copy "/" count_back slice_right`.
            }
            else if (symbol == "copy") {
                program.push_back(new CopyOperation());
            }
            else if (symbol == "count_back") {
                program.push_back(new Counter(-1, -1));
            }
            else if (symbol == "slice_right") {
                program.push_back(new Slicer(Slicer::RightLow));
            }
            else if (symbol == "slice_right_inc") {
                program.push_back(new Slicer(Slicer::RightHigh));
            }
            else if (symbol == "slice_left") {
                program.push_back(new Slicer(Slicer::LeftLow));
            }
            else if (symbol == "slice_left_inc") {
                program.push_back(new Slicer(Slicer::LeftHigh));
            }
            else if (symbol == "filenameify") {
                program.push_back(new FilenameifyOperation());
            }
            else if (symbol == "trim") {
                program.push_back(new TrimOperation());
            }
            else {
                Object* o = parent -> lookup(symbol);
                if (o == NULL) {
                    o = scope -> lookup(symbol);
                }
                program.push_back(new StackPush(new SitixVariableObject(o, scope)));
            }
        }
    }
}

void EvalsFunction::run(EvalsStackType stack) {
    for (EvalsOperation* op : program) {
        op -> run(stack);
    }
}

EvalsFunction::~EvalsFunction() {
    for (EvalsOperation* op : program) {
        delete op;
    }
}

StackPush::StackPush(EvalsObject* obj) : data(obj){}

void StackPush::run(EvalsStackType stack) {
    stack.push_back(data);
}

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

void EvalsOperation::parseBinary(EvalsStackType stack, EvalsObject::Type t1, EvalsObject::Type t2) {
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
    delete one;
    delete two;
}


EvalsObject* EvalsSession::render(MapView data) { // all Evals commands produce an EvalsObject, so we can be sure that we'll be returning an EvalsObject
    std::vector<EvalsObject*> stack;
    EvalsFunction func(data, parent, scope);
    func.run(stack);
    if (stack.size() != 1) {
        printf(ERROR "Bad Evals program!\n");
        printf("\tstack size is %d, must be 1\n", stack.size());
        for (EvalsObject* o : stack) {
            delete o;
        }
        return new ErrorObject();
    }
    return stack[0];
}



EvalsBlob::EvalsBlob(Session* session, MapView d) : Node(session), data(d) {};

void EvalsBlob::render(SitixWriter* out, Object* scope, bool dereference) {
    EvalsSession session{parent, scope};
    EvalsObject* result = session.render(data);
    out -> write(result -> toString());
    delete result;
}

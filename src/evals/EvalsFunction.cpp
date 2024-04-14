#include <evals/evals.hpp>
#include <math.h>
#include <types/Object.hpp>


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
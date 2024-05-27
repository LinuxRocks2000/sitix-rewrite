#include <evals/evals.hpp>
#ifdef INLINE_MODE_EVALS


Slicer::Slicer(Type m) : mode(m) {}

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

#endif
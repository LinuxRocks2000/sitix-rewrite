#include <evals/evals.hpp>
#include <util.hpp>
#ifdef INLINE_MODE_EVALS


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

#endif
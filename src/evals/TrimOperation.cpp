#include <evals/evals.hpp>
#include <util.hpp>
#ifdef INLINE_MODE_EVALS


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

#endif
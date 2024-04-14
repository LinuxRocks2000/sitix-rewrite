#include <evals/evals.hpp>


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
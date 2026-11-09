#include "assets/texture_binding.hpp"

#include <cstdio>
#include <vector>

int main() {
    int failures = 0;
    auto check = [&](int actual, int expected, const char* label) {
        if (actual != expected) {
            std::fprintf(stderr, "texture binding FAILED: %s (got %d, expected %d)\n",
                         label, actual, expected);
            ++failures;
        }
    };

    check(cr::lastTextureIndexById({17, 42, 17}, 17), 2,
          "last duplicate wins, not first");
    check(cr::lastTextureIndexById({0, 0, 0}, 0), 2,
          "zero-valued duplicate ID");
    check(cr::lastTextureIndexById({3, 8, 12}, 8), 1,
          "unique ID keeps its only match");
    check(cr::lastTextureIndexById({3, 8, 12}, 99), -1,
          "missing ID remains unresolved");
    check(cr::lastTextureIndexById({}, 1), -1,
          "empty package remains unresolved");

    std::printf("texture binding: %d failures\n", failures);
    return failures ? 1 : 0;
}

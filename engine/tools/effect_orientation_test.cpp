#include "gfx/effect_orientation.hpp"

#include <cstdio>

int main() {
    int failures = 0;
    auto check = [&](bool actual, bool expected, const char* label) {
        if (actual != expected) {
            std::fprintf(stderr, "effect orientation FAILED: %s (got %d, expected %d)\n",
                         label, int(actual), int(expected));
            ++failures;
        }
    };

    check(cr::effectFacesCamera(false, true, true), false,
          "no camera vectors means retain effect yaw");
    check(cr::effectFacesCamera(true, true, false), true,
          "battle effects face the battle camera");
    check(cr::effectFacesCamera(true, false, false), false,
          "ordinary field effects retain their own yaw");
    check(cr::effectFacesCamera(true, false, true), true,
          "field effect flagged by battle-time setup faces the camera");

    std::printf("effect orientation: %d failures\n", failures);
    return failures ? 1 : 0;
}

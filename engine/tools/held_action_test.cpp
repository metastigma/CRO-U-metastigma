#include "platform/held_action.hpp"

#include <cstdio>

int main() {
    crinput::HeldAction skip;
    int problems = 0;
    const auto key = crinput::HeldSource::Keyboard;
    const auto pad = crinput::HeldSource::Gamepad;
    const auto pointer = crinput::HeldSource::Pointer;

    skip.set(key, true);
    problems += !skip.active();
    skip.set(key, false);
    problems += skip.active();

    skip.set(key, true);
    skip.set(pad, true);
    skip.set(key, false); // releasing the keyboard must not release the held pad
    problems += !skip.active();
    skip.set(pad, false);
    problems += skip.active();

    skip.set(pointer, true);
    skip.set(key, true);
    skip.set(pointer, false); // mouse/touch release while X remains held
    problems += !skip.active();
    skip.set(key, false);
    problems += skip.active();

    skip.set(key, true);
    skip.set(pad, true);
    skip.set(pointer, true);
    skip.clear(); // focus loss / scene boundary
    problems += skip.active();
    skip.set(key, false); // a late release cannot reactivate anything
    problems += skip.active();

    std::printf("held skip inputs: independent key/pad/pointer release and global cancel; %d problems\n", problems);
    return problems ? 1 : 0;
}

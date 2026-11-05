#pragma once

namespace crinput {
enum class HeldSource : unsigned { Keyboard = 1u << 0, Gamepad = 1u << 1, Pointer = 1u << 2 };
class HeldAction {
public:
    void set(HeldSource source, bool held) noexcept {
        const unsigned bit = static_cast<unsigned>(source);
        if (held) held_ |= bit;
        else held_ &= ~bit;
    }
    bool active() const noexcept { return held_ != 0; }
    void clear() noexcept { held_ = 0; }
private:
    unsigned held_ = 0;
};
} // namespace crinput

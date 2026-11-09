#pragma once

namespace cr {

// Omega's mdl_put turns an effect toward the camera only when the caller set the
// camera-facing flag. Battle effects always face the battle camera; field effects
// do so only when they were started while a battle was active.
constexpr bool effectFacesCamera(bool hasEyeAndTarget, bool battleEffectList, bool requested) noexcept {
    return hasEyeAndTarget && (battleEffectList || requested);
}

} // namespace cr

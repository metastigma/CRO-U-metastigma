#pragma once

#include <cstddef>
#include <vector>

namespace cr {

// The original engine binds package textures in order. A later texture with the
// same MCT ID replaces the earlier binding, so retain the last matching index.
inline int lastTextureIndexById(const std::vector<int>& textureIds, int id) {
    int result = -1;
    for (std::size_t i = 0; i < textureIds.size(); ++i)
        if (textureIds[i] == id) result = static_cast<int>(i);
    return result;
}

} // namespace cr

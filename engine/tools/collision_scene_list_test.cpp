#include "assets/collision.hpp"
#include "game/field_walker.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

template <typename T>
void put(std::vector<uint8_t>& bytes, size_t offset, T value) {
    std::memcpy(bytes.data() + offset, &value, sizeof value);
}

std::vector<uint8_t> sceneStage(bool hasExtraOnlyList) {
    // One triangle, one KD node. Its optional +10 list is deliberately not a floor or wall list.
    constexpr size_t header = 32, vertices = 3 * 12, polygon = 32, node = 32;
    const size_t nodeAt = header + vertices + polygon;
    const size_t listAt = nodeAt + node;
    std::vector<uint8_t> bytes(listAt + (hasExtraOnlyList ? 2 : 0), 0);

    put<uint32_t>(bytes, 0, 0);       // KD tree
    put<int16_t>(bytes, 4, 3);        // vertices
    put<int16_t>(bytes, 6, 1);        // polygon
    put<int16_t>(bytes, 8, 1);        // node
    put<float>(bytes, 12, 3.0f);      // collision radius

    const float v[9] = {-10, 0, -10, 10, 0, -10, 0, 0, 10};
    std::memcpy(bytes.data() + header, v, sizeof v);

    const size_t polyAt = header + vertices;
    put<float>(bytes, polyAt + 4, 1.0f); // upward normal
    put<int16_t>(bytes, polyAt + 12, 0);
    put<int16_t>(bytes, polyAt + 14, 2);
    put<int16_t>(bytes, polyAt + 16, 1); // winding agrees with the upward normal
    put<int16_t>(bytes, polyAt + 20, 3); // triangle

    put<int16_t>(bytes, nodeAt + 4, 0);  // floors
    put<int16_t>(bytes, nodeAt + 6, 0);  // walls
    put<int16_t>(bytes, nodeAt + 8, 0);  // other list A
    put<int16_t>(bytes, nodeAt + 10, hasExtraOnlyList ? 1 : 0); // scene-stage square list
    put<int32_t>(bytes, nodeAt + 20, 0); // x split axis
    put<int16_t>(bytes, nodeAt + 24, -1);
    put<int16_t>(bytes, nodeAt + 26, -1);
    if (hasExtraOnlyList) put<int16_t>(bytes, listAt, 0); // polygon 0 is present only in +10

    return bytes;
}

} // namespace

int main() {
    int failures = 0;
    cr::Collision scene;
    std::string error;

    // The scene-stage format has a node list at +10 but no floor/wall entries. The parser must
    // retain the empty floor list; deriving a floor from the polygon normal is not equivalent.
    const auto listed = sceneStage(true);
    if (!cr::parseCollision(listed, scene, &error)) {
        std::fprintf(stderr, "scene-stage list parse failed: %s\n", error.c_str());
        return 1;
    }
    float height = 123.0f;
    const int node = cr::searchNode(scene, {0, 12.7f, 0}, 10.0f);
    const bool emptyLists = scene.nodes.size() == 1 && scene.nodes[0].floors.empty() &&
                            scene.nodes[0].walls.empty();
    const bool noFloor = node == 0 && cr::getFloor(scene, node, {0, 12.7f, 0}, 20.0f, height) < 0;
    if (!emptyLists || !noFloor) {
        std::fprintf(stderr, "extra-only scene list became a floor: empty=%d no-floor=%d\n",
                     int(emptyLists), int(noFloor));
        ++failures;
    }

    // Files with no node lists at all keep the legacy normal-based fallback.
    const auto legacy = sceneStage(false);
    if (!cr::parseCollision(legacy, scene, &error)) {
        std::fprintf(stderr, "legacy no-list parse failed: %s\n", error.c_str());
        return 1;
    }
    const int legacyNode = cr::searchNode(scene, {0, 0, 0}, 10.0f);
    const bool legacyFloor = legacyNode == 0 &&
                             cr::getFloor(scene, legacyNode, {0, 0, 0}, 20.0f, height) == 0 &&
                             height == 0.0f;
    if (!legacyFloor) {
        std::fprintf(stderr, "no-list legacy fallback failed\n");
        ++failures;
    }

    std::printf("collision scene node lists: extra-only stays empty %s; no-list fallback %s; %d failures\n",
                noFloor ? "yes" : "NO", legacyFloor ? "yes" : "NO", failures);
    return failures ? 1 : 0;
}

#include "game/battle_rules.hpp"

#include <cstdio>

int main() {
    int problems = 0;
    auto check = [&](bool ok, const char* name) {
        if (!ok) {
            std::fprintf(stderr, "battle rules FAILED: %s\n", name);
            ++problems;
        }
    };

    // Omega's pair actions 36 / 37 become the living member's own record + 27.
    for (int character = 0; character < 8; ++character) {
        for (int firstActor = 0; firstActor < 2; ++firstActor) {
            check(cr::battlePairSurvivorAction(cr::kCmdAttack, 36, firstActor, -1, character) == character + 27,
                  "Omega pair action 36 resolves to surviving member");
            check(cr::battlePairSurvivorAction(cr::kCmdAttack, 37, firstActor, -1, character) == character + 27,
                  "Omega pair action 37 resolves to surviving member");
        }
    }

    // The shared engine's CR1 pair records retain the original character + 10 mapping.
    for (int action = 18; action <= 21; ++action) {
        check(cr::battlePairSurvivorAction(cr::kCmdAttack, action, 0, -1, 3) == 13,
              "CR1 pair action resolves to member attack");
    }

    // No rewrite while both actors remain, for enemies, or for non-attack commands.
    check(cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 0, 1, 3) == 36,
          "intact pair keeps pair action");
    check(cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 2, -1, 3) == 36,
          "enemy actor is not rewritten");
    check(cr::battlePairSurvivorAction(cr::kCmdGene, 36, 0, -1, 3) == 36,
          "gene command is not rewritten");
    check(cr::battlePairSurvivorAction(cr::kCmdAttack, 16, 0, -1, 3) == 16,
          "ordinary solo attack is unchanged");
    check(cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 0, -1, -1) == 36,
          "invalid character record is left unchanged");
    check(cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 0, -1, 8) == 36,
          "out-of-range character record is left unchanged");

    std::printf("battle rules: fallen pair member resolves to survivor attack; %d problems\n", problems);
    return problems ? 1 : 0;
}

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
    for (int character = 0; character < cr::kCharacters; ++character) {
        for (int firstActor = 0; firstActor < 2; ++firstActor) {
            const auto from36 = cr::battlePairSurvivorAction(cr::kCmdAttack, 36, firstActor, -1, character);
            const auto from37 = cr::battlePairSurvivorAction(cr::kCmdAttack, 37, firstActor, -1, character);
            check(from36.reannounce && from36.action == character + 27,
                  "Omega pair action 36 resolves and is announced as surviving member");
            check(from37.reannounce && from37.action == character + 27,
                  "Omega pair action 37 resolves and is announced as surviving member");
        }
    }

    // The shared engine's CR1 pair records retain the original character + 10 mapping.
    for (int action = 18; action <= 21; ++action) {
        const auto result = cr::battlePairSurvivorAction(cr::kCmdAttack, action, 0, -1, 3);
        check(result.reannounce && result.action == 13, "CR1 pair action resolves to member attack");
    }

    // No rewrite while both actors remain, for enemies, or for non-attack commands.
    check(!cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 0, 1, 3).reannounce &&
          cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 0, 1, 3).action == 36,
          "intact pair keeps pair action");
    check(!cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 2, -1, 3).reannounce &&
          cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 2, -1, 3).action == 36,
          "enemy actor is not rewritten");
    check(!cr::battlePairSurvivorAction(cr::kCmdGene, 36, 0, -1, 3).reannounce &&
          cr::battlePairSurvivorAction(cr::kCmdGene, 36, 0, -1, 3).action == 36,
          "gene command is not rewritten");
    check(!cr::battlePairSurvivorAction(cr::kCmdAttack, 16, 0, -1, 3).reannounce &&
          cr::battlePairSurvivorAction(cr::kCmdAttack, 16, 0, -1, 3).action == 16,
          "ordinary solo attack is unchanged");
    check(!cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 0, -1, -1).reannounce &&
          cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 0, -1, -1).action == 36,
          "invalid character record is left unchanged");
    check(!cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 0, -1, cr::kCharacters).reannounce &&
          cr::battlePairSurvivorAction(cr::kCmdAttack, 36, 0, -1, cr::kCharacters).action == 36,
          "character index past the ten saved party records is rejected");

    std::printf("battle rules: fallen pair member resolves to survivor attack; %d problems\n", problems);
    return problems ? 1 : 0;
}

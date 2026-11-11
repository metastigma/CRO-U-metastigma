#include "assets/battle_data.hpp"
#include "game/stage_tables.hpp"

#include <cstdio>

int main() {
    int problems = 0;
    const cr::BattleAnim noWindow{};
    cr::BattleAnim meleeWindow;
    meleeWindow.moveStart = 4;
    meleeWindow.moveEnd = 9;
    const cr::BattleAnim geneApproach = cr::battleApproachAnimation(noWindow, meleeWindow, true, true);
    const cr::BattleAnim noApproachSelf = cr::battleApproachAnimation(noWindow, meleeWindow, true, false);
    const cr::BattleAnim noApproachNonGene = cr::battleApproachAnimation(noWindow, meleeWindow, false, true);
    problems += geneApproach.moveStart != 4 || geneApproach.moveEnd != 9;
    problems += noApproachSelf.hasMoveWindow();
    problems += noApproachNonGene.hasMoveWindow();
    problems += cr::battleCameraDuration(10001, 25) != 25;
    problems += cr::battleCameraDuration(10002, 25) != 25;
    problems += cr::battleCameraDuration(10003, 25) != 40;
    problems += cr::battleCameraDuration(15, 25) != 15;

    cr::BattleAnim startsAtZero;
    startsAtZero.moveStart = 0;
    startsAtZero.moveEnd = 12;
    cr::BattleAnim delayedWindow;
    delayedWindow.moveStart = 7;
    delayedWindow.moveEnd = 14;
    cr::BattleAnim invalidWindow;
    invalidWindow.moveStart = 14;
    invalidWindow.moveEnd = 7;

    problems += noWindow.hasMoveWindow();
    problems += !startsAtZero.hasMoveWindow();
    problems += !delayedWindow.hasMoveWindow();
    problems += invalidWindow.hasMoveWindow();

    // Only actor 1 of an attack with a second participant receives the fallback.
    problems += cr::battleApproachPairedSecondary(true, true, 0);
    problems += !cr::battleApproachPairedSecondary(true, true, 1);
    problems += cr::battleApproachPairedSecondary(true, false, 1);
    problems += cr::battleApproachPairedSecondary(false, true, 1);

    // The return clip is selected by action motion + opposing target, independently of translation.
    problems += !cr::battleShouldPlayReturn(1, true);
    problems += cr::battleShouldPlayReturn(0, true);
    problems += cr::battleShouldPlayReturn(1, false);
    // Both approach paths still require motion == 1 and an opposing target.
    problems += cr::battleApproachEnabled(0, true, startsAtZero);
    problems += cr::battleApproachEnabled(1, false, startsAtZero);
    problems += !cr::battleApproachEnabled(1, true, startsAtZero);
    problems += cr::battleApproachEnabled(1, true, noWindow);
    problems += !cr::battleApproachEnabled(1, true, noWindow, true);
    problems += cr::battleApproachEnabled(0, true, noWindow, true);
    problems += cr::battleApproachEnabled(1, false, noWindow, true);
    // The already-merged M225 in-place fix must survive normal and paired-secondary paths.
    // M225 remains eligible for its post-hit return clip but never approaches.
    problems += cr::battleApproachEnabled(1, true, startsAtZero, false, true);
    problems += cr::battleApproachEnabled(1, true, noWindow, true, true);
    problems += !cr::battleApproachEnabled(1, true, delayedWindow);

    // A valid Section 13 window takes precedence over the fallback.
    problems += cr::battleApproachMoveStart(startsAtZero) != 0;
    problems += cr::battleApproachMoveEnd(startsAtZero, true, 20) != 12;
    problems += cr::battleApproachMoveStart(delayedWindow) != 7;
    problems += cr::battleApproachMoveEnd(delayedWindow, true, 20) != 14;

    // Missing windows: paired secondary gets 0..max(1, hitFrame - 4), solo gets none.
    problems += cr::battleApproachMoveStart(noWindow) != 0;
    problems += cr::battleApproachMoveEnd(noWindow, false, 20) != 0;
    problems += cr::battleApproachMoveFrame(true, 9, 2.0f) != 9.0f;
    problems += cr::battleApproachMoveFrame(false, 9, 2.0f) != 2.0f;
    problems += cr::battleApproachMoveEnd(noWindow, true, 20) != 16;
    problems += cr::battleApproachMoveEnd(noWindow, true, 2) != 1;
    problems += cr::battleApproachMoveEnd(invalidWindow, false, 20) != 0;
    problems += cr::battleApproachMoveEnd(invalidWindow, true, 20) != 16;

    // Omega's formation table is keyed by the selected enemy (fighters 2..4).
    // Solo uses slots 3..5; a pair has two exact points per enemy in slots 6..11.
    for (int target = 2; target <= 4; ++target) {
        const int enemy = target - 2;
        problems += cr::battleApproachPositionSlot(target, 0, false) != 3 + enemy;
        problems += cr::battleApproachPositionSlot(target, 1, false) != 3 + enemy;
        problems += cr::battleApproachPositionSlot(target, 0, true) != 6 + enemy * 2;
        problems += cr::battleApproachPositionSlot(target, 1, true) != 7 + enemy * 2;
    }
    problems += cr::battleApproachPositionSlot(1, 0, true) != -1;
    problems += cr::battleApproachPositionSlot(5, 0, true) != -1;
    problems += cr::battleApproachPositionSlot(2, 2, true) != -1;

    // Omega's per-participant attack clip selection: secondary uses its own clip, not slot 9.
    auto attack = cr::battleAttackMotion(0, 2, 0, 8, true, false, false);
    problems += attack.slot != 8 || attack.inPlace;
    attack = cr::battleAttackMotion(1, 3, 1, 8, true, false, true);
    problems += attack.slot != 8 || attack.inPlace;
    attack = cr::battleAttackMotion(0, 2, 0, 8, true, true, false);
    problems += attack.slot != 9 || !attack.inPlace;
    attack = cr::battleAttackMotion(0, 2, 0, 10, true, false, false);
    problems += attack.slot != 10 || !attack.inPlace;
    attack = cr::battleAttackMotion(1, 7, 1, 8, true, false, true);
    problems += attack.slot != 13 || !attack.inPlace;
    attack = cr::battleAttackMotion(3, 0xa1, -1, 8, true, false, true);
    problems += attack.slot != 13 || !attack.inPlace;
    attack = cr::battleAttackMotion(0, 0x9d, 0, 8, true, false, false);
    problems += attack.slot != 13 || attack.inPlace;

    std::printf("battle approach: timing, formation target slots and M225 gating; %d problems\\n", problems);
    // Validate the actual Omega table, not only the index arithmetic. The three
    // three-enemy formations each provide one solo point and two distinct pair points
    // for every possible enemy target.
    for (int formation : {6, 9, 13}) {
        for (int target = 2; target <= 4; ++target) {
            const int solo = cr::battleApproachPositionSlot(target, 0, false);
            const int first = cr::battleApproachPositionSlot(target, 0, true);
            const int second = cr::battleApproachPositionSlot(target, 1, true);
            const auto& soloPos = cr::kBattlePosition[formation][solo];
            const auto& firstPos = cr::kBattlePosition[formation][first];
            const auto& secondPos = cr::kBattlePosition[formation][second];
            problems += (soloPos[0] == 0 && soloPos[1] == 0);
            problems += (firstPos[0] == 0 && firstPos[1] == 0);
            problems += (secondPos[0] == 0 && secondPos[1] == 0);
            problems += firstPos[0] == secondPos[0] && firstPos[1] == secondPos[1];
        }
    }
    // Formation 6 is the three-equal-enemy case (222); assert the exact target-specific spots.
    problems += cr::kBattlePosition[6][6][0] != 490 || cr::kBattlePosition[6][6][1] != 492;
    problems += cr::kBattlePosition[6][7][0] != 484 || cr::kBattlePosition[6][7][1] != 494;
    problems += cr::kBattlePosition[6][8][0] != 504 || cr::kBattlePosition[6][8][1] != 494;
    problems += cr::kBattlePosition[6][9][0] != 496 || cr::kBattlePosition[6][9][1] != 494;
    problems += cr::kBattlePosition[6][10][0] != 516 || cr::kBattlePosition[6][10][1] != 494;
    problems += cr::kBattlePosition[6][11][0] != 510 || cr::kBattlePosition[6][11][1] != 492;

    return problems ? 1 : 0;
}

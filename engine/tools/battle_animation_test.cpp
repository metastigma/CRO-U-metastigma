#include "assets/battle_data.hpp"

#include <cstdio>

int main() {
    int problems = 0;
    const cr::BattleAnim noWindow{};

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
    problems += cr::battleApproachMoveEnd(noWindow, true, 20) != 16;
    problems += cr::battleApproachMoveEnd(noWindow, true, 2) != 1;
    problems += cr::battleApproachMoveEnd(invalidWindow, false, 20) != 0;
    problems += cr::battleApproachMoveEnd(invalidWindow, true, 20) != 16;

    std::printf("battle approach: paired-secondary fallback and solo gating; %d problems\n", problems);
    return problems ? 1 : 0;
}

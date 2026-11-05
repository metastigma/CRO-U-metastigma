#include "assets/battle_data.hpp"

#include <cstdio>

int main() {
    int problems = 0;

    const cr::BattleAnim noWindow{};
    problems += noWindow.hasMoveWindow();

    cr::BattleAnim startsAtZero;
    startsAtZero.moveStart = 0;
    startsAtZero.moveEnd = 12;
    problems += !startsAtZero.hasMoveWindow();

    cr::BattleAnim delayedWindow;
    delayedWindow.moveStart = 7;
    delayedWindow.moveEnd = 14;
    problems += !delayedWindow.hasMoveWindow();

    cr::BattleAnim invalidWindow;
    invalidWindow.moveStart = 14;
    invalidWindow.moveEnd = 7;
    problems += invalidWindow.hasMoveWindow();

    std::printf("battle animation movement windows: empty/stated/invalid intervals; %d problems\n", problems);
    return problems ? 1 : 0;
}

#include "circuit_escape/controllers/guard_controller.hpp"

Action actionTowards(Position from, Position to)
{
    if (to.row + 1 == from.row && to.column == from.column) {
        return Action::up;
    }
    if (to.row == from.row + 1 && to.column == from.column) {
        return Action::down;
    }
    if (to.row == from.row && to.column + 1 == from.column) {
        return Action::left;
    }
    if (to.row == from.row && to.column == from.column + 1) {
        return Action::right;
    }
    return Action::wait;
}

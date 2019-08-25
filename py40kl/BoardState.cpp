#include "BoostPython.h"
#include <Board.h>
using namespace c40kl;


class_<BoardState>("BoardState", init<int, float>())
	.def("is_occupied", &BoardState::IsOccupied)
	.def("set_unit_on_square", &BoardState::SetUnitOnSquare)
	.def("get_unit_on_square", &BoardState::GetUnitOnSquare)
	.def("get_team_on_square", &BoardState::GetTeamOnSquare,
		return_value_policy<copy_const_reference>())
	.def("get_all_units", &BoardState::GetAllUnits)
	.def("clear_square", &BoardState::ClearSquare)
	.def("has_adjacent_enemy", &BoardState::HasAdjacentEnemy)
	.def("get_squares_in_range", &BoardState::GetSquaresInRange)
	.def("get_distance", &BoardState::GetDistance)
	.def(self == self);



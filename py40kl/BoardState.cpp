#include "BoostPython.h"
#include <Board.h>
using namespace c40kl;


void ExportBoardState()
{
	class_<BoardState>("BoardState", init<int, float>())
		.def("is_occupied", &BoardState::IsOccupied)
		.def("set_unit_on_square", &BoardState::SetUnitOnSquare)
		.def("get_unit_on_square", &BoardState::GetUnitOnSquare,
			return_value_policy<copy_const_reference>())
		.def("get_team_on_square", &BoardState::GetTeamOnSquare)
		.def("get_all_units", &BoardState::GetAllUnits)
		.def("clear_square", &BoardState::ClearSquare)
		.def("has_adjacent_enemy", &BoardState::HasAdjacentEnemy)
		.def("get_squares_in_range", &BoardState::GetSquaresInRange)
		.def("get_distance", &BoardState::GetDistance)
		.def("get_size", &BoardState::GetSize)
		.def("get_scale", &BoardState::GetScale)
		.def(self == self);
}



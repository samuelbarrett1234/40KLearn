#include "BoostPython.h"
#include <GameState.h>
using namespace c40kl;


class_<GameState>("GameState", init<int, int, Phase, BoardState>())
	.def("get_acting_team", &GameState::GetActingTeam)
	.def("get_internal_team", &GameState::GetInternalTeam)
	.def("get_phase", &GameState::GetPhase)
	.def("get_commands", &GameState::GetCommands)
	.def("is_finished", &GameState::IsFinished)
	.def("get_game_value", &GameState::GetGameValue)
	.def("get_board_state", &GameState::GetBoardState,
		return_value_policy<copy_const_reference>())
	.def(self == self);



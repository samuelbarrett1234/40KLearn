#include "BoostPython.h"
#include "CommandWrapper.h"
#include <GameState.h>
using namespace c40kl;


//Need special get commands function for the command wrapper class
std::vector<CommandWrapper> WrappedGetCommands(const GameState& gs)
{
	auto arr = gs.GetCommands();

	//This will automatically perform the array conversion
	return std::vector<CommandWrapper>(arr.begin(), arr.end());
}


void ExportGameState()
{
	class_<std::vector<GameState>>("GameStateArray")
		.def(vector_indexing_suite<std::vector<GameState>>());


	class_<GameState>("GameState", init<int, int, Phase, const BoardState&>())
		.def("get_acting_team", &GameState::GetActingTeam)
		.def("get_internal_team", &GameState::GetInternalTeam)
		.def("get_phase", &GameState::GetPhase)
		.def("get_commands", &WrappedGetCommands)
		.def("is_finished", &GameState::IsFinished)
		.def("get_game_value", &GameState::GetGameValue)
		.def("get_board_state", &GameState::GetBoardState,
			return_value_policy<copy_const_reference>())
		.def(self == self)
		.def("__str__", &GameState::ToString);
}



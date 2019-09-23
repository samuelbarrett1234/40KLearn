#include "EndPhaseCommand.h"
#include "GameState.h"
#include "GameMechanics.h"
#include "CompositeCommand.h"
#include "MoraleCheckCommand.h"


namespace c40kl
{


void EndPhaseCommand::GetPossibleCommands(const GameState& state, GameCommandArray& outCommands)
{
	if (state.GetPhase() == Phase::FIGHT)
	{
		//Note: we cannot end the fight phase until all fights are finished
		//Hacky solution: UnitFightCommand::GetPossibleCommands and then check
		// there are none possible!

		PositionArray fightableUnits;
		GetFightableUnits(state.GetBoardState(), 0, fightableUnits);
		GetFightableUnits(state.GetBoardState(), 1, fightableUnits);

		//If there are any units with fighting options, they must fight before
		// we can allow them to end turn:
		if (!fightableUnits.empty())
			return;
	}

	//Construct composite command which performs a morale
	// check for every unit which has taken damage,
	// and then ends the phase:

	GameCommandArray arr;

	//Get all units and their stats (need to do it team-by-team)
	auto allUnits = state.GetBoardState().GetAllUnits(0);
	auto allUnitStats = state.GetBoardState().GetAllUnitStats(0);
	{
		auto restOfUnits = state.GetBoardState().GetAllUnits(1);
		allUnits.insert(allUnits.end(), restOfUnits.begin(), restOfUnits.end());
		auto restOfUnitStats = state.GetBoardState().GetAllUnitStats(1);
		allUnitStats.insert(allUnitStats.end(), restOfUnitStats.begin(), restOfUnitStats.end());
	}

	C40KL_ASSERT_INVARIANT(allUnits.size() == allUnitStats.size(),
		"Unit positions and unit stats arrays must tie up.");

	for (size_t i = 0; i < allUnits.size(); i++)
	{
		const auto& unitPos = allUnits[i];
		const auto& stats = allUnitStats[i];

		if (stats.modelsLostThisPhase > 0)
		{
			//Need to book in a morale check!
			arr.push_back(std::make_shared<MoraleCheckCommand>(unitPos));
		}
	}

	//Follow all morale checks up with the end phase command
	// then add the composite command to the output
	arr.push_back(std::make_shared<EndPhaseCommand>());
	outCommands.push_back(std::make_shared<CompositeCommand>(arr.begin(), arr.end(), CommandType::END_PHASE));
}


void EndPhaseCommand::Apply(const GameState& state,
	std::vector<GameState>& outStates, std::vector<float>& outDistribution) const
{
	auto board = state.GetBoardState();

	//Get list of all units
	PositionArray allUnits = state.GetBoardState().GetAllUnits(0);
	{
		PositionArray restOfUnits = state.GetBoardState().GetAllUnits(1);
		allUnits.insert(allUnits.end(), restOfUnits.begin(), restOfUnits.end());
	}

	//Check if about to change turn
	const bool bChangingTurn = (state.GetPhase() == Phase::FIGHT);

	//Reset turn-specific info for all units
	for (const auto& unitPos : allUnits)
	{
		Unit stats = board.GetUnitOnSquare(unitPos);

		//Things to reset for this phase:

		stats.modelsLostThisPhase = 0;

		//Things to reset for this turn:
		if (bChangingTurn)
		{
			stats.attemptedChargeThisTurn = false;
			stats.firedThisTurn = false;
			stats.foughtThisTurn = false;
			stats.movedOutOfCombatThisTurn = false;
			stats.movedThisTurn = false;
			stats.successfulChargeThisTurn = false;
		}

		//Update changes:
		board.SetUnitOnSquare(unitPos, stats, board.GetTeamOnSquare(unitPos));
	}

	//Determine the next phase and team:
	int nextTeam = state.GetInternalTeam();
	Phase nextPhase = state.GetPhase();
	switch (nextPhase)
	{
	case Phase::MOVEMENT:
		nextPhase = Phase::SHOOTING;
		break;
	case Phase::SHOOTING:
		nextPhase = Phase::CHARGE;
		break;
	case Phase::CHARGE:
		nextPhase = Phase::FIGHT;
		break;
	case Phase::FIGHT:
		nextPhase = Phase::MOVEMENT;
		nextTeam = 1 - nextTeam;
		break;
	default:
		C40KL_ASSERT_INVARIANT(false, "Invalid game phase! Corrupted memory?");
	}

	//Edge case: if we are about to change to the fight
	// phase, and if the internal team has no units that
	// can possibly fight BUT if the opposing team DOES,
	// which will happen if one of them has no melee weapon,
	// then we need to set the active team to be the opposing
	// team.

	int activeTeam = nextTeam;

	if (nextPhase == Phase::FIGHT)
	{
		//Get fightable units for each team
		PositionArray curTeamFightableUnits, oppTeamFightableUnits;
		GetFightableUnits(board, nextTeam, curTeamFightableUnits);
		GetFightableUnits(board, 1 - nextTeam, oppTeamFightableUnits);

		//Handle the edge case:
		if (curTeamFightableUnits.empty() && !oppTeamFightableUnits.empty())
		{
			activeTeam = 1 - nextTeam;
		}
	}

	//Create next game state
	outStates.emplace_back(nextTeam, activeTeam, nextPhase, board);
	outDistribution.push_back(1.0f);
}


bool EndPhaseCommand::Equals(const IGameCommand& cmd) const
{
	if (auto pCmd = dynamic_cast<const EndPhaseCommand*>(&cmd))
	{
		return true;
	}
	else return false;
}


String EndPhaseCommand::ToString() const
{
	return std::string("end phase command");
}


} // namespace c40kl



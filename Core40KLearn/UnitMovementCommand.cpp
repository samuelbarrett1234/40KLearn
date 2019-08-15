#include "UnitMovementCommand.h"
#include "GameState.h"
#include <sstream>


namespace c40kl
{


void UnitMovementCommand::GetPossibleCommands(const GameState& state, GameCommandArray& outCommands)
{
	//If not in the movement phase, we can't do anything.
	if (state.GetPhase() != Phase::MOVEMENT)
		return;

	PositionArray possiblePositions;
	Unit stats;

	//Get all units for this team:
	auto units = state.GetBoardState().GetAllUnits(state.GetTeam());

	//Consider each unit for movement
	for (const auto& unitPos : units)
	{
		//Get the unit's stats
		stats = state.GetBoardState().GetUnitOnSquare(unitPos.first, unitPos.second);

		//Can't move twice per turn!
		if (!stats.movedThisTurn)
		{
			//Get all of the possible positions to move to
			// (just by returning all cells within distance).
			possiblePositions = state.GetBoardState().GetSquaresInRange(unitPos.first, unitPos.second, stats.movement);

			//For each possible position...
			for (const auto& targetPos : possiblePositions)
			{
				//Can only move to a position which is not occupied and has no adjacent enemies
				// otherwise we'd be moving into melee combat.
				//Note that we don't care whether or not the unit is in combat already.
				if (!state.GetBoardState().IsOccupied(targetPos.first, targetPos.second)
					&& !state.GetBoardState().HasAdjacentEnemy(targetPos.first, targetPos.second, state.GetTeam()))
				{
					outCommands.push_back(std::make_shared<UnitMovementCommand>(unitPos, targetPos));
				}
			}
		}
	}
}


UnitMovementCommand::UnitMovementCommand(Position source, Position target) :
	m_Source(source),
	m_Target(target)
{
}


void UnitMovementCommand::Apply(const GameState& state,
	std::vector<GameState>& outStates, std::vector<float>& outDistribution) const
{
	auto board = state.GetBoardState();

	//Check that this action is still valid:
	C40KL_ASSERT_PRECONDITION(
		state.GetPhase() == Phase::MOVEMENT
		&& board.IsOccupied(m_Source.first, m_Source.second)
		&& !board.IsOccupied(m_Target.first, m_Target.second)
		&& !board.HasAdjacentEnemy(m_Target.first, m_Target.second, 
			board.GetTeamOnSquare(m_Source.first, m_Source.second)),
		"Movement action preconditions must be satisfied.");

	//Get info:
	auto team = board.GetTeamOnSquare(m_Source.first, m_Source.second);
	auto unitStats = board.GetUnitOnSquare(m_Source.first, m_Source.second);

	//Flag that this unit has moved
	unitStats.movedThisTurn = true;

	//We need to also flag whether or not the unit
	// has just fallen out of combat (which happens
	// if there is an adjacent enemy from the position
	// it has moved from).
	unitStats.movedOutOfCombatThisTurn = board.HasAdjacentEnemy(m_Source.first, m_Source.second, team);

	//Move the unit:
	board.ClearSquare(m_Source.first, m_Source.second);
	board.SetUnitOnSquare(m_Target.first, m_Target.second, unitStats, team);

	//Deterministic action:
	outStates.emplace_back(team, Phase::MOVEMENT, board);
	outDistribution.push_back(1.0f);
}


bool UnitMovementCommand::Equals(const IGameCommand& cmd) const
{
	if (auto pCmd = dynamic_cast<const UnitMovementCommand*>(&cmd))
	{
		return (m_Source == pCmd->m_Source && m_Target == pCmd->m_Target);
	}
	else return false;
}


String UnitMovementCommand::ToString() const
{
	std::stringstream c;
	c << "movement order from (" << m_Source.first << ',' << m_Source.second
		<< ") to (" << m_Target.first << ',' << m_Target.second << ')';
	return c.str();
}


} // namespace c40kl



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

	//Cache these:
	const auto ourTeam = state.GetActingTeam();
	const auto& board = state.GetBoardState();

	//Get all units for this team:
	const auto unitPositions = board.GetAllUnits(ourTeam);
	const auto unitStats = board.GetAllUnitStats(ourTeam);

	C40KL_ASSERT_INVARIANT(unitPositions.size() == unitStats.size(),
		"Unit positions and unit stats arrays must tie up.");

	//Consider each unit for movement
	for (size_t i = 0; i < unitPositions.size(); i++)
	{
		const auto& unitPos = unitPositions[i];
		const auto& stats = unitStats[i];

		//Can't move twice per turn!
		if (!stats.movedThisTurn)
		{
			//Get all of the possible positions to move to
			// (just by returning all cells within distance).
			possiblePositions = board.GetSquaresInRange(unitPos, (float)stats.movement);

			//For each possible position...
			for (const auto& targetPos : possiblePositions)
			{
				//Can only move to a position which is not occupied and has no adjacent enemies
				// otherwise we'd be moving into melee combat.
				//Note that we don't care whether or not the unit is in combat already.
				if (!board.IsOccupied(targetPos)
					&& !board.HasAdjacentEnemy(targetPos, ourTeam))
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
		&& board.IsOccupied(m_Source)
		&& !board.IsOccupied(m_Target)
		&& !board.HasAdjacentEnemy(m_Target, 
			board.GetTeamOnSquare(m_Source))
		&& !board.GetUnitOnSquare(m_Source).movedThisTurn
		,"Movement action preconditions must be satisfied.");

	//Get info:
	auto team = board.GetTeamOnSquare(m_Source);
	auto unitStats = board.GetUnitOnSquare(m_Source);

	//Flag that this unit has moved
	unitStats.movedThisTurn = true;

	//We need to also flag whether or not the unit
	// has just fallen out of combat (which happens
	// if there is an adjacent enemy from the position
	// it has moved from).
	unitStats.movedOutOfCombatThisTurn = board.HasAdjacentEnemy(m_Source, team);

	//Move the unit:
	board.ClearSquare(m_Source);
	board.SetUnitOnSquare(m_Target, unitStats, team);

	//Deterministic action:
	outStates.emplace_back(team, team, Phase::MOVEMENT, board);
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



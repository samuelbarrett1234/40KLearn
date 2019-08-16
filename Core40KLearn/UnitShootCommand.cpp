#include "UnitShootCommand.h"
#include "GameState.h"
#include "GameMechanics.h"
#include <sstream>


namespace c40kl
{


void UnitShootCommand::GetPossibleCommands(const GameState& state, GameCommandArray& outCommands)
{
	//If not in the shooting phase, we can't do anything.
	if (state.GetPhase() != Phase::SHOOTING)
		return;

	Unit stats;

	//Cache these:
	const auto ourTeam = state.GetActingTeam();
	const auto& board = state.GetBoardState();

	//Get all units for this team:
	const auto units = board.GetAllUnits(state.GetActingTeam());
	
	//Get all units for the other team, to check distances
	const auto targets = board.GetAllUnits(1 - state.GetActingTeam());

	//Consider each (allied) unit for shooting
	for (const auto& unitPos : units)
	{
		//Get the unit's stats
		stats = board.GetUnitOnSquare(unitPos);

		if (!stats.firedThisTurn //Can't shoot twice per turn!
			&& !board.HasAdjacentEnemy(unitPos, ourTeam) //We also can't shoot if we're in melee
			//Can't shoot if just left combat
			&& !stats.movedOutOfCombatThisTurn
			&& HasStandardRangedWeapon(stats)) //We also actually need a ranged weapon
		{
			//For each possible target to shoot at...
			for (const auto& targetPos : targets)
			{
				//If weapon is in range...
				if (board.GetDistance(unitPos, targetPos) <= stats.rg_range
					&& !board.HasAdjacentEnemy(targetPos, 1-ourTeam)) //And the enemy is not in melee!
				{
					//The command is viable!
					outCommands.push_back(std::make_shared<UnitShootCommand>(unitPos, targetPos));
				}
			}
		}
	}
}


UnitShootCommand::UnitShootCommand(Position source, Position target) :
	m_Source(source),
	m_Target(target)
{
}


void UnitShootCommand::Apply(const GameState& state,
	std::vector<GameState>& outStates, std::vector<float>& outDistribution) const
{
	const auto board = state.GetBoardState();
	const auto distance = board.GetDistance(m_Source, m_Target);

	//Check that this action is still valid:
	C40KL_ASSERT_PRECONDITION(
		state.GetPhase() == Phase::SHOOTING
		&& board.IsOccupied(m_Source) //Must be a unit at source position
		&& board.IsOccupied(m_Target) //Must be a unit at target position
		&& !board.HasAdjacentEnemy(m_Source, //Shooter cannot be in melee
			board.GetTeamOnSquare(m_Source))
		&& !board.HasAdjacentEnemy(m_Target, //Target cannot be in melee
			board.GetTeamOnSquare(m_Target))
		//No friendly fire:
		&& board.GetTeamOnSquare(m_Source) != board.GetTeamOnSquare(m_Target)
		//Unit must be in range:
		&& distance <= board.GetUnitOnSquare(m_Source).rg_range
		//Can't shoot if just left combat:
		&& !board.GetUnitOnSquare(m_Source).movedOutOfCombatThisTurn
		//Needs ranged weapon:
		&& HasStandardRangedWeapon(board.GetUnitOnSquare(m_Source))
		,"Shooting action preconditions must be satisfied.");

	//Get info:
	const auto team = board.GetTeamOnSquare(m_Source);
	auto unitStats = board.GetUnitOnSquare(m_Source);
	const auto targetStats = board.GetUnitOnSquare(m_Target);

	//This shooting attack will result in a probability
	// distribution of different targets.
	std::vector<Unit> targetResults;
	std::vector<float> targetProbs;

	//Use this function to resolve the damage:
	ResolveRawShootingDamage(unitStats, targetStats,
		distance, targetResults, targetProbs);

	C40KL_ASSERT_INVARIANT(targetResults.size() == targetProbs.size(),
		"Needs to return valid distribution.");

	//Flag that this unit has fired
	unitStats.firedThisTurn = true;

	const size_t n = targetResults.size();
	outStates.reserve(outStates.size() + n);
	outDistribution.reserve(outDistribution.size() + n);

	//Construct a state for each target result:
	for (size_t i = 0; i < n; i++)
	{
		const Unit& newStats = targetResults[i];
		BoardState newBoard = board;

		//Check that target health has been handled correctly:
		C40KL_ASSERT_INVARIANT(
			targetStats.count == (targetStats.total_w + targetStats.w - 1) / targetStats.w,
			"Shooting damage calculation has forgot to sync count and total_w.");

		//Update shooter:
		newBoard.SetUnitOnSquare(m_Source, unitStats, team);

		//If target alive, update info, else clear the cell:
		if (targetStats.count > 0)
		{
			newBoard.SetUnitOnSquare(m_Target, targetStats, 1 - team);
		}
		else
		{
			newBoard.ClearSquare(m_Target);
		}

		outStates.emplace_back(team, team, Phase::SHOOTING, newBoard);
		outDistribution.push_back(targetProbs[i]);
	}
	
}


bool UnitShootCommand::Equals(const IGameCommand& cmd) const
{
	if (auto pCmd = dynamic_cast<const UnitShootCommand*>(&cmd))
	{
		return (m_Source == pCmd->m_Source && m_Target == pCmd->m_Target);
	}
	else return false;
}


String UnitShootCommand::ToString() const
{
	std::stringstream c;
	c << "shoot order from (" << m_Source.first << ',' << m_Source.second
		<< ") at (" << m_Target.first << ',' << m_Target.second << ')';
	return c.str();
}


} // namespace c40kl



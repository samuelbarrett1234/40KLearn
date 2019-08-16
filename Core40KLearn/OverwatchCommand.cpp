#include "OverwatchCommand.h"
#include "GameState.h"
#include "GameMechanics.h"
#include <sstream>


namespace c40kl
{


OverwatchCommand::OverwatchCommand(Position source, Position target) :
	m_Source(source),
	m_Target(target)
{
}


void OverwatchCommand::Apply(const GameState& state,
	std::vector<GameState>& outStates, std::vector<float>& outDistribution) const
{
	const auto board = state.GetBoardState();
	const auto distance = board.GetDistance(m_Source, m_Target);
	
	//Note: if the target position is not occupied,
	// then the target unit was killed in prior overwatch,
	// and we can ignore this command.
	if (!board.IsOccupied(m_Target))
	{
		//Keep state as-is:
		outStates.push_back(state);
		outDistribution.push_back(1.0f);
		return;
	}

	//Check that this action is still valid:
	C40KL_ASSERT_PRECONDITION(
		state.GetPhase() == Phase::CHARGE
		&& board.IsOccupied(m_Source) //Must be a unit at source position
		//[Don't need to check target position]
		&& !board.HasAdjacentEnemy(m_Source, //Shooter cannot be in melee
			board.GetTeamOnSquare(m_Source))
		&& !board.HasAdjacentEnemy(m_Target, //Target cannot be in melee
			board.GetTeamOnSquare(m_Target))
		//No friendly fire:
		&& board.GetTeamOnSquare(m_Source) != board.GetTeamOnSquare(m_Target)
		//Unit must be in range:
		&& distance <= board.GetUnitOnSquare(m_Source).rg_range
		//Needs ranged weapon:
		&& HasStandardRangedWeapon(board.GetUnitOnSquare(m_Source))
		, "Overwatch action preconditions must be satisfied.");

	//Get info:
	const auto team = board.GetTeamOnSquare(m_Source);
	auto unitStats = board.GetUnitOnSquare(m_Source);
	const auto targetStats = board.GetUnitOnSquare(m_Target);

	//This shooting attack will result in a probability
	// distribution of different targets.
	std::vector<Unit> targetResults;
	std::vector<float> targetProbs;

	//NOTE: SET HIT SKILL TO 6 BECAUSE OVERWATCH!
	unitStats.bs = 6;

	//Use this function to resolve the damage:
	ResolveRawShootingDamage(unitStats, targetStats,
		distance, targetResults, targetProbs);

	C40KL_ASSERT_INVARIANT(targetResults.size() == targetProbs.size(),
		"Needs to return valid distribution.");
	
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

		//If target alive, update info, else clear the cell:
		if (targetStats.count > 0)
		{
			newBoard.SetUnitOnSquare(m_Target, targetStats, 1 - team);
		}
		else
		{
			newBoard.ClearSquare(m_Target);
		}

		outStates.emplace_back(team, Phase::CHARGE, newBoard);
		outDistribution.push_back(targetProbs[i]);
	}

}


bool OverwatchCommand::Equals(const IGameCommand& cmd) const
{
	if (auto pCmd = dynamic_cast<const OverwatchCommand*>(&cmd))
	{
		return (m_Source == pCmd->m_Source && m_Target == pCmd->m_Target);
	}
	else return false;
}


String OverwatchCommand::ToString() const
{
	std::stringstream c;
	c << "shoot order from (" << m_Source.first << ',' << m_Source.second
		<< ") at (" << m_Target.first << ',' << m_Target.second << ')';
	return c.str();
}


} // namespace c40kl



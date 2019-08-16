#include "MoraleCheckCommand.h"
#include "GameState.h"
#include <sstream>
#include <algorithm>


namespace c40kl
{


MoraleCheckCommand::MoraleCheckCommand(Position unitPos) :
	m_UnitPos(unitPos)
{
}


void MoraleCheckCommand::Apply(const GameState& state,
	std::vector<GameState>& outStates, std::vector<float>& outDistribution) const
{
	auto board = state.GetBoardState();

	//Check that this action is still valid:
	C40KL_ASSERT_PRECONDITION(
		board.IsOccupied(m_UnitPos)
		&& board.GetUnitOnSquare(m_UnitPos).modelsLostThisPhase > 0
		, "Morale check action preconditions must be satisfied.");

	//Get info:
	auto team = board.GetTeamOnSquare(m_UnitPos);
	auto unitStats = board.GetUnitOnSquare(m_UnitPos);
	
	int minRollForLoss = unitStats.ld - unitStats.modelsLostThisPhase + 1;

	if (minRollForLoss >= 7)
	{
		//Unit does not waver:

		outStates.push_back(state);
		outDistribution.push_back(1.0f);
	}
	else
	{
		//Consider the case where we don't lose any:
		if (minRollForLoss > 1)
		{
			outStates.push_back(state);
			outDistribution.push_back((minRollForLoss - 1.0f) / 6.0f);
		}

		//Simulate the dice roll for when we lose models:
		for (int i = std::max(minRollForLoss,1); i <= 6; i++)
		{
			int numRunAway = unitStats.modelsLostThisPhase + i - unitStats.ld;

			C40KL_ASSERT_INVARIANT(numRunAway > 0,
				"Should've already handled the case where we don't lose any models.");
		
			BoardState newboard = board;

			Unit newStats = unitStats;

			newStats.count -= numRunAway;
			newStats.total_w = newStats.count * newStats.w;

			if (newStats.count <= 0)
			{
				newboard.ClearSquare(m_UnitPos);
			}
			else
			{
				newboard.SetUnitOnSquare(m_UnitPos, newStats, team);
			}

			//Avoid creating new states where necessary
			//This may happen when several dice rolls
			// result in the total destruction of a unit.
			if (outStates.back().GetBoardState() == newboard)
			{
				//Add to the probability
				outDistribution.back() += 1.0f / 6.0f;
			}
			else
			{
				outStates.emplace_back(state.GetInternalTeam(), state.GetActiveTeam(), state.GetPhase(), newboard);
				outDistribution.push_back(1.0f / 6.0f);
			}
		}
	}
}


bool MoraleCheckCommand::Equals(const IGameCommand& cmd) const
{
	if (auto pCmd = dynamic_cast<const MoraleCheckCommand*>(&cmd))
	{
		return (m_UnitPos == pCmd->m_UnitPos);
	}
	else return false;
}


String MoraleCheckCommand::ToString() const
{
	std::stringstream c;
	c << "morale check unit (" << m_UnitPos.first << ',' << m_UnitPos.second << ')';
	return c.str();
}


} // namespace c40kl



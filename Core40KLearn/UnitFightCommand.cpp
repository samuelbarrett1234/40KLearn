#include "UnitFightCommand.h"
#include "GameState.h"
#include "GameMechanics.h"
#include <sstream>


namespace c40kl
{


void UnitFightCommand::GetPossibleCommands(const GameState& state, GameCommandArray& outCommands)
{
	//If not in the fighting phase, we can't do anything.
	if (state.GetPhase() != Phase::FIGHT)
		return;

	Unit stats;

	//Cache these:
	const auto ourTeam = state.GetActingTeam();
	const auto& board = state.GetBoardState();

	//Get all units for this team:
	const auto units = board.GetAllUnits(ourTeam);
	const auto targets = board.GetAllUnits(1 - ourTeam);

	//Consider each (allied) unit for fighting
	for (const auto& unitPos : units)
	{
		//Get the unit's stats
		stats = board.GetUnitOnSquare(unitPos);

		//We can't fight if we don't have a melee weapon
		if (HasStandardMeleeWeapon(stats))
		{
			for (const auto& targetPos : targets)
			{
				//Must be adjacent:
				if (std::abs(unitPos.first - targetPos.first) <= 1
					&& std::abs(unitPos.second - targetPos.second) <= 1)
				{
					//Valid command!
					outCommands.push_back(
						std::make_shared<UnitFightCommand>(unitPos, targetPos));
				}
			}
		}
	}
}


UnitFightCommand::UnitFightCommand(Position source, Position target) :
	m_Source(source),
	m_Target(target)
{
}


void UnitFightCommand::Apply(const GameState& state,
	std::vector<GameState>& outStates, std::vector<float>& outDistribution) const
{
	const auto board = state.GetBoardState();

	//Check that this action is still valid:
	C40KL_ASSERT_PRECONDITION(
		state.GetPhase() == Phase::FIGHT
		&& board.IsOccupied(m_Source) //Must be a unit at source position
		&& board.IsOccupied(m_Target) //Must be a unit at target position
		//No friendly fighting:
		&& board.GetTeamOnSquare(m_Source) != board.GetTeamOnSquare(m_Target)
		//Squares must be adjacent
		&& std::abs(m_Source.first-m_Target.first) <= 1
		&& std::abs(m_Source.second-m_Target.second) <= 1,
		"Fighting action preconditions must be satisfied.");

	//Get info:
	const auto team = board.GetTeamOnSquare(m_Source);
	auto unitStats = board.GetUnitOnSquare(m_Source);
	const auto targetStats = board.GetUnitOnSquare(m_Target);

	//This fighting attack will result in a probability
	// distribution of different targets.
	std::vector<Unit> targetResults;
	std::vector<float> targetProbs;

	//Use this function to resolve the damage:
	ResolveRawMeleeDamage(unitStats, targetStats,
		targetResults, targetProbs);

	C40KL_ASSERT_INVARIANT(targetResults.size() == targetProbs.size(),
		"Needs to return valid distribution.");

	//Flag that this unit has fired
	unitStats.foughtThisTurn = true;

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
			"Fighting damage calculation has forgot to sync count and total_w.");

		//Update fighter:
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

		//After we have fought, change the acting
		// team (because we are supposed to alternate
		// between teams in the fight phase), however
		// the internal team (whose turn it is) doesn't
		// change.
		outStates.emplace_back(state.GetInternalTeam(), 1-state.GetActingTeam(), Phase::FIGHT, newBoard);
		outDistribution.push_back(targetProbs[i]);
	}

}


bool UnitFightCommand::Equals(const IGameCommand& cmd) const
{
	if (auto pCmd = dynamic_cast<const UnitFightCommand*>(&cmd))
	{
		return (m_Source == pCmd->m_Source && m_Target == pCmd->m_Target);
	}
	else return false;
}


String UnitFightCommand::ToString() const
{
	std::stringstream c;
	c << "fight order from (" << m_Source.first << ',' << m_Source.second
		<< ") at (" << m_Target.first << ',' << m_Target.second << ')';
	return c.str();
}


} // namespace c40kl



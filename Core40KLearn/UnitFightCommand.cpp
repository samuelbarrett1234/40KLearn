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
	const auto targets = board.GetAllUnits(1 - ourTeam);

	//Get the positions of all our units which can possibly fight:
	PositionArray fightable;
	GetFightableUnits(board, ourTeam, fightable);

	for (auto unitPos : fightable)
	{
		//Convert position to command
		for (const auto& targetPos : targets)
		{
			//Must be adjacent:
			if (std::abs(unitPos.first - targetPos.first) <= 1
				&& std::abs(unitPos.second - targetPos.second) <= 1)
			{
				//Push command:
				outCommands.push_back(
					std::make_shared<UnitFightCommand>(unitPos, targetPos));
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

	//Check that the unit is initially in a valid state:
	C40KL_ASSERT_PRECONDITION(
		targetStats.count == (targetStats.total_w + targetStats.w - 1) / targetStats.w,
		"Total wounds / wounds per model / model count must be in sync.");

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
			newStats.count == (newStats.total_w + newStats.w - 1) / newStats.w,
			"Fighting damage calculation has forgot to sync count and total_w.");

		//Update fighter:
		newBoard.SetUnitOnSquare(m_Source, unitStats, team);

		//If target alive, update info, else clear the cell:
		if (newStats.count > 0)
		{
			newBoard.SetUnitOnSquare(m_Target, newStats, 1 - team);
		}
		else
		{
			newBoard.ClearSquare(m_Target);
		}

		// * Determining the next active team *
		// If there are units available left for the other team, switch team
		// Else if there are no units left for EITHER team, set active team = internal team
		// Else stay same team because the other team has run out of units

		bool bUnitsLeft[2];
		for (int i = 0; i < 2; i++)
		{
			PositionArray temp;
			GetFightableUnits(newBoard, i, temp);
			bUnitsLeft[i] = !temp.empty();
		}

		int nextTeam;
		if (bUnitsLeft[1 - team])
			nextTeam = 1 - team; //Time for opponent to make move
		else if (!bUnitsLeft[team])
			nextTeam = state.GetInternalTeam(); //Time for internal team to end phase
		else
			nextTeam = team; //Opponent has no fight moves left so we proceed by default		

		outStates.emplace_back(state.GetInternalTeam(), nextTeam, Phase::FIGHT, newBoard,
			state.HasTurnLimit() ? state.GetTurnLimit() : (-1), state.GetTurnNumber());
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



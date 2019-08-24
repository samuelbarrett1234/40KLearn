#include "GameState.h"
#include "UnitMovementCommand.h"
#include "UnitShootCommand.h"
#include "UnitChargeCommand.h"
#include "UnitFightCommand.h"
#include "EndPhaseCommand.h"
#include <functional>


namespace c40kl
{


std::function<void(const GameState&, GameCommandArray&)> commandCreators[] =
{
	&UnitMovementCommand::GetPossibleCommands,
	&UnitShootCommand::GetPossibleCommands,
	&UnitChargeCommand::GetPossibleCommands,
	&UnitFightCommand::GetPossibleCommands,
	&EndPhaseCommand::GetPossibleCommands
};


GameState::GameState(int internalTeam, int actingTeam, Phase phase, const BoardState& board) :
	m_InternalTeam(internalTeam),
	m_ActingTeam(actingTeam),
	m_Phase(phase),
	m_Board(board)
{
	C40KL_ASSERT_PRECONDITION(internalTeam == 0 || internalTeam == 1, "Must be a valid team.");
	C40KL_ASSERT_PRECONDITION(actingTeam == 0 || actingTeam == 1, "Must be a valid team.");
	C40KL_ASSERT_PRECONDITION(m_ActingTeam == m_InternalTeam || m_Phase == Phase::FIGHT,
		"Acting team and internal team should be the same unless it's the fight phase.");

	//Ensure the game state is valid:
	if (!IsFinished())
	{
		//This situation might occur when the game starts in the fight phase
		// for a team which has no units that can possibly fight, BUT the
		// opposing team DOES. In this case, the end phase command won't be
		// available.
		if (GetCommands().empty())
		{
			throw std::runtime_error("Invalid game state - was not finished but no available actions to take.");
		}
	}
}


int GameState::GetActingTeam() const
{
	C40KL_ASSERT_PRECONDITION(!IsFinished(),
		"Can't produce current team value for finished game.");
	C40KL_ASSERT_INVARIANT(m_ActingTeam == m_InternalTeam || m_Phase == Phase::FIGHT,
		"Acting team and internal team should be the same unless it's the fight phase.");
	return m_ActingTeam;
}


int GameState::GetInternalTeam() const
{
	C40KL_ASSERT_PRECONDITION(!IsFinished(),
		"Can't produce current team value for finished game.");
	C40KL_ASSERT_INVARIANT(m_ActingTeam == m_InternalTeam || m_Phase == Phase::FIGHT,
		"Acting team and internal team should be the same unless it's the fight phase.");
	return m_InternalTeam;
}


GameCommandArray GameState::GetCommands() const
{
	C40KL_ASSERT_PRECONDITION(!IsFinished(), "Can't produce command list for finished game.");

	//Just loop through this array, and let the
	// commands decide whether or not they are
	// applicable.
	GameCommandArray cmds;
	for (const auto& getCmds : commandCreators)
	{
		getCmds(*this, cmds);
	}
	return cmds;
}


bool GameState::IsFinished() const
{
	return (m_Board.GetAllUnits(0).empty() || m_Board.GetAllUnits(1).empty());
}


int GameState::GetGameValue(int team) const
{
	C40KL_ASSERT_PRECONDITION(IsFinished(), "Can't produce winner value for unfinished game.");

	const bool bAlliesFinished = m_Board.GetAllUnits(team).empty();
	const bool bEnemiesFinished = m_Board.GetAllUnits(1-team).empty(); //1-team is the enemy team

	C40KL_ASSERT_INVARIANT(bAlliesFinished || bEnemiesFinished, "At least one team must have no units!");

	if (bAlliesFinished && bEnemiesFinished)
		return 0; //Draw
	else if (bAlliesFinished)
		return -1; //Loss
	else
		return 1; //Win
}


} // namespace c40kl



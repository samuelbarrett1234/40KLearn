#include "GameState.h"


namespace c40kl
{


GameState::GameState(int team, Phase phase, const BoardState& board) :
	m_Team(team),
	m_Phase(phase),
	m_Board(board)
{
	C40KL_ASSERT_PRECONDITION(team == 0 || team == 1, "Must be a valid team.");
}


GameCommandArray GameState::GetCommands() const
{
	C40KL_ASSERT_PRECONDITION(!IsFinished(), "Can't produce command list for finished game.");


}


bool GameState::IsFinished() const
{
	return (m_Board.GetAllUnits(0).empty() || m_Board.GetAllUnits(1).empty());
}


int GameState::GetGameValue(int team) const
{
	C40KL_ASSERT_PRECONDITION(!IsFinished(), "Can't produce winner value for unfinished game.");

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



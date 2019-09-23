#pragma once


#include "Utility.h"
#include "Board.h"
#include "IGameCommand.h"


namespace c40kl
{


/// <summary>
/// A game state encapsulates all information in a given game.
/// If an AI or human were making a decision about what commands
/// to give, all they need is the current game state.
/// The game state is an immutable data structure (cannot be modified).
/// Instead, you apply commands to it, which return a resulting
/// distribution.
/// NOTE: being an immutable datastructure makes this play very
/// nicely with multithreading, and hence applying commands will
/// work in any thread, and game states can be shared between threads.
/// </summary>
class C40KL_API GameState
{
public:
	/// <summary>
	/// Create a new game state which starts
	/// with the given team and phase (this
	/// will be the next source of actions)
	/// and the given board state.
	/// Furthermore, constructing states like
	/// this follows the RAII principle.
	/// </summary>
	/// <param name="internalTeam">0 or 1, representing the team whose turn it is.</param>
	/// <param name="actingTeam">0 or 1, representing the team to make
	/// the next decision. If not fight phase, should be the same as internalTeam!</param>
	/// <param name="phase">The phase to begin in.</param>
	/// <param name="board">The state of the board when the game begins.</param>
	GameState(int internalTeam, int actingTeam, Phase phase, const BoardState& board);

	/// <summary>
	/// Get the team which is currently making decisions.
	/// Precondition: !IsFinished()
	/// </summary>
	/// <returns>0 or 1, depending on the team.</returns>
	int GetActingTeam() const;

	/// <summary>
	/// Get the team which whose turn it is.
	/// Precondition: !IsFinished()
	/// </summary>
	/// <returns>0 or 1, depending on the team.</returns>
	int GetInternalTeam() const;

	/// <summary>
	/// Get the current phase.
	/// Precondition: !IsFinished()
	/// </summary>
	/// <returns>The phase we are currently in.</returns>
	inline Phase GetPhase() const
	{
		C40KL_ASSERT_PRECONDITION(!IsFinished(),
			"Can't produce current phase value for finished game.");
		return m_Phase;
	}

	/// <summary>
	/// Get the array of commands which could currently
	/// be executed. These should be chosen by the team
	/// represented by GetTeam(), and represent actions
	/// in the phase represented by GetPhase().
	/// Precondition: !IsFinished()
	/// </summary>
	/// <returns>A new array of command objects.</returns>
	GameCommandArray GetCommands() const;

	/// <summary>
	/// Determine if this game state is finished (which
	/// happens when at least one team has no units
	/// remaining.)
	/// </summary>
	/// <returns>True if finished, false if not.</returns>
	bool IsFinished() const;

	/// <summary>
	/// If the game has finished, return the 'game value'
	/// for the given team.
	/// The game value is 1 if the given team won, 0 for
	/// a draw, and -1 if the given team lost.
	/// Precondition: IsFinished()
	/// </summary>
	/// <param name="team">The team from whose perspective you will see the score.</param>
	/// <returns>1 if team won, 0 if draw, -1 if team lost.</returns>
	int GetGameValue(int team) const;

	/// <summary>
	/// Get the board state in this game.
	/// </summary>
	/// <returns>An immutable reference to the game state.</returns>
	inline const BoardState& GetBoardState() const
	{
		return m_Board;
	}


	std::string ToString() const;


	inline bool operator == (const GameState& other) const
	{
		return (m_InternalTeam == other.m_InternalTeam && m_Phase == other.m_Phase
			&& m_Board == other.m_Board && m_ActingTeam == other.m_ActingTeam);
	}


private:
	int m_InternalTeam, //The internal team is "whose turn it is"
		m_ActingTeam; //The acting team is "who is about to perform the next move".
	//The distinction is required because players take turns in fighting in the fight phase.

	Phase m_Phase;
	BoardState m_Board;
};


} // namespace c40kl



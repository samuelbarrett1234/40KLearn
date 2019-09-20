#pragma once


#include "GameState.h"
#include "MCTSNode.h"
#include "UCB1PolicyStrategy.h"
#include <random>


namespace c40kl
{


/// <summary>
/// The 'self-play manager' is a system which manages
/// several simultaneous games where an AI plays against
/// itself. It is packaged into its own class for efficiency
/// so threading can be utilised.
/// This class has three main functions, Select(), Update()
/// and Commit(), which the user should cycle through. The
/// Commit() function may require Select()/Update() to be called
/// several times before it is ready.
/// </summary>
class C40KL_API SelfPlayManager
{
public:
	/// <summary>
	/// Create a new self-play manager system. It must be
	/// provided with a UCB1 policy strategy (since UCB1
	/// is the best way of performing selection in the tree)
	/// and the number of simulations the AI makes at each
	/// state.
	/// </summary>
	/// <param name="ucb1ExplorationParameter">
	/// The UCB1 exploration parameter, typically equal to root 2, must be greater than 0.
	/// </param>
	/// <param name="numSimulations">
	/// The number of simulations the AI will do in the search tree before making a decision.
	/// </param>
	SelfPlayManager(float ucb1ExplorationParameter, size_t numSimulations);

	
	/// <summary>
	/// Cancel all current games, and restart play with the given new game.
	/// All games initially start out identical, but after a short while
	/// in play, due to randomness, they will end up quite different.
	/// PRECONDITION: the given initial state is not finished.
	/// </summary>
	/// <param name="numGames">The number of simultaneous games to be played.</param>
	/// <param name="initialState">
	/// The state each of the simultaneous games begins in. Must be an unfinished
	/// game (we cannot play from a finished state).
	/// </param>
	void Reset(size_t numGames, const GameState& initialState);


	/// <summary>
	/// Perform the 'selection' portion of the tree search algorithm. This is where
	/// the search trees will traverse the tree from the root until they find a leaf
	/// making selections via UCB1.
	/// PRECONDITION: !IsWaiting() && !ReadyToCommit() && !AllFinished()
	/// </summary>
	/// <param name="outLeafStates">
	/// This vector will be cleared, and the game states of a subset of selected leaf nodes will be put into
	/// this array. Note that not all leaf states will be put here, because (say) if the game's search tree
	/// is already full, or the selected leaf node is terminal, there is no reason why it should be selected.
	/// </param>
	void Select(std::vector<GameState>& outLeafStates);


	/// <summary>
	/// Perform the expand & backpropagation steps of the tree, by updating the tree
	/// with information about the values of the current leaf nodes, and the prior
	/// policy distribution over what to do next. The arrays given as argument to this
	/// function must correspond to the game states given by the Select() call.
	/// PRECONDITION: IsWaiting() && !ReadyToCommit() && !AllFinished()
	/// </summary>
	/// <param name="valueEstimates">
	/// This is the list of value estimates (+1 for win, -1 for loss, and all the values in between)
	/// for the corresponding game states returned from the Select() call. IMPORTANT: all given values
	/// must be with respect to the CURRENT ACTING TEAM of each leaf game state!
	/// </param>
	/// <param name="policies">
	/// This is the list of prior policies (distribution over actions) for the corresponding game states
	/// returned from the Select() call. Obviously, being a distribution, all elements must be nonnegative
	/// and sum to one.
	/// </param>
	void Update(const std::vector<float>& valueEstimates,
		const std::vector<std::vector<float>>& policies);


	/// <summary>
	/// Once all search trees have been searched throroughly enough, and are of the
	/// required size, the AIs will be ready to commit to a decision in each game.
	/// This function does just that. Of course, committing will reduce the size of
	/// the tree, thus requiring further Select()/Update() calls.
	/// PRECONDITION: ReadyToCommit() && !AllFinished()
	/// </summary>
	void Commit();


	/// <summary>
	/// Determine if we have selected leaf nodes and are waiting on values and prior policies
	/// from the user.
	/// </summary>
	/// <returns>True in between Select() and Update() calls, false outside.</returns>
	bool IsWaiting() const;


	/// <summary>
	/// Determine if the AIs are ready to commit, which happens when every
	/// search tree has reached the required size.
	/// </summary>
	/// <returns>True if ready for Commit(), false if not.</returns>
	bool ReadyToCommit() const;


	/// <summary>
	/// Determine if all games are finished.
	/// </summary>
	/// <returns>True if and only if all search tree roots have finished states.</returns>
	bool AllFinished() const;


	/// <summary>
	/// Get the current state of every game. Note that this is distinct from the game states
	/// returned from the Select() call. This returns the state of the roots of every search tree.
	/// Warning: only returns for the unfinished games.
	/// </summary>
	/// <returns>The state at the root of every search tree (i.e. what each game is currently in).</returns>
	std::vector<GameState> GetCurrentGameStates() const;


	/// <summary>
	/// Return the current action distribution of every game. This is obtained by
	/// picking the best action, according to the search tree results, either
	/// deterministically or stochastically.
	/// Warning: only returns for the unfinished games.
	/// PRECONDITION: ReadyToCommit()
	/// </summary>
	/// <returns>A list of action distributions (policies) for each root node.</returns>
	std::vector<std::vector<float>> GetCurrentActionDistributions() const;


	/// <summary>
	/// Get the number of samples in each search tree.
	/// </summary>
	/// <returns>The number of samples at the root of each search tree.</returns>
	std::vector<int> GetTreeSizes() const;


	/// <summary>
	/// Determine the index of each running game (a game is defined to
	/// be running if it has a tree rooted in this object). Once a game
	/// is finished it is considered no longer running, and its tree is
	/// deleted.
	/// </summary>
	/// <returns>
	/// An array equal to the length of the number of currently running
	/// games, where the value of each element represents the index that
	/// the game initially had (before any games were finished).
	/// </returns>
	std::vector<size_t> GetRunningGameIds() const;


	/// <summary>
	/// Once all games have finished, calling this gets the game value (who
	/// won) for every game. Important: all returned values are with respect
	/// to team 0 (so +1 means team 0 won, -1 means team 1 won).
	/// PRECONDITION: AllFinished().
	/// </summary>
	/// <returns>The end result for every game.</returns>
	std::vector<float> GetGameValues() const;


private:
	/// <summary>
	/// Traverse the root from m_pRoots[gameIdx]
	/// to find a leaf node, and write the leaf
	/// node into m_pSelectedLeaves[gameIdx],
	/// UNLESS the tree is of desired size (==
	/// m_NumSimulations), else don't do selection.
	/// </summary>
	/// <param name="gameIdx">The index of the game tree to select in.</param>
	void SelectLeafForGame(size_t gameIdx);


	/// <summary>
	/// Perform the expand/backpropagate steps in
	/// MCTS for the given game. Do this by (i) adding
	/// the value statistic and backpropagating it,
	/// (ii) expanding the leaf node with the given prior policy.
	/// PRECONDITION: m_pSelectedLeaves[gameIdx] != nullptr.
	/// </summary>
	/// <param name="gameIdx">The index of the game search tree to expand/backprop in.</param>
	/// <param name="valEst">
	/// The value estimate at the given selected leaf node. IMPORTANT: this must be
	/// with respect to team 0!
	/// </param>
	/// <param name="policy">The prior policy to expand the leaf node with.</param>
	void ExpandBackpropagate(size_t gameIdx, float valEst, const std::vector<float>& policy);


	/// <summary>
	/// Determine what action to take, given the game's
	/// complete search tree. Typically this will just
	/// be: selecting the action with the highest number
	/// of visits.
	/// </summary>
	/// <param name="gameIdx">The game search tree to examine.</param>
	/// <returns>The index of the action to take in this game.</returns>
	size_t GetFinalPolicy(size_t gameIdx) const;


private:
	//Invariant:
	// m_pSelectedLeaves is nonempty
	// if and only if we are waiting.

	std::mt19937 m_RandEng;

	const size_t m_NumSimulations;
	UCB1PolicyStrategy m_TreePolicy;

	//IMPORTANT NOTE about tree value estimates:
	// all value estimates are converted to their
	// value with respect to team 0, to stay consistent.
	// Of course, if team 1 is acting, UCB1 will take
	// this into account and select the WORST action for
	// team 0.
	MCTSNodeArray m_pRoots,
		//This array has the same size as m_pRoots after a Select() call
		// but some elements may be empty on purpose.
		m_pSelectedLeaves;

	//Not all games have representative trees in the
	// m_pRoots array - this vector (which always has
	// the same length as m_pRoots) tells us the index
	// of this game. On resetting, this is just the list
	// [0, 1, 2, ...], but as games have their trees
	// removed, their indices are removed as well.
	std::vector<size_t> m_GameIDs;

	//As games finish, their values	WITH RESPECT TO TEAM 0
	// are recorded here. This array is only valid once all
	// games have finished (otherwise it will have entries
	// not yet filled in).
	std::vector<float> m_GameValues;

	//This array is to map the returned vector in Select() and the
	// vectors given as argument in Update() to their corresponding
	// games. Warning: it is possible, although unlikely, that this
	// array is empty when m_pSelectedLeaves is nonempty (we may
	// select a terminal node for every search tree, by chance.)
	std::vector<size_t> m_SelectedIndices; 
};


} // namespace c40kl



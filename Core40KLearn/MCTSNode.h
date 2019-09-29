#pragma once


#include "Utility.h"
#include "GameState.h"


namespace c40kl
{


class MCTSNode;
typedef std::shared_ptr<MCTSNode> MCTSNodePtr;


typedef std::vector<MCTSNodePtr> MCTSNodeArray;


/// <summary>
/// This represents a STATE NODE in the MCTS tree,
/// which has a set of actions it can perform, and
/// each action has a distribution of resulting states.
/// 
/// </summary>
class C40KL_API MCTSNode
{
public:
	/// <summary>
	/// Create a new MCTS tree root node from the given
	/// state, and return it.
	/// </summary>
	static MCTSNodePtr CreateRootNode(const GameState& state);


private:
	/// <summary>
	/// Create a new MCTS node, passing the given data to its
	/// constructor (see constructor for argument explanation).
	/// </summary>
	static MCTSNodePtr CreateChildNode(const GameState& state, MCTSNode* pParent, float weightFromParent);


	/// <summary>
	/// Create a new MCTS node from the given state.
	/// If you want this to be a root node, then pass nullptr
	/// for the parent. If you do not pass nullptr, you must
	/// also pass weightFromParent, which is the probability
	/// of ending in this state by applying whatever action was
	/// used to get here from the parent.
	/// NOTE: this is a private constructor because users
	/// of the MCTS tree will only ever be allocating root nodes.
	/// </summary>
	MCTSNode(const GameState& state, MCTSNode* pParent, float weightFromParent);


public:
	/// <summary>
	/// Determine if the current node is a leaf node, meaning
	/// that it has not been expanded yet. Note that terminal
	/// nodes count as leaf nodes, too.
	/// </summary>
	bool IsLeaf() const;


	/// <summary>
	/// Determine if the current node is terminal, meaning that
	/// its state is "finished", and it cannot have children. This
	/// also means that the true value is known for this state.
	/// </summary>
	bool IsTerminal() const;


	/// <summary>
	/// Determine if this node is the root node (which means it
	/// has no parent node).
	/// </summary>
	bool IsRoot() const;


	/// <summary>
	/// Expand this leaf node, by giving it a prior action distribution,
	/// and letting it generate child nodes.
	/// PRECONDITION: IsLeaf()
	/// POSTCONDITION: !IsLeaf()
	/// </summary>
	void Expand(const std::vector<float>& priorActionDistribution);


	/// <summary>
	/// Add a value estimate for this state to this node.
	/// This AUTOMATICALLY backpropagates this value to
	/// all parent nodes in the tree, up to the root.
	///
	/// Important note: the terminal node edge case!
	/// If this is a terminal node, you shouldn't need
	/// to estimate the state's value, however this
	/// function will check nothing of the sort and will
	/// update its value just as any other node would.
	/// </summary>
	void AddValueStatistic(float value);


	/// <summary>
	/// Get the estimated value of this state using ONLY
	/// the available statistics (and not anything about
	/// the state). Returns zero if GetNumValueSamples()
	/// is zero (i.e. our prior is a draw before any information
	/// is received.)
	/// </summary>
	float GetValueEstimate() const;


	/// <summary>
	/// Return the number of samples used to compute the
	/// GetValueEstimate() estimate.
	/// </summary>
	size_t GetNumValueSamples() const;


	/// <summary>
	/// Remove this node's parent pointer, making it a
	/// root node. This means values will no longer be
	/// backpropagated through this node, and leaves you
	/// free to delete the parent node.
	/// PRECONDITION: !IsRoot()
	/// </summary>
	void Detach();


	/// <summary>
	/// Returns the number of possible actions to take.
	/// (no precondition; terminal states always return
	/// 0 here and leaf states still return the correct
	/// number).
	/// </summary>
	size_t GetNumActions() const;


	/// <summary>
	/// Return the actions possible from this node.
	/// (Can be called on non-leaf nodes).
	/// The size of the returned array is equal to
	/// GetNumPossibleActions().
	/// </summary>
	GameCommandArray GetActions() const;


	/// <summary>
	/// Return the prior distribution assigned to this
	/// node when it was expanded.
	/// PRECONDITION: !IsLeaf().
	/// </summary>
	std::vector<float> GetActionPriorDistribution() const;


	/// <summary>
	/// Return the number of times each action has been
	/// visited (which is computed by aggregating across
	/// the child states which have had values estimaed.)
	/// PRECONDITION: !IsLeaf().
	/// </summary>
	std::vector<int> GetActionVisitCounts() const;


	/// <summary>
	/// Return the estimated values for each action, which
	/// are computed using an aggregation of value estimates
	/// of the resulting states which have been visited.
	/// Note: if the action hasn't been visited yet, the value
	/// defaults to zero.
	/// PRECONDITION: !IsLeaf().
	/// </summary>
	std::vector<float> GetActionValueEstimates() const;


	/// <summary>
	/// Returns the number of states that result
	/// from applying action with index = actionIdx.
	/// This is the same as the size of the GetStateResultDistribution()
	/// and GetStateResults() returned arrays.
	/// PRECONDITION: !IsLeaf().
	/// </summary>
	size_t GetNumResultingStates(size_t actionIdx) const;


	/// <summary>
	/// Return the state transition probabilities for
	/// a given action, specified by its index.
	/// PRECONDITION: !IsLeaf().
	/// </summary>
	std::vector<float> GetStateResultDistribution(size_t actionIdx) const;


	/// <summary>
	/// Return the possible state nodes that can result
	/// from the given action, specified by its index.
	/// PRECONDITION: !IsLeaf().
	/// </summary>
	const MCTSNodeArray& GetStateResults(size_t actionIdx) const;


	/// <summary>
	/// Get the game state represented by this node.
	/// </summary>
	const GameState& GetState() const;


	/// <summary>
	/// Return the depth of this node in the search tree.
	/// The depth is the minimum number of edges required
	/// to traverse through the tree to reach the root.
	/// I.e. the root is depth 0.
	/// </summary>
	size_t GetDepth() const;


private:
	const GameCommandArray& GetMyActions() const;


private:
	//The current game state at this node:
	const GameState m_State;
	
	//The parent node:
	MCTSNode *m_pParent;

	const float m_WeightFromParent;

	//True if and only if this node is not a leaf
	// node (if this state is terminal then this
	// cannot be true; this is made true by calling
	// Expand()).
	bool m_bExpanded;

	//The action prior is set when expanded
	std::vector<float> m_ActionPrior;

	//This is the number of value samples we have
	// received for this node
	size_t m_NumEstimates;

	//This is the sum of the values*weights, and
	// just the sum of the weights, respectively.
	// m_ValueSum / m_WeightSum gives us the estimate
	// of this node's value, provided m_WeightSum > 0.
	float m_ValueSum, m_WeightSum;

	//This is the list of actions we can take from
	// this node, which is initialised lazily:
	mutable GameCommandArray m_pActions;
	mutable bool m_bInitialisedActions;

	//This is the list of child nodes PER ACTION:
	// So m_pChildren[i] represents all the resulting
	// states from applying action i.
	std::vector<MCTSNodeArray> m_pChildren;
	
	//This is the distribution of states PER ACTION:
	// So m_Weights[i] represents the probabilities
	// of each state resulting from action i.
	std::vector<std::vector<float>> m_Weights;
};


} // namespace c40kl



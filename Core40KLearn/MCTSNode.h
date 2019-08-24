#pragma once


#include "Utility.h"
#include "GameState.h"


namespace c40kl
{


class MCTSNode;
typedef std::shared_ptr<MCTSNode> MCTSNodePtr;


/// <summary>
/// This represents a STATE NODE in the MCTS tree,
/// which has a set of actions it can perform, and
/// each action has a distribution of resulting states.
/// 
/// </summary>
class C40KL_API MCTSNode
{
public:
	MCTSNode(GameState state, MCTSNode* pParent);


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
	/// If this is a terminal node, we already know its
	/// true value, however it is still worth knowing
	/// that we've visited this node again, as it affects
	/// our action distributions much earlier on in the
	/// tree. Hence, if you call this on a terminal node,
	/// the value estimate you provide will be ignored, but
	/// will still be backpropagated!
	/// </summary>
	void AddValueStatistic(float value);


	/// <summary>
	/// Remove this node's parent pointer, making it a
	/// root node. This means values will no longer be
	/// backpropagated through this node, and leaves you
	/// free to delete the parent node.
	/// PRECONDITION: !IsRoot()
	/// </summary>
	void Detach();


	/// <summary>
	/// Return the prior distribution assigned to this
	/// node when it was expanded.
	/// PRECONDITION: !IsLeaf().
	/// </summary>
	std::vector<float> GetActionPriorDisribution() const;


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
	/// Return the actions possible from this node.
	/// (Can be called on non-leaf nodes).
	/// </summary>
	GameCommandArray GetActions() const;


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
	std::vector<MCTSNodePtr> GetStateResults(size_t actionIdx) const;


	/// <summary>
	/// Get the game state represented by this node.
	/// </summary>
	const GameState& GetCurrentState() const;


	/// <summary>
	/// Convert the tree rooted at this node to a
	/// multiline string format.
	/// </summary>
	String ToString() const;


private:
	// Current state
	// Parent node pointer
	// Is expanded?
	// Priors (only if expanded)
	// Value statistic sum and counts and weights (can be before expanding)
	// Child actions (only if expanded)
	// Child states per action (only if expanded) AS NODES
	// Child weights per action (only if expanded)
};


} // namespace c40kl



#pragma once


#include "Utility.h"
#include "MCTSNode.h"


namespace c40kl
{


/// <summary>
/// A policy strategy is an object whose purpose it is to
/// compute policies from existing information.
/// </summary>
class C40KL_API IPolicyStrategy
{
public:
	virtual ~IPolicyStrategy() = default;

	/// <summary>
	/// Get a distribution over the possible actions to take from
	/// a given node.
	/// Postconditions: the returned vector must contain nonnegative
	/// values and must sum to 1 (so it is a distrbution) and must
	/// be the same length as the number of actions available from the
	/// given node (and the probabilities correspond to the actions).
	/// </summary>
	virtual std::vector<float> GetActionDistribution(const MCTSNode& node) const = 0;
};


} // namespace c40kl



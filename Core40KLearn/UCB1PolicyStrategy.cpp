#include "UCB1PolicyStrategy.h"
#include <cmath>
#include <numeric>
#include <algorithm>


namespace c40kl
{


UCB1PolicyStrategy::UCB1PolicyStrategy(float exploratoryParam, int rootTeam) :
	m_ExploratoryParam(exploratoryParam),
	m_Team(rootTeam)
{
	C40KL_ASSERT_PRECONDITION(m_ExploratoryParam >= 0.0f,
		"Exploratory parameter must be nonnegative.");
	C40KL_ASSERT_PRECONDITION(m_Team == 0 || m_Team == 1,
		"Need valid team value.");
}


std::vector<float> UCB1PolicyStrategy::GetActionDistribution(const MCTSNode& node) const
{
	size_t maxIdx = ActionArgMax(node);

	//Now construct distribution:
	std::vector<float> dist(node.GetNumActions(), 0.0f);
	dist[maxIdx] = 1.0f;
	return dist;
}


size_t UCB1PolicyStrategy::ActionArgMax(const MCTSNode& node) const
{
	C40KL_ASSERT_PRECONDITION(!node.IsLeaf(),
		"UCB1 only works for non-leaf nodes!");

	const int curTeam = node.GetState().GetActingTeam();
	const auto priors = node.GetActionPriorDistribution();
	const auto actionVals = node.GetActionValueEstimates();
	const auto actionVisitCounts = node.GetActionVisitCounts();

	const float teamMultiplier = (curTeam == m_Team) ? 1.0f : (-1.0f);

	C40KL_ASSERT_INVARIANT(priors.size() == actionVals.size()
		&& actionVals.size() == actionVisitCounts.size(),
		"MCTS Node needs valid action info!");

	const size_t n = priors.size();

	//Determine the total number of visits:
	size_t totalVisits = 0;
	for (size_t k : actionVisitCounts)
		totalVisits += k;

	//Note: if it's the case that we have never visited this
	// node, then just select straight from the prior. To do
	// this we just clip the logVisits to be >= 0, i.e. if
	// totalVisits == 1 or 0 then logVisits is just 0.
	const float logVisits = (totalVisits > 0) ? std::log((float)totalVisits) : 0.0f;

	std::vector<float> ucbValues(n, 0.0f);

	for (size_t i = 0; i < n; i++)
	{
		ucbValues[i] = actionVals[i] * teamMultiplier
			+ m_ExploratoryParam * priors[i] * std::sqrtf(
				logVisits / (1.0f + (float)actionVisitCounts[i])
			);
	}

	//UCB1 always picks the best one, hence we
	// return a distribution of zero everywhere
	// except the action with highest UCB value,
	// which has probability 1:

	auto iter = std::max_element(ucbValues.begin(), ucbValues.end());

	C40KL_ASSERT_INVARIANT(iter != ucbValues.end(),
		"Need valid best action for UCB1.");

	return std::distance(ucbValues.begin(), iter);
}


} // namespace c40kl



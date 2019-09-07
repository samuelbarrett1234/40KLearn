#include "UCB1PolicyStrategy.h"
#include <cmath>
#include <numeric>


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
	const float logVisits = std::log((float)totalVisits);

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

	float maxVal = -std::numeric_limits<float>::max();
	size_t maxIdx = (size_t)(-1);

	for (size_t i = 0; i < n; i++)
	{
		if (ucbValues[i] > maxVal)
		{
			maxVal = ucbValues[i];
			maxIdx = i;
		}
	}

	C40KL_ASSERT_INVARIANT(maxIdx < n,
		"Need valid best action for UCB1.");

	//Now construct distribution:
	std::vector<float> dist(n, 0.0f);
	dist[maxIdx] = 1.0f;
	return dist;
}


} // namespace c40kl



#pragma once


#include "GameState.h"
#include <random>


namespace c40kl
{


/// <summary>
/// This class is used for computing the value estimates of
/// arbitrary game states through random simulation.
/// </summary>
class C40KL_API UniformRandomEstimator
{
public:
	/// <summary>
	/// Compute the estimate of who is going to win in
	/// the given game state. Do this by performing a number
	/// of simulations where every player selects from their
	/// available actions uniformly at random.
	/// </summary>
	/// <param name="state">The state to estimate the winner in</param>
	/// <param name="team">The team to compute the value with respect to</param>
	/// <param name="numSimulations">The number of simulations to perform and average across.</param>
	/// <returns>1.0f if 'team' will won in all simulations, -1.0 if 'team' lost in all simulations, etc.</returns>
	float ComputeValueEstimate(const GameState& state, int team, size_t numSimulations);

private:
	std::mt19937 m_RandEng;
};


} // namespace c40kl



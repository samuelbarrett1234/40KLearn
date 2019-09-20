#pragma once


#include <vector>
#include <random>
#include <cstdlib>


namespace c40kl
{


template<typename RandomEngine_t>
size_t SelectRandomly(RandomEngine_t& eng, const std::vector<float>& probabilities)
{
	//Determine cumulative probability distribution

	std::vector<float> cumulative;
	cumulative.resize(probabilities.size(), 0.0f);

	cumulative[0] = probabilities[0];
	for (size_t i = 1; i < probabilities.size(); i++)
	{
		cumulative[i] = probabilities[i] + cumulative[i - 1];
	}

	//Select at random by generating a random number between 0 and 1
	// and finding the first entry in cumulative which is greater than
	// or equal to it.

	const float rand_value = std::uniform_real_distribution<float>(0.0f, 1.0f)(eng);

	auto selected_iter = std::lower_bound(cumulative.begin(), cumulative.end(), rand_value);

	return std::distance(cumulative.begin(), selected_iter);
}


} // namespace c40kl



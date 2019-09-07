#include "UniformRandomEstimator.h"
#include "SelectRandomly.h"


namespace c40kl
{


float UniformRandomEstimator::ComputeValueEstimate(const GameState& state, int team, size_t numSimulations)
{
	float resultSum = 0.0f;

	//TODO: parallelise this loop
	for (size_t i = 0; i < numSimulations; i++)
	{
		GameState curState = state;

		//Simulate until finished
		while (!curState.IsFinished())
		{
			std::vector<GameState> results;
			std::vector<float> probs;

			auto cmds = curState.GetCommands();

			//Choose a command uniformly at random:
			std::uniform_int_distribution<size_t> cmdDist(0, cmds.size() - 1);
			auto pChosenCmd = cmds[cmdDist(m_RandEng)];

			//Now apply the command:
			pChosenCmd->Apply(curState, results, probs);

			//Choose a result at random, according to the probabilities:
			const size_t resultIdx = SelectRandomly(m_RandEng, probs);

			//Now update the resulting state:
			curState = results[resultIdx];
		}

		//Add resulting game value:
		resultSum += curState.GetGameValue(team);
	}

	return resultSum / (float)numSimulations;
}


} // namespace c40kl



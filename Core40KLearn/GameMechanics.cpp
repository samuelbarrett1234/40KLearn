#include "GameMechanics.h"


namespace c40kl
{


void ApplyCommand(GameCommandPtr pCmd,
	const std::vector<GameState>& inStates,
	const std::vector<float>& inProbabilities,
	std::vector<GameState>& outStates,
	std::vector<float>& outProbabilities)
{
	C40KL_ASSERT_PRECONDITION(inStates.size() == inProbabilities.size(),
		"Need same number of states as probabilities");
	
	C40KL_ASSERT_PRECONDITION(pCmd != nullptr, "Need a valid command.");

	const size_t n = inStates.size();

	std::vector<float> probs;
	for (size_t i = 0; i < n; i++)
	{
		//Apply to get states and probabilities of result of command
		pCmd->Apply(inStates[i], outStates, probs);

		//The law of total probability
		const float p = inProbabilities[i];
		for (size_t j = 0; j < probs.size(); j++)
			probs[j] *= p;

		outProbabilities.insert(outProbabilities.end(), probs.begin(), probs.end());

		//Clear for next round of the loop
		probs.clear();
	}
}

} // namespace c40kl



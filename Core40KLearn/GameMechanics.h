#pragma once


#include "Unit.h"
#include "GameState.h"


namespace c40kl
{


/// <summary>
/// Apply the given command to the given
/// state distribution, resulting in another
/// state distribution.
/// </summary>
/// <param name="pCmd">The command to apply (must be non-null).</param>
/// <param name="inStates">The array of states; the action will be applied to each.</param>
/// <param name="inProbabilities">The probabilities associated with each state; must be an equally-sized array.</param>
/// <param name="outStates">Resulting states from the action will be written to this array.</param>
/// <param name="outProbabilities">The probabilities associated with each state in outStates will be written to this array.</param>
void ApplyCommand(GameCommandPtr pCmd,
	const std::vector<GameState>& inStates,
	const std::vector<float>& inProbabilities,
	std::vector<GameState>& outStates,
	std::vector<float>& outProbabilities);


} // namespace c40kl



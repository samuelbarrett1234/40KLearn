#include "CompositeCommand.h"
#include "GameState.h"
#include "GameMechanics.h"
#include <sstream>


namespace c40kl
{


void CompositeCommand::Apply(const GameState& startState,
	std::vector<GameState>& outStates, std::vector<float>& outDistribution) const
{
	std::vector<GameState> statesA, statesB;
	std::vector<float> probsA, probsB;

	//Initialise:

	statesA.push_back(startState);
	probsA.push_back(1.0f);

	//Set up double buffer with states/probs

	std::vector<GameState> *pStatesReading = &statesA,
		*pStatesWriting = &statesB;
	std::vector<float> *pProbsReading = &probsA,
		*pProbsWriting = &probsB;

	//Apply each command
	for (const auto& pCmd : m_pCommands)
	{
		//Apply the command and write output to the writing buffers

		ApplyCommand(pCmd, *pStatesReading, *pProbsReading, *pStatesWriting, *pProbsWriting);

		//After applying the command, swap the buffers and
		// clear the writing buffers

		std::swap(pStatesReading, pStatesWriting);
		std::swap(pProbsReading, pProbsWriting);
		pStatesWriting->clear();
		pProbsWriting->clear();
	}

	//The latest results are currently stored in the reading buffers:
	outStates.insert(outStates.end(),
		pStatesReading->begin(), pStatesReading->end());
	outDistribution.insert(outDistribution.end(),
		pProbsReading->begin(), pProbsReading->end());
}


bool CompositeCommand::Equals(const IGameCommand& cmd) const
{
	if (auto pCmd = dynamic_cast<const CompositeCommand*>(&cmd))
	{
		if (m_pCommands.size() != pCmd->m_pCommands.size())
			return false;
		const size_t n = m_pCommands.size();
		for (size_t i = 0; i < n; i++)
		{
			if (!m_pCommands[i]->Equals(*pCmd->m_pCommands[i]))
				return false;
		}
		return true;
	}
	else return false;
}


String CompositeCommand::ToString() const
{
	std::stringstream c;
	c << "composite command: [";
	for (auto pCmd : m_pCommands)
	{
		c << pCmd->ToString();
		c << ", ";
	}
	c << "]";
	return c.str();
}


} // namespace c40kl



#pragma once


#include "GameState.h"
#include "MCTSNode.h"
#include "UCB1PolicyStrategy.h"


namespace c40kl
{


class SelfPlayManager
{
public:
	SelfPlayManager(const UCB1PolicyStrategy& tree_policy, size_t numSimulations);

	void Reset(size_t numGames, const GameState& initialState);

	void Select(std::vector<GameState>& outLeafStates);

	void Update(const std::vector<float>& valueEstimates,
		const std::vector<std::vector<float>>& policies);

	bool IsWaiting() const;

	std::vector<GameState> GetCurrentGameStates() const;

private:
	MCTSNodeArray m_pRoots;
};


} // namespace c40kl



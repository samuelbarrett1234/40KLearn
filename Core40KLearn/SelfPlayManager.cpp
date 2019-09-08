#include "SelfPlayManager.h"
#include "SelectRandomly.h"


namespace c40kl
{


SelfPlayManager::SelfPlayManager(float ucb1ExplorationParameter, size_t numSimulations) :
	m_TreePolicy(ucb1ExplorationParameter, 0), //Always evaluate with respect to team 0
	m_NumSimulations(numSimulations)
{
	C40KL_ASSERT_PRECONDITION(ucb1ExplorationParameter > 0,
		"UCB1 exploration parameter must be > 0.");
}


void SelfPlayManager::Reset(size_t numGames, const GameState& initialState)
{
	m_pRoots.clear();
	m_pRoots.resize(numGames, MCTSNodePtr());
	m_pSelectedLeaves.clear();
	m_SelectedIndices.clear();

	for (size_t i = 0; i < numGames; i++)
	{
		m_pRoots[i] = MCTSNode::CreateRootNode(initialState);
	}
}


void SelfPlayManager::Select(std::vector<GameState>& outLeafStates)
{
	C40KL_ASSERT_PRECONDITION(!IsWaiting(),
		"Select() should not be called twice in succession.");

	C40KL_ASSERT_PRECONDITION(!ReadyToCommit(),
		"Do not call Select() when ReadyToCommit().");

	C40KL_ASSERT_PRECONDITION(!AllFinished(),
		"Cannot call Select() when all games are finished.");

	//Clear input vector, and reserve the amount of space we expect to use:
	outLeafStates.clear();
	outLeafStates.reserve(m_pRoots.size());

	//We know exactly how big m_pSelectedLeaves needs to be, but
	// not m_SelectedIndices, however it will usually be just as big.
	m_pSelectedLeaves.resize(m_pRoots.size());
	m_SelectedIndices.reserve(m_pRoots.size());

	//TODO: parallelise this loop.
	for (size_t i = 0; i < m_pRoots.size(); i++)
	{
		SelectLeafForGame(i);

		//If a leaf node needed to be selected for this game...
		if(m_pSelectedLeaves[i])
		{
			//If leaf is nonterminal...
			if (!m_pSelectedLeaves[i]->GetState().IsFinished())
			{
				outLeafStates.push_back(m_pSelectedLeaves[i]->GetState());
				m_SelectedIndices.push_back(i);
			}
		}
	}
}


void SelfPlayManager::Update(const std::vector<float>& valueEstimates,
	const std::vector<std::vector<float>>& policies)
{
	C40KL_ASSERT_PRECONDITION(IsWaiting(),
		"Select() should be called before Update().");

	C40KL_ASSERT_PRECONDITION(!ReadyToCommit(),
		"Do not call Update() when ReadyToCommit().");

	C40KL_ASSERT_PRECONDITION(!AllFinished(),
		"Cannot call Update() when all games are finished.");

	C40KL_ASSERT_PRECONDITION(valueEstimates.size() == m_SelectedIndices.size(),
		"Need correct number of value estimates.");

	C40KL_ASSERT_PRECONDITION(policies.size() == m_SelectedIndices.size(),
		"Need correct number of policies.");

	//First, perform a check to make sure the
	// individual policy sizes are correct. Do
	// this before any updates to ensure we don't
	// make changes to some games before throwing
	// an error.

	for (size_t i = 0; i < m_SelectedIndices.size(); i++)
	{
		const size_t j = m_SelectedIndices[i];

		const std::vector<float>& policy = policies[i];

		const MCTSNodePtr& pLeaf = m_pSelectedLeaves[j];

		//This should never fail, and it's not the user's fault if it fails, it's our fault.
		C40KL_ASSERT_INVARIANT((pLeaf.get() != nullptr) && !pLeaf->GetState().IsFinished(),
			"Selected leaf must exist and be nonterminal.");

		C40KL_ASSERT_PRECONDITION(pLeaf->GetNumActions() == policy.size(),
			"Policy size needs to match number of actions in leaf.");
	}

	//TODO: parallelise this loop!
	for (size_t i = 0; i < m_SelectedIndices.size(); i++)
	{
		C40KL_ASSERT_INVARIANT(!m_pSelectedLeaves[m_SelectedIndices[i]]->GetState().IsFinished(),
			"Selected indices must point to nonterminal nodes.");
		ExpandBackpropagate(m_SelectedIndices[i], valueEstimates[i], policies[i]);
	}
	
	//Don't forget that the selected indices DO NOT include
	// terminal states, however we still want to backpropagate
	// terminal values:
	for (size_t i = 0; i < m_pSelectedLeaves.size(); i++)
	{
		const auto state = m_pSelectedLeaves[i]->GetState();
		if (state.IsFinished())
		{
			//Note: ExpandBackpropagate automatically converts
			// the value estimate to the perspective of team 0.
			const float valEst = state.GetGameValue(state.GetActingTeam());

			ExpandBackpropagate(i, valEst, std::vector<float>());
		}
	}

	//Clear everything as we are no longer in a waiting state:
	m_SelectedIndices.clear();
	m_pSelectedLeaves.clear();
}


void SelfPlayManager::Commit()
{
	C40KL_ASSERT_PRECONDITION(ReadyToCommit(),
		"Must be ready to commit before calling Commit()!");

	C40KL_ASSERT_PRECONDITION(!AllFinished(),
		"Cannot Commit() when all games are finished.");

	//TODO: parallelise this. BUT, warning: there is random generation
	// involved in this loop. To parallelise this loop, we would need
	// to do all random generation beforehand, in the main thread (which
	// is easy enough to do).
	for (size_t i = 0; i < m_pRoots.size(); i++)
	{
		const auto actions = m_pRoots[i]->GetActions();

		//Use tree search data available at the ith root to select an action.
		//WARNING: if the final policy is stochastic, and this loop is parallelised,
		// then this won't work as the threads will all be sharing the random engine!
		const size_t actionIdx = GetFinalPolicy(i);
		
		C40KL_ASSERT_INVARIANT(actionIdx < m_pRoots[i]->GetNumActions(),
			"Final policy must return valid action index.");


		std::vector<GameState> results;
		std::vector<float> probs;

		//Apply the selected action
		actions[actionIdx]->Apply(m_pRoots[i]->GetState(), results, probs);

		//Now randomly select a resulting state:
		// (Will need to change this if the loop is parallelised!)
		const size_t resultIdx = SelectRandomly(m_RandEng, probs);
		const GameState resultingState = results[resultIdx];
		
		//We now need to find the MCTS node corresponding to this state and commit.
		auto actionChildNodes = m_pRoots[i]->GetStateResults(actionIdx);

		C40KL_ASSERT_INVARIANT(actionChildNodes.size() == results.size(),
			"MCTS must tie up with command results.");
		
		C40KL_ASSERT_INVARIANT(actionChildNodes[resultIdx]->GetState() == resultingState,
			"MCTS child nodes must be correctly ordered, corresponding to action results.");

		//Now we get to re-root the tree as a result of the action!
		m_pRoots[i] = actionChildNodes[resultIdx];
	}
}


bool SelfPlayManager::IsWaiting() const
{
	return !m_pSelectedLeaves.empty();
}


bool SelfPlayManager::ReadyToCommit() const
{
	for (size_t i = 0; i < m_pRoots.size(); i++)
	{
		if (m_pRoots[i]->GetNumValueSamples() < m_NumSimulations)
		{
			//We have found a tree which has not had enough simulations
			return false;
		}
	}

	//Every tree has enough simulations:
	return true;
}


bool SelfPlayManager::AllFinished() const
{
	for (size_t i = 0; i < m_pRoots.size(); i++)
	{
		if (!m_pRoots[i]->GetState().IsFinished())
		{
			C40KL_ASSERT_INVARIANT(m_pRoots[i]->GetNumActions() > 0,
				"Unfinished games must have available actions!");

			//We have found a game which isn't yet finished.
			return false;
		}
	}

	//All games are finished:
	return true;
}


std::vector<GameState> SelfPlayManager::GetCurrentGameStates() const
{
	std::vector<GameState> gss;
	gss.reserve(m_pRoots.size());

	for (size_t i = 0; i < m_pRoots.size(); i++)
	{
		gss.push_back(m_pRoots[i]->GetState());
	}

	return gss;
}


std::vector<std::vector<float>> SelfPlayManager::GetCurrentActionDistributions() const
{
	std::vector<std::vector<float>> actionDists;
	actionDists.resize(m_pRoots.size());

	for (size_t i = 0; i < m_pRoots.size(); i++)
	{
		actionDists[i].resize(m_pRoots[i]->GetNumActions(), 0.0f);
		
		//For now, we are doing a deterministic final policy:
		const size_t selectedIdx = GetFinalPolicy(i);

		actionDists[i][selectedIdx] = 1.0f;
	}

	return actionDists;
}


void SelfPlayManager::SelectLeafForGame(size_t gameIdx)
{
	auto pNode = m_pRoots[gameIdx];

	while (!pNode->IsLeaf() && !pNode->IsTerminal())
	{
		const auto actions = pNode->GetActions();

		//Choose the action which maximises UCB1:
		const size_t actionIdx = m_TreePolicy.ActionArgMax(*pNode);

		//Apply the action:
		std::vector<GameState> results;
		std::vector<float> probs;
		actions[actionIdx]->Apply(pNode->GetState(), results, probs);

		//WARNING: if parallelising, we need to remove the
		// dependency on m_RandEng here!

		//Select a random result:
		const size_t resultingIdx = SelectRandomly(m_RandEng, probs);

		//Now choose the resulting MCTSNodePtr:
		auto children = pNode->GetStateResults(actionIdx);

		C40KL_ASSERT_INVARIANT(children.size() == results.size(),
			"MCTS node results must tie up with action results.");

		pNode = children[resultingIdx];

		C40KL_ASSERT_INVARIANT(pNode->GetState() == results[resultingIdx],
			"Action results and MCTS node results must be in the same order.");
	}

	//We have selected a leaf node!
	m_pSelectedLeaves[gameIdx] = pNode;
}


void SelfPlayManager::ExpandBackpropagate(size_t gameIdx, float valEst, const std::vector<float>& policy)
{
	C40KL_ASSERT_INVARIANT(m_pSelectedLeaves[gameIdx].get() != nullptr,
		"Needs a selected leaf!");

	if (!m_pSelectedLeaves[gameIdx]->GetState().IsFinished())
	{
		//Expand if not finished:
		m_pSelectedLeaves[gameIdx]->Expand(policy);
	}

	//Now add the statistic (automatically backpropagates):

	m_pSelectedLeaves[gameIdx]->AddValueStatistic(valEst);
}


size_t SelfPlayManager::GetFinalPolicy(size_t gameIdx) const
{
	const auto actionVisitCounts = m_pRoots[gameIdx]->GetActionVisitCounts();

	C40KL_ASSERT_INVARIANT(actionVisitCounts.size() > 0,
		"Need actions in nonterminal state.");

	//Return argmax of action visit counts:

	size_t bestAction = 0;
	size_t bestVisits = actionVisitCounts.front();
	for (size_t i = 1; i < actionVisitCounts.size(); i++)
	{
		if (bestVisits < actionVisitCounts[i])
		{
			bestAction = i;
			bestVisits = actionVisitCounts[i];
		}
	}
	return bestAction;
}


} // namespace c40kl



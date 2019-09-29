#include "SelfPlayManager.h"
#include "SelectRandomly.h"
#include <algorithm>
#include <numeric>
#include <boost/range/combine.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/bind.hpp>


namespace c40kl
{


SelfPlayManager::SelfPlayManager(float ucb1ExplorationParameter, float temperature,
	size_t numSimulations, size_t numThreads) :
	m_TreePolicy(ucb1ExplorationParameter, 0), //Always evaluate with respect to team 0
	m_NumSimulations(numSimulations),
	m_NumThreads(std::max(numThreads, (size_t)1)),
	m_Temperature(temperature)
{
	C40KL_ASSERT_PRECONDITION(ucb1ExplorationParameter > 0,
		"UCB1 exploration parameter must be > 0.");
	C40KL_ASSERT_PRECONDITION(temperature >= 0,
		"Temperature must be >= 0.");
}


SelfPlayManager::~SelfPlayManager()
{
}


void SelfPlayManager::Reset(size_t numGames, const GameState& initialState)
{
	C40KL_ASSERT_PRECONDITION(!initialState.IsFinished(),
		"Cannot play finished initial state.");

	m_pRoots.clear();
	m_pRoots.resize(numGames, MCTSNodePtr());

	m_GameIDs.clear();
	m_GameIDs.resize(numGames, 0);

	m_GameValues.clear();
	m_GameValues.resize(numGames);

	m_pSelectedLeaves.clear();
	m_SelectedIndices.clear();

	for (size_t i = 0; i < numGames; i++)
	{
		m_GameIDs[i] = i;
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

	//Clear output vector, and reserve the amount of space we expect to use:
	outLeafStates.clear();
	outLeafStates.reserve(m_pRoots.size());

	//We know exactly how big m_pSelectedLeaves needs to be, but
	// not m_SelectedIndices, however it will usually be just as big.
	m_pSelectedLeaves.resize(m_pRoots.size(), MCTSNodePtr());
	m_SelectedIndices.reserve(m_pRoots.size());

	boost::asio::thread_pool jobService(m_NumThreads);

	for (size_t i = 0; i < m_pRoots.size(); i++)
	{
		C40KL_ASSERT_INVARIANT(!m_pRoots[i]->IsTerminal(),
			"Roots should be nonterminal.");

		//If this tree hasn't had enough samples yet...
		if (m_pRoots[i]->GetNumValueSamples() < m_NumSimulations)
		{
			boost::asio::post(jobService, [i, this]() { SelectLeafForGame(i); });
		}
	}

	jobService.join();

	for (size_t i = 0; i < m_pRoots.size(); i++)
	{
		//If a leaf node needed to be selected for this game...
		if (m_pSelectedLeaves[i])
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
		const MCTSNodePtr& pLeaf = m_pSelectedLeaves[j];

		const std::vector<float>& policy = policies[i];

		//This should never fail, and it's not the user's fault if it fails, it's our fault.
		C40KL_ASSERT_INVARIANT((pLeaf.get() != nullptr) && !pLeaf->GetState().IsFinished(),
			"Selected leaf must exist and be nonterminal.");

		C40KL_ASSERT_PRECONDITION(pLeaf->GetNumActions() == policy.size(),
			"Policy size needs to match number of actions in leaf.");
	}

	boost::asio::thread_pool jobService(m_NumThreads);

	for (size_t i = 0; i < m_SelectedIndices.size(); i++)
	{
		auto job = [i, this, &policies, &valueEstimates]()
		{
			const size_t j = m_SelectedIndices[i];
			const MCTSNodePtr& pLeaf = m_pSelectedLeaves[j];

			const std::vector<float>& policy = policies[i];

			float valEst = valueEstimates[i];

			//Convert valEst to be from the perspective of team 0, if applicable:
			if (pLeaf->GetState().GetActingTeam() != 0)
			{
				valEst *= -1.0f;
			}

			ExpandBackpropagate(m_SelectedIndices[i], valEst, policy);
		};
		boost::asio::post(jobService, job);
	}

	//Don't forget that the selected indices DO NOT include
	// terminal states, however we still want to backpropagate
	// terminal values, in cases where a node was selected:
	for (size_t i = 0; i < m_pSelectedLeaves.size(); i++)
	{
		auto job = [i, this]()
		{
			//If there was any terminal node selected...
			if (m_pSelectedLeaves[i].get() != nullptr)
			{
				const auto state = m_pSelectedLeaves[i]->GetState();
				if (state.IsFinished())
				{
					//Note: ExpandBackpropagate automatically converts
					// the value estimate to the perspective of team 0.
					const float valEst = state.GetGameValue(0);

					ExpandBackpropagate(i, valEst, std::vector<float>());
				}
			}
		};
		boost::asio::post(jobService, job);
	}

	//Wait until jobs complete:
	jobService.join();

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
		const auto finalPolicyDistribution = GetFinalPolicy(i);
		const size_t actionIdx = SelectRandomly(m_RandEng, finalPolicyDistribution);
		
		C40KL_ASSERT_INVARIANT(actionIdx < m_pRoots[i]->GetNumActions(),
			"SelectRandomly must return valid action index.");
		
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

		//And don't forget to detach!
		m_pRoots[i]->Detach();

		//Now, if the game has finished, record its value:
		if (m_pRoots[i]->GetState().IsFinished())
		{
			//Store game's value with respect to team 0:
			m_GameValues[m_GameIDs[i]] = m_pRoots[i]->GetState().GetGameValue(0);
		}
	}

	//Remove trees representing finished games:

	C40KL_ASSERT_INVARIANT(m_pRoots.size() == m_GameIDs.size(),
		"Tree roots and IDs must be equal length!");
	
	//Use Boost remove_if and combine to erase from both m_pRoots and
	// from m_GameIDs at the same time:

	auto remove_iter = boost::remove_if(
		boost::combine(m_pRoots, m_GameIDs),
		[](const boost::tuple<MCTSNodePtr, size_t>& node)
			{ return node.get<0>()->GetState().IsFinished(); }
		);
	
	m_pRoots.erase(boost::get<0>(remove_iter.get_iterator_tuple()), m_pRoots.end());
	m_GameIDs.erase(boost::get<1>(remove_iter.get_iterator_tuple()), m_GameIDs.end());
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
	//Note: we specifically want this function to return
	// true when m_pRoots is empty.

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
		actionDists[i] = GetFinalPolicy(i);
	}

	return actionDists;
}


std::vector<std::vector<int>> SelfPlayManager::GetActionVisitCounts() const
{
	std::vector<std::vector<int>> actionVisitCounts;
	actionVisitCounts.resize(m_pRoots.size());

	for (size_t i = 0; i < m_pRoots.size(); i++)
	{
		actionVisitCounts[i] = m_pRoots[i]->GetActionVisitCounts();
	}

	return actionVisitCounts;
}


std::vector<int> SelfPlayManager::GetTreeSizes() const
{
	std::vector<int> result;
	result.resize(m_pRoots.size());

	for (size_t i = 0; i < m_pRoots.size(); i++)
	{
		result[i] = m_pRoots[i]->GetNumValueSamples();
	}

	return result;
}


std::vector<size_t> SelfPlayManager::GetRunningGameIds() const
{
	return m_GameIDs;
}


std::vector<float> SelfPlayManager::GetGameValues() const
{
	C40KL_ASSERT_PRECONDITION(AllFinished(),
		"Can only get game values when all games finished.");

	return m_GameValues;
}


void SelfPlayManager::SelectLeafForGame(size_t gameIdx)
{
	C40KL_ASSERT_INVARIANT(gameIdx < m_pRoots.size(),
		"Need valid game index.");

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

		C40KL_ASSERT_INVARIANT(results.size() == probs.size(),
			"Need to return valid distribution.");

		//WARNING: if parallelising, we need to remove the
		// dependency on m_RandEng here!

		//Select a random result:
		const size_t resultingIdx = SelectRandomly(m_RandEng, probs);

		C40KL_ASSERT_INVARIANT(resultingIdx < results.size(),
			"SelectRandomly must return valid index.");

		//Now choose the resulting MCTSNodePtr:
		auto children = pNode->GetStateResults(actionIdx);

		C40KL_ASSERT_INVARIANT(children.size() == results.size(),
			"MCTS node results must tie up with action results.");

		pNode = children[resultingIdx];

		C40KL_ASSERT_INVARIANT(pNode->GetState() == results[resultingIdx],
			"Action results and MCTS node results must be in the same order.");

		//If we have found a leaf node with no choice to be made (which
		// happens when there is exactly one command you can make) then
		// we may as well expand it here and continue:
		if (pNode->IsLeaf() && pNode->GetNumActions() == 1)
		{
			C40KL_ASSERT_INVARIANT(!pNode->IsTerminal(),
				"Nodes with one action shouldn't be terminal.");

			//Only one possible prior:
			pNode->Expand({ 1.0f });

			//Note that, since this node is nonterminal, expansion
			// has now made it not a leaf! Thus we will definitely
			// perform another iteration.
		}
	}

	//We have selected a leaf node!
	m_pSelectedLeaves[gameIdx] = pNode;
}


void SelfPlayManager::ExpandBackpropagate(size_t gameIdx, float valEst, const std::vector<float>& policy)
{
	C40KL_ASSERT_INVARIANT(gameIdx < m_pRoots.size(),
		"Need valid game index.");

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


std::vector<float> SelfPlayManager::GetFinalPolicy(size_t gameIdx) const
{
	C40KL_ASSERT_INVARIANT(gameIdx < m_pRoots.size(),
		"Need valid game index.");

	const auto actionVisitCounts = m_pRoots[gameIdx]->GetActionVisitCounts();

	C40KL_ASSERT_INVARIANT(actionVisitCounts.size() > 0,
		"Need actions in nonterminal state.");

	if (m_Temperature == 0.0f)
	{
		//If temperature is zero then just do an argmax:

		const size_t bestAction = std::distance(actionVisitCounts.begin(),
			std::max_element(actionVisitCounts.begin(), actionVisitCounts.end()));

		C40KL_ASSERT_INVARIANT(bestAction < actionVisitCounts.size(),
			"Need to find valid best action.");

		std::vector<float> outPolicy;
		outPolicy.resize(actionVisitCounts.size(), 0.0f);
		outPolicy[bestAction] = 1.0f;
		return outPolicy;
	}
	else
	{
		std::vector<float> outPolicy;
		outPolicy.resize(actionVisitCounts.size());
		
		std::transform(actionVisitCounts.begin(),
			actionVisitCounts.end(),
			outPolicy.begin(),
			boost::bind(std::powf, _1, 1.0f / m_Temperature));

		const float sum = std::accumulate(outPolicy.begin(),
			outPolicy.end(), 0.0f);

		C40KL_ASSERT_INVARIANT(sum > 0.0f,
			"Need nonzero policy sum.");

		for (auto& x : outPolicy)
			x /= sum;

		return outPolicy;
	}
}


} // namespace c40kl



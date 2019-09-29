#include "MCTSNode.h"


namespace c40kl
{


MCTSNodePtr MCTSNode::CreateRootNode(const GameState& state)
{
	//I would like to use make_shared here, but I'm going to
	// prioritise making the constructor private - unfortunately
	// I don't think I can have both!
	//Fortunately this won't be called very often so won't be
	// a performance hit.
	return MCTSNodePtr(new MCTSNode(state, nullptr, 0.0f));
}


MCTSNode::MCTSNode(const GameState& state, MCTSNode* pParent, float weightFromParent) :
	m_State(state),
	m_pParent(pParent),
	m_bExpanded(false),
	m_NumEstimates(0),
	m_ValueSum(0),
	m_WeightSum(0),
	m_bInitialisedActions(false),
	m_WeightFromParent(weightFromParent)
{
}


bool MCTSNode::IsLeaf() const
{
	return !m_bExpanded;
}


bool MCTSNode::IsTerminal() const
{
	return m_State.IsFinished();
}


bool MCTSNode::IsRoot() const
{
	return (m_pParent == nullptr);
}


void MCTSNode::Expand(const std::vector<float>& priorActionDistribution)
{
	const auto& actions = GetMyActions();

	C40KL_ASSERT_PRECONDITION(priorActionDistribution.size() == actions.size(),
		"Prior distribution needs to match actions.");
	C40KL_ASSERT_PRECONDITION(IsLeaf() && !IsTerminal(),
		"Cannot expand non-leaf or terminal nodes.");
	C40KL_ASSERT_INVARIANT(!GetMyActions().empty(),
		"Unfinished game states should always have available actions.");

	m_bExpanded = true;
	m_ActionPrior = priorActionDistribution;

	m_pChildren.reserve(actions.size());
	m_Weights.reserve(actions.size());

	//Now create child nodes:
	for (const auto& pCmd : actions)
	{
		//Get distribution resulting from the command
		std::vector<GameState> results;
		std::vector<float> probs;
		pCmd->Apply(m_State, results, probs);

		C40KL_ASSERT_INVARIANT(results.size() == probs.size(),
			"Invalid distribution.");

		//Turn game states into child nodes:
		MCTSNodeArray children;
		children.reserve(results.size());
		for (size_t i = 0; i < results.size(); i++)
		{
			//I can't use make_shared here; see the comment in CreateRootNode().
			children.emplace_back(new MCTSNode(results[i], this, probs[i]));
		}

		//Now save the info
		m_Weights.push_back(std::move(probs));
		m_pChildren.push_back(std::move(children));
	}
}


void MCTSNode::AddValueStatistic(float value)
{
	float weight = 1.0f;
	MCTSNode* pNode = this;
	while (pNode != nullptr)
	{
		//Add the statistic to pNode,
		// then traverse to parent and
		// multiply by weight of edge
		// from parent to pNode

		//Update value
		pNode->m_ValueSum += value * weight;
		pNode->m_WeightSum += weight;
		pNode->m_NumEstimates++;

		//Update weight
		if (pNode->m_pParent != nullptr)
		{
			weight *= pNode->m_WeightFromParent;
		}

		//Traverse to parent
		pNode = pNode->m_pParent;
	}
}


float MCTSNode::GetValueEstimate() const
{
	if (m_NumEstimates > 0)
	{
		C40KL_ASSERT_INVARIANT(m_WeightSum > 0.0f,
			"Weight sum must be strictly positive if received any samples.");

		return m_ValueSum / m_WeightSum;
	}
	else
	{
		//Default to zero when no estimates available
		return 0.0f;
	}
}


size_t MCTSNode::GetNumValueSamples() const
{
	return m_NumEstimates;
}


void MCTSNode::Detach()
{
	C40KL_ASSERT_PRECONDITION(!IsRoot(), "Cannot detach root.");
	m_pParent = nullptr;
}


size_t MCTSNode::GetNumActions() const
{
	return GetMyActions().size();
}


GameCommandArray MCTSNode::GetActions() const
{
	return GetMyActions();
}


std::vector<float> MCTSNode::GetActionPriorDistribution() const
{
	C40KL_ASSERT_PRECONDITION(!IsLeaf(), "Cannot get priors of leaf node.");

	return m_ActionPrior;
}


std::vector<int> MCTSNode::GetActionVisitCounts() const
{
	C40KL_ASSERT_PRECONDITION(!IsLeaf(), "Cannot get action visit counts of leaf node.");

	std::vector<int> visitCounts(GetMyActions().size(), 0);
	for (size_t i = 0; i < GetMyActions().size(); i++)
	{
		for (const auto& pChild : m_pChildren[i])
		{
			visitCounts[i] += pChild->GetNumValueSamples();
		}
	}
	return visitCounts;
}


std::vector<float> MCTSNode::GetActionValueEstimates() const
{
	C40KL_ASSERT_PRECONDITION(!IsLeaf(), "Cannot get action visit counts of leaf node.");

	std::vector<float> values(GetMyActions().size(), 0.0f), weights(GetMyActions().size(), 0.0f);

	//Aggregate values and weights over all samples
	// from resulting states *only including states
	// which have at least one estimate*.
	for (size_t i = 0; i < GetMyActions().size(); i++)
	{
		const auto& childrenFromAction = m_pChildren[i];
		const auto& weightsFromAction = m_Weights[i];

		for (size_t j = 0; j < m_pChildren[i].size(); j++)
		{
			if (childrenFromAction[j]->GetNumValueSamples() > 0)
			{
				values[i] += m_pChildren[i][j]->GetValueEstimate() * weightsFromAction[j] * (float)childrenFromAction[j]->GetNumValueSamples();
				weights[i] += weightsFromAction[j] * (float)childrenFromAction[j]->GetNumValueSamples();
			}
		}
	}

	//Now average the results we computed above. Note that
	// if an action has been unvisited, its weight will be zero,
	// hence we leave its estimated value as zero.
	std::vector<float> averages(GetMyActions().size(), 0.0f);
	for (size_t i = 0; i < GetMyActions().size(); i++)
	{
		if (weights[i] > 0)
		{
			averages[i] = values[i] / weights[i];
		}
		//Else leave it as zero
	}

	return averages;
}


size_t MCTSNode::GetNumResultingStates(size_t actionIdx) const
{
	C40KL_ASSERT_PRECONDITION(!IsLeaf(),
		"Can only get the number of resulting states for non leaf nodes.");
	C40KL_ASSERT_PRECONDITION(actionIdx < m_pChildren.size(),
		"Need valid action index.");

	return m_pChildren[actionIdx].size();
}


std::vector<float> MCTSNode::GetStateResultDistribution(size_t actionIdx) const
{
	C40KL_ASSERT_PRECONDITION(!IsLeaf(),
		"Can only get the resulting state distribution for non leaf nodes.");
	C40KL_ASSERT_PRECONDITION(actionIdx < m_Weights.size(),
		"Need valid action index.");

	return m_Weights[actionIdx];
}


const MCTSNodeArray& MCTSNode::GetStateResults(size_t actionIdx) const
{
	C40KL_ASSERT_PRECONDITION(!IsLeaf(),
		"Can only get the resulting state distribution for non leaf nodes.");
	C40KL_ASSERT_PRECONDITION(actionIdx < m_pChildren.size(),
		"Need valid action index.");

	return m_pChildren[actionIdx];
}


const GameState& MCTSNode::GetState() const
{
	return m_State;
}


size_t MCTSNode::GetDepth() const
{
	//Don't use recursion for this because it's inefficient
	//Don't forget that the root has zero depth

	const MCTSNode* pCurNode = this;
	size_t depth = 0;
	while (pCurNode->m_pParent != nullptr)
	{
		depth++;
		pCurNode = pCurNode->m_pParent;
	}
	return depth;
}


const GameCommandArray& MCTSNode::GetMyActions() const
{
	if (!m_bInitialisedActions)
	{
		m_bInitialisedActions = true;

		//Can only get actions for nonterminal state
		if (!m_State.IsFinished())
		{
			m_pActions = m_State.GetCommands();
		}
	}
	return m_pActions;
}


} // namespace c40kl



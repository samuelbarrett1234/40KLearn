#include "MCTSNodeWrapper.h"


MCTSNodeWrapper MCTSNodeWrapper::CreateRootNode(GameState state)
{
	return MCTSNodeWrapper(MCTSNode::CreateRootNode(state));
}


MCTSNodeWrapper::MCTSNodeWrapper(const MCTSNodePtr& pNode) :
	m_pNode(pNode)
{ }


bool MCTSNodeWrapper::IsLeaf() const
{
	return m_pNode->IsLeaf();
}


bool MCTSNodeWrapper::IsTerminal() const
{
	return m_pNode->IsTerminal();
}


bool MCTSNodeWrapper::IsRoot() const
{
	return m_pNode->IsRoot();
}


void MCTSNodeWrapper::Expand(const std::vector<float>& priorActionDistribution)
{
	m_pNode->Expand(priorActionDistribution);
}


void MCTSNodeWrapper::AddValueStatistic(float value)
{
	m_pNode->AddValueStatistic(value);
}


float MCTSNodeWrapper::GetValueEstimate() const
{
	return m_pNode->GetValueEstimate();
}


size_t MCTSNodeWrapper::GetNumValueSamples() const
{
	return m_pNode->GetNumValueSamples();
}


void MCTSNodeWrapper::Detach()
{
	m_pNode->Detach();
}


size_t MCTSNodeWrapper::GetNumActions() const
{
	return m_pNode->GetNumActions();
}


std::vector<CommandWrapper> MCTSNodeWrapper::GetActions() const
{
	//Convert command ptr to command wrapper:
	auto actions = m_pNode->GetActions();
	return std::vector<CommandWrapper>(actions.begin(), actions.end());
}


std::vector<float> MCTSNodeWrapper::GetActionPriorDistribution() const
{
	return m_pNode->GetActionPriorDistribution();
}


std::vector<int> MCTSNodeWrapper::GetActionVisitCounts() const
{
	return m_pNode->GetActionVisitCounts();
}


std::vector<float> MCTSNodeWrapper::GetActionValueEstimates() const
{
	return m_pNode->GetActionValueEstimates();
}


size_t MCTSNodeWrapper::GetNumResultingStates(size_t actionIdx) const
{
	return m_pNode->GetNumResultingStates(actionIdx);
}


std::vector<float> MCTSNodeWrapper::GetStateResultDistribution(size_t actionIdx) const
{
	return m_pNode->GetStateResultDistribution(actionIdx);
}


std::vector<MCTSNodeWrapper> MCTSNodeWrapper::GetStateResults(size_t actionIdx) const
{
	//Need to convert results
	auto results = m_pNode->GetStateResults(actionIdx);
	return std::vector<MCTSNodeWrapper>(results.begin(), results.end());
}


const GameState& MCTSNodeWrapper::GetState() const
{
	return m_pNode->GetState();
}


size_t MCTSNodeWrapper::GetDepth() const
{
	return m_pNode->GetDepth();
}



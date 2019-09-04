#pragma once


#include "BoostPython.h"
#include "CommandWrapper.h"
#include <MCTSNode.h>
using namespace c40kl;


//Wrapper class needed to wrap shared_ptr type.
//It is unfortunate that we have to wrap this, but as
// of yet, Boost cannot handle vectors of shared-ptrs!
class MCTSNodeWrapper
{
public:
	static MCTSNodeWrapper CreateRootNode(GameState state);

public:
	MCTSNodeWrapper(const MCTSNodePtr& pNode);

public:
	bool IsLeaf() const;
	bool IsTerminal() const;
	bool IsRoot() const;
	void Expand(const std::vector<float>& priorActionDistribution);
	void AddValueStatistic(float value);
	float GetValueEstimate() const;
	size_t GetNumValueSamples() const;
	void Detach();
	size_t GetNumActions() const;
	std::vector<CommandWrapper> GetActions() const;
	std::vector<float> GetActionPriorDistribution() const;
	std::vector<int> GetActionVisitCounts() const;
	std::vector<float> GetActionValueEstimates() const;
	size_t GetNumResultingStates(size_t actionIdx) const;
	std::vector<float> GetStateResultDistribution(size_t actionIdx) const;
	std::vector<MCTSNodeWrapper> GetStateResults(size_t actionIdx) const;
	const GameState& GetState() const;
	size_t GetDepth() const;

	inline const MCTSNodePtr& GetRawPtr() const
	{
		return m_pNode;
	}
	inline bool operator ==(const MCTSNodeWrapper& other) const
	{
		return m_pNode.get() == other.m_pNode.get();
	}

private:
	MCTSNodePtr m_pNode;
};



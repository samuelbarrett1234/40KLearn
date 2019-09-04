#include "BoostPython.h"
#include "MCTSNodeWrapper.h"
#include <UCB1PolicyStrategy.h>
#include <iostream>
using namespace c40kl;


//Simple wrapper to accomodate MCTS wrapper type
struct UCB1PolicyStrategyWrapper
{
	UCB1PolicyStrategyWrapper(float expParam, int team) :
		m_Policy(expParam, team)
	{ }

	std::vector<float> GetActionDistribution(const MCTSNodeWrapper& node) const
	{
		return m_Policy.GetActionDistribution(*node.GetRawPtr());
	}

	UCB1PolicyStrategy m_Policy;
};


void ExportUCB1PolicyStrategy()
{
	class_<UCB1PolicyStrategyWrapper>("UCB1PolicyStrategy", init<float, int>())
		.def("get_action_distribution", &UCB1PolicyStrategyWrapper::GetActionDistribution);
}



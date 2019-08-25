#include "BoostPython.h"
#include <UCB1PolicyStrategy.h>
using namespace c40kl;


//Need wrapper because Python doesn't like the pass by reference
struct UCB1PolicyStrategyWrapper
{
	UCB1PolicyStrategyWrapper(float expParam, int team) :
		m_Policy(expParam, team)
	{ }

	std::vector<float> GetActionDistribution(std::shared_ptr<MCTSNode> pNode) const
	{
		return m_Policy.GetActionDistribution(*pNode);
	}

	UCB1PolicyStrategy m_Policy;
};


void ExportUCB1PolicyStrategy()
{
	class_<UCB1PolicyStrategyWrapper>("UCB1PolicyStrategy", init<float, int>())
		.def("get_action_distribution", &UCB1PolicyStrategyWrapper::GetActionDistribution);
}



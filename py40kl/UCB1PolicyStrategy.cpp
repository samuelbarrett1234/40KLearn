#include "BoostPython.h"
#include <UCB1PolicyStrategy.h>
using namespace c40kl;


class_<UCB1PolicyStrategy>("UCB1PolicyStrategy", init<float, int>())
	.def("get_action_distribution", &UCB1PolicyStrategy::GetActionDistribution);



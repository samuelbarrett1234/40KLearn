#include "BoostPython.h"
#include <MCTSNode.h>
using namespace c40kl;


class_<MCTSNode, shared_ptr<MCTSNode>>("MCTSNode")
	.def("create_root_node", &MCTSNode::CreateRootNode)
	.staticmethod("create_root_node")
	.def("is_leaf", &MCTSNode::IsLeaf)
	.def("is_terminal", &MCTSNode::IsTerminal)
	.def("is_root", &MCTSNode::IsRoot)
	.def("expand", &MCTSNode::Expand)
	.def("add_value_statistic", &MCTSNode::AddValueStatistic)
	.def("get_value_estimate", &MCTSNode::GetValueEstimate)
	.def("get_num_value_samples", &MCTSNode::GetNumValueSamples)
	.def("detach", &MCTSNode::Detach)
	.def("get_num_actions", &MCTSNode::GetNumActions)
	.def("get_actions", &MCTSNode::GetActions)
	.def("get_action_prior_distribution", &MCTSNode::GetActionPriorDistribution)
	.def("get_action_visit_counts", &MCTSNode::GetActionVisitCounts)
	.def("get_action_value_estimates", &MCTSNode::GetActionValueEstimates)
	.def("get_num_resulting_states", &MCTSNode::GetNumResultingStates)
	.def("get_state_result_distribution", &MCTSNode::GetStateResultDistribution)
	.def("get_state_results", &MCTSNode::GetStateResults)
	.def("get_state", &MCTSNode::GetState,
	return_value_policy<copy_const_reference>())
	.def("get_depth", &MCTSNode::GetDepth);



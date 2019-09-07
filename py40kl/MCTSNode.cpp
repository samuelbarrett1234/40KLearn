#include "BoostPython.h"
#include "MCTSNodeWrapper.h"
#include <boost/python/list.hpp>
using namespace c40kl;


//Need special override function for generic Python lists.
void MCTSNodeExpandWithPythonList(MCTSNodeWrapper& node, list& py_list)
{
	std::vector<float> prior;
	prior.resize(len(py_list));
	for (size_t i = 0; i < prior.size(); i++)
	{
		prior[i] = extract<float>(py_list[i]);
	}
	node.Expand(prior);
}


void ExportMCTS()
{        
	class_<MCTSNodeWrapper>("MCTSNode", no_init)
		.def("create_root_node", &MCTSNodeWrapper::CreateRootNode)
		.staticmethod("create_root_node")
		.def("is_leaf", &MCTSNodeWrapper::IsLeaf)
		.def("is_terminal", &MCTSNodeWrapper::IsTerminal)
		.def("is_root", &MCTSNodeWrapper::IsRoot)
		.def("expand", &MCTSNodeWrapper::Expand)
		.def("expand", &MCTSNodeExpandWithPythonList)
		.def("add_value_statistic", &MCTSNodeWrapper::AddValueStatistic)
		.def("get_value_estimate", &MCTSNodeWrapper::GetValueEstimate)
		.def("get_num_value_samples", &MCTSNodeWrapper::GetNumValueSamples)
		.def("detach", &MCTSNodeWrapper::Detach)
		.def("get_num_actions", &MCTSNodeWrapper::GetNumActions)
		.def("get_actions", &MCTSNodeWrapper::GetActions)
		.def("get_action_prior_distribution", &MCTSNodeWrapper::GetActionPriorDistribution)
		.def("get_action_visit_counts", &MCTSNodeWrapper::GetActionVisitCounts)
		.def("get_action_value_estimates", &MCTSNodeWrapper::GetActionValueEstimates)
		.def("get_num_resulting_states", &MCTSNodeWrapper::GetNumResultingStates)
		.def("get_state_result_distribution", &MCTSNodeWrapper::GetStateResultDistribution)
		.def("get_state_results", &MCTSNodeWrapper::GetStateResults)
		.def("get_state", &MCTSNodeWrapper::GetState,
			return_value_policy<copy_const_reference>())
		.def("get_depth", &MCTSNodeWrapper::GetDepth);

	class_<std::vector<MCTSNodeWrapper>>("MCTSNodeArray")
		.def(vector_indexing_suite<std::vector<MCTSNodeWrapper>, true>());
}



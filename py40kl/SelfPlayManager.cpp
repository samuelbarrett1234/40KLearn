#include "BoostPython.h"
#include <SelfPlayManager.h>
using namespace c40kl;


//Special version of Update() for arbitrary Python iterables
void SelfPlayManager_PyUpdate(SelfPlayManager& mgr, object values, object policies)
{
	//Convert values to CPP

	std::vector<float> cppValues;
	cppValues.reserve(len(values));
	for (size_t i = 0; i < len(values); i++)
	{
		auto val = values[i];
		cppValues.push_back(extract<float>(val));
	}

	//Convert policies to CPP

	std::vector<std::vector<float>> cppPolicies;
	cppPolicies.reserve(len(policies));
	for (size_t i = 0; i < len(policies); i++)
	{
		auto policy = policies[i];

		cppPolicies.emplace_back();
		cppPolicies.back().reserve(len(policy));

		for (size_t j = 0; j < len(policy); j++)
		{
			cppPolicies.back().push_back(extract<float>(policy[j]));
		}
	}

	//Now call the Update function:
	mgr.Update(cppValues, cppPolicies);
}


void ExportSelfPlayManager()
{
	class_<SelfPlayManager>("SelfPlayManager", init<float, size_t>())
		.def("reset", &SelfPlayManager::Reset)
		.def("select", &SelfPlayManager::Select)
		.def("update", &SelfPlayManager::Update)
		.def("update", &SelfPlayManager_PyUpdate)
		.def("commit", &SelfPlayManager::Commit)
		.def("is_waiting", &SelfPlayManager::IsWaiting)
		.def("ready_to_commit", &SelfPlayManager::ReadyToCommit)
		.def("all_finished", &SelfPlayManager::AllFinished)
		.def("get_current_game_states", &SelfPlayManager::GetCurrentGameStates)
		.def("get_current_action_distributions", &SelfPlayManager::GetCurrentActionDistributions);
}



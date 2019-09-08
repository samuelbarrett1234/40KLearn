#include "Test.h"
#include <UCB1PolicyStrategy.h>
using namespace c40kl;


//A space marine with an AP-1 bolter.
static const Unit unitWithGun{
	"", 1, 6, 3, 3,
	4, 1, 1, 1, 8,
	3, 7, 24, 4, -1,
	1, 1, 4, 0, 1, 0,
	true, false, false,
	false, false, false,
	false, false
};


BOOST_AUTO_TEST_SUITE(UCB1PolicyStrategyTests);


BOOST_AUTO_TEST_CASE(TestUCB1SelectsFromPriorWithNoVisits)
{
	UCB1PolicyStrategy policy(1.4f, 0);

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 1), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 0), unitWithGun, 1);
	GameState gs(0, 0, Phase::FIGHT, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	std::vector<float> prior = { 1.0f, 0.0f }; //Prior says always select action 0
	pRoot->Expand(prior);

	const size_t ucb1Decision = policy.ActionArgMax(*pRoot);

	//The decision should be the first one, and the UCB1 selection
	// distribution should be equal to the prior in this case (since
	// the prior is deterministic.)
	BOOST_TEST(ucb1Decision == 0);
	BOOST_TEST((policy.GetActionDistribution(*pRoot) == prior));
}


BOOST_AUTO_TEST_CASE(TestUCB1SelectsWorstActionForOtherTeam)
{
	UCB1PolicyStrategy policy(1.4f, 0);

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 1), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 0), unitWithGun, 1);
	GameState gs(1, 1, Phase::FIGHT, b); //Team 1 acting!

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	std::vector<float> prior = { 0.5f, 0.5f }; //Uniform prior
	pRoot->Expand(prior);

	auto action0Results = pRoot->GetStateResults(0);
	auto action1Results = pRoot->GetStateResults(1);

	//Heavily suggest that action 0 is better for team 0 than action 1:
	for (auto pNode : action0Results)
	{
		pNode->AddValueStatistic(1.0f);
	}
	for (auto pNode : action1Results)
	{
		pNode->AddValueStatistic(-1.0f);
	}

	const size_t ucb1Decision = policy.ActionArgMax(*pRoot);

	//The action should be the second one, because it is WORST for
	// team 0, which is what the values are with respect to.
	BOOST_TEST(ucb1Decision == 1);
}


BOOST_AUTO_TEST_SUITE_END();



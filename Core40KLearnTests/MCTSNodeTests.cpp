#include "Test.h"
#include <MCTSNode.h>


//A space marine squad of 5.
static const Unit unitWithGun{
	"", 5, 6, 3, 3,
	4, 1, 5, 1, 8,
	3, 7, 24, 4, -1,
	1, 1, 4, 0, 1, 0,
	true, false, false,
	false, false, false,
	false, false
};


//TODO: make this depend on all of the tests for the
// game implementation.
BOOST_AUTO_TEST_SUITE(MCTSNodeTests);


BOOST_AUTO_TEST_CASE(AddLeafStatisticTest, *boost::unit_test::tolerance(1.0e-4f))
{
	//Construct a tree
	//Add a value to one of the end nodes and
	// see that it backpropagates properly but
	// does not update nodes that it shouldn't.
	//Do not use a weighted tree - this is tested
	// later on. E.g. use movement commands (which
	// are deterministic).

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::MOVEMENT, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	BOOST_TEST(pRoot->IsLeaf());
	BOOST_TEST(pRoot->IsRoot());
	BOOST_TEST(!pRoot->IsTerminal());

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	//Expand the root to give it some children:
	pRoot->Expand(prior);

	BOOST_TEST(!pRoot->IsLeaf());

	BOOST_TEST(pRoot->GetNumValueSamples() == 0);

	//Now add statistics to the children and hope
	// that pRoot->GetValueEstimate() is updated correctly:

	float valSum = 0.0f;
	size_t numVals = 0;

	for (size_t i = 0; i < pRoot->GetNumActions(); i++)
	{
		//Movement phase is deterministic:
		BOOST_REQUIRE(pRoot->GetNumResultingStates(i) == 1);

		//Choose some value arbitrarily:
		const float val = (i % 2 == 0) ? 1.0f : (-1.0f);

		auto actionResults = pRoot->GetStateResults(i);

		BOOST_REQUIRE(actionResults.size() == 1);

		auto pChild = actionResults.front();

		pChild->AddValueStatistic(val);

		valSum += val;
		numVals++;
	}

	//Now test that the average at the root is correct:
	BOOST_TEST(pRoot->GetValueEstimate() == valSum / (float)numVals);
}


BOOST_AUTO_TEST_CASE(CorrectActionsTest)
{
	//Test that a state node, even before it is
	// expanded, has the correct GetActions() and
	// the correct GetNumActions().

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::SHOOTING, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	BOOST_TEST(pRoot->GetNumActions() == 2);

	auto cmds = gs.GetCommands();
	BOOST_REQUIRE(cmds.size() == 2);

	auto nodeCmds = pRoot->GetActions();
	BOOST_REQUIRE(nodeCmds.size() == 2);

	//The order should be the same really, so
	// I think we can rely on this behaviour.

	BOOST_TEST(nodeCmds.front()->Equals(*cmds.front()));
	BOOST_TEST(nodeCmds.back()->Equals(*cmds.back()));
}


BOOST_AUTO_TEST_CASE(DetachTest)
{
	//Detach a node and then add a value statistic and
	// test that it does not update its old parent.
	//Also test if the correct nodes think they're roots.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::MOVEMENT, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	BOOST_TEST(pRoot->IsRoot());

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	//Expand the root to give it some children:
	pRoot->Expand(prior);

	//This operation should be valid if the node
	// has been handling the game state correctly:
	auto pChild = pRoot->GetStateResults(0).front();

	BOOST_TEST(!pChild->IsRoot());

	//Test these while we're here:
	BOOST_TEST(pChild->IsLeaf());
	BOOST_TEST(!pChild->IsTerminal());

	pChild->Detach();

	//Now detach, and they should both be roots:
	BOOST_TEST(pChild->IsRoot());
	BOOST_TEST(pRoot->IsRoot());

	//Furthermore, adding a statistic to pChild
	// should not update the value of pRoot:

	pChild->AddValueStatistic(1.0f);

	BOOST_TEST(pChild->GetValueEstimate() == 1.0f);
	BOOST_TEST(pChild->GetNumValueSamples() == 1);
	BOOST_TEST(pRoot->GetValueEstimate() == 0.0f);
	BOOST_TEST(pRoot->GetNumValueSamples() == 0);
}


BOOST_AUTO_TEST_CASE(WeightedAddValueStatistic)
{
	//Set up a tree which has a nondeterministic state
	// distribution, and add some statistics to those
	// weighted children and ensure the root node receives
	// a correct distribution.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::SHOOTING, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	//Expand the root to give it some children:
	pRoot->Expand(prior);

	//Now add statistics to the children and hope
	// that pRoot->GetValueEstimate() is updated correctly:

	float valSum = 0.0f, weightSum = 0.0f;

	for (size_t i = 0; i < pRoot->GetNumActions(); i++)
	{
		auto actionResults = pRoot->GetStateResults(i);
		auto stateWeights = pRoot->GetStateResultDistribution(i);
		BOOST_REQUIRE(actionResults.size() == pRoot->GetNumResultingStates(i));
		BOOST_REQUIRE(stateWeights.size() == pRoot->GetNumResultingStates(i));

		for (size_t j = 0; j < pRoot->GetNumResultingStates(i); j++)
		{
			//Choose some value arbitrarily:
			const float val = ((i + j) % 2 == 0) ? 1.0f : (-1.0f);

			//Get the weight
			const float weight = stateWeights[j];
			
			auto pChild = actionResults[j];

			pChild->AddValueStatistic(val);
			
			valSum += val * weight;
			weightSum += weight;
		}
	}

	//Now test that the average at the root is correct:
	const float rootValEstimate = pRoot->GetValueEstimate();
	BOOST_TEST(rootValEstimate == valSum / weightSum);
}


BOOST_AUTO_TEST_CASE(DeterministicExpansionTest)
{
	//Test that expansion of a leaf node with deterministic
	// actions works properly and produces the right children.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::MOVEMENT, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	BOOST_TEST(pRoot->IsLeaf());
	BOOST_TEST(pRoot->IsRoot());
	BOOST_TEST(!pRoot->IsTerminal());

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	//Expand the root to give it some children:
	pRoot->Expand(prior);

	BOOST_TEST(!pRoot->IsLeaf());

	//Now test the children:

	auto trueCmds = gs.GetCommands();
	auto nodeCmds = pRoot->GetActions();

	BOOST_REQUIRE(trueCmds.size() == nodeCmds.size());
	BOOST_TEST(pRoot->GetNumActions() == trueCmds.size());

	//It is safe to assume that trueCmds and nodeCmds are
	// in the same order:

	for (size_t i = 0; i < trueCmds.size(); i++)
	{
		BOOST_TEST(trueCmds[i]->Equals(*nodeCmds[i]));

		//Test the resulting states:
		auto children = pRoot->GetStateResults(i);

		//Should be deterministic:
		BOOST_REQUIRE(children.size() == 1);

		BOOST_TEST(children.front()->IsLeaf());

		//Get the actual state:
		std::vector<GameState> results;
		std::vector<float> probs;
		trueCmds[i]->Apply(gs, results, probs);

		//This shouldn't fail but just double check
		BOOST_REQUIRE(results.size() == 1);

		//Now test resulting state equality:
		BOOST_TEST((children.front()->GetState() == results.front()));
	}
}


BOOST_AUTO_TEST_CASE(NondeterministicExpansionTest, *boost::unit_test::tolerance(1.0e-4f))
{
	//Test that a state node, when expanded and which
	// contains actions with nondeterministic effects,
	// creates the correct child nodes and weights them
	// appropriately.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::SHOOTING, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	BOOST_TEST(pRoot->IsLeaf());
	BOOST_TEST(pRoot->IsRoot());
	BOOST_TEST(!pRoot->IsTerminal());

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	//Expand the root to give it some children:
	pRoot->Expand(prior);

	BOOST_TEST(!pRoot->IsLeaf());

	//Now test the children:

	auto trueCmds = gs.GetCommands();
	auto nodeCmds = pRoot->GetActions();

	BOOST_REQUIRE(trueCmds.size() == nodeCmds.size());
	BOOST_TEST(pRoot->GetNumActions() == trueCmds.size());

	//It is safe to assume that trueCmds and nodeCmds are
	// in the same order:

	for (size_t i = 0; i < trueCmds.size(); i++)
	{
		BOOST_TEST(trueCmds[i]->Equals(*nodeCmds[i]));

		//Test the resulting states:
		const auto children = pRoot->GetStateResults(i);
		const auto weights = pRoot->GetStateResultDistribution(i);

		//Get the actual state:
		std::vector<GameState> results;
		std::vector<float> probs;
		trueCmds[i]->Apply(gs, results, probs);

		BOOST_REQUIRE(results.size() == children.size());
		BOOST_REQUIRE(probs.size() == results.size());
		BOOST_REQUIRE(probs.size() == weights.size());

		//It is safe to assume the states are in the correct
		// order:
		for (size_t j = 0; j < children.size(); j++)
		{
			BOOST_TEST(children[j]->IsLeaf());

			//Now test resulting state equality:
			BOOST_TEST((children[j]->GetState() == results[j]));

			//And test probabilities (automatic floating point
			// equality tolerance):
			BOOST_TEST(probs[j] == weights[j]);
		}
	}
}


BOOST_AUTO_TEST_CASE(ActionValueEstimateTest, *boost::unit_test::tolerance(1.0e-4f))
{
	//Test that the action value estimates correctly aggregate
	// over the resulting states, and ONLY those who have been
	// estimated before (i.e. if at least one resulting state
	// has been estimated, we only account for estimated states).
	// Furthermore, note that if no state has been estimated,
	// the action value defaults to zero, but we have a test for
	// that below.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::SHOOTING, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	BOOST_TEST(pRoot->IsLeaf());
	BOOST_TEST(pRoot->IsRoot());
	BOOST_TEST(!pRoot->IsTerminal());

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	//Expand the root to give it some children:
	pRoot->Expand(prior);

	BOOST_TEST(!pRoot->IsLeaf());

	auto cmds = pRoot->GetActions();

	std::vector<float> valueEstimates(cmds.size(), 0.0f),
		weightSums(cmds.size(), 0.0f);

	for (size_t i = 0; i < cmds.size(); i++)
	{
		//Test the resulting states:
		const auto children = pRoot->GetStateResults(i);
		const auto weights = pRoot->GetStateResultDistribution(i);

		BOOST_REQUIRE(children.size() == weights.size());

		for (size_t j = 0; j < children.size(); j++)
		{
			//Arbitrary: don't sample the j=0th state!
			//This is just to ensure that the action value
			// estimate doesn't aggregate over states without
			// an estimate.
			if (j > 0)
			{
				//Arbitrary value
				const float val = (j % 2 == 0) ? 1.0f : (-1.0f);

				//Add it to the child node and our tracked estimate:
				children[j]->AddValueStatistic(val);
				valueEstimates[i] += val * weights[j];
				weightSums[i] += weights[j];
			}
		}
	}

	//Check equality:
	const auto actionValueEstimates = pRoot->GetActionValueEstimates();
	BOOST_REQUIRE(cmds.size() == actionValueEstimates.size());
	for (size_t i = 0; i < cmds.size(); i++)
	{
		if (weightSums[i] > 0)
		{
			BOOST_TEST(valueEstimates[i] / weightSums[i] == actionValueEstimates[i]);
		}
		else
		{
			BOOST_TEST(actionValueEstimates[i] == 0.0f);
		}
	}
}


BOOST_AUTO_TEST_CASE(TerminalExpansionThrowTest)
{
	//Test that calling Expand() on a terminal node throws
	// an error.

	GameState gs(0, 0, Phase::MOVEMENT, BoardState(25, 1.0f));

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	BOOST_TEST(pRoot->IsTerminal());

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	C40KL_CHECK_PRE_POST_EXCEPTION(pRoot->Expand(prior), std::runtime_error);
}


BOOST_AUTO_TEST_CASE(NonLeafExpansionThrowTest)
{
	//Test that calling Expand() on a non-leaf node throws
	// an error - easiest way to do this is to construct a
	// non-terminal node and call Expand() twice, expecting
	// the second call to throw.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::SHOOTING, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	pRoot->Expand(prior);
	BOOST_TEST(!pRoot->IsLeaf());

	//Calling expand for a second time should throw:
	C40KL_CHECK_PRE_POST_EXCEPTION(pRoot->Expand(prior), std::runtime_error);
}


BOOST_AUTO_TEST_CASE(PriorTest, *boost::unit_test::tolerance(1.0e-4f))
{
	//Test that the prior provided in Expand() is correctly
	// stored.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::MOVEMENT, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	BOOST_TEST(pRoot->IsLeaf());
	BOOST_TEST(pRoot->IsRoot());
	BOOST_TEST(!pRoot->IsTerminal());

	//Construct an uneven prior:
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior(numActions);
	const float sum = 0.5f * static_cast<float>(numActions * (numActions + 1));
	for (size_t i = 0; i < numActions; i++)
	{
		prior[i] = static_cast<float>(i + 1) / sum;
	}

	//Expand the root to give it some children:
	pRoot->Expand(prior);

	//Now test the prior:
	const auto testPrior = pRoot->GetActionPriorDistribution();

	//Check the priors are equal:
	// (note that, here, they must be the same order)
	BOOST_REQUIRE(testPrior.size() == prior.size());
	for (size_t i = 0; i < prior.size(); i++)
	{
		BOOST_TEST(testPrior[i] == prior[i]);
	}
}


BOOST_AUTO_TEST_CASE(ActionVisitCountsTest)
{
	//Set up a tree with nondeterministic states, and
	// add some statistics for the child states, and
	// then test that the GetActionVisitCounts() correctly
	// aggregates these counts.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::SHOOTING, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	//Expand the root to give it some children:
	pRoot->Expand(prior);

	//Now add statistics to the children and hope
	// that the action visit counts are updated appropriately

	//Here is where we will store the number of visit counts
	// we ACTUALLY did:
	std::vector<size_t> visitCounts(pRoot->GetNumActions(), 0);

	for (size_t i = 0; i < pRoot->GetNumActions(); i++)
	{
		auto actionResults = pRoot->GetStateResults(i);
		auto stateWeights = pRoot->GetStateResultDistribution(i);
		BOOST_REQUIRE(actionResults.size() == pRoot->GetNumResultingStates(i));
		BOOST_REQUIRE(stateWeights.size() == pRoot->GetNumResultingStates(i));

		for (size_t j = 0; j < pRoot->GetNumResultingStates(i); j++)
		{
			//Add some statistic randomly:
			auto pChild = actionResults[j];

			//Repeat this some arbitrary number of times:
			for (size_t k = 0; k < 1 + (i + 2 * j) % 3; k++)
			{
				pChild->AddValueStatistic(1.0f);
				visitCounts[i]++;
			}
		}
	}

	//Now check the results were correct:
	auto testCounts = pRoot->GetActionVisitCounts();
	for (size_t i = 0; i < pRoot->GetNumActions(); i++)
	{
		BOOST_TEST(visitCounts[i] == testCounts[i]);
	}
}


BOOST_AUTO_TEST_CASE(DepthTest)
{
	//Set up a small tree and test that the depth values
	// for the nodes are correct.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::SHOOTING, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	//Expand the root to give it some children:
	pRoot->Expand(prior);

	//Now get to testing the depths:
	BOOST_TEST(pRoot->GetDepth() == 0);
	for (size_t i = 0; i < pRoot->GetNumActions(); i++)
	{
		auto children = pRoot->GetStateResults(i);
		for (auto pChildNode : children)
		{
			BOOST_TEST(pChildNode->GetDepth() == 1);
		}
	}
}


BOOST_AUTO_TEST_CASE(LeafThrowTest)
{
	//Test that the following functions throw if called
	// on a leaf node:
	// GetActionPriorDistribution()
	// GetActionVisitCounts()
	// GetActionValueEstimates()
	// GetStateResultDistribution()
	// GetNumResultingStates()
	// GetStateResults()

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::SHOOTING, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	//This is a leaf node
	BOOST_REQUIRE(pRoot->IsLeaf());

	//All of these functions make no sense for a leaf
	// node, hence should all throw exceptions:
	C40KL_CHECK_PRE_POST_EXCEPTION(pRoot->GetActionPriorDistribution(), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(pRoot->GetActionVisitCounts(), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(pRoot->GetActionValueEstimates(), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(pRoot->GetStateResultDistribution(0), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(pRoot->GetNumResultingStates(0), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(pRoot->GetStateResults(0), std::runtime_error);

	//However, these are fine:
	pRoot->GetNumActions();
	pRoot->GetActions();
}


BOOST_AUTO_TEST_CASE(TerminalIsLeafTest)
{
	//Test that a terminal node is also classed as a
	// leaf node.

	BoardState b(25, 1.0f);
	GameState gs(0, 0, Phase::SHOOTING, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	//This is a terminal node:
	BOOST_REQUIRE(pRoot->IsTerminal());

	//This should be a leaf node
	BOOST_TEST(pRoot->IsLeaf());
}


BOOST_AUTO_TEST_CASE(WrongSizedPriorThrowTest)
{
	//Test that if Expand() is called with a prior
	// which is the wrong size, then an exception
	// is thrown.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::MOVEMENT, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	BOOST_TEST(pRoot->IsLeaf());
	BOOST_TEST(pRoot->IsRoot());
	BOOST_TEST(!pRoot->IsTerminal());

	//Check that we cannot obtain the prior as a leaf node:
	C40KL_CHECK_PRE_POST_EXCEPTION(pRoot->GetActionPriorDistribution(), std::runtime_error);

	//This should be true
	BOOST_REQUIRE(pRoot->GetNumActions() > 1);

	//Construct uniform prior OVER THE WRONG DISTRIBUTION
	// OF ACTIONS! (Notice the -1).
	size_t numActions = pRoot->GetNumActions() - 1;
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	//We have a prior which is one size too small, so should throw:
	C40KL_CHECK_PRE_POST_EXCEPTION(pRoot->Expand(prior), std::runtime_error);

	//Should've failed:
	BOOST_REQUIRE(pRoot->IsLeaf());

	//Now make it too big, to ensure it still throws:

	//Construct uniform prior OVER THE WRONG DISTRIBUTION
	// OF ACTIONS! (Notice the +1).
	numActions = pRoot->GetNumActions() + 1;
	prior.clear();
	prior.resize(numActions, 1.0f / (float)numActions);

	//We have a prior which is one size too big, so should throw:
	C40KL_CHECK_PRE_POST_EXCEPTION(pRoot->Expand(prior), std::runtime_error);
}


BOOST_AUTO_TEST_CASE(UnvisitedStateValueEstimateTest, *boost::unit_test::tolerance(1.0e-4f))
{
	//Test that a state defaults its value estimate
	// to zero before it receives any statistics.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::MOVEMENT, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	//Prior value estimates should always be zero:
	BOOST_TEST(pRoot->GetValueEstimate() == 0.0f);
	BOOST_TEST(pRoot->GetNumValueSamples() == 0);
}


BOOST_AUTO_TEST_CASE(UnvisitedActionValueEstimateTest)
{
	//Test that if an action has not been tried
	// then its value estimate defaults to zero.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::MOVEMENT, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	BOOST_TEST(pRoot->IsLeaf());
	BOOST_TEST(pRoot->IsRoot());
	BOOST_TEST(!pRoot->IsTerminal());

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	//Expand the root to give it some children:
	pRoot->Expand(prior);

	BOOST_TEST(!pRoot->IsLeaf());

	BOOST_TEST(pRoot->GetNumValueSamples() == 0);

	//Now test that the action values are all zero
	// because we haven't visited any of them!

	std::vector<float> zeroVec(pRoot->GetNumActions(), 0.0f);
	BOOST_TEST((zeroVec == pRoot->GetActionValueEstimates()));
}


BOOST_AUTO_TEST_CASE(ChildOrderingTest)
{
	//Test that the MCTS node children appear in the same order
	// as the state results from their corresponding action.

	//First set up a tree with a root and children:

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 2), unitWithGun, 1);
	GameState gs(0, 0, Phase::SHOOTING, b);

	MCTSNodePtr pRoot = MCTSNode::CreateRootNode(gs);

	//Construct uniform prior
	const size_t numActions = pRoot->GetNumActions();
	std::vector<float> prior;
	prior.resize(numActions, 1.0f / (float)numActions);

	//Expand the root to give it some children:
	pRoot->Expand(prior);

	auto commands = gs.GetCommands();

	//Now test that we have the same number of actions,
	// and that each action has the same number of resulting
	// states, and that the resulting states are in the same
	// order for action's Apply() and MCTS node children.
	BOOST_REQUIRE(commands.size() == pRoot->GetNumActions());
	for (size_t i = 0; i < commands.size(); i++)
	{
		auto nodeResults = pRoot->GetStateResults(i);

		std::vector<GameState> actionResults;
		std::vector<float> actionProbs;
		commands[i]->Apply(gs, actionResults, actionProbs);

		BOOST_REQUIRE(actionResults.size() == nodeResults.size());

		for (size_t j = 0; j < actionResults.size(); j++)
		{
			BOOST_TEST((actionResults[j] == nodeResults[j]->GetState()));
		}
	}
}


BOOST_AUTO_TEST_SUITE_END();



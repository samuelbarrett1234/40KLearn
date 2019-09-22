#include "Test.h"
#include <SelfPlayManager.h>
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


BOOST_AUTO_TEST_SUITE(SelfPlayManagerTests, *boost::unit_test::depends_on("MCTSNodeTests"));


BOOST_AUTO_TEST_CASE(BasicUsageTest)
{
	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 1), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 0), unitWithGun, 1);
	GameState gs(0, 0, Phase::FIGHT, b);
	
	SelfPlayManager mgr(1.4f, 1);

	//Test that we count as finished when there are no games running:
	BOOST_TEST(mgr.AllFinished());

	mgr.Reset(2, gs);

	//Test all flags are correct:
	BOOST_TEST(!mgr.AllFinished());
	BOOST_TEST(!mgr.IsWaiting());
	BOOST_TEST(!mgr.ReadyToCommit());

	//Select() should just return the starting game state
	// because the trees are currently just the root:

	std::vector<GameState> selectedStates;
	mgr.Select(selectedStates);

	BOOST_REQUIRE(selectedStates.size() == 2);
	BOOST_TEST((selectedStates.front() == gs
		&& selectedStates.back() == gs));

	//Test all flags are correct:
	BOOST_TEST(!mgr.AllFinished());
	BOOST_TEST(mgr.IsWaiting());
	BOOST_TEST(!mgr.ReadyToCommit());

	//Now provide priors and value estimates
	//We provide the same value for the root nodes, but we give different
	// prior policies, so the manager should select different actions to
	// explore for each game.
	std::vector<float> valueEstimates = { 0.0f, 0.0f };
	std::vector<std::vector<float>> priorPolicies = { { 1.0f, 0.0f }, { 0.0f, 1.0f } };
	mgr.Update(valueEstimates, priorPolicies);

	//Test all flags are correct:
	BOOST_TEST(!mgr.AllFinished());
	BOOST_TEST(!mgr.IsWaiting());
	BOOST_TEST(mgr.ReadyToCommit());

	//Make a decision:
	mgr.Commit();

	//Test flags are correct:
	BOOST_TEST(!mgr.IsWaiting());
	BOOST_TEST(!mgr.ReadyToCommit());
	//We actually can't determine if we are all finished or not!

	//Check the resulting states are correct by checking that the correct agent fought:
	auto curStates = mgr.GetCurrentGameStates();

	BOOST_REQUIRE(curStates.size() == 2);

	//In the first state, the first action should've been taken, and
	// in the second state, the second action should've been taken.
	// We know that both actions are Fight commands, so we will assert
	// the correct choice by checking the "source position" of the commands:
	std::vector<Position> sourcePositions = {
		dynamic_cast<IUnitOrderCommand*>(gs.GetCommands().front().get())->GetSourcePosition(),
		dynamic_cast<IUnitOrderCommand*>(gs.GetCommands().back().get())->GetSourcePosition()
	};
	BOOST_TEST(curStates.front().GetBoardState().GetUnitOnSquare(sourcePositions.front()).foughtThisTurn);
	BOOST_TEST(curStates.back().GetBoardState().GetUnitOnSquare(sourcePositions.back()).foughtThisTurn);
}


BOOST_AUTO_TEST_CASE(TestSelfPlayManagerThrowsWhenResetWithFinishedState)
{
	//A finished game state:
	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	GameState gs(0, 0, Phase::FIGHT, b);

	SelfPlayManager mgr(1.4f, 1);

	BOOST_CHECK_THROW(mgr.Reset(5, gs), std::runtime_error);
}


BOOST_AUTO_TEST_SUITE_END();



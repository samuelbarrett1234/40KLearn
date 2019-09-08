#include "Test.h"
#include <UniformRandomEstimator.h>
using namespace c40kl;


BOOST_AUTO_TEST_SUITE(UniformRandomEstimatorTests);


BOOST_AUTO_TEST_CASE(TestEstimatorReturnsCorrectValueWithFinishedState)
{
	UniformRandomEstimator est;

	BoardState b0(25, 1.0f), b1(25, 1.0f);

	b0.SetUnitOnSquare(Position(0, 0), Unit(), 0);
	b1.SetUnitOnSquare(Position(0, 0), Unit(), 1);

	GameState gs0(0, 0, Phase::MOVEMENT, b0);
	GameState gs1(0, 0, Phase::MOVEMENT, b1);

	//Check state where team 0 wins:

	BOOST_TEST(est.ComputeValueEstimate(gs0, 0, 100) == 1.0f);
	BOOST_TEST(est.ComputeValueEstimate(gs0, 1, 100) == -1.0f);

	//Check state where team 1 wins:

	BOOST_TEST(est.ComputeValueEstimate(gs1, 0, 100) == -1.0f);
	BOOST_TEST(est.ComputeValueEstimate(gs1, 1, 100) == 1.0f);

	//Also check draw:

	BoardState emptyb(25, 1.0f);
	GameState emptygs(0, 0, Phase::MOVEMENT, emptyb);
	BOOST_TEST(est.ComputeValueEstimate(emptygs, 0, 100) == 0.0f);
	BOOST_TEST(est.ComputeValueEstimate(emptygs, 1, 100) == 0.0f);
}


BOOST_AUTO_TEST_SUITE_END();



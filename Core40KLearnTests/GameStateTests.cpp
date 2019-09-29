#include "Test.h"


BOOST_AUTO_TEST_SUITE(GameStateTests, *boost::unit_test::depends_on("BoardTests"));


//Test the special logic tracking internal teams and acting teams
BOOST_AUTO_TEST_CASE(TeamTest)
{
	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), Unit(), 0);
	b.SetUnitOnSquare(Position(0, 1), Unit(), 1);

	//Internal and active teams must be the same for all but the fight phase
	C40KL_CHECK_PRE_POST_EXCEPTION(GameState(0, 1, Phase::MOVEMENT, b), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(GameState(0, 1, Phase::SHOOTING, b), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(GameState(0, 1, Phase::CHARGE, b), std::runtime_error);

	GameState s(0, 0, Phase::MOVEMENT, b);
	BOOST_TEST(s.GetInternalTeam() == 0);
	BOOST_TEST(s.GetActingTeam() == 0);
	BOOST_TEST((s.GetPhase() == Phase::MOVEMENT));

	s = GameState(0, 1, Phase::FIGHT, b);
	BOOST_TEST(s.GetInternalTeam() == 0);
	BOOST_TEST(s.GetActingTeam() == 1);
	BOOST_TEST((s.GetPhase() == Phase::FIGHT));
}


BOOST_AUTO_TEST_CASE(GameCompletionTests)
{
	Unit u;
	BoardState bEmpty(25, 1.0f),
		bOneSide(25, 1.0f),
		bBothSides(25, 1.0f);

	bOneSide.SetUnitOnSquare(Position(0, 0), u, 0);
	bBothSides.SetUnitOnSquare(Position(0, 0), u, 0);
	bBothSides.SetUnitOnSquare(Position(0, 1), u, 1);

	GameState sDraw(0, 0, Phase::MOVEMENT, bEmpty);
	GameState sTeam0Win(0, 0, Phase::MOVEMENT, bOneSide);
	GameState sUnfinished(0, 0, Phase::MOVEMENT, bBothSides);

	BOOST_TEST(sDraw.IsFinished());
	BOOST_TEST(sDraw.GetGameValue(0) == 0);
	BOOST_TEST(sDraw.GetGameValue(1) == 0);

	BOOST_TEST(sTeam0Win.IsFinished());
	BOOST_TEST(sTeam0Win.GetGameValue(0) == 1);
	BOOST_TEST(sTeam0Win.GetGameValue(1) == -1);

	BOOST_TEST(!sUnfinished.IsFinished());
	C40KL_CHECK_PRE_POST_EXCEPTION(sUnfinished.GetGameValue(0), std::runtime_error);

	//Can't get team/phase values when the game is finishec
	C40KL_CHECK_PRE_POST_EXCEPTION(sDraw.GetInternalTeam(), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(sDraw.GetActingTeam(), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(sDraw.GetPhase(), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(sTeam0Win.GetInternalTeam(), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(sTeam0Win.GetActingTeam(), std::runtime_error);
	C40KL_CHECK_PRE_POST_EXCEPTION(sTeam0Win.GetPhase(), std::runtime_error);
}


BOOST_AUTO_TEST_SUITE_END();



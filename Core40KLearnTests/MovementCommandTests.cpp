#include "Test.h"
#include <set>


BOOST_AUTO_TEST_SUITE(MovementCommandTests, *boost::unit_test::depends_on("GameStateTests"));


//Test that it registers the right commands
BOOST_AUTO_TEST_CASE(PossibleCommandsTest)
{
	//Set up unit, board, and game state:
	Unit u;
	u.movement = 1;
	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(1, 1), u, 0);
	b.SetUnitOnSquare(Position(3, 4), u, 1);
	GameState s(0, 0, Phase::MOVEMENT, b);

	//Check that the right movement commands are available:
	auto cmds = s.GetCommands();

	//Extract the movement command positions:
	PositionArray targets;

	for (auto pCmd : cmds)
	{
		if (auto pMvmtCmd = dynamic_cast<const IUnitOrderCommand*>(pCmd.get()))
		{
			BOOST_TEST((pMvmtCmd->GetSourcePosition() == Position(1, 1)));
			targets.push_back(pMvmtCmd->GetTargetPosition());
		}
	}

	auto correct = b.GetSquaresInRange(Position(1, 1), 1);

	//The aim is for correct=target
	//However GetSquaresInRange also returns the
	// centre square, whereas target will not (and
	// should not), but to check equality we will
	// add (1,1) to target, which should make the
	// arrays equal:
	targets.push_back(Position(1, 1));

	std::sort(targets.begin(), targets.end());
	std::sort(correct.begin(), correct.end());

	BOOST_TEST((correct == targets));
}


BOOST_AUTO_TEST_CASE(MovementCommandIssueTest)
{
	//Set up unit, board, and game state:
	Unit u;
	u.movement = 1;
	BoardState b(25, 1.0f);

	//Set up so that there is only one possible square to move to:
	b.SetUnitOnSquare(Position(0, 1), u, 0);
	b.SetUnitOnSquare(Position(1, 1), u, 0); 
	b.SetUnitOnSquare(Position(0, 2), u, 0);
	b.SetUnitOnSquare(Position(3, 2), u, 1);

	GameState s(0, 0, Phase::MOVEMENT, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 1), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	//Get movement command:
	GameCommandPtr pMvmtCmd = cmds.front();

	//Test application of movement:
	std::vector<GameState> resultStates;
	std::vector<float> resultProbabilities;

	pMvmtCmd->Apply(s, resultStates, resultProbabilities);

	BOOST_REQUIRE(resultStates.size() == 1);
	BOOST_REQUIRE(resultProbabilities.size() == 1);

	BOOST_TEST(resultProbabilities.front() == 1.0f); //Deterministic

	auto resultBoard = resultStates.front().GetBoardState();

	//The unit can only possibly move to (0,0)
	BOOST_TEST(!resultBoard.IsOccupied(Position(0, 1)));
	BOOST_TEST(resultBoard.IsOccupied(Position(0, 0)));
	BOOST_TEST(resultBoard.GetTeamOnSquare(Position(0, 0)) == 0);
	BOOST_TEST(resultBoard.GetUnitOnSquare(Position(0, 0)).movedThisTurn);

	//It wasn't in combat to begin with, so this shouldn't be set:
	BOOST_TEST(!resultBoard.GetUnitOnSquare(Position(0, 0)).movedOutOfCombatThisTurn);
}


BOOST_AUTO_TEST_CASE(MovingOutOfMeleeTest)
{
	//Set up unit, board, and game state:
	Unit u;
	u.movement = 1;
	BoardState b(25, 1.0f);

	//Set up so that there is only one possible square to move to:
	b.SetUnitOnSquare(Position(0, 1), u, 0);
	b.SetUnitOnSquare(Position(1, 1), u, 0);
	b.SetUnitOnSquare(Position(0, 2), u, 1);

	GameState s(0, 0, Phase::MOVEMENT, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 1), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	//Get movement command:
	GameCommandPtr pMvmtCmd = cmds.front();

	//Test application of movement:
	std::vector<GameState> resultStates;
	std::vector<float> resultProbabilities;

	pMvmtCmd->Apply(s, resultStates, resultProbabilities);

	BOOST_REQUIRE(resultStates.size() == 1);
	BOOST_REQUIRE(resultProbabilities.size() == 1);

	auto resultBoard = resultStates.front().GetBoardState();

	//The unit can only possibly move to (0,0)
	BOOST_TEST(!resultBoard.IsOccupied(Position(0, 1)));
	BOOST_TEST(resultBoard.IsOccupied(Position(0, 0)));
	BOOST_TEST(resultBoard.GetUnitOnSquare(Position(0, 0)).movedOutOfCombatThisTurn);
}


BOOST_AUTO_TEST_CASE(NotMovingIntoCombatOrEnemiesTest)
{

	//Set up unit, board, and game state:
	Unit u;
	u.movement = 1;
	BoardState b(25, 1.0f);

	//Set up so that there is only one possible square to move to:
	b.SetUnitOnSquare(Position(0, 0), u, 0);
	b.SetUnitOnSquare(Position(0, 2), u, 1);
	b.SetUnitOnSquare(Position(1, 1), u, 1);
	b.SetUnitOnSquare(Position(2, 0), u, 1);

	GameState s(0, 0, Phase::MOVEMENT, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_TEST(cmds.empty());
}


BOOST_AUTO_TEST_SUITE_END();



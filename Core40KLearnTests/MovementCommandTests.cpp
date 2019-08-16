#include "Test.h"


BOOST_AUTO_TEST_SUITE(MovementCommandTests, *boost::unit_test::depends_on("GameStateTests"));


//Test that it registers the right commands
BOOST_AUTO_TEST_CASE(PossibleCommandsTest)
{
	//Set up unit, board, and game state:
	Unit u;
	u.movement = 1;
	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(1, 1), u, 0);
	GameState s(0, 0, Phase::MOVEMENT, b);

	//Check that the right movement commands are available:
	auto cmds = s.GetCommands();

	//Extract the movement command positions:
	PositionArray sources, targets;

	for (auto pCmd : cmds)
	{
		if (auto pMvmtCmd = dynamic_cast<const IUnitOrderCommand*>(pCmd.get()))
		{
			sources.push_back(pMvmtCmd->GetSourcePosition());
			targets.push_back(pMvmtCmd->GetTargetPosition());
		}
	}

	BOOST_REQUIRE(sources.size() == 1);
	BOOST_TEST((sources[0] == Position(1, 1)));

	auto correct = b.GetSquaresInRange(Position(1, 1), 1);
	std::sort(targets.begin(), targets.end());
	std::sort(correct.begin(), correct.end());

	BOOST_TEST((correct == targets));
}


BOOST_AUTO_TEST_SUITE_END();



#include "Test.h"


BOOST_AUTO_TEST_SUITE(ChargeCommandTests, *boost::unit_test::depends_on("GameStateTests"));


//A space marine with an AP-1 bolter.
static Unit unitWithGun{
	"", 5, 6, 3, 3,
	4, 1, 5, 1, 8,
	3, 7, 24, 4, -1,
	1, 1, 4, 0, 1, 0,
	true, false, false,
	false, false, false,
	false, false
};


BOOST_AUTO_TEST_CASE(DistributionTest)
{
	//Test that a basic charge provides the correct pass/fail distribution (no overwatch)
	//Also test that each pass/fail leaves the charging unit in the right place

	BoardState b(25, 1.0f);

	Unit unitWithoutGun = unitWithGun;
	unitWithoutGun.rg_range = 0; //Set range to 0 to disable gun

	b.SetUnitOnSquare(Position(0, 0), unitWithoutGun, 0);
	b.SetUnitOnSquare(Position(0, 4), unitWithoutGun, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	//Since we are at an edge, and quite close to the
	// enemy, there should be 5 possible charging locations
	BOOST_TEST(cmds.size() == 5);

	//Determine the closest charging position, which is (0,3)
	GameCommandPtr pClosest;
	for (auto pCmd : cmds)
	{
		if (auto pOrder = dynamic_cast<IUnitOrderCommand*>(pCmd.get()))
		{
			if (pOrder->GetTargetPosition() == Position(0, 3))
			{
				pClosest = pCmd;
			}
		}
	}

	BOOST_REQUIRE(pClosest.get() != nullptr);

	std::vector<GameState> results;
	std::vector<float> probs;
	pClosest->Apply(gs, results, probs);

	//No overwatch, so only results are pass and fail
	BOOST_REQUIRE(results.size() == 2);
	BOOST_REQUIRE(probs.size() == results.size());

	//Check probabilities: the only way to fail is to
	// get a double 1 on the charge roll, hence probabilities
	// should be 1/36 for fail and 35/36 for pass.

	BOOST_REQUIRE((std::abs(1.0f / 36.0f - probs[0]) < 1.0e-6f
		|| std::abs(1.0f / 36.0f - probs[1]) < 1.0e-6f));

	//Determine which states we passed and which we failed
	// in based on the probabilities:

	GameState *pPass = nullptr, *pFail = nullptr;
	if (std::abs(1.0f / 36.0f - probs[0]) < 1.0e-6f)
	{
		pFail = &results[0];
		pPass = &results[1];
	}
	else
	{
		pFail = &results[1];
		pPass = &results[0];
	}

	//Now test that those states are correct:

	BOOST_TEST(pPass->GetBoardState().IsOccupied(Position(0, 3)));
	BOOST_TEST(!pPass->GetBoardState().IsOccupied(Position(0, 0)));
	BOOST_TEST(!pFail->GetBoardState().IsOccupied(Position(0, 3)));
	BOOST_TEST(pFail->GetBoardState().IsOccupied(Position(0, 0)));
}


BOOST_AUTO_TEST_CASE(OutOfChargeRangeTest)
{
	//Test that you can't charge further than 12 units of distance away

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 14), unitWithGun, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	//No charge possibilities
	BOOST_TEST(cmds.empty());
}


BOOST_AUTO_TEST_CASE(ChargeRangeLimitTest)
{
	//Test that you can charge exactly 12 units of distance away

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 13), unitWithGun, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	//Only one charge location
	BOOST_TEST(cmds.size() == 1);
}


BOOST_AUTO_TEST_CASE(OverwatchDistributionTest)
{
	//Test that overwatch provides a correct damage distribution (6+ to hit)

	BoardState b(25, 1.0f);

	//Simplify the unit so that there is only one shot
	//Also 2 wounds so we can't get destroyed by overwatch.
	Unit u = unitWithGun;
	u.rg_is_rapid = false;
	u.count = 1;
	u.w = 2;
	u.total_w = 2;
	u.rg_shots = 1;

	b.SetUnitOnSquare(Position(0, 0), u, 0);
	b.SetUnitOnSquare(Position(0, 13), u, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	//Four possibilities: we either succeed the
	// charge or not, and we either lose a wound
	// or we don't (no possibility of being
	// destroyed).

	BOOST_REQUIRE(cmds.size() == 1); //Only one charge location

	//Apply the only possible charge command:
	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//Check number of results:
	BOOST_REQUIRE(results.size() == 4);
	BOOST_REQUIRE(probs.size() == 4);

	//Sort the probabilities to make it easier
	// to check if they're right:
	auto probsSorted = probs;
	std::sort(probsSorted.begin(), probsSorted.end());

	float correctProbs[4] = {
		(1.0f / 6.0f) * 0.5f * 0.5f * (1.0f / 36.0f), //Take wound and make charge
		(1.0f / 6.0f) * 0.5f * 0.5f * (35.0f / 36.0f), //Take wound and fail charge
		(1.0f - (1.0f / 6.0f) * 0.5f * 0.5f) * (1.0f / 36.0f), //No wound and make charge
		(1.0f - (1.0f / 6.0f) * 0.5f * 0.5f) * (35.0f / 36.0f), //No wound and fail charge
	};
	std::sort(correctProbs, correctProbs + 4);

	//Test correct probabilities:
	BOOST_TEST(std::abs(correctProbs[0] - probsSorted[0]) < 1.0e-6f);
	BOOST_TEST(std::abs(correctProbs[1] - probsSorted[1]) < 1.0e-6f);
	BOOST_TEST(std::abs(correctProbs[2] - probsSorted[2]) < 1.0e-6f);
	BOOST_TEST(std::abs(correctProbs[3] - probsSorted[3]) < 1.0e-6f);

	//We may assume that charging is done correctly (tested earlier) and
	// we may assume that overwatch results are resolved correctly (shares
	// code with shooting commands).
}


BOOST_AUTO_TEST_CASE(OverwatchRangeTest)
{
	//Test that overwatch measures weapon range correctly (from charging
	// position, not from charging target.)

	BoardState b(25, 1.0f);

	Unit u = unitWithGun;
	u.rg_range = 6; //Short range

	b.SetUnitOnSquare(Position(0, 0), u, 0);
	b.SetUnitOnSquare(Position(0, 13), u, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	//Should be one charge possibility:
	BOOST_REQUIRE(cmds.size() == 1);

	//Apply the command:
	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//We should get exactly two results because the weapon
	// should be out of range:
	BOOST_TEST(results.size() == 2);
	BOOST_TEST(probs.size() == results.size());
}


BOOST_AUTO_TEST_CASE(ChargingUnitDestroyedTest)
{
	//Test that, if the charging unit is destroyed by overwatch, it is removed from the board
	//Make sure to test that it wasn't moved to the charge target.

	BoardState b(25, 1.0f);

	//Simplify the unit so that there is only one shot
	Unit u = unitWithGun;
	u.rg_is_rapid = false;
	u.count = 1;
	u.total_w = 1;
	u.rg_shots = 1;

	//Units are very close so we are guaranteed to make it into combat:
	b.SetUnitOnSquare(Position(0, 0), u, 0);
	b.SetUnitOnSquare(Position(0, 2), u, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);
	   
	//Determine the command for the charging position (0,1)
	GameCommandPtr pTargetCmd;
	for (auto pCmd : cmds)
	{
		if (auto pOrder = dynamic_cast<IUnitOrderCommand*>(pCmd.get()))
		{
			if (pOrder->GetTargetPosition() == Position(0, 1))
			{
				pTargetCmd = pCmd;
			}
		}
	}

	BOOST_REQUIRE(pTargetCmd.get() != nullptr);

	//Apply the command:
	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//Check number of results (only possibilities are overwatch
	// killed charging unit or missed completely):
	BOOST_REQUIRE(results.size() == 2);
	BOOST_REQUIRE(probs.size() == 2);

	//Whether or not the unit survived, the cell (0,0) should be empty:
	BOOST_TEST(!results[0].GetBoardState().IsOccupied(Position(0, 0)));
	BOOST_TEST(!results[1].GetBoardState().IsOccupied(Position(0, 0)));

	//Now, the target square (0, 1), should be occupied in exactly
	// one scenario:
	BOOST_TEST(results[0].GetBoardState().IsOccupied(Position(0, 1))
		!= results[1].GetBoardState().IsOccupied(Position(0, 1)));
}


BOOST_AUTO_TEST_CASE(NoChargingOutOfMeleeTest)
{
	//Test that we can't charge out of one combat and into another

	BoardState b(25, 1.0f);

	//(0,0) starts engaged with (1,0) so should not be able to
	// perform any charges, in particular cannot charge to a
	// cell adjacent to (0,3).
	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 3), unitWithGun, 1);
	b.SetUnitOnSquare(Position(1, 0), unitWithGun, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_TEST(cmds.empty());
}


BOOST_AUTO_TEST_CASE(NoChargingIfMovedOutOfMelee)
{
	//Test that, if we have moved out of combat this turn, we cannot charge

	BoardState b(25, 1.0f);

	Unit retreatedUnit = unitWithGun;
	retreatedUnit.movedOutOfCombatThisTurn = true;

	b.SetUnitOnSquare(Position(0, 0), retreatedUnit, 0);
	b.SetUnitOnSquare(Position(0, 3), unitWithGun, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	//Should be no possible charges for (0,0) because
	// the unit has just retreated from combat this turn.
	BOOST_TEST(cmds.empty());
}


BOOST_AUTO_TEST_CASE(MultipleOverwatchTest)
{
	//Test that, if we are charging to a position with multiple surrounding enemy
	// units, that we receive overwatch fire from all of them.

	BoardState b(25, 1.0f);

	//Simplify the unit so that there is only one shot
	//Also 4 wounds so we can't get destroyed by 3 rounds of overwatch
	Unit u = unitWithGun;
	u.rg_is_rapid = false;
	u.count = 1;
	u.w = 4;
	u.total_w = 4;
	u.rg_shots = 1;

	b.SetUnitOnSquare(Position(1, 0), u, 0);
	b.SetUnitOnSquare(Position(0, 13), u, 1);
	b.SetUnitOnSquare(Position(1, 13), u, 1);
	b.SetUnitOnSquare(Position(2, 13), u, 1);
	//Set up so only possible charge position is (1,11).

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(1, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//There should be three people firing on overwatch
	//Even if all cause a wound, the charging unit would not die,
	// so there should be 8 possibilities:
	// (charge pass / charge fail) x (0, 1, 2, or 3 wounds caused)

	BOOST_TEST(results.size() == 8);
	BOOST_TEST(probs.size() == results.size());
}


BOOST_AUTO_TEST_CASE(CloseChargeTest)
{
	//Test that if we are charging from 2 units of distance,
	// then we always make it into combat.

	BoardState b(25, 1.0f);

	Unit unitWithoutGun = unitWithGun;
	unitWithoutGun.rg_range = 0; //Set range to 0 to disable gun

	b.SetUnitOnSquare(Position(0, 0), unitWithoutGun, 0);
	b.SetUnitOnSquare(Position(0, 3), unitWithoutGun, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	//Since we are at an edge, and quite close to the
	// enemy, there should be 5 possible charging locations
	BOOST_TEST(cmds.size() == 5);

	//Determine the closest charging position, which is (0,2)
	GameCommandPtr pClosest;
	for (auto pCmd : cmds)
	{
		if (auto pOrder = dynamic_cast<IUnitOrderCommand*>(pCmd.get()))
		{
			if (pOrder->GetTargetPosition() == Position(0, 2))
			{
				pClosest = pCmd;
			}
		}
	}

	BOOST_REQUIRE(pClosest.get() != nullptr);

	std::vector<GameState> results;
	std::vector<float> probs;
	pClosest->Apply(gs, results, probs);

	//Now, for the key part of this test: this charge range is
	// so short that it should be certain that we pass:
	BOOST_TEST(results.size() == 1);
	BOOST_TEST(probs.size() == results.size());
}


BOOST_AUTO_TEST_CASE(EvenCloserChargeTest)
{
	//Test that if we are charging from 1 unit of distance,
	// then we always make it into combat.

	BoardState b(25, 1.0f);

	Unit unitWithoutGun = unitWithGun;
	unitWithoutGun.rg_range = 0; //Set range to 0 to disable gun

	b.SetUnitOnSquare(Position(0, 0), unitWithoutGun, 0);
	b.SetUnitOnSquare(Position(0, 2), unitWithoutGun, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	//Since we are at an edge, and quite close to the
	// enemy, there should be 5 possible charging locations
	BOOST_TEST(cmds.size() == 5);

	//Determine the closest charging position, which is (0,1)
	GameCommandPtr pClosest;
	for (auto pCmd : cmds)
	{
		if (auto pOrder = dynamic_cast<IUnitOrderCommand*>(pCmd.get()))
		{
			if (pOrder->GetTargetPosition() == Position(0, 1))
			{
				pClosest = pCmd;
			}
		}
	}

	BOOST_REQUIRE(pClosest.get() != nullptr);

	std::vector<GameState> results;
	std::vector<float> probs;
	pClosest->Apply(gs, results, probs);

	//Now, for the key part of this test: this charge range is
	// so short that it should be certain that we pass:
	BOOST_TEST(results.size() == 1);
	BOOST_TEST(probs.size() == results.size());
}


BOOST_AUTO_TEST_CASE(MultipleOverwatchAndDestroyTest)
{
	//Test that if we are receiving overwatch fire from multiple
	// units, and any successful shot would destroy the charging
	// unit, then make sure that the remainder of the overwatch
	// shots don't fail because they are firing at a blank cell.

	BoardState b(25, 1.0f);

	//Simplify the unit so that there is only one shot
	Unit u = unitWithGun;
	u.rg_is_rapid = false;
	u.count = 1;
	u.total_w = 1;
	u.rg_shots = 1;

	b.SetUnitOnSquare(Position(1, 0), u, 0);
	b.SetUnitOnSquare(Position(0, 13), u, 1);
	b.SetUnitOnSquare(Position(1, 13), u, 1);
	b.SetUnitOnSquare(Position(2, 13), u, 1);
	//Set up so only possible charge position is (1,11).

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(1, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//Possibilities: either we pass or fail the charge and take no damage,
	// or we take at least one damage and die. Thus, three possibilities,
	// even though 3 shots.

	BOOST_TEST(results.size() == 3);
	BOOST_TEST(probs.size() == results.size());
}


BOOST_AUTO_TEST_CASE(RapidFireInOverwatchTest)
{
	//Test that rapid fire still works in overwatch (and distance
	// is measured from the charging position, not the charging
	// target.)

	BoardState b(25, 1.0f);

	Unit u = unitWithGun;
	u.rg_range = 10000; //Definitely within rapid fire range
	u.rg_is_rapid = true;
	u.w = 3; //3 wounds so can't be killed by overwatch
	u.total_w = 3;
	u.count = 1;
	u.rg_shots = 1;

	b.SetUnitOnSquare(Position(0, 0), u, 0);
	b.SetUnitOnSquare(Position(0, 13), u, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	//Should be one charge possibility:
	BOOST_REQUIRE(cmds.size() == 1);

	//Apply the command:
	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//6 possibilities because:
	// (charge pass / charge fail) x (0, 1 or 2 wounds taken from overwatch)
	BOOST_TEST(results.size() == 6);
	BOOST_TEST(probs.size() == results.size());
}


BOOST_AUTO_TEST_SUITE_END();



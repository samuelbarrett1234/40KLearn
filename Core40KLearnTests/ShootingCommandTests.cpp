#include "Test.h"


BOOST_AUTO_TEST_SUITE(ShootingCommandTests, *boost::unit_test::depends_on("GameStateTests"));


//A space marine with an AP-1 bolter.
static const Unit unitWithGun{
	"", 5, 6, 3, 3,
	4, 1, 5, 1, 8,
	3, 7, 24, 4, -1,
	1, 1, 4, 0, 1, 0,
	true, false, false,
	false, false, false,
	false, false
};


BOOST_AUTO_TEST_CASE(TargetSelectionTest)
{
	BoardState b(50, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(2, 2), unitWithGun, 1); //Valid target
	b.SetUnitOnSquare(Position(4, 4), unitWithGun, 0); //Not valid target because ally
	b.SetUnitOnSquare(Position(49, 49), unitWithGun, 1); //Not valid target because out of range

	GameState s(0, 0, Phase::SHOOTING, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	auto pCmd = dynamic_cast<const IUnitOrderCommand*>(cmds.front().get());

	BOOST_REQUIRE(pCmd != nullptr);

	BOOST_TEST((pCmd->GetSourcePosition() == Position(0, 0)));
	BOOST_TEST((pCmd->GetTargetPosition() == Position(2, 2)));
}


BOOST_AUTO_TEST_CASE(NotShootingOutOfMeleeTest)
{
	BoardState b(50, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 1), unitWithGun, 1); //In melee
	b.SetUnitOnSquare(Position(4, 4), unitWithGun, 1); //Not valid target because shooter is in melee

	GameState s(0, 0, Phase::SHOOTING, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_TEST(cmds.empty());
}


BOOST_AUTO_TEST_CASE(NotShootingIntoMeleeTest)
{
	BoardState b(50, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(4, 3), unitWithGun, 0); //In melee
	b.SetUnitOnSquare(Position(4, 4), unitWithGun, 1); //Not valid target because in melee

	GameState s(0, 0, Phase::SHOOTING, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_TEST(cmds.empty());
}


BOOST_AUTO_TEST_CASE(NotShootingAfterMovedOutOfMeleeTest)
{
	BoardState b(50, 1.0f);

	Unit u = unitWithGun;
	u.movedOutOfCombatThisTurn = true;

	b.SetUnitOnSquare(Position(0, 0), u, 0);
	b.SetUnitOnSquare(Position(4, 4), unitWithGun, 1); //Not valid target because shooter has moved out of combat

	GameState s(0, 0, Phase::SHOOTING, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_TEST(cmds.empty());
}


BOOST_AUTO_TEST_CASE(HeavyWeaponWithMovementTest)
{
	//We want to test that moving makes heavy weapons hit on 6+
	//To do this it will be easiest to use a 1 shot weapon, and
	// only one model in the squad.

	Unit u = unitWithGun;
	u.rg_is_heavy = true;
	u.movedThisTurn = true;
	u.rg_is_rapid = false;
	u.rg_ap = -1;
	u.rg_shots = 1;
	u.count = 1;
	u.total_w = 1;
	
	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), u, 0);
	b.SetUnitOnSquare(Position(2, 2), unitWithGun, 1);

	GameState s(0, 0, Phase::SHOOTING, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(s, results, probs);

	BOOST_REQUIRE(results.size() == 2);
	BOOST_REQUIRE(probs.size() == 2);

	//Hitting on 6+ wounding on 4+, 4+ armour save => 1/24 chance of causing a wound.
	const float pPenetratingHit = (1.0f / 6.0f) * 0.5f * 0.5f;

	//Test that the probabilities were correct (this is the main
	// result of this test case).
	BOOST_TEST((std::abs(probs.front() - pPenetratingHit) < 1.0e-6f
		|| std::abs(probs.back() - pPenetratingHit) < 1.0e-6f));

	//Determine what result is what
	const GameState* pHitState, *pNoHitState;
	if (std::abs(probs.front() - pPenetratingHit) < 1.0e-6f)
	{
		pHitState = &results.front();
		pNoHitState = &results.back();
	}
	else
	{
		pHitState = &results.back();
		pNoHitState = &results.front();
	}

	//Test the effects of shooting were correct

	BOOST_TEST(pHitState->GetBoardState().IsOccupied(Position(2, 2)));
	BOOST_TEST(pHitState->GetBoardState().GetUnitOnSquare(Position(2, 2)).total_w == 4);
	BOOST_TEST(pHitState->GetBoardState().GetUnitOnSquare(Position(2, 2)).count == 4);

	BOOST_TEST(pNoHitState->GetBoardState().IsOccupied(Position(2, 2)));
	BOOST_TEST(pNoHitState->GetBoardState().GetUnitOnSquare(Position(2, 2)).total_w == 5);
	BOOST_TEST(pNoHitState->GetBoardState().GetUnitOnSquare(Position(2, 2)).count == 5);
}


BOOST_AUTO_TEST_CASE(RapidFireTest)
{
	BoardState b(50, 1.0f);

	//Target needs more wounds than shots
	Unit u = unitWithGun;
	u.count = 15;
	u.total_w = 15;

	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(2, 2), u, 1); //Within rapid fire range

	GameState s(0, 0, Phase::SHOOTING, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);
	
	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(s, results, probs);

	//There are five models, each firing TWO shots
	// because of rapid fire, for ten shots in total,
	// hence there are 11 possible resulting states:
	BOOST_TEST(results.size() == 11);
	BOOST_TEST(probs.size() == results.size());
}


BOOST_AUTO_TEST_CASE(DamageDistributionTest)
{
	BoardState b(50, 1.0f);

	//Make the shooter only fire 4 shots to make calculations easier
	// (4 because rapid fire).
	Unit u = unitWithGun;
	u.count = 2;
	u.total_w = 2;

	b.SetUnitOnSquare(Position(0, 0), u, 0);
	b.SetUnitOnSquare(Position(2, 2), unitWithGun, 1);

	GameState s(0, 0, Phase::SHOOTING, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(s, results, probs);

	BOOST_TEST(results.size() == 5);
	BOOST_TEST(probs.size() == results.size());


}


BOOST_AUTO_TEST_CASE(InvulnerableSaveTest)
{
	//We want to test that moving makes heavy weapons hit on 6+
	//To do this it will be easiest to use a 1 shot weapon, and
	// only one model in the squad.

	Unit u = unitWithGun;
	u.rg_is_rapid = false;
	u.rg_ap = -5;
	u.rg_shots = 1;
	u.count = 1;
	u.total_w = 1;
	u.inv = 2;

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), u, 0);
	b.SetUnitOnSquare(Position(2, 2), u, 1);

	GameState s(0, 0, Phase::SHOOTING, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(s, results, probs);

	BOOST_REQUIRE(results.size() == 2);
	BOOST_REQUIRE(probs.size() == 2);

	//Hitting on 3+ wounding on 4+, no armour save but 2+ invulnerable save => 2/3*1/2*1/6 chance of causing a wound.
	const float pPenetratingHit = (2.0f / 3.0f) * 0.5f * (1.0f / 6.0f);

	//Test that the probabilities were correct (this is the main
	// result of this test case).
	BOOST_TEST((std::abs(probs.front() - pPenetratingHit) < 1.0e-6f
		|| std::abs(probs.back() - pPenetratingHit) < 1.0e-6f));

	//Determine what result is what
	const GameState* pHitState, *pNoHitState;
	if (std::abs(probs.front() - pPenetratingHit) < 1.0e-6f)
	{
		pHitState = &results.front();
		pNoHitState = &results.back();
	}
	else
	{
		pHitState = &results.back();
		pNoHitState = &results.front();
	}

	//Test the effects of shooting were correct

	BOOST_TEST(!pHitState->GetBoardState().IsOccupied(Position(2, 2)));

	BOOST_TEST(pNoHitState->GetBoardState().IsOccupied(Position(2, 2)));
	BOOST_TEST(pNoHitState->GetBoardState().GetUnitOnSquare(Position(2, 2)).total_w == 1);
	BOOST_TEST(pNoHitState->GetBoardState().GetUnitOnSquare(Position(2, 2)).count == 1);
}


BOOST_AUTO_TEST_CASE(OverkillDistributionTest)
{
	//Test that, if we expect to cause many more wounds than
	// the target squad has available, then we group up the
	// excess damage into a single 'entry' in the distribution,
	// instead of having many copies of the distribution which
	// are equal.
	//Also test that, when a unit is destroyed, it is cleared
	// from its square!
	//We want to test that moving makes heavy weapons hit on 6+
	//To do this it will be easiest to use a 1 shot weapon, and
	// only one model in the squad.

	Unit u = unitWithGun;
	u.rg_shots = 5; //Lots of shots! (doesn't really matter how many, preferably more than one though)
	u.count = 2;
	u.total_w = 2;

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), u, 0);
	b.SetUnitOnSquare(Position(2, 2), u, 1);

	GameState s(0, 0, Phase::SHOOTING, b);

	auto cmds = s.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(s, results, probs);

	//Important: there should be exactly three different outcomes
	// here, even though there are 10 shots. Either the unit survives
	// without harm, loses one wound, or is destroyed completely.
	//What we are trying to avoid here is having many results, most
	// of which resulting in the same outcome: total destruction of
	// the unit.
	BOOST_REQUIRE(results.size() == 3);
	BOOST_REQUIRE(probs.size() == 3);
}


BOOST_AUTO_TEST_CASE(MultiDamageWeaponTest)
{
	//Test that a weapon with more than 1 damage
	// correctly allocates that damage to the target

	Unit unit = unitWithGun;
	unit.w = 3;
	unit.count = 1;
	unit.total_w = unit.w * unit.count;
	unit.rg_dmg = 2;
	unit.rg_shots = 2;
	unit.rg_is_rapid = false;

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unit, 0);
	b.SetUnitOnSquare(Position(0, 3), unit, 1);

	GameState gs(0, 0, Phase::SHOOTING, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_REQUIRE((cmds.front()->GetType() == CommandType::UNIT_ORDER));

	//Apply it:
	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//Since the unit has three wounds and two damage,
	// the only three possible results are (i) dead and
	// (ii) lost two wounds (down to one) and (iii) no
	// damage taken:

	BOOST_TEST(results.size() == 3);

	//numEncounters[i] is the number of states
	// which resulted in i wounds left for the
	// target.
	std::vector<int> numEncounters;
	numEncounters.resize(4, 0);

	for (const auto& state : results)
	{
		if (state.GetBoardState().IsOccupied(Position(0, 3)))
		{
			const int wleft = state.GetBoardState().GetUnitOnSquare(Position(0, 3)).total_w;
			BOOST_REQUIRE(wleft <= 3);
			numEncounters[wleft]++;
		}
		else
		{
			numEncounters[0]++;
		}
	}

	//Now test results:
	BOOST_TEST(numEncounters[0] == 1);
	BOOST_TEST(numEncounters[1] == 1);
	BOOST_TEST(numEncounters[2] == 0);
	BOOST_TEST(numEncounters[3] == 1);
}


BOOST_AUTO_TEST_CASE(ExcessDamageNoSpillTest)
{
	//Test that a single shot from a high damage weapon
	// does not let its damage "spill over" to wound
	// more models than there are shots.
	
	Unit unit = unitWithGun;
	unit.count = 1;
	unit.total_w = unit.w * unit.count;
	unit.rg_dmg = 6;
	unit.rg_shots = 1;
	unit.rg_is_rapid = false;

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unit, 0);
	b.SetUnitOnSquare(Position(0, 3), unitWithGun, 1);

	GameState gs(0, 0, Phase::SHOOTING, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_REQUIRE((cmds.front()->GetType() == CommandType::UNIT_ORDER));

	//Apply it:
	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//Since the shooting weapon is one shot only,
	// there should only be two results (hit/miss),
	// killing up to one model:

	BOOST_REQUIRE(results.size() == 2);
	BOOST_TEST(probs.size() == results.size());

	for (const auto& state : results)
	{
		BOOST_REQUIRE(state.GetBoardState().IsOccupied(Position(0, 3)));

		int modelsLeft = state.GetBoardState().GetUnitOnSquare(Position(0, 3)).count;

		//Can lose either 0 or 1 models, but no more
		BOOST_TEST((modelsLeft == unitWithGun.count || modelsLeft == unitWithGun.count - 1));
	}
}


BOOST_AUTO_TEST_CASE(ModelsLostTest)
{
	//Test that damage caused is added to the modelsLostThisPhase variable.

	const int numModels = unitWithGun.count;

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unitWithGun, 0);
	b.SetUnitOnSquare(Position(0, 3), unitWithGun, 1);

	GameState gs(0, 0, Phase::SHOOTING, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	// numTimesEncountered[i] is the number of
	// states where unit (0,3) had i models left.
	std::vector<int> numTimesEncountered;
	numTimesEncountered.resize(numModels + 1, 0);

	//Check that, for every possible result, that the number of
	// models left and the number of models lost this phase
	// tie up.
	for (size_t i = 0; i < results.size(); i++)
	{
		if (results[i].GetBoardState().IsOccupied(Position(0, 3)))
		{
			auto unit = results[i].GetBoardState().GetUnitOnSquare(Position(0, 3));

			BOOST_REQUIRE(unit.count <= numModels);

			numTimesEncountered[unit.count]++;

			BOOST_TEST(unit.modelsLostThisPhase == numModels - unit.count);
		}
		else
		{
			numTimesEncountered[0]++;
		}
	}

	//While we're here, check distinctness of output states:
	for (auto n : numTimesEncountered)
	{
		BOOST_TEST(n == 1);
	}
}


BOOST_AUTO_TEST_SUITE_END();



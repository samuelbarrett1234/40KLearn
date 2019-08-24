#include "Test.h"
#include <boost/math/distributions.hpp>


BOOST_AUTO_TEST_SUITE(FightCommandTests, *boost::unit_test::depends_on("GameStateTests"));


//A single space marine
static const Unit unitSingleAttack{
	"", 1, 6, 3, 3,
	4, 1, 1, 1, 8,
	3, 7, 24, 4, 0,
	1, 1, 4, 0, 1, 0,
	true, false, false,
	false, false, false,
	false, false
};
//2 space marines with 2 attacks each
static const Unit squadMultipleAttacks{
"", 2, 6, 3, 3,
4, 1, 2, 2, 8,
3, 7, 24, 4, 0,
1, 1, 4, 0, 1, 0,
true, false, false,
false, false, false,
false, false
};


BOOST_AUTO_TEST_CASE(FightDistributionTest, *boost::unit_test::tolerance(1.0e-4f))
{
	//Test that, for a small unit with a small total number of attacks,
	// its damage distribution is correct

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), squadMultipleAttacks, 0);
	b.SetUnitOnSquare(Position(0, 1), squadMultipleAttacks, 1);

	GameState gs(0, 0, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_REQUIRE((cmds.front()->GetType() == CommandType::UNIT_ORDER));
	BOOST_REQUIRE(dynamic_cast<IUnitOrderCommand*>(cmds.front().get()) != nullptr);
	BOOST_REQUIRE((dynamic_cast<IUnitOrderCommand*>(
		cmds.front().get())->GetSourcePosition() == Position(0, 0)));
	BOOST_REQUIRE((dynamic_cast<IUnitOrderCommand*>(
		cmds.front().get())->GetTargetPosition() == Position(0, 1)));

	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//Both squads have 2 wounds so three possibilities (0/1/2)
	BOOST_REQUIRE(results.size() == 3);
	BOOST_REQUIRE(results.size() == probs.size());

	//Each attach has a 2/3 * 1/2 * 1/3 = 1/9 chance of
	// wounding (3+ to hit, 4+ to wound, 3+ armour save)
	//Hence it is (8/9)^4 chance of no wounds total,
	// 4*(1/9)*(8/9)^3 chance of exactly one wound total,
	// and 1-the sum of the above for exactly two wounds total:

	const auto woundDist = boost::math::binomial_distribution<float>(4.0f, 1.0f / 9.0f);
	const float woundProbs[3] =
	{
		boost::math::pdf(woundDist, 0.0f),
		boost::math::pdf(woundDist, 1.0f),
		boost::math::pdf(woundDist, 2.0f) + boost::math::pdf(woundDist, 3.0f) + boost::math::pdf(woundDist, 4.0f),
	};
	// woundProbs[i] is the probability of causing i wounds

	//Test that each resulting number of wounds was (i) encountered
	// in the array, and (ii) had the correct probability.
	
	// bEncountered[j] is true iff the state where the target had j wounds
	// left has been encountered in the loop below
	bool bEncountered[3] = { false, false, false };

	for (size_t i = 0; i < 3; i++)
	{
		//First, determine what type of result this is.
		//j = number of wounds left
		size_t j = 0;

		if (results[i].GetBoardState().IsOccupied(Position(0, 1)))
		{
			j = results[i].GetBoardState().GetUnitOnSquare(Position(0, 1)).total_w;

			BOOST_TEST(results[i].GetBoardState().GetTeamOnSquare(Position(0, 1)) == 1);
		}
		// else 0 wounds left, hence destroyed

		BOOST_TEST(j <= 2U);

		//Perform sanity check:
		BOOST_TEST(results[i].GetBoardState().IsOccupied(Position(0, 0)));
		BOOST_TEST(results[i].GetBoardState().GetTeamOnSquare(Position(0, 0)) == 0);
		BOOST_TEST(results[i].GetBoardState().GetUnitOnSquare(Position(0, 0)).total_w == 2);

		bEncountered[j] = true;

		//Boost automatically checks for tolerance (see test decorator)
		//Need 2-j because j is number of wounds REMAINING, but woundProbs[j]
		// is the probability of CAUSING j wounds.
		BOOST_TEST(probs[i] == woundProbs[2 - j]);
	}

	BOOST_TEST(bEncountered[0]);
	BOOST_TEST(bEncountered[1]);
	BOOST_TEST(bEncountered[2]);
}


BOOST_AUTO_TEST_CASE(FightOrderTwoTwoTest)
{
	//Test that all players engaged in combat must fight,
	// and test the internal team cannot end phase until
	// all units have fought (on both sides).
	//Perform this test in the case where both players have
	// 2 units

	//Create a unit with lots of extra wounds because
	// then we don't need to worry about them dying
	// during our test
	Unit unit = unitSingleAttack;
	unit.w = 5;
	unit.total_w = 5;
	
	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unit, 0);
	b.SetUnitOnSquare(Position(0, 1), unit, 1);
	b.SetUnitOnSquare(Position(2, 0), unit, 0);
	b.SetUnitOnSquare(Position(2, 1), unit, 1);

	GameState gs(0, 0, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();
	
	BOOST_REQUIRE(cmds.size() == 2);
	BOOST_TEST((cmds.front()->GetType() == CommandType::UNIT_ORDER));
	BOOST_TEST((cmds.back()->GetType() == CommandType::UNIT_ORDER));

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	std::vector<GameState> results;
	std::vector<float> probs;

	//Apply the fight command
	cmds.front()->Apply(gs, results, probs);

	//Doesn't really matter which we pick
	gs = results.front();

	BOOST_TEST(gs.GetActingTeam() == 1);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	//Now do the fight for the other team
	
	cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 2);
	BOOST_TEST((cmds.front()->GetType() == CommandType::UNIT_ORDER));
	BOOST_TEST((cmds.back()->GetType() == CommandType::UNIT_ORDER));

	stripCommandsNotFor(Position(0, 1), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	//Apply the fight command
	results.clear();
	probs.clear();
	cmds.front()->Apply(gs, results, probs);

	//Doesn't really matter which we pick
	gs = results.front();

	BOOST_TEST(gs.GetActingTeam() == 0);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	//Now go back to team 0 fighting

	cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_TEST((cmds.front()->GetType() == CommandType::UNIT_ORDER));

	//Apply the fight command
	results.clear();
	probs.clear();
	cmds.front()->Apply(gs, results, probs);

	//Doesn't really matter which we pick
	gs = results.front();

	BOOST_TEST(gs.GetActingTeam() == 1);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	//Now go back to team 1 fighting

	cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_TEST((cmds.front()->GetType() == CommandType::UNIT_ORDER));

	//Apply the fight command
	results.clear();
	probs.clear();
	cmds.front()->Apply(gs, results, probs);

	//Doesn't really matter which we pick
	gs = results.front();

	BOOST_TEST(gs.GetActingTeam() == 0);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	//Now the fighting should be over and team 0 should have the end phase command

	cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_TEST((cmds.front()->GetType() == CommandType::END_PHASE));

	//Done!
}


BOOST_AUTO_TEST_CASE(FightOrderTwoOneTest)
{
	//Test that all players engaged in combat must fight,
	// and test the internal team cannot end phase until
	// all units have fought (on both sides).
	//Perform this test in the case where team 0 has two
	// units and team 1 has one unit

	//Create a unit with lots of extra wounds because
	// then we don't need to worry about them dying
	// during our test
	Unit unit = unitSingleAttack;
	unit.w = 5;
	unit.total_w = 5;

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unit, 0);
	b.SetUnitOnSquare(Position(0, 1), unit, 1);
	b.SetUnitOnSquare(Position(1, 0), unit, 0);

	GameState gs(0, 0, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 2);
	BOOST_TEST((cmds.front()->GetType() == CommandType::UNIT_ORDER));
	BOOST_TEST((cmds.back()->GetType() == CommandType::UNIT_ORDER));

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_REQUIRE(cmds.size() == 1);

	std::vector<GameState> results;
	std::vector<float> probs;

	//Apply the fight command
	cmds.front()->Apply(gs, results, probs);

	//Doesn't really matter which we pick
	gs = results.front();

	BOOST_TEST(gs.GetActingTeam() == 1);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	//Now do the fight for the other team

	cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 2); //Should still be two commands because there are two adjacent enemies
	BOOST_TEST((cmds.front()->GetType() == CommandType::UNIT_ORDER));
	BOOST_TEST((cmds.back()->GetType() == CommandType::UNIT_ORDER));

	//Apply a fight command
	results.clear();
	probs.clear();
	cmds.front()->Apply(gs, results, probs);

	//Doesn't really matter which we pick
	gs = results.front();

	BOOST_TEST(gs.GetActingTeam() == 0);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	//Now go back to team 0 fighting

	cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_TEST((cmds.front()->GetType() == CommandType::UNIT_ORDER));

	//Apply the fight command
	results.clear();
	probs.clear();
	cmds.front()->Apply(gs, results, probs);

	//Doesn't really matter which we pick
	gs = results.front();

	BOOST_TEST(gs.GetActingTeam() == 0);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	//Now the fighting should be over and team 0 should have the end phase command

	cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_TEST((cmds.front()->GetType() == CommandType::END_PHASE));

	//Done!
}


BOOST_AUTO_TEST_CASE(FightOrderOneTwoTest)
{
	//Test that all players engaged in combat must fight,
	// and test the internal team cannot end phase until
	// all units have fought (on both sides).
	//Perform this test in the case where team 0 has one
	// unit and team 1 has two units

	//Create a unit with lots of extra wounds because
	// then we don't need to worry about them dying
	// during our test
	Unit unit = unitSingleAttack;
	unit.w = 5;
	unit.total_w = 5;

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unit, 0);
	b.SetUnitOnSquare(Position(0, 1), unit, 1);
	b.SetUnitOnSquare(Position(1, 0), unit, 1);

	GameState gs(0, 0, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 2); //Should still have two commands because there are two adjacent enemies
	BOOST_TEST((cmds.front()->GetType() == CommandType::UNIT_ORDER));
	BOOST_TEST((cmds.back()->GetType() == CommandType::UNIT_ORDER));

	std::vector<GameState> results;
	std::vector<float> probs;

	//Apply the fight command
	cmds.front()->Apply(gs, results, probs);

	//Doesn't really matter which we pick
	gs = results.front();

	BOOST_TEST(gs.GetActingTeam() == 1);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	//Now do the fight for the other team

	cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 2);
	BOOST_TEST((cmds.front()->GetType() == CommandType::UNIT_ORDER));
	BOOST_TEST((cmds.back()->GetType() == CommandType::UNIT_ORDER));

	//Apply a fight command
	results.clear();
	probs.clear();
	cmds.front()->Apply(gs, results, probs);

	//Doesn't really matter which we pick
	gs = results.front();

	BOOST_TEST(gs.GetActingTeam() == 1);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	//Still team 1 fighting

	cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_TEST((cmds.front()->GetType() == CommandType::UNIT_ORDER));

	//Apply the fight command
	results.clear();
	probs.clear();
	cmds.front()->Apply(gs, results, probs);

	//Doesn't really matter which we pick
	gs = results.front();

	BOOST_TEST(gs.GetActingTeam() == 0);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	//Now the fighting should be over and team 0 should have the end phase command

	cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_TEST((cmds.front()->GetType() == CommandType::END_PHASE));

	//Done!
}


BOOST_AUTO_TEST_CASE(AdjacentTargetsTest)
{
	//Test that we can only fight with adjacent enemies

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unitSingleAttack, 0);
	b.SetUnitOnSquare(Position(0, 1), unitSingleAttack, 1); //Can fight this
	b.SetUnitOnSquare(Position(1, 1), unitSingleAttack, 1); //Can fight this
	b.SetUnitOnSquare(Position(0, 2), unitSingleAttack, 1); //Cannot fight this
	b.SetUnitOnSquare(Position(2, 0), unitSingleAttack, 1); //Cannot fight this

	GameState gs(0, 0, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();

	stripCommandsNotFor(Position(0, 0), cmds);

	BOOST_TEST(cmds.size() == 2);
}


BOOST_AUTO_TEST_CASE(ModelsLostTest)
{
	const int numModels = 5;

	Unit unit = squadMultipleAttacks;
	unit.count = numModels;
	unit.total_w = unit.w * numModels;

	//Test that damage caused is added to the modelsLostThisPhase variable.

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unit, 0);
	b.SetUnitOnSquare(Position(0, 1), unit, 1);

	GameState gs(0, 0, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);

	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	// numTimesEncountered[i] is the number of
	// states where unit (0,1) had i models left.
	std::vector<int> numTimesEncountered;
	numTimesEncountered.resize(numModels+1, 0);

	//Check that, for every possible result, that the number of
	// models left and the number of models lost this phase
	// tie up.
	for (size_t i = 0; i < results.size(); i++)
	{
		if (results[i].GetBoardState().IsOccupied(Position(0, 1)))
		{
			auto unit = results[i].GetBoardState().GetUnitOnSquare(Position(0, 1));

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


BOOST_AUTO_TEST_CASE(MultiDamageWeaponTest)
{
	//Test that a weapon with more than 1 damage
	// correctly allocates that damage to the target

	Unit unit = unitSingleAttack;
	unit.w = unit.total_w = 3;
	unit.ml_dmg = 2;
	unit.a = 2; //Two attacks to include possibility of destroying enemy unit

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unit, 0);
	b.SetUnitOnSquare(Position(0, 1), unit, 1);

	GameState gs(0, 0, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();

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
		if (state.GetBoardState().IsOccupied(Position(0, 1)))
		{
			const int wleft = state.GetBoardState().GetUnitOnSquare(Position(0, 1)).total_w;
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
	//Test that a single attack from a high damage weapon
	// does not let its damage "spill over" to wound
	// more models than there are shots.

	Unit unit = unitSingleAttack;
	unit.count = 1;
	unit.total_w = unit.w * unit.count;
	unit.ml_dmg = 6;
	unit.a = 1;

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), unit, 0);
	b.SetUnitOnSquare(Position(0, 1), squadMultipleAttacks, 1);

	GameState gs(0, 0, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();

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
		BOOST_REQUIRE(state.GetBoardState().IsOccupied(Position(0, 1)));

		int modelsLeft = state.GetBoardState().GetUnitOnSquare(Position(0, 1)).count;

		//Can lose either 0 or 1 models, but no more
		BOOST_TEST((modelsLeft == squadMultipleAttacks.count || modelsLeft == squadMultipleAttacks.count - 1));
	}
}


BOOST_AUTO_TEST_CASE(TestNoFriendlyFighting)
{
	//Test that two adjacent friendly units don't
	// have fight options!

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), squadMultipleAttacks, 0);
	b.SetUnitOnSquare(Position(0, 1), squadMultipleAttacks, 0);

	//Need an enemy unit to ensure unfinished game
	b.SetUnitOnSquare(Position(3, 3), squadMultipleAttacks, 1);

	GameState gs(0, 0, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_TEST((cmds.front()->GetType() == CommandType::END_PHASE));
}


BOOST_AUTO_TEST_CASE(TestNoMeleeWeaponCantFight)
{
	//Test that a unit which happens to be adjacent to
	// an enemy unit, but without a melee weapon, cannot
	// fight.

	Unit noMeleeWeaponUnit = unitSingleAttack;
	noMeleeWeaponUnit.a = 0;
	noMeleeWeaponUnit.w = noMeleeWeaponUnit.total_w = 5; //High W so doesn't die

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), noMeleeWeaponUnit, 0);
	b.SetUnitOnSquare(Position(0, 1), squadMultipleAttacks, 1);

	GameState gs(0, 1, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_TEST((cmds.front()->GetType() == CommandType::UNIT_ORDER));

	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//Pick any
	gs = results[0];

	BOOST_TEST(gs.GetActingTeam() == 0);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	cmds = gs.GetCommands();

	//Here, this is to ensure the no-melee-weapon unit doesn't
	// get a fight command; it should end turn straight away.

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_TEST((cmds.front()->GetType() == CommandType::END_PHASE));
}


BOOST_AUTO_TEST_CASE(NoAbleUnitsTest)
{
	//Test that a team which has just ended its charge
	// phase, but has one unit WITHOUT A MELEE WEAPON
	// which is engaged in combat with an enemy which
	// DOES, then the end phase command transitions
	// straight into the enemy's command (since their
	// technically is a melee fight going on, but the
	// internal team has no moves to make!)

	Unit noMeleeWeaponUnit = squadMultipleAttacks;
	noMeleeWeaponUnit.a = 0;
	noMeleeWeaponUnit.w = noMeleeWeaponUnit.total_w = 5; //High wounds so can't die
	noMeleeWeaponUnit.count = 1;

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), noMeleeWeaponUnit, 0);
	b.SetUnitOnSquare(Position(0, 1), squadMultipleAttacks, 1);

	GameState gs(0, 0, Phase::CHARGE, b);

	auto cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_REQUIRE((cmds.front()->GetType() == CommandType::END_PHASE));

	//End phase

	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);
	BOOST_REQUIRE(results.size() == 1);
	gs = results.front();

	//Now test that it has skipped team 0's fight phase and is
	// going straight to team 1, because team 0's unit has no
	// melee weapon.

	BOOST_TEST(gs.GetActingTeam() == 1);
	BOOST_TEST(gs.GetInternalTeam() == 0);

	//Apply that fight command, and then check that team 0 still
	// has to issue the end phase command:

	cmds = gs.GetCommands();
	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_REQUIRE((cmds.front()->GetType() == CommandType::UNIT_ORDER));
	results.clear();
	probs.clear();
	cmds.front()->Apply(gs, results, probs);

	//Pick any:
	gs = results[0];

	//Check state:
	BOOST_TEST(gs.GetInternalTeam() == 0);
	BOOST_TEST(gs.GetActingTeam() == 0);
	cmds = gs.GetCommands();
	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_TEST((cmds.front()->GetType() == CommandType::END_PHASE));
}


BOOST_AUTO_TEST_CASE(NoAbleUnitsInvalidStateTest)
{
	//Test that it is impossible to accidentally create
	// a game state where the active team (0) has no possible
	// commands - this is because team 0's unit in melee
	// has no melee weapon, but team 1's unit does, hence we
	// cannot let team 0 end turn yet, but they also cannot
	// fight.

	Unit noMeleeWeaponUnit = squadMultipleAttacks;
	noMeleeWeaponUnit.a = 0;

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), noMeleeWeaponUnit, 0);
	b.SetUnitOnSquare(Position(0, 1), squadMultipleAttacks, 1);

	//This is an invalid state because it says the active team is team 0
	// but that team has no possible fighting units!
	BOOST_CHECK_THROW(GameState(0, 0, Phase::FIGHT, b), std::runtime_error);
}


BOOST_AUTO_TEST_CASE(FlagsFoughtTest)
{
	//Test that, when a unit fights, its foughtThisTurn
	// flag is set to true

	BoardState b(25, 1.0f);

	b.SetUnitOnSquare(Position(0, 0), squadMultipleAttacks, 0);
	b.SetUnitOnSquare(Position(0, 1), squadMultipleAttacks, 1);

	GameState gs(0, 0, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();

	BOOST_REQUIRE(cmds.size() == 1);

	//Apply command
	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//Check that, in all resulting states, the unit is classed as having fought.
	for (const auto& state : results)
	{
		BOOST_REQUIRE(state.GetBoardState().IsOccupied(Position(0, 0)));
		BOOST_TEST(state.GetBoardState().GetUnitOnSquare(Position(0, 0)).foughtThisTurn);
	}
}


BOOST_AUTO_TEST_SUITE_END();



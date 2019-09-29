#include "Test.h"
#include <algorithm>
#include <boost/optional.hpp>


//Helper function
static int clamp(int val, int lo, int hi)
{
	return (val < lo) ? lo : ((val > hi) ? hi : val);
}


//A space marine squad of 10
static const Unit exampleUnit{
	"", 10, 6, 3, 3,
	4, 1, 10, 1, 8,
	3, 7, 24, 4, 0,
	1, 1, 4, 0, 1, 0,
	true, false, false,
	false, false, false,
	false, false
};


BOOST_AUTO_TEST_SUITE(EndPhaseTests);


BOOST_AUTO_TEST_CASE(ResetFlagsTest)
{
	//Test that the flags are reset at the end of the turn
	// (we will test that it doesn't reset them on earlier
	// phases in a later test).

	Unit unit1 = exampleUnit;
	unit1.movedThisTurn = true;
	unit1.firedThisTurn = true;
	unit1.attemptedChargeThisTurn = true;
	unit1.successfulChargeThisTurn = true;
	unit1.foughtThisTurn = true;

	Unit unit2 = exampleUnit;
	unit2.movedThisTurn = true;
	unit2.movedOutOfCombatThisTurn = true;

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unit1, 0);
	b.SetUnitOnSquare(Position(0, 2), unit2, 1);
	GameState gs(0, 0, Phase::FIGHT, b);

	auto cmds = gs.GetCommands();

	//Should only be one command because nobody is locked in combat
	BOOST_REQUIRE(cmds.size() == 1);
	BOOST_REQUIRE(cmds.front()->GetType() == CommandType::END_PHASE);

	//Apply command
	std::vector<GameState> results;
	std::vector<float> probs;
	cmds.front()->Apply(gs, results, probs);

	//Should only be one result because no morale:
	BOOST_REQUIRE(results.size() == 1);
	BOOST_TEST(results.size() == probs.size());

	//Update state
	gs = results.front();

	//Get the units:
	BOOST_REQUIRE(gs.GetBoardState().IsOccupied(Position(0, 0)));
	BOOST_REQUIRE(gs.GetBoardState().IsOccupied(Position(0, 2)));
	unit1 = gs.GetBoardState().GetUnitOnSquare(Position(0, 0));
	unit2 = gs.GetBoardState().GetUnitOnSquare(Position(0, 2));

	//Check the flags were reset:
	BOOST_TEST(!unit1.movedThisTurn);
	BOOST_TEST(!unit1.firedThisTurn);
	BOOST_TEST(!unit1.attemptedChargeThisTurn);
	BOOST_TEST(!unit1.successfulChargeThisTurn);
	BOOST_TEST(!unit1.foughtThisTurn);
	BOOST_TEST(!unit2.movedThisTurn);
	BOOST_TEST(!unit2.movedOutOfCombatThisTurn);
}


BOOST_AUTO_TEST_CASE(NoResetFlagsInEarlierPhasesTest)
{
	//Test that the flags are NOT reset UNTIL the end of the turn
	// (i.e. not in the earlier phases).

	Unit unit1 = exampleUnit;
	unit1.movedThisTurn = true;
	unit1.firedThisTurn = true;
	unit1.attemptedChargeThisTurn = true;
	unit1.successfulChargeThisTurn = true;
	unit1.foughtThisTurn = true;

	Unit unit2 = exampleUnit;
	unit2.movedThisTurn = true;
	unit2.movedOutOfCombatThisTurn = true;

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unit1, 0);
	b.SetUnitOnSquare(Position(0, 2), unit2, 1);

	//Set up other states for each phase except the last:
	GameState gs[3] = {
		GameState(0, 0, Phase::MOVEMENT, b),
		GameState(0, 0, Phase::SHOOTING, b),
		GameState(0, 0, Phase::CHARGE, b),
	};

	for (int i = 0; i < 3; i++)
	{
		auto cmds = gs[i].GetCommands();

		//Find the end phase command:
		GameCommandPtr pEndPhaseCmd;
		for (auto pCmd : cmds)
		{
			if (pCmd->GetType() == CommandType::END_PHASE)
			{
				pEndPhaseCmd = pCmd;
				break;
			}
		}
		BOOST_REQUIRE(pEndPhaseCmd.get() != nullptr);

		//Apply command
		std::vector<GameState> results;
		std::vector<float> probs;
		pEndPhaseCmd->Apply(gs[i], results, probs);

		//Should only be one result because no morale:
		BOOST_REQUIRE(results.size() == 1);
		BOOST_TEST(results.size() == probs.size());

		//Get resulting state:
		GameState gs = results.front();

		//Get the units:
		BOOST_REQUIRE(gs.GetBoardState().IsOccupied(Position(0, 0)));
		BOOST_REQUIRE(gs.GetBoardState().IsOccupied(Position(0, 2)));
		unit1 = gs.GetBoardState().GetUnitOnSquare(Position(0, 0));
		unit2 = gs.GetBoardState().GetUnitOnSquare(Position(0, 2));

		//Check the flags were reset:
		BOOST_TEST(unit1.movedThisTurn);
		BOOST_TEST(unit1.firedThisTurn);
		BOOST_TEST(unit1.attemptedChargeThisTurn);
		BOOST_TEST(unit1.successfulChargeThisTurn);
		BOOST_TEST(unit1.foughtThisTurn);
		BOOST_TEST(unit2.movedThisTurn);
		BOOST_TEST(unit2.movedOutOfCombatThisTurn);
	}
}


BOOST_AUTO_TEST_CASE(ResetModelsLostTest)
{
	//Test that the number of models lost is reset to zero
	// at the end of each phase

	Unit unit = exampleUnit;
	unit.modelsLostThisPhase = 2;

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unit, 0);
	b.SetUnitOnSquare(Position(0, 2), unit, 1);

	//Set up other states for each phase:
	GameState gs[4] = {
		GameState(0, 0, Phase::MOVEMENT, b),
		GameState(0, 0, Phase::SHOOTING, b),
		GameState(0, 0, Phase::CHARGE, b),
		GameState(0, 0, Phase::FIGHT, b),
	};

	for (int i = 0; i < 4; i++)
	{
		auto cmds = gs[i].GetCommands();

		//Find the end phase command:
		GameCommandPtr pEndPhaseCmd;
		for (auto pCmd : cmds)
		{
			if (pCmd->GetType() == CommandType::END_PHASE)
			{
				pEndPhaseCmd = pCmd;
				break;
			}
		}
		BOOST_REQUIRE(pEndPhaseCmd.get() != nullptr);

		//Apply command
		std::vector<GameState> results;
		std::vector<float> probs;
		pEndPhaseCmd->Apply(gs[i], results, probs);

		for (const auto& state : results)
		{
			if (state.GetBoardState().IsOccupied(Position(0, 0)))
			{
				BOOST_TEST(state.GetBoardState().GetUnitOnSquare(Position(0, 0)).modelsLostThisPhase == 0);
			}
			//Need to reset enemy stuff too!
			if (state.GetBoardState().IsOccupied(Position(0, 2)))
			{
				BOOST_TEST(state.GetBoardState().GetUnitOnSquare(Position(0, 2)).modelsLostThisPhase == 0);
			}
		}
	}
}


BOOST_TEST_DECORATOR(*boost::unit_test::tolerance(1.0e-4f))
BOOST_DATA_TEST_CASE(MoraleCheckDistributionTest, boost::unit_test::data::xrange(0, exampleUnit.count), numLost)
{
	//Test that, for each value, if the unit has lost that many
	// models then they have the correct distribution for morale

	//Note: for the data in this test case to cover all code pathways
	// (no risk of losing models due to morale; risk of losing some
	// models to morale; certain to use entire unit due to morale)
	// it is important that the number of models lost ranges from 0 to
	// the leadership value, and the squad size must be one greater than
	// the upper limit of the number of models lost.

	BOOST_REQUIRE(exampleUnit.count > numLost); //TEMP

	Unit unit = exampleUnit;
	unit.modelsLostThisPhase = numLost;
	unit.count -= numLost;

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 2), exampleUnit, 0);
	b.SetUnitOnSquare(Position(0, 0), unit, 1);

	//Set up other states for each phase:
	GameState gs[4] = {
		GameState(0, 0, Phase::MOVEMENT, b),
		GameState(0, 0, Phase::SHOOTING, b),
		GameState(0, 0, Phase::CHARGE, b),
		GameState(0, 0, Phase::FIGHT, b),
	};

	for (int i = 0; i < 4; i++)
	{
		auto cmds = gs[i].GetCommands();

		//Find the end phase command:
		GameCommandPtr pEndPhaseCmd;
		for (auto pCmd : cmds)
		{
			if (pCmd->GetType() == CommandType::END_PHASE)
			{
				pEndPhaseCmd = pCmd;
				break;
			}
		}
		BOOST_REQUIRE(pEndPhaseCmd.get() != nullptr);

		//Apply command
		std::vector<GameState> results;
		std::vector<float> probs;
		pEndPhaseCmd->Apply(gs[i], results, probs);

		const size_t n = results.size();
		BOOST_REQUIRE(n == probs.size());

		//Compute the minimum and maximum number of models we
		// can lose due to morale:
		const int lossesUB = clamp(numLost + 6 - unit.ld, 0, unit.count);
		const int lossesLB = clamp(numLost + 1 - unit.ld, 0, unit.count);

		//There must be a result for each value lb<=x<=ub:
		BOOST_REQUIRE(results.size() == lossesUB - lossesLB + 1);

		//Now construct an array such that orderedResults[i] is
		// the result where lossesLB+i models were lost from
		// morale.
		std::vector<boost::optional<GameState>> orderedResults;
		std::vector<float> orderedProbs;

		orderedResults.resize(n);
		orderedProbs.resize(n, 0.0f);

		for (size_t i = 0; i < n; i++)
		{
			const auto& state = results[i];

			if (state.GetBoardState().IsOccupied(Position(0, 0)))
			{
				int left = state.GetBoardState().GetUnitOnSquare(Position(0, 0)).count;

				BOOST_REQUIRE(lossesLB <= unit.count - left);
				BOOST_REQUIRE(unit.count - left <= lossesUB);

				size_t idx = (size_t)(unit.count - left - lossesLB);

				//Ensure this state hasn't been encountered before
				BOOST_REQUIRE(!static_cast<bool>(orderedResults[idx]));

				orderedResults[idx] = state;
				orderedProbs[idx] = probs[i];
			}
			else
			{
				//In this case, we must've lost the whole unit!
				BOOST_REQUIRE(lossesUB == unit.count);

				//Ensure this state hasn't been encountered before
				BOOST_REQUIRE(!static_cast<bool>(orderedResults.back()));

				orderedResults.back() = state;
				orderedProbs.back() = probs[i];
			}
		}

		//Now that we've constructed the array in a guaranteed order
		// and checked for validity, we check for probabilities.
		//The probablility of any morale check result, except for the
		// boundaries, is 1/6, however the boundary result probabilities
		// may be different.

		//Determine the "width" of the lower and upper bounds, to determine
		// the probability (width=number of dice roll results which would
		// produce this resulting state).
		int rollToWipeOutSquad = clamp(unit.count - numLost + unit.ld, 1, 6);
		int rollToJustPass = clamp(unit.ld - numLost, 1, 6);
		int lbWidth = (lossesLB != lossesUB) ? rollToJustPass : 6,
			ubWidth = 7 - rollToWipeOutSquad;

		for (size_t i = 0; i < n; i++)
		{
			if (i == 0)
				BOOST_TEST(orderedProbs[i] == (float)lbWidth / 6.0f);
			else if (i == n - 1)
				BOOST_TEST(orderedProbs[i] == (float)ubWidth / 6.0f);
			else
				BOOST_TEST(orderedProbs[i] == 1.0f / 6.0f);
		}
	}
}


BOOST_AUTO_TEST_CASE(TestEndPhaseIncrementsTurnNumberAppropriately)
{
	//Test that the turn number is incremented only at the
	// end of team 1's turn, not the end of team 0's turn,
	// and test that, when the turn limit is reached, the
	// game state marks itself as finished.

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), exampleUnit, 0);
	b.SetUnitOnSquare(Position(0, 2), exampleUnit, 1);
	GameState gs1(0, 0, Phase::FIGHT, b, 3, 2); //End of team 0's turn
	GameState gs2(1, 1, Phase::FIGHT, b, 3, 2); //End of team 1's turn

	//Test basic getters:
	BOOST_TEST(gs1.HasTurnLimit());
	BOOST_TEST(gs1.GetTurnLimit() == 3);
	BOOST_TEST(gs1.GetTurnNumber() == 2);
	BOOST_TEST(gs2.HasTurnLimit());
	BOOST_TEST(gs2.GetTurnLimit() == 3);
	BOOST_TEST(gs2.GetTurnNumber() == 2);

	//gs1 should not reach limit but gs2 should, after ending phase:

	auto gs1Commands = gs1.GetCommands();
	auto gs2Commands = gs2.GetCommands();
	BOOST_REQUIRE(gs1Commands.size() == 1);
	BOOST_REQUIRE(gs2Commands.size() == 1);

	//Apply commands:
	std::vector<GameState> results;
	std::vector<float> probs;

	gs1Commands.front()->Apply(gs1, results, probs);
	gs1 = results.front();
	results.clear();
	probs.clear();
	gs2Commands.front()->Apply(gs2, results, probs);
	gs2 = results.front();

	//Check:
	BOOST_TEST(!gs1.IsFinished());
	BOOST_TEST(gs1.HasTurnLimit());
	BOOST_TEST(gs1.GetTurnLimit() == 3);
	BOOST_TEST(gs1.GetTurnNumber() == 2);
	BOOST_TEST(gs2.IsFinished()); //difference
	BOOST_TEST(gs2.HasTurnLimit());
	BOOST_TEST(gs2.GetTurnLimit() == 3);
	BOOST_TEST(gs2.GetTurnNumber() == 3); //difference
	BOOST_TEST(gs2.GetGameValue(0) == 0);
	BOOST_TEST(gs2.GetGameValue(1) == 0);
}


BOOST_AUTO_TEST_CASE(TestMoraleCheckPreservesTurnNumber)
{
	//Test that the morale check command preserves the turn
	// limit and current turn number value.

	Unit unit = exampleUnit;
	unit.modelsLostThisPhase = 2;

	BoardState b(25, 1.0f);
	b.SetUnitOnSquare(Position(0, 0), unit, 0);
	b.SetUnitOnSquare(Position(0, 2), unit, 1);

	const int turnLimit = 4, turnNumber = 2;

	//Set up other states for each phase:
	GameState gs[4] = {
		GameState(0, 0, Phase::MOVEMENT, b, turnLimit, turnNumber),
		GameState(0, 0, Phase::SHOOTING, b, turnLimit, turnNumber),
		GameState(0, 0, Phase::CHARGE, b, turnLimit, turnNumber),
		GameState(0, 0, Phase::FIGHT, b, turnLimit, turnNumber),
	};

	for (int i = 0; i < 4; i++)
	{
		auto cmds = gs[i].GetCommands();

		for (auto pCmd : cmds)
		{
			//Apply command
			std::vector<GameState> results;
			std::vector<float> probs;
			pCmd->Apply(gs[i], results, probs);

			for (const auto& state : results)
			{
				BOOST_TEST(state.HasTurnLimit());
				BOOST_TEST(state.GetTurnLimit() == turnLimit);
				BOOST_TEST(state.GetTurnNumber() == turnNumber);
			}
		}
	}
}


BOOST_AUTO_TEST_SUITE_END();



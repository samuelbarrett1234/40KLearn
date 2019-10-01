#include "GameMechanics.h"
#include <algorithm>
#include <boost/math/distributions.hpp>


namespace c40kl
{


/// <summary>
/// Helper function for inserting states and corresponding
/// probabilities into an output distribution. This is
/// equivalent to adding results and probs to their respective
/// output vectors and then removing duplicates appropriately.
/// PRECONDITIONS: outStates.size() == outProbabilities.size()
/// && results.size() == probs.size() && sum(outProbabilities
/// ++ probs) less than or equal to 1.0f && results are unique
/// && outStates are unique.
/// </summary>
void InsertResultsToDistribution(std::vector<GameState>& outStates,
	std::vector<float>& outProbabilities,
	std::vector<GameState> results,
	std::vector<float> probs);


float BinomialProbability(int n, int r, float p)
{
	const auto dist = boost::math::binomial_distribution<float>((float)n, p);

	return boost::math::pdf(dist, r);
}


void ApplyCommand(GameCommandPtr pCmd,
	const std::vector<GameState>& inStates,
	const std::vector<float>& inProbabilities,
	std::vector<GameState>& outStates,
	std::vector<float>& outProbabilities)
{
	C40KL_ASSERT_PRECONDITION(inStates.size() == inProbabilities.size(),
		"Need same number of states as probabilities");
	
	C40KL_ASSERT_PRECONDITION(pCmd != nullptr, "Need a valid command.");

	C40KL_ASSERT_PRECONDITION(outStates.empty() && outProbabilities.empty(),
		"Output parameters must start empty.");

	const size_t n = inStates.size();

	for (size_t i = 0; i < n; i++)
	{
		//Can only apply commands to states which are not finished
		if (!inStates[i].IsFinished())
		{
			std::vector<GameState> results;
			std::vector<float> probs;

			//Apply to get states and probabilities of result of command
			pCmd->Apply(inStates[i], results, probs);

			C40KL_ASSERT_INVARIANT(results.size() == probs.size(),
				"Need to return valid state distribution.");

			//The law of total probability
			const float p = inProbabilities[i];
			for (size_t j = 0; j < probs.size(); j++)
			{
				probs[j] *= p;
			}

			//Now, output all results, ensuring uniqueness!

			InsertResultsToDistribution(outStates, outProbabilities,
				results, probs);
		}
		else
		{
			//If state is finished then don't apply the command
			// and just directly add the input state, ensuring
			// uniqueness:

			InsertResultsToDistribution(outStates, outProbabilities,
				{ inStates[i] }, { inProbabilities[i] });
		}
	}

	//Check this right at the end
	C40KL_ASSERT_INVARIANT(outStates.size() == outProbabilities.size(),
		"States and probabilities must tie up.");
}


bool HasStandardRangedWeapon(const Unit& unit)
{
	//All of these things have to be strictly positive
	// in order to hope to cause any damage against
	// anything:
	return (unit.rg_range > 0 && unit.rg_s > 0
		&& unit.rg_dmg > 0 && unit.rg_shots > 0);
}


bool HasStandardMeleeWeapon(const Unit& unit)
{
	//All of these things have to be strictly positive
	// in order to hope to cause any damage against
	// anything:
	return (unit.ml_s > 0 && unit.ml_dmg > 0 && unit.a > 0);
}


float GetPenetrationProbability(int hitSkill, int wpnS, int wpnAp, int targetT, int targetSv, int targetInv)
{
	C40KL_ASSERT_PRECONDITION(
		hitSkill > 0
		&& hitSkill <= 7
		&& wpnS > 0
		&& targetT > 0
		&& targetSv > 0
		&& targetSv <= 7
		&& targetInv > 0
		&& targetInv <= 7,
		"Weapon statistics must be in a valid range."
	);

	float pHit = (7.0f - hitSkill) / 6.0f;

	float strengthRatio = (float)wpnS / (float)targetT;
	float pWound = 0.5;
	if (strengthRatio >= 2.0f)
		pWound = 5.0f / 6.0f;
	else if (strengthRatio > 1.0f)
		pWound = 4.0f / 6.0f;
	else if (strengthRatio <= 0.5f)
		pWound = 1.0f / 6.0f;
	else if (strengthRatio < 1.0f)
		pWound = 2.0f / 6.0f;
	//Else it will be left as 3 / 6

	float pArmourSv = std::max((7.0f - targetSv + wpnAp) / 6.0f, 0.0f);
	float pInvSv = (7.0f - targetInv) / 6.0f;
	//We only get to make one saving throw:
	float pOverallSv = std::max(pArmourSv, pInvSv);

	return pHit * pWound * (1.0f - pOverallSv);
}


void ResolveRawShootingDamage(const Unit& shooter, const Unit& target, float distanceApart,
	std::vector<Unit>& results, std::vector<float>& probabilities)
{
	C40KL_ASSERT_PRECONDITION(distanceApart <= shooter.rg_range, "Weapon needs to be in range.");
	C40KL_ASSERT_PRECONDITION(HasStandardRangedWeapon(shooter), "Shooter needs a ranged weapon.");

	int hitSkill = shooter.bs;

	//Heavy weapons and movement don't mix!
	if (shooter.rg_is_heavy && shooter.movedThisTurn)
		hitSkill = 6;

	//Get damage
	//Don't forget that damage doesn't spill over
	// per model, hence clip the damage as so:
	const int dmg = std::min(shooter.rg_dmg, target.w);

	int numShots = shooter.rg_shots * shooter.count;

	//Rapid fire:
	if (shooter.rg_is_rapid && distanceApart <= 0.5f * shooter.rg_range)
		numShots *= 2;

	//Penetration probability:
	const float pPen = GetPenetrationProbability(hitSkill, shooter.rg_s,
		shooter.rg_ap, target.t, target.sv, target.inv);

	//Successful attack distribution
	const auto dist = boost::math::binomial_distribution<float>((float)numShots, pPen);
	
	//Each different number of shots represents
	// a different resulting target state
	for (int i = 0; i <= numShots; i++)
	{
		Unit newTarget = target;

		//Apply damage (make sure total_w doesn't go negative).
		newTarget.total_w -= dmg * i;
		if (newTarget.total_w < 0)
			newTarget.total_w = 0;

		//Compute the new number of models in the unit (wounds
		// are applied to a single model until that model is
		// destroyed and then spill over into the next model.)
		newTarget.count = newTarget.total_w / newTarget.w;
		if (newTarget.total_w % newTarget.w != 0)
			newTarget.count++;

		//Stack losses:
		newTarget.modelsLostThisPhase += (target.count - newTarget.count);

		//Compute the probability of achieving this number
		// of penetrating shots
		const float probOfResult = boost::math::pdf(dist, i);

		//Note: we need to make sure that the targets
		// we return are distinct. The only way this
		// would fail is if different numbers of shots
		// all resulted in reducing the target wounds
		// to zero. Hence check if the target is a change
		// from the last one.
		if (results.empty() || results.back() != newTarget)
		{
			results.push_back(newTarget);
			probabilities.push_back(probOfResult);
		}
		else
		{
			probabilities.back() += probOfResult;
		}
	}
}


void ResolveRawMeleeDamage(const Unit& fighter, const Unit& target,
	std::vector<Unit>& results, std::vector<float>& probabilities)
{
	C40KL_ASSERT_PRECONDITION(HasStandardMeleeWeapon(fighter), "Fighter needs a melee weapon.");

	//Get damage
	//Don't forget that damage doesn't spill over
	// per model, hence clip the damage as so:
	const int dmg = std::min(fighter.ml_dmg, target.w);

	const int numHits = fighter.a * fighter.count;

	//Penetration probability:
	const float pPen = GetPenetrationProbability(fighter.ws, fighter.ml_s,
		fighter.ml_ap, target.t, target.sv, target.inv);

	//Successful attack distribution
	const auto dist = boost::math::binomial_distribution<float>((float)numHits, pPen);

	//Each different number of shots represents
	// a different resulting target state
	for (int i = 0; i <= numHits; i++)
	{
		Unit newTarget = target;

		//Apply damage (make sure total_w doesn't go negative).
		newTarget.total_w -= dmg * i;
		if (newTarget.total_w < 0)
			newTarget.total_w = 0;

		//Compute the new number of models in the unit (wounds
		// are applied to a single model until that model is
		// destroyed and then spill over into the next model.)
		newTarget.count = newTarget.total_w / newTarget.w;
		if (newTarget.total_w % newTarget.w != 0)
			newTarget.count++;

		//Stack losses:
		newTarget.modelsLostThisPhase += (target.count - newTarget.count);

		//Compute the probability of achieving this number
		// of penetrating shots
		const float probOfResult = boost::math::pdf(dist, i);

		//Note: we need to make sure that the targets
		// we return are distinct. The only way this
		// would fail is if different numbers of shots
		// all resulted in reducing the target wounds
		// to zero. Hence check if the target is a change
		// from the last one.
		if (results.empty() || results.back() != newTarget)
		{
			results.push_back(newTarget);
			probabilities.push_back(probOfResult);
		}
		else
		{
			probabilities.back() += probOfResult;
		}
	}
}


void GetFightableUnits(const BoardState& board, int team, PositionArray& outPositions)
{
	//Get all units for this team:
	const auto units = board.GetAllUnits(team);
	const auto unitStats = board.GetAllUnitStats(team);
	const auto targets = board.GetAllUnits(1 - team);

	C40KL_ASSERT_INVARIANT(units.size() == unitStats.size(),
		"Unit positions and unit stats must tie up.");

	//Reserve once to prevent multiple allocations in the loop:
	outPositions.reserve(units.size());

	//Consider each (allied) unit for fighting
	for (size_t i = 0; i < units.size(); i++)
	{
		const auto& unitPos = units[i];
		const auto& stats = unitStats[i];

		//We can't fight if we don't have a melee weapon
		if (HasStandardMeleeWeapon(stats) && !stats.foughtThisTurn)
		{
			for (const auto& targetPos : targets)
			{
				//Must be adjacent:
				if (std::abs(unitPos.first - targetPos.first) <= 1
					&& std::abs(unitPos.second - targetPos.second) <= 1)
				{
					//Success! This is a "fightable" unit:
					outPositions.push_back(unitPos);
					break;
				}
			}
		}
	}
}


void InsertResultsToDistribution(std::vector<GameState>& outStates,
	std::vector<float>& outProbabilities,
	std::vector<GameState> results,
	std::vector<float> probs)
{
	//Check these preconditions:
	C40KL_ASSERT_PRECONDITION(outStates.size() == outProbabilities.size()
		&& results.size() == probs.size(),
		"Probabilities need to match states.");

	//Note that we can assume that the states in 'results' are unique,
	// so there is no point checking for duplicates against them (so
	// basically we only need to check duplicates for the states already
	// in the outStates array before we start).
	const size_t startOutStatesSize = outStates.size();

	//Insert everything - we will remove duplicates next.
	outStates.insert(outStates.end(), results.begin(), results.end());
	outProbabilities.insert(outProbabilities.end(), probs.begin(), probs.end());

	C40KL_ASSERT_INVARIANT(outStates.size() == outProbabilities.size(),
		"Output distribution sizes need to tie up.");

	//Now erase any element whose index is greater than or equal to
	// startOutStatesSize if it is equal to a state with index less
	// than startOutStatesSize

	size_t numDuplicates = 0;
	for (size_t j = 0; j < startOutStatesSize; j++)
	{
		for (size_t k = startOutStatesSize; k < outStates.size() - numDuplicates; k++)
		{
			if (outStates[j] == outStates[k])
			{
				// 'Transer' the probability of the duplicate state
				// to the first copy. This is an important step which
				// ensures that the probabilities add up to one after
				// the duplicates are erased.
				outProbabilities[j] += outProbabilities[k];

				//Swap the duplicates with the end
				// element so we can erase them all
				// in one go (swap and pop):
				numDuplicates++;

				//Move the state at index k to the back (at swapIndex)
				// so we can remove all duplicates in one go
				const size_t swapIndex = outStates.size() - numDuplicates;

				C40KL_ASSERT_INVARIANT(k <= swapIndex
					&& k < outStates.size()
					&& swapIndex < outStates.size()
					&& k >= startOutStatesSize
					&& numDuplicates <= outStates.size() - startOutStatesSize,
					"Duplicate removing swap-and-pop invariant was false.");

				if (swapIndex != k)
				{
					std::swap(outStates[k], outStates[swapIndex]);
					std::swap(outProbabilities[k],
						outProbabilities[swapIndex]);
				}
			}
		}
	}

	C40KL_ASSERT_INVARIANT(outStates.size() == outProbabilities.size(),
		"States and probabilities must tie up.");

	const size_t duplicateOffset = outStates.size() - numDuplicates;
	outStates.erase(outStates.begin() + duplicateOffset, outStates.end());
	outProbabilities.erase(outProbabilities.begin() + duplicateOffset, outProbabilities.end());
}


} // namespace c40kl



#include "GameMechanics.h"
#include <algorithm>
#include <boost/math/distributions.hpp>


namespace c40kl
{


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

			//The law of total probability
			const float p = inProbabilities[i];
			for (size_t j = 0; j < probs.size(); j++)
			{
				probs[j] *= p;
			}

			//Add each state in turn:
			for (size_t j = 0; j < results.size(); j++)
			{
				bool bAddedYet = false;

				//Ensure that we never add duplicate states:
				for (size_t k = 0; k < outStates.size(); k++)
				{
					//Don't add duplicate states!
					if (outStates[k] == results[j])
					{
						outProbabilities[k] += probs[j];
						bAddedYet = true;
						break;
					}
				}

				//Else we have a new state!
				if (!bAddedYet)
				{
					outStates.push_back(results[j]);
					outProbabilities.push_back(probs[j]);
				}
			}
		}
		else
		{
			//If state is finished then ignore the command

			bool bAddedYet = false;

			//Ensure that we never add duplicate states:
			for (size_t j = 0; j < outStates.size(); j++)
			{
				//Don't add duplicate states!
				if (outStates[j] == inStates[i])
				{
					outProbabilities[j] += inProbabilities[i];
					bAddedYet = true;
					break;
				}
			}

			//Else we have a new state!
			if (!bAddedYet)
			{
				outStates.push_back(inStates[i]);
				outProbabilities.push_back(inProbabilities[i]);
			}
		}
	}
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
	int dmg = shooter.rg_dmg;

	//Don't forget that damage doesn't spill over
	// per model, hence clip the damage as so:
	dmg = std::min(dmg, target.w);

	int numShots = shooter.rg_shots * shooter.count;

	//Rapid fire:
	if (shooter.rg_is_rapid && distanceApart <= 0.5f * shooter.rg_range)
		numShots *= 2;

	//Penetration probability:
	float pPen = GetPenetrationProbability(hitSkill, shooter.rg_s,
		shooter.rg_ap, target.t, target.sv, target.inv);

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
		const float probOfResult = BinomialProbability(numShots, i, pPen);

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
	int dmg = fighter.ml_dmg;

	//Don't forget that damage doesn't spill over
	// per model, hence clip the damage as so:
	dmg = std::min(dmg, target.w);

	int numHits = fighter.a * fighter.count;

	//Penetration probability:
	float pPen = GetPenetrationProbability(fighter.ws, fighter.ml_s,
		fighter.ml_ap, target.t, target.sv, target.inv);

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
		const float probOfResult = BinomialProbability(numHits, i, pPen);

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


} // namespace c40kl



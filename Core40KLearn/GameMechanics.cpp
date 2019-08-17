#include "GameMechanics.h"
#include <algorithm>


namespace c40kl
{


int nCr(int n, int r)
{
	C40KL_ASSERT_PRECONDITION(n > 0 && r <= n && r >= 0,
		"Need valid nCr arguments.");

	int nFact = 1, rFact = 1, nmrFact = 1;
	for (int i = 2; i <= n; i++)
	{
		nFact *= i;
		if (i <= r)
			rFact *= i;
		if (i <= n - r)
			nmrFact *= i;
	}
	return nFact / (rFact * nmrFact);
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

	std::vector<float> probs;
	for (size_t i = 0; i < n; i++)
	{
		//Can only apply commands to states which are not finished
		if (!inStates[i].IsFinished())
		{
			//Apply to get states and probabilities of result of command
			pCmd->Apply(inStates[i], outStates, probs);

			//The law of total probability
			const float p = inProbabilities[i];
			for (size_t j = 0; j < probs.size(); j++)
				probs[j] *= p;

			outProbabilities.insert(outProbabilities.end(), probs.begin(), probs.end());

			//Clear for next round of the loop
			probs.clear();
		}
		else
		{
			//If state is finished then ignore the command
			outStates.push_back(inStates[i]);
			outProbabilities.push_back(inProbabilities[i]);
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
		const float probOfResult = nCr(numShots, i)
			* std::pow(pPen, i) * std::pow(1.0f - pPen, numShots - i);

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
		const float probOfResult = nCr(numHits, i)
			* std::pow(pPen, i) * std::pow(1.0f - pPen, numHits - i);

		//Note: we need to make sure that the targets
		// we return are distinct. The only way this
		// would fail is if different numbers of shots
		// all resulted in reducing the target wounds
		// to zero. Hence check if the target is a change
		// from the last one.
		if (results.back() != newTarget)
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


} // namespace c40kl



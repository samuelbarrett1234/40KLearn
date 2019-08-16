#pragma once


#include "Unit.h"
#include "GameState.h"


namespace c40kl
{


/// <summary>
/// Apply the given command to the given
/// state distribution, resulting in another
/// state distribution.
/// </summary>
/// <param name="pCmd">The command to apply (must be non-null).</param>
/// <param name="inStates">The array of states; the action will be applied to each.</param>
/// <param name="inProbabilities">The probabilities associated with each state; must be an equally-sized array.</param>
/// <param name="outStates">Resulting states from the action will be written to this array.</param>
/// <param name="outProbabilities">The probabilities associated with each state in outStates will be written to this array.</param>
void ApplyCommand(GameCommandPtr pCmd,
	const std::vector<GameState>& inStates,
	const std::vector<float>& inProbabilities,
	std::vector<GameState>& outStates,
	std::vector<float>& outProbabilities);


/// <summary>
/// Determine if the given unit has a standard ranged weapon.
/// Standard ranged weapons are fully defined by range/strength/ap/damage/numshots
/// and require no special rules for firing.
/// </summary>
bool HasStandardRangedWeapon(const Unit& unit);


/// <summary>
/// Determine if the given unit has a standard ranged weapon.
/// Standard ranged weapons are fully defined by range/strength/ap/damage/numshots
/// and require no special rules for firing.
/// </summary>
bool HasStandardMeleeWeapon(const Unit& unit);


/// <summary>
/// Return the probability of a shot causing damage,
/// given all the relevant information.
/// </summary>
float GetPenetrationProbability(int hitSkill, int wpnS, int wpnAp,
	int targetT, int targetSv, int targetInv);


/// <summary>
/// Let the shooter unit fire at the target unit
/// and inflict any damage on the target unit by
/// modifying its stats.
/// Does not set firedThisTurn or anything like that.
/// NOTE: this function returns distinct units as the
/// result, i.e. it won't return the same target unit
/// twice.
/// </summary>
void ResolveRawShootingDamage(const Unit& shooter, const Unit& target, float distanceApart,
	std::vector<Unit>& results, std::vector<float>& probabilities);


} // namespace c40kl



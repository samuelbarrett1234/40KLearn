#pragma once


#include "Utility.h"
#include <vector>


namespace c40kl
{


struct Unit
{
	String name;
	int count,
		movement,
		ws,
		bs,
		t,
		w,
		total_w,
		a,
		ld,
		sv,
		inv,
		rg_range,
		rg_s,
		rg_ap,
		rg_dmg,
		rg_shots,
		ml_s,
		ml_ap,
		ml_dmg,
		modelsLostThisPhase;
	bool rg_is_rapid,
		rg_is_heavy,
		movedThisTurn,
		firedThisTurn,
		attemptedChargeThisTurn,
		successfulChargeThisTurn,
		foughtThisTurn,
		movedOutOfCombatThisTurn;
};



class C40KL_API UnitArray : public std::vector<Unit>
{ };


} // namespace c40kl



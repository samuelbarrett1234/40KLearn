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

	bool operator == (const Unit& other) const
	{
		return (name == other.name
			&& count == other.count
			&& movement == other.movement
			&& ws == other.ws
			&& bs == other.bs
			&& t == other.t
			&& w == other.w
			&& total_w == other.total_w
			&& a == other.a
			&& ld == other.ld
			&& sv == other.sv
			&& inv == other.inv
			&& rg_range == other.rg_range
			&& rg_s == other.rg_s
			&& rg_ap == other.rg_ap
			&& rg_dmg == other.rg_dmg
			&& rg_shots == other.rg_shots
			&& ml_s == other.ml_s
			&& ml_ap == other.ml_ap
			&& ml_dmg == other.ml_dmg
			&& modelsLostThisPhase == other.modelsLostThisPhase
			&& rg_is_rapid == other.rg_is_rapid
			&& rg_is_heavy == other.rg_is_heavy
			&& movedThisTurn == other.movedThisTurn
			&& firedThisTurn == attemptedChargeThisTurn
			&& successfulChargeThisTurn == other.successfulChargeThisTurn
			&& foughtThisTurn == other.foughtThisTurn
			&& movedOutOfCombatThisTurn == other.movedOutOfCombatThisTurn);
	}

	bool operator != (const Unit& other) const
	{
		return !(*this == other);
	}
};



class C40KL_API UnitArray : public std::vector<Unit>
{ };


} // namespace c40kl



#pragma once


#include "Utility.h"
#include <vector>


namespace c40kl
{


struct Unit
{
	String name;
	int count = 0,
		movement = 0,
		ws = 0,
		bs = 0,
		t = 0,
		w = 0,
		total_w = 0,
		a = 0,
		ld = 0,
		sv = 0,
		inv = 0,
		rg_range = 0,
		rg_s = 0,
		rg_ap = 0,
		rg_dmg = 0,
		rg_shots = 0,
		ml_s = 0,
		ml_ap = 0,
		ml_dmg = 0,
		modelsLostThisPhase = 0;
	bool rg_is_rapid = false,
		rg_is_heavy = false,
		movedThisTurn = false,
		firedThisTurn = false,
		attemptedChargeThisTurn = false,
		successfulChargeThisTurn = false,
		foughtThisTurn = false,
		movedOutOfCombatThisTurn = false;

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



#include "BoostPython.h"
#include <Unit.h>
using namespace c40kl;


class_<UnitArray>("UnitArray")
	.def(vector_indexing_suite<UnitArray>());


class_<Unit>("Unit")
	.def_readwrite("name", &Unit::name)
	.def_readwrite("count", &Unit::count)
	.def_readwrite("movement", &Unit::movement)
	.def_readwrite("ws", &Unit::ws)
	.def_readwrite("bs", &Unit::bs)
	.def_readwrite("t", &Unit::t)
	.def_readwrite("w", &Unit::w)
	.def_readwrite("total_w", &Unit::total_w)
	.def_readwrite("a", &Unit::a)
	.def_readwrite("ld", &Unit::ld)
	.def_readwrite("sv", &Unit::sv)
	.def_readwrite("inv", &Unit::inv)
	.def_readwrite("rg_range", &Unit::rg_range)
	.def_readwrite("rg_s", &Unit::rg_s)
	.def_readwrite("rg_ap", &Unit::rg_ap)
	.def_readwrite("rg_dmg", &Unit::rg_dmg)
	.def_readwrite("rg_shots", &Unit::rg_shots)
	.def_readwrite("ml_s", &Unit::ml_s)
	.def_readwrite("ml_ap", &Unit::ml_ap)
	.def_readwrite("ml_dmg", &Unit::ml_dmg)
	.def_readwrite("models_lost_this_phase", &Unit::modelsLostThisPhase)
	.def_readwrite("rg_is_rapid", &Unit::rg_is_rapid)
	.def_readwrite("rg_is_heavy", &Unit::rg_is_heavy)
	.def_readwrite("moved_this_turn", &Unit::movedThisTurn)
	.def_readwrite("fired_this_turn", &Unit::firedThisTurn)
	.def_readwrite("attempted_charge_this_turn", &Unit::attemptedChargeThisTurn)
	.def_readwrite("successful_charge_this_turn", &Unit::successfulChargeThisTurn)
	.def_readwrite("fought_this_turn", &Unit::foughtThisTurn)
	.def_readwrite("moved_out_of_combat_this_turn", &Unit::movedOutOfCombatThisTurn)
	.def(self == self);
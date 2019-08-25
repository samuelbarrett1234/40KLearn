#include "BoostPython.h"
#include <IGameCommand.h>
using namespace c40kl;


class_<GameCommandArray>("GameCommandArray")
	.def(vector_indexing_suite<GameCommandArray>());


enum_<CommandType>("CommandType")
	.value("UNIT_ORDER", CommandType::UNIT_ORDER)
	.value("END_PHASE", CommandType::END_PHASE)
	.value("HELPER", CommandType::HELPER);


class_<IGameCommand, shared_ptr<IGameCommand>>("IGameCommand")
	.def("apply", &IGameCommand::Apply)
	.def("equals", &IGameCommand::Equals)
	.def("__str__", &IGameCommand::ToString)
	.def("get_type", &IGameCommand::GetType);


class_<IUnitOrderCommand, bases<IGameCommand>>("IUnitOrderCommand")
	.def("get_source_position", &IUnitOrderCommand::GetSourcePosition)
	.def("get_target_position", &IUnitOrderCommand::GetTargetPosition);



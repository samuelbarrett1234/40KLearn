#include "BoostPython.h"
#include <IGameCommand.h>
#include <GameState.h>
using namespace c40kl;


//Note: this function is to allow Python users to call
// get_source_position on any command object, throwing
// an exception if the command does not inherit from
// IUnitOrderCommand.
Position GetSourcePositionOptional(const IGameCommand* _pCmd)
{
	if (auto pCmd = dynamic_cast<const IUnitOrderCommand*>(_pCmd))
	{
		return pCmd->GetSourcePosition();
	}
	else throw std::runtime_error("Command object was not a unit order command.");
}


//Note: this function is to allow Python users to call
// get_target_position on any command object, throwing
// an exception if the command does not inherit from
// IUnitOrderCommand.
Position GetTargetPositionOptional(const IGameCommand* _pCmd)
{
	if (auto pCmd = dynamic_cast<const IUnitOrderCommand*>(_pCmd))
	{
		return pCmd->GetTargetPosition();
	}
	else throw std::runtime_error("Command object was not a unit order command.");
}


void ExportCommands()
{
	class_<GameCommandArray>("GameCommandArray")
		.def(vector_indexing_suite<GameCommandArray>());
	

	enum_<CommandType>("CommandType")
		.value("UNIT_ORDER", CommandType::UNIT_ORDER)
		.value("END_PHASE", CommandType::END_PHASE)
		.value("HELPER", CommandType::HELPER);


	class_<IGameCommand, std::shared_ptr<IGameCommand>, boost::noncopyable>("IGameCommand", no_init)
		.def("apply", pure_virtual(&IGameCommand::Apply))
		.def("equals", pure_virtual(&IGameCommand::Equals))
		.def("__str__", pure_virtual(&IGameCommand::ToString))
		.def("get_type", pure_virtual(&IGameCommand::GetType))
		.def("get_source_position", &GetSourcePositionOptional)
		.def("get_target_position", &GetTargetPositionOptional);
}



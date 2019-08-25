#include "BoostPython.h"
#include <IGameCommand.h>
using namespace c40kl;


struct CommandWrapper :
	public IGameCommand, wrapper<IGameCommand>
{
	void apply(const GameState& state, std::vector<GameState>& outStates,
		std::vector<float>& outDistribution) const
	{
		get_override("apply")(state, outStates, outDistribution);
	}

	bool equals(GameCommandPtr pCmd) const
	{
		return get_override("equals")(*pCmd);
	}

	String to_string() const
	{
		return get_override("__str__")();
	}

	CommandType get_type() const
	{
		return get_override("get_type")();
	}
};


void ExportCommands()
{
	class_<GameCommandArray>("GameCommandArray")
		.def(vector_indexing_suite<GameCommandArray>());
	

	enum_<CommandType>("CommandType")
		.value("UNIT_ORDER", CommandType::UNIT_ORDER)
		.value("END_PHASE", CommandType::END_PHASE)
		.value("HELPER", CommandType::HELPER);


	class_<CommandWrapper, std::shared_ptr<IGameCommand>>("IGameCommand", no_init)
		.def("apply", pure_virtual(&IGameCommand::Apply))
		.def("equals", pure_virtual(&IGameCommand::Equals))
		.def("__str__", pure_virtual(&IGameCommand::ToString))
		.def("get_type", pure_virtual(&IGameCommand::GetType));


	class_<IUnitOrderCommand, bases<IGameCommand>>("IUnitOrderCommand")
		.def("get_source_position", pure_virtual(&IUnitOrderCommand::GetSourcePosition))
		.def("get_target_position", pure_virtual(&IUnitOrderCommand::GetTargetPosition));
}



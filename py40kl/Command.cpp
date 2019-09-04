#include "BoostPython.h"
#include "CommandWrapper.h"
#include <IGameCommand.h>
using namespace c40kl;


void ExportCommands()
{
	enum_<CommandType>("CommandType")
		.value("UNIT_ORDER", CommandType::UNIT_ORDER)
		.value("END_PHASE", CommandType::END_PHASE)
		.value("HELPER", CommandType::HELPER);


	class_<CommandWrapper, boost::noncopyable>("GameCommand", no_init)
		.def("apply", &CommandWrapper::Apply)
		.def(self == self)
		.def("__str__", &CommandWrapper::ToString)
		.def("get_type", &CommandWrapper::GetType)
		.def("get_source_position", &CommandWrapper::GetSourcePosition)
		.def("get_target_position", &CommandWrapper::GetTargetPosition);


	class_<std::vector<CommandWrapper>>("GameCommandArray")
		.def(vector_indexing_suite<std::vector<CommandWrapper>>());
}



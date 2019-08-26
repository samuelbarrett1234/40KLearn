#include "BoostPython.h"
#include <Utility.h>
using namespace c40kl;


void ExportUtilities()
{
    //TODO: export a function converting Python lists of floats
    // to vector, to allow the expand() function to work!
    
	class_<std::vector<int>>("IntArray")
		.def(vector_indexing_suite<std::vector<int>>());


	class_<std::vector<float>>("FloatArray")
		.def(vector_indexing_suite<std::vector<float>>());


	enum_<Phase>("Phase")
		.value("MOVEMENT", Phase::MOVEMENT)
		.value("SHOOTING", Phase::SHOOTING)
		.value("CHARGE", Phase::CHARGE)
		.value("FIGHT", Phase::FIGHT);


	class_<Position>("Position", init<int, int>())
		.def_readwrite("x", &Position::first)
		.def_readwrite("y", &Position::second)
		.def(self == self);


	class_<PositionArray>("PositionArray")
		.def(vector_indexing_suite<PositionArray>());
}



	
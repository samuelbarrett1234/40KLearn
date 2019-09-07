#include "BoostPython.h"


BOOST_PYTHON_MODULE(py40kl)
{
	ExportUtilities();
	ExportUnits();
	ExportBoardState();
	ExportGameState();
	ExportCommands();
	ExportMCTS();
	ExportUCB1PolicyStrategy();
	ExportUniformRandomEstimator();
}



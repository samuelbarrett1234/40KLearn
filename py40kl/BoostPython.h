#pragma once


#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/str.hpp>
#include <boost/python/ptr.hpp>
using namespace boost::python;


void ExportUtilities();
void ExportUnits();
void ExportBoardState();
void ExportGameState();
void ExportCommands();
void ExportMCTS();
void ExportUCB1PolicyStrategy();
void ExportUniformRandomEstimator();



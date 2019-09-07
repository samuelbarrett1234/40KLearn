#include "BoostPython.h"
#include <UniformRandomEstimator.h>
using namespace c40kl;


void ExportUniformRandomEstimator()
{
	class_<UniformRandomEstimator>("UniformRandomEstimator")
		.def("compute_value_estimate", &UniformRandomEstimator::ComputeValueEstimate);
}



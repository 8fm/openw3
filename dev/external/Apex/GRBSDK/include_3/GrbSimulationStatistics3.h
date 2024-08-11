#ifndef GRB_SIMULATION_STATISTICS
#define GRB_SIMULATION_STATISTICS

namespace physx
{

struct GrbSimulationStatistic3
{
	PxI32 curValue;
	PxI32 maxValue;
	const char *name;
	PxU32 parent;
};

struct GrbSimulationStatistics3
{
	PxU32				       numStats;
	GrbSimulationStatistic3	*  stats;
};

};

#endif
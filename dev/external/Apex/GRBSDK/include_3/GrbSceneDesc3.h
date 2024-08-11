#ifndef GRBSCENEDESC3_H
#define GRBSCENEDESC3_H

#include "PxSceneDesc.h"
#include "GrbUserContactReport.h"

namespace physx
{

class GrbSceneDesc3 : public PxSceneDesc
{
public:
	PxReal				meshCellSize;
	PxReal				particleSpacing;
	PxU32				nonPenSolverPosIterCount;
	PxU32				frictionSolverPosIterCount;
	PxU32				frictionSolverVelIterCount;
	PxReal				maxLinAcceleration;

	//contact parameters
	PxReal				skinWidth;
	PxReal				staticFriction;
	PxReal				dynamicFriction;
	PxReal				restitution;

	/**
	\brief The amount of GPU memory to allocate for scene data 
	       (shapes, actors etc).
	
	Unit is megabyte.
	Default value is 128 MB. 
	*/
	PxU32 gpuMemSceneSize;

	/**
	\brief The amount of GPU memory to allocate for temporary data
	       (broadphase pairs, contacts etc).
	
	Unit is megabyte.
	Default value is 64 MB. 
	*/
	PxU32 gpuMemTempDataSize;


	GrbUserContactReport * userContactReport;

	GrbSceneDesc3(const PxTolerancesScale & scale):
		PxSceneDesc(scale)
	{
		setToDefault(scale);
	}
	~GrbSceneDesc3()	{}

	bool isValid() const
	{
		return true;
	}

	void setToDefault(const PxTolerancesScale & scale)
	{ 
		PxSceneDesc::setToDefault(scale);

		userContactReport			= 0;

		gravity						= PxVec3(0.0f, -9.81f, 0.0f);
		meshCellSize				= 2.0f/8.0f;
		particleSpacing				= 2.0f/8.0f;
		skinWidth					= 0.01f;
		nonPenSolverPosIterCount	= 18;
		frictionSolverPosIterCount	= 6;
		frictionSolverVelIterCount	= 6;

		maxLinAcceleration			= FLT_MAX;
		staticFriction				= 0.5f;
		dynamicFriction				= 0.5f;
		restitution					= 0.0f;

		gpuMemSceneSize				= 128;
		gpuMemTempDataSize			= 64;
	}
};

};

#endif

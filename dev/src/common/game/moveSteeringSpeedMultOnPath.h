#pragma once

#include "moveSteeringTask.h"

class CMoveSTSpeedMultOnPath : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTSpeedMultOnPath, IMoveSteeringTask, 0 )

protected:

public:
	CMoveSTSpeedMultOnPath()						{}


	void				CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String				GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTSpeedMultOnPath );
	PARENT_CLASS( IMoveSteeringTask );
END_CLASS_RTTI();
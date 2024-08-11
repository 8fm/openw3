#pragma once

#include "../../common/game/moveSteeringTask.h"

///////////////////////////////////////////////////////////////////////////////

class CMoveSTApplySteeringToPlayerVariables : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTApplySteeringToPlayerVariables, IMoveSteeringTask, 0 );

public:
	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTApplySteeringToPlayerVariables );
	PARENT_CLASS( IMoveSteeringTask );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "build.h"
#include "../../common/game/moveSteeringTask.h"

class CMoveSTAvoidEnemies : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTAvoidEnemies, IMoveSteeringTask, 0 );

private:
	Float	m_headingImportance;
	Float	m_minDistanceFromEnemies;
	Bool	m_overrideSteering;

public:
	CMoveSTAvoidEnemies();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String GetTaskName() const override;

};

BEGIN_CLASS_RTTI( CMoveSTAvoidEnemies );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_headingImportance		, TXT("Heading importance")						);	
	PROPERTY_EDIT( m_minDistanceFromEnemies	, TXT("Distance from enemies")					);	
	PROPERTY_EDIT( m_overrideSteering		, TXT("Shoult task override or add steering")	);	
END_CLASS_RTTI();
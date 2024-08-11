#pragma once

#include "..\physics\physicsContactListener.h"
#include "soundSettings.h"
#include "..\core\math.h"

class CSoundContactListener : public CPhysicsContactListener
{
protected:
	virtual void OnContactThresholdFound( const Vector& position, const char* materialName0, const char* materialName1, Float mass, Float velocity )
	{
		if( velocity > SSoundSettings::m_contactSoundsMaxVelocityClamp )
		{
			velocity = SSoundSettings::m_contactSoundsMaxVelocityClamp;
		}

		CLightweightSoundEmitter( position, StringAnsi("SoundContactListener") ).Switch( "object_material", materialName0 ).Switch( "material", materialName1 ).Parameter( "phx_object_mass", mass, 0.0f ).Parameter( "phx_object_velocity", velocity, 0.0f ).Event( "collision" );

	}

public:
	CSoundContactListener() : CPhysicsContactListener( EPCL_THRESHOLD_FOUND, SSoundSettings::m_contactSoundPerTimeIntervalLimit, SSoundSettings::m_contactSoundsDistanceFromCamera * SSoundSettings::m_contactSoundsDistanceFromCamera, SSoundSettings::m_contactSoundsMinVelocityLimit ) {}
};
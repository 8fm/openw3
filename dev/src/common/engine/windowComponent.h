/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "meshComponent.h"


/// Component rendering mesh
class CWindowComponent : public CMeshComponent
{
	DECLARE_ENGINE_CLASS( CWindowComponent, CMeshComponent, 0 )

protected:
	// Properties
	Float				m_startEmissiveHour;
	Float				m_startEmissiveFadeTime;
	Float				m_endEmissiveHour;
	Float				m_endEmissiveFadeTime;
	Float				m_randomRange;

	// Vars used
	Float				m_randomizedStartHour;
	Float				m_randomizedStartHourFadeEnd;
	Float				m_randomizedEndHour;
	Float				m_randomizedEndHourFadeEnd;
	Float				m_lastVal;
	Uint32				m_tickCounter;

public:
	CWindowComponent();

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	// On tick
	virtual void OnTick( Float timeDelta );

	// Recalc effect parameter
	void RecalcAndSendEffectParameter( Float time );
};

BEGIN_CLASS_RTTI( CWindowComponent );
	PARENT_CLASS( CMeshComponent );
	PROPERTY_EDIT( m_startEmissiveHour, TXT("When emissive starts") );
	PROPERTY_EDIT( m_startEmissiveFadeTime, TXT("How quickly emissive fades in") );
	PROPERTY_EDIT( m_endEmissiveHour, TXT("When emissive ends") );
	PROPERTY_EDIT( m_endEmissiveFadeTime, TXT("How quickly emissive fades out") );
	PROPERTY_EDIT( m_randomRange, TXT("Randomized range") );
END_CLASS_RTTI();

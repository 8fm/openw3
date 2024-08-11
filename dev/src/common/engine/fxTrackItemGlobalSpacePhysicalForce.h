/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItemCurveBase.h"
#include "fxphysicalforce.h"

/// Particle emitter spawner
class CFXTrackItemGlobalSpacePhysicalForce : public CFXTrackItemCurveBase
{
	DECLARE_ENGINE_CLASS( CFXTrackItemGlobalSpacePhysicalForce, CFXTrackItem, 0 );

private:
	IFXPhysicalForce*	m_forceObject;

public:
	CFXTrackItemGlobalSpacePhysicalForce();

	//! Change name of track item
	virtual void			SetName( const String& name ){};

	//! Get name of track item
	virtual String			GetName() const { return TXT("Physics world force"); };

	virtual String			GetCurveName( Uint32 i = 0 ) const { return m_forceObject ? m_forceObject->GetParameterNames()[i] : TXT(""); }

	virtual Uint32			GetCurvesCount() const { return m_forceObject ? m_forceObject->GetParameterCount() : 0; }

	IFXPhysicalForce*		GetForceObject() const { return m_forceObject; } 

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemGlobalSpacePhysicalForce );
PARENT_CLASS( CFXTrackItem );
PROPERTY_INLINED( m_forceObject, TXT("Force phantom") );
END_CLASS_RTTI();

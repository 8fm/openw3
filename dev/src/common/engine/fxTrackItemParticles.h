/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItemCurveBase.h"
#include "fxSpawner.h"

class CParticleSystem;

/// Particle emitter spawner
class CFXTrackItemParticles : public CFXTrackItemCurveBase
{
	DECLARE_ENGINE_CLASS( CFXTrackItemParticles, CFXTrackItem, 0 );

private:
	TSoftHandle< CParticleSystem >		m_particleSystem;
	IFXSpawner*							m_spawner;

public:
	CFXTrackItemParticles();

	//! Change name of track item
	virtual void SetName( const String& name );

	//! Get name of track item
	virtual String GetName() const;

	//! Get curve name
	virtual String GetCurveName( Uint32 i = 0 ) const;

	//! Get number of curves here
	virtual Uint32 GetCurvesCount() const { return 3; }

	//! Check if track item uses component with given name
	virtual Bool UsesComponent( const CName& componentName ) const;

    const TSoftHandle< CParticleSystem >& GetParticleSystemResource() const;

	void SetSpawner( IFXSpawner* spawner );

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;

	//! Prefetch resources that are needed for scenes
	virtual void PrefetchResources(TDynArray< TSoftHandle< CResource > >& requiredResources) const override;
};

BEGIN_CLASS_RTTI( CFXTrackItemParticles );
	PARENT_CLASS( CFXTrackItem );
	PROPERTY_EDIT( m_particleSystem, TXT("Particle system") );
	PROPERTY_INLINED( m_spawner, TXT("Spawner for particle effect") );
END_CLASS_RTTI();

/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "particleComponent.h"
#include "sectorDataObjects.h"

/// Simulated mesh component created from cooked data
/// This uses different class only for more fine-grain profiling
class CParticleComponentCooked : public CParticleComponent
{
	DECLARE_ENGINE_CLASS( CParticleComponentCooked, CParticleComponent, 0 );

public:
	CParticleComponentCooked();

	// Initialize from packed form
	void SetupFromCookedData( CParticleSystem* particleSystem, const SectorData::PackedParticles& data );
};

BEGIN_CLASS_RTTI( CParticleComponentCooked );
	PARENT_CLASS( CParticleComponent );
END_CLASS_RTTI();

/// Sector streaming runtime for particle system
class CSectorDataObjectParticles : public TSectorDataObject< SectorData::PackedParticles >
{
public:
	CSectorDataObjectParticles();
	virtual ~CSectorDataObjectParticles();
	virtual EResult Stream( const Context& context, const Bool asyncPipeline ) override;
	virtual EResult Unstream( const Context& context, const Bool asyncPipeline ) override;

private:
	THandle< CEntity >			m_entity;
	CSectorDataResourceRef		m_resourceToUse;
};

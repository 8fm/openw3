/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "rigidMeshComponent.h"
#include "sectorDataObjects.h"

/// Simulated mesh component created from cooked data
/// This uses different class only for more fine-grain profiling
class CRigidMeshComponentCooked : public CRigidMeshComponent
{
	DECLARE_ENGINE_CLASS( CRigidMeshComponentCooked, CRigidMeshComponent, 0 );

public:
	CRigidMeshComponentCooked();

	// Initialize from packed form
	void SetupFromCookedData( CMesh* mesh, const SectorData::PackedRigidBody& data );
};

BEGIN_CLASS_RTTI( CRigidMeshComponentCooked );
	PARENT_CLASS( CRigidMeshComponent );
END_CLASS_RTTI();

/// Sector streaming runtime for rigid body
class CSectorDataObjectRigidBody : public TSectorDataObject< SectorData::PackedRigidBody >
{
public:
	CSectorDataObjectRigidBody();
	virtual ~CSectorDataObjectRigidBody();
	virtual EResult Stream( const Context& context, const Bool asyncPipeline ) override;
	virtual EResult Unstream( const Context& context, const Bool asyncPipeline ) override;

private:
	THandle< CEntity >			m_entity;
	CSectorDataResourceRef		m_resourceToUse;
};

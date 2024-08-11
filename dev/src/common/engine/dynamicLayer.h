/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "layer.h"

class CEntity;

/// Dynamic layer - special layer for dynamic gameplay entities
/// This class is added to support game save/load
class CDynamicLayer : public CLayer
{
	DECLARE_ENGINE_RESOURCE_CLASS( CDynamicLayer, CLayer, "w2dl", "Dynamic Layer" );

protected:
	CLayerStorage*	m_storage;			//!< Dynamic entity storage

public:
	CDynamicLayer();

	//! Sets layer storage
	void SetStorage( CLayerStorage* storage );

	// Attach entity to this layer
	void AttachEntity( CEntity* entity );

	// Detach entity from this layer
	RED_MOCKABLE void DetachEntity( CEntity* entity );

public:
	// Get layer storage for entities
	virtual CLayerStorage* GetLayerStorage() override;

protected:
	virtual void OnEntityAdded( const CEntity* entity );
	virtual void OnEntityRemoved( const CEntity* entity );

public:
	virtual Uint32 CalcObjectDynamicDataSize() const;
	virtual Uint32 CalcObjectsSize() const;
};

BEGIN_CLASS_RTTI( CDynamicLayer );
	PARENT_CLASS( CLayer );
END_CLASS_RTTI();


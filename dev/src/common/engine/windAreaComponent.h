/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "areaComponent.h"
#include "../physics/physicalCallbacks.h"


/// Component rendering mesh
class CWindAreaComponent : public CAreaComponent, public IPhysicalCollisionTriggerCallback
{
	DECLARE_ENGINE_CLASS( CWindAreaComponent, CAreaComponent, 0 );


public:
	CWindAreaComponent();

	// Attached to world
	virtual void						OnAttached( CWorld* world );

	// Detached from world
	virtual void						OnDetached( CWorld* world );

	// Serialization
	virtual void						OnSerialize( IFile& file );

	//! Serialize additional component data (if entity is templated)
	virtual void						OnSerializeAdditionalData( IFile& file );

	// End vertex edit mode
	virtual void						OnEditorEndVertexEdit() override;

private:
	void HandleOldFileVersion() const;
};

BEGIN_CLASS_RTTI( CWindAreaComponent );
PARENT_CLASS( CAreaComponent );
END_CLASS_RTTI();

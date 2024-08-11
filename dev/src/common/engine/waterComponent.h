/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "areaComponent.h"
#include "../physics/physicalCallbacks.h"


/// Component rendering mesh
class CWaterComponent : public CAreaComponent, public IPhysicalCollisionTriggerCallback
{
	DECLARE_ENGINE_CLASS( CWaterComponent, CAreaComponent, 0 );
	

public:
	CWaterComponent();	

	// Attached to world
	virtual void						OnAttached( CWorld* world ) override;

	// Detached from world
	virtual void						OnDetached( CWorld* world ) override;

	// Serialization
	virtual void						OnSerialize( IFile& file ) override;

	//! Serialize additional component data (if entity is templated)
	virtual void						OnSerializeAdditionalData( IFile& file ) override;

#ifndef NO_EDITOR
	// Vertex edit mode
	virtual void						OnEditorEndVertexEdit() override;
	virtual Bool						OnEditorVertexInsert( Int32 edge, const Vector& wishedPosition, Vector& allowedPosition, Int32& outInsertPos ) override;
	virtual Bool						OnEditorVertexDestroy( Int32 vertexIndex ) override;
	virtual void						OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition ) override;

	virtual void						EditorOnTransformChanged() override;
#endif

	virtual void						OnPropertyPostChange( CProperty* prop ) override;
	virtual void						OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

#ifndef NO_EDITOR
	virtual void						OnNavigationCookerInitialization( CWorld* world, CNavigationCookingContext* context ) override;
#endif

private:
	void								NotifyGlobalWater( Bool propertyChanged );	
	void								HandleOldFileVersion() const;
	CWorld*								m_world;
	Bool								m_transformNeedsUpdate;

};

BEGIN_CLASS_RTTI( CWaterComponent );
PARENT_CLASS( CAreaComponent );
END_CLASS_RTTI();

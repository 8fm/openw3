/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "meshComponent.h"
#include "mergedMeshBuilder.h"

class IRenderVisibilityExclusion;

/// Mesh component containing unique mesh usually resulting from merging
/// This needs to be on a layer to work
class CMergedMeshComponent : public CMeshComponent
{
	DECLARE_ENGINE_CLASS( CMergedMeshComponent, CMeshComponent, 0 )
	NO_DEFAULT_CONSTRUCTOR( CMergedMeshComponent );

public:
	CMergedMeshComponent( const THandle< CMesh > mesh, const TDynArray< GlobalVisID >& objects, const Float streamingDistance, const Uint8 renderMask );
	virtual ~CMergedMeshComponent();

	// Get number of stored object
	RED_INLINE const Uint32 GetNumObjects() const { return m_objects.Size(); }

private:
	// Objects that were merged
	TDynArray< GlobalVisID, MC_RenderVisibility >	m_objects;

	// Streaming distance override
	Float							m_streamingDistance;

	// Render mask baked in the mesh
	Uint8							m_renderMask;

	// Visibility exclusion filter
	IRenderVisibilityExclusion*		m_renderingExclusionFilter;

	//! Do not allow merging already merged meshes (to avoid recursion :P)
	virtual Bool IsMergableIntoWorldGeometry() const { return false; }

	//! No occlusion for this shit
	virtual Bool ShouldBeCookedAsOcclusionData() const { return false; }

	//! Get streaming distance, overriden
	virtual Uint32 GetMinimumStreamingDistance() const override;

protected:
	//! Attach/Detach from rendering scene (mostly for the rendering exclusion integration)
	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
};

BEGIN_CLASS_RTTI( CMergedMeshComponent );
	PARENT_CLASS( CMeshComponent );
	PROPERTY_RO( m_objects, TXT("Objects merged in this mesh") );
	PROPERTY_RO( m_renderMask, TXT("Render mask of the merged content") );
	PROPERTY_EDIT( m_streamingDistance, TXT("Custom streaming distance override") );
END_CLASS_RTTI();

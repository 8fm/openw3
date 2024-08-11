/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "mergedMeshComponent.h"

/// Merged mesh used only for shadows - has some more debug functionality
class CMergedShadowMeshComponent : public CMergedMeshComponent
{
	DECLARE_ENGINE_CLASS( CMergedShadowMeshComponent, CMergedMeshComponent, 0 );
	NO_DEFAULT_CONSTRUCTOR( CMergedShadowMeshComponent );

public:
	CMergedShadowMeshComponent( const THandle< CMesh > mesh, const TDynArray< GlobalVisID >& objects, const Float streamingDistance, const Uint8 renderMask );
	virtual ~CMergedShadowMeshComponent();

private:
	// Debug mesh
	IRenderResource*				m_compiledDebugMesh;

	//! Attach/Detach from rendering scene (debug rendering)
	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

#ifndef NO_RESOURCE_IMPORT
	// Debug drawing
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
#endif
};

BEGIN_CLASS_RTTI( CMergedShadowMeshComponent );
	PARENT_CLASS( CMergedMeshComponent );
END_CLASS_RTTI();
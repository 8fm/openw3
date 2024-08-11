/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/engine/meshComponent.h"
#include "../../common/core/events.h"

class CEdMeshEditor;
class CMeshPreviewComponent;

/// Special mesh component used to preview LOD
class CMeshPreviewComponent : public CMeshComponent, public IEdEventListener
{
	DECLARE_ENGINE_CLASS( CMeshPreviewComponent, CMeshComponent, 0 );

public:
	Bool				m_showBoundingBox;
	Bool				m_showCollision;

	Int32				m_activeCollisionShape;
	IRenderResource*	m_activeCollisionShapeDebugMesh;

public:
	CMeshPreviewComponent();
	virtual ~CMeshPreviewComponent();

	void ShowBoundingBox( Bool box ) { m_showBoundingBox = box; }
	void ShowCollision( Bool box ) { m_showCollision = box; }
	void SetActiveCollisionShape( Int32 shapeIndex );

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

};

BEGIN_CLASS_RTTI( CMeshPreviewComponent );
PARENT_CLASS( CMeshComponent );
END_CLASS_RTTI();

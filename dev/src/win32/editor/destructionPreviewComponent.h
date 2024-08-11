/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/engine/destructionComponent.h"
#include "../../common/core/events.h"

class CDestructionPreviewComponent : public CDestructionComponent, public IEdEventListener
{
	DECLARE_ENGINE_CLASS( CDestructionPreviewComponent, CDestructionComponent, 0 );

public:
	Bool				m_showBoundingBox;
	Bool				m_showCollision;

	Int32				m_activeCollisionShape;
	IRenderResource*	m_activeCollisionShapeDebugMesh;

public:
	CDestructionPreviewComponent();
	virtual ~CDestructionPreviewComponent();

	void				ShowBoundingBox( Bool box ) { m_showBoundingBox = box; }
	void				ShowCollision( Bool box ) { m_showCollision = box; }
	void				SetActiveCollisionShape( Int32 shapeIndex );

	virtual void		OnAttached( CWorld* world );
	virtual void		OnDetached( CWorld* world );

	virtual void		OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

	virtual void		DispatchEditorEvent( const CName& name, IEdEventData* data );


};

BEGIN_CLASS_RTTI( CDestructionPreviewComponent );
PARENT_CLASS( CDestructionComponent );
END_CLASS_RTTI();



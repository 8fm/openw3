/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "entity.h"
#include "component.h"

class CCurveTangentControlPointEntity;

class CCurveControlPointEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CCurveControlPointEntity, CEntity, 0 );

public:
	CCurveControlPointEntity();
	virtual ~CCurveControlPointEntity();
#ifndef NO_EDITOR
	virtual void EditorOnTransformChanged() override;
	virtual void EditorPreDeletion() override;
#endif
	virtual void OnSelectionChanged() override;
	virtual void OnDestroyed( CLayer* layer ) override;

	void Setup( CCurveEntity* curveEntity, Uint32 controlPointIndex );
	void RefreshTangentControlPoints( Uint32 controlPointIndex );
	RED_INLINE CCurveTangentControlPointEntity* GetTangentControlPoint( Uint32 tangentIndex ) { return m_tangentControlPoints[ tangentIndex ]; }
	RED_INLINE CCurveEntity* GetCurveEntity() const { return m_curveEntity; }
	Uint32 GetControlPointIndex();

private:
	void DeleteTangentControlPoints();

	CCurveEntity* m_curveEntity;
	CCurveTangentControlPointEntity* m_tangentControlPoints[2];
};

BEGIN_CLASS_RTTI( CCurveControlPointEntity )
	PARENT_CLASS( CEntity )
END_CLASS_RTTI()

//! Helper component used for editing single curve control point
class CCurveControlPointComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CCurveControlPointComponent, CComponent, 0 );

public:
	CCurveControlPointComponent();
	virtual ~CCurveControlPointComponent();
	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;

	void Setup( EShowFlags showFlags ) { m_showFlags = showFlags; }
	CCurveEntity* GetCurveEntity();

private:
	EShowFlags m_showFlags;
};

BEGIN_CLASS_RTTI( CCurveControlPointComponent )
	PARENT_CLASS( CComponent )
END_CLASS_RTTI()

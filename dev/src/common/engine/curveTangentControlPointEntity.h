/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "entity.h"
#include "component.h"

class CCurveTangentControlPointEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CCurveTangentControlPointEntity, CEntity, 0 );

public:
	CCurveTangentControlPointEntity();
	virtual ~CCurveTangentControlPointEntity();
#ifndef NO_EDITOR
	virtual void EditorOnTransformChanged() override;
#endif

	void Setup( CCurveControlPointEntity* controlPointEntity, Uint32 tangentIndex );
	CCurveEntity* GetCurveEntity() const;
	CCurveControlPointEntity* GetControlPointEntity() const;

private:
	CCurveControlPointEntity* m_controlPointEntity;
	Uint32 m_tangentIndex;
};

BEGIN_CLASS_RTTI( CCurveTangentControlPointEntity )
	PARENT_CLASS( CEntity )
END_CLASS_RTTI()

//! Helper component used for editing single curve tangent control point
class CCurveTangentControlPointComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CCurveTangentControlPointComponent, CComponent, 0 );

public:
	CCurveTangentControlPointComponent();
	virtual ~CCurveTangentControlPointComponent();
	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;

	CCurveEntity* GetCurveEntity();
	void Setup( EShowFlags showFlags ) { m_showFlags = showFlags; }

private:
	EShowFlags m_showFlags;
};

BEGIN_CLASS_RTTI( CCurveTangentControlPointComponent )
	PARENT_CLASS( CComponent )
END_CLASS_RTTI()

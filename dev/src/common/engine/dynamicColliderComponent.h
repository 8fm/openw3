/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "component.h"

struct SDynamicCollider;
class CWorld;

class CDynamicColliderComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CDynamicColliderComponent, CComponent, 0 );

public:
	Bool	m_useInWaterNormal;
	Bool	m_useInWaterDisplacement;
	Bool	m_useInGrassDisplacement;
	Bool	m_useHideFactor;

public:
	CDynamicColliderComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;
};

BEGIN_CLASS_RTTI( CDynamicColliderComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_useInWaterNormal,				TXT( "Water normals distortion." ) );
	PROPERTY_EDIT( m_useInWaterDisplacement,		TXT( "Water geometry displacement." ) );
	PROPERTY_EDIT( m_useInGrassDisplacement,		TXT( "Grass geometry displacement." ) );
	PROPERTY_EDIT( m_useHideFactor,					TXT( "Smoothly hide grass below ground." ) );
END_CLASS_RTTI();

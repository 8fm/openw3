/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "curveTangentControlPointEntity.h"
#include "curveControlPointEntity.h"
#include "curveEntity.h"
#include "renderFrame.h"
#include "world.h"

IMPLEMENT_ENGINE_CLASS( CCurveTangentControlPointEntity );

CCurveTangentControlPointEntity::CCurveTangentControlPointEntity()
{
}

CCurveTangentControlPointEntity::~CCurveTangentControlPointEntity()
{
}

CCurveEntity* CCurveTangentControlPointEntity::GetCurveEntity() const
{
	return m_controlPointEntity->GetCurveEntity();
}

CCurveControlPointEntity* CCurveTangentControlPointEntity::GetControlPointEntity() const
{
	return m_controlPointEntity;
}

void CCurveTangentControlPointEntity::Setup( CCurveControlPointEntity* controlPointEntity, Uint32 tangentIndex )
{
	m_controlPointEntity = controlPointEntity;
	m_tangentIndex = tangentIndex;

	CCurveTangentControlPointComponent* tangentControlPointComponent = CreateObject<CCurveTangentControlPointComponent>();
	tangentControlPointComponent->Setup( GetCurveEntity()->GetShowFlags() );
	AddComponent( tangentControlPointComponent );
}

#ifndef NO_EDITOR
void CCurveTangentControlPointEntity::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();
	GetCurveEntity()->OnTangentControlPointMoved( m_controlPointEntity, this, m_tangentIndex );
}
#endif

// ----------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CCurveTangentControlPointComponent );

const Uint16 g_boxIndices[]	= 
{ 
	1, 0, 4,    4, 5, 1,
	3, 1, 5,    5, 7, 3,
	0, 2, 6,    6, 4, 0,
	3, 7, 6,    6, 2, 3,
	2, 0, 1,    1, 3, 2,
	5, 4, 6,    6, 7, 5,
};

const float g_CurveTangentControlPointRadius = 0.05f;

CCurveTangentControlPointComponent::CCurveTangentControlPointComponent()
{
}

CCurveTangentControlPointComponent::~CCurveTangentControlPointComponent()
{
}

CCurveEntity* CCurveTangentControlPointComponent::GetCurveEntity()
{
	return static_cast<CCurveTangentControlPointEntity*>( GetEntity() )->GetCurveEntity();
}

void CCurveTangentControlPointComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, m_showFlags );
}

void CCurveTangentControlPointComponent::OnDetached( CWorld* world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, m_showFlags );
	TBaseClass::OnDetached( world );
}

void CCurveTangentControlPointComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	Color color = Color(55, 255, 0, 70);
	Color wireFrameColor = Color(55, 255, 0, 130);

	CCurveEntity* curveEntity = GetCurveEntity();
	if ( curveEntity->GetHoverControlPoint() == static_cast<CCurveTangentControlPointEntity*>( GetEntity() ) )
	{
		wireFrameColor = Color(255, 0, 0, 255);
	}

	// Get local-to-world transform without scale
	Matrix localToWorld;
	GetLocalToWorld( localToWorld );

	Matrix localToWorldNoScale;
	Vector scale;
	localToWorld.ExtractScale( localToWorldNoScale, scale );
	localToWorldNoScale.SetTranslation( localToWorld.GetTranslation() );

	// Get bounding box
	const Box boundingBox = Box( Vector::ZEROS, g_CurveTangentControlPointRadius );

	// Hit proxy mode
	if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
#ifndef NO_COMPONENT_GRAPH
		frame->AddDebugSolidBox( boundingBox, localToWorldNoScale, m_hitProxyId.GetColor() );
#endif
		return;
	}

	// Bright when selected
	if( ! IsSelected() )
	{
		color.R /= 2;
		color.G /= 2;
		color.B /= 2;

		wireFrameColor.R /= 2;
		wireFrameColor.G /= 2;
		wireFrameColor.B /= 2;
	}

	frame->AddDebugSolidBox( boundingBox, localToWorldNoScale, color, true );
	frame->AddDebugBox( boundingBox, localToWorldNoScale, wireFrameColor, true );

	// Line between tangent control point and control point

	CCurveControlPointEntity* controlPointEntity = static_cast<CCurveTangentControlPointEntity*>( GetEntity() )->GetControlPointEntity();

	frame->AddDebugFatLine( GetWorldPosition(), controlPointEntity->GetWorldPosition(), color, 0.005f, curveEntity->GetDrawOnTop() );
}
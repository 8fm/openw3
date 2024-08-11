/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "curveControlPointEntity.h"
#include "curveTangentControlPointEntity.h"
#include "curveEntity.h"
#include "renderFrame.h"
#include "multiCurve.h"
#include "layer.h"
#include "world.h"

namespace RenderingUtils
{
	void CreateArrow( const Vector & startPoint, const Vector & endPoint, TDynArray<Vector> & outPoints, Float length = 0.2f, Float sideSize = 0.05f );
	void RenderArrow( CRenderFrame* frame, const Color& wireframeColor, const Color& color, const TDynArray<Vector> & points, Bool overlay = false );
}

IMPLEMENT_ENGINE_CLASS( CCurveControlPointEntity );

CCurveControlPointEntity::CCurveControlPointEntity()
{
}

CCurveControlPointEntity::~CCurveControlPointEntity()
{
}

void CCurveControlPointEntity::OnDestroyed( CLayer* layer )
{
	TBaseClass::OnDestroyed( layer );
	DeleteTangentControlPoints();
}

void CCurveControlPointEntity::OnSelectionChanged()
{
	GetCurveEntity()->NotifySelectionChange();
}

void CCurveControlPointEntity::Setup( CCurveEntity* curveEntity, Uint32 controlPointIndex )
{
	m_curveEntity = curveEntity;

	CCurveControlPointComponent* controlPointComponent = CreateObject<CCurveControlPointComponent>();
	controlPointComponent->Setup( curveEntity->GetShowFlags() );
	AddComponent( controlPointComponent );

	RefreshTangentControlPoints( controlPointIndex );
}

void CCurveControlPointEntity::DeleteTangentControlPoints()
{
	if ( m_tangentControlPoints[0] )
	{
		m_tangentControlPoints[0]->Destroy();
		m_tangentControlPoints[0] = NULL;
	}

	if ( m_tangentControlPoints[1] )
	{
		m_tangentControlPoints[1]->Destroy();
		m_tangentControlPoints[1] = NULL;
	}
}

Uint32 CCurveControlPointEntity::GetControlPointIndex()
{
	return GetCurveEntity()->GetControlPointIndex( this );
}

void CCurveControlPointEntity::RefreshTangentControlPoints( Uint32 controlPointIndex )
{
	CCurveEntity* curveEntity = GetCurveEntity();
	SMultiCurve* curve = curveEntity->GetCurve();

	const Bool isSelectedForTangentEditing =
		IsSelected() ||
		( m_tangentControlPoints[0] && m_tangentControlPoints[0]->IsSelected() ) ||
		( m_tangentControlPoints[1] && m_tangentControlPoints[1]->IsSelected() );

	if ( !isSelectedForTangentEditing || curve->GetPositionInterpolationMode() != ECurveInterpolationMode_Manual )
	{
		DeleteTangentControlPoints();
		return;
	}

	// Create tangent control points and set their transforms

	for ( Uint32 i = 0; i < 2; i++ )
	{
		if ( !m_tangentControlPoints[i] )
		{
			EntitySpawnInfo info;
			info.m_entityClass = ClassID<CCurveTangentControlPointEntity>();
			m_tangentControlPoints[i] = Cast<CCurveTangentControlPointEntity>( GetLayer()->CreateEntitySync( info )  );
			m_tangentControlPoints[i]->Setup( this, i );
		}

		// Get local tangent transform

		Vector tangent;
		curve->GetControlPointTangent( controlPointIndex, i, tangent );
		Vector controlPointPosition;
		curve->GetControlPointPosition( controlPointIndex, controlPointPosition );
		
		EngineTransform tangentTransform;
		tangentTransform.SetPosition( controlPointPosition + tangent );

		// Convert to world space tangent transform

		EngineTransform tangentWorldTransform;
		curveEntity->ToWorldTransform( tangentTransform, tangentWorldTransform );

		// Update position of the tangent control point

		m_tangentControlPoints[ i ]->SetPosition( tangentWorldTransform.GetPosition() );
	}
}

#ifndef NO_EDITOR
void CCurveControlPointEntity::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();
	m_curveEntity->OnControlPointMoved( this );
}

void CCurveControlPointEntity::EditorPreDeletion()
{
	TBaseClass::EditorPreDeletion();
	m_curveEntity->OnControlPointDeleted( this );
}
#endif

// ----------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CCurveControlPointComponent );

const Uint16 g_boxIndices[]	= 
{ 
	1, 0, 4,    4, 5, 1,
	3, 1, 5,    5, 7, 3,
	0, 2, 6,    6, 4, 0,
	3, 7, 6,    6, 2, 3,
	2, 0, 1,    1, 3, 2,
	5, 4, 6,    6, 7, 5,
};

const float g_curveControlPointRadius = 0.1f;

CCurveControlPointComponent::CCurveControlPointComponent()
{
}

CCurveControlPointComponent::~CCurveControlPointComponent()
{
}

CCurveEntity* CCurveControlPointComponent::GetCurveEntity()
{
	return static_cast<CCurveControlPointEntity*>( GetEntity() )->GetCurveEntity();
}

void CCurveControlPointComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, m_showFlags );
}

void CCurveControlPointComponent::OnDetached( CWorld* world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, m_showFlags );
	TBaseClass::OnDetached( world );
}

void CCurveControlPointComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	Color color = Color( 255, 0, 255, 50 );
	Color wireFrameColor = Color( 255, 0, 255, 230 );

	CCurveEntity* curveEntity = GetCurveEntity();
	if ( curveEntity->GetHoverControlPoint() == static_cast<CCurveControlPointEntity*>( GetEntity() ) )
	{
		wireFrameColor = Color(255, 0, 0, 255);
	}

	if ( curveEntity->GetDrawArrowControlPoints() )
	{
		// Get arrow start and end position

		Vector start, end;

		const Uint32 controlPointIndex = curveEntity->GetControlPointIndex( static_cast<CCurveControlPointEntity*>( GetEntity() ) );

		SMultiCurve* curve = curveEntity->GetCurve();
		curve->GetAbsoluteControlPointPosition( controlPointIndex, end );
		
		Float controlPointTime = curve->GetControlPointTime( controlPointIndex );
		if ( controlPointIndex == 0 )
		{
			curve->GetAbsolutePosition( controlPointTime + 0.01f, start );
			start = end - ( start - end );
		}
		else
		{
			curve->GetAbsolutePosition( controlPointTime - 0.01f, start );
		}

		// Generate arrow verts

		const Float arrowLength = 0.3f;
		const Float arrowSideSize = 0.1f;

		TDynArray<Vector> arrowPoints;
		RenderingUtils::CreateArrow( start, end, arrowPoints, arrowLength, arrowSideSize );

		// Handle hit proxy mode

		if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
		{
#ifndef NO_COMPONENT_GRAPH
			RenderingUtils::RenderArrow( frame, m_hitProxyId.GetColor(), m_hitProxyId.GetColor(), arrowPoints, curveEntity->GetDrawOnTop() );
#endif
			return;
		}

		if ( !IsSelected() )
		{
			color.R /= 2;
			color.G /= 2;
			color.B /= 2;
		}

		RenderingUtils::RenderArrow( frame, wireFrameColor, color, arrowPoints, curveEntity->GetDrawOnTop() );
	}
	else
	{
		// Get local-to-world transform without scale

		Matrix localToWorld;
		GetLocalToWorld( localToWorld );

		Matrix localToWorldNoScale;
		Vector scale;
		localToWorld.ExtractScale( localToWorldNoScale, scale );
		localToWorldNoScale.SetTranslation( localToWorld.GetTranslation() );

		// Get bounding box

		const Box boundingBox = Box( Vector::ZEROS, g_curveControlPointRadius );

		// Handle hit proxy mode

		if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
		{
#ifndef NO_COMPONENT_GRAPH
			frame->AddDebugSolidBox( boundingBox, localToWorldNoScale, m_hitProxyId.GetColor(), curveEntity->GetDrawOnTop() );
#endif
			return;
		}

		if ( !IsSelected() )
		{
			color.R /= 2;
			color.G /= 2;
			color.B /= 2;
		}

		frame->AddDebugSolidBox( boundingBox, localToWorldNoScale, color, curveEntity->GetDrawOnTop() );
		frame->AddDebugBox( boundingBox, localToWorldNoScale, wireFrameColor, curveEntity->GetDrawOnTop() );
	}

	if ( IsSelected() )
	{
		const Uint32 controlPointIndex = curveEntity->GetControlPointIndex( static_cast<CCurveControlPointEntity*>( GetEntity() ) );
		SMultiCurve* curve = curveEntity->GetCurve();
		Vector pos;
		curve->GetAbsoluteControlPointPosition( controlPointIndex, pos );

		frame->AddDebugText( pos, String::Printf( TXT("%d"), controlPointIndex ), 0, -5 );
	}
}
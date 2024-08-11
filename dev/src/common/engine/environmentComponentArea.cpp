/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "environmentManager.h"
#include "environmentComponentArea.h"
#include "renderFrame.h"
#include "world.h"
#include "environmentDefinition.h"

IMPLEMENT_RTTI_ENUM( EAreaEnvironmentPointType );
IMPLEMENT_RTTI_ENUM( EAreaEnvironmentPointBlend );
IMPLEMENT_ENGINE_CLASS( SAreaEnvironmentPoint );
IMPLEMENT_ENGINE_CLASS( CAreaEnvironmentComponent );

// SAreaEnvironmentPoint
SAreaEnvironmentPoint::SAreaEnvironmentPoint()
	: m_position( 0, 0, 0, 1 )
	, m_direction( 0, 0, 0 )
	, m_type( AEPT_FadeOut )
	, m_blend( AEPB_DistanceOnly )
	, m_innerRadius( 0 )
	, m_outerRadius( 4 )
	, m_scaleX( 1 )
	, m_scaleY( 1 )
	, m_scaleZ( 1 )
	, m_useCurve( false )
	, m_blendScale( 1 )
	, m_environmentDefinition( NULL )
{
	m_curve.SetLoop( 0 );
	m_curve.AddPoint( 0, 0 );
	m_curve.AddPoint( 1, 1 );
}

// CAreaEnvironmentComponent
CAreaEnvironmentComponent::CAreaEnvironmentComponent ()
: m_environmentDefinition( NULL )
, m_world (NULL)
, m_id (-1)
, m_priority(DEFAULT_ENVIRONMENT_AREA_COMPONENT_PRIORITY)
, m_blendingDistance (1)
, m_blendingScale (1)
, m_terrainBlendingDistance (0)
, m_blendAboveAndBelow( false )
, m_blendingCurveEnabled( false )
, m_blendingCurve( SCT_Float )
{
	m_color = Color::CYAN;

	m_includedChannels = TC_Camera;
	m_excludedChannels = 0;

	m_blendingCurve.SetLoop( false );
	m_blendingCurve.AddPoint( 0, 0 );
	m_blendingCurve.AddPoint( 1, 1 );
}

CAreaEnvironmentComponent::~CAreaEnvironmentComponent()
{
}

const CAreaEnvironmentParams* CAreaEnvironmentComponent::GetParameters() const
{
	return m_environmentDefinition ? &m_environmentDefinition->GetAreaEnvironmentParams() : NULL;
}

void CAreaEnvironmentComponent::SetEnvironmentDefinition( CEnvironmentDefinition* definition )
{
	if ( m_environmentDefinition != definition )
	{
		// Deactivate the area environment if it is active
		Bool wasActive = IsActive();
		if ( wasActive )
		{
			Deactivate( true );
			wasActive = true;
		}

		// Set the new definition
		m_environmentDefinition = definition;

		// Activate it again if it was active
		if ( wasActive )
		{
			Activate( true );
		}
	}
}

#ifndef NO_EDITOR
void CAreaEnvironmentComponent::AddPoint( const SAreaEnvironmentPoint& point )
{
	m_points.PushBack( point );
	NotifyPropertiesImplicitChange();
}

void CAreaEnvironmentComponent::RemovePoint( Uint32 pointIndex )
{
	m_points.RemoveAt( pointIndex );
	NotifyPropertiesImplicitChange();
}

void CAreaEnvironmentComponent::RemoveAllPoints()
{
	m_points.Clear();
	NotifyPropertiesImplicitChange();
}

void CAreaEnvironmentComponent::UpdatePoint( Uint32 pointIndex, const SAreaEnvironmentPoint& point )
{
	m_points[pointIndex] = point;
	NotifyPropertiesImplicitChange();
}
#endif

void CAreaEnvironmentComponent::NotifyPropertiesImplicitChange()
{
	CEnvironmentManager *manager = m_world ? m_world->GetEnvironmentManager() : NULL;
	const CAreaEnvironmentParams* params = GetParameters();
	if ( manager && params )
	{
		manager->ChangeAreaEnvironmentParameters( m_id, *params );
	}
}

void CAreaEnvironmentComponent::SetActivated( bool activate )
{
	if ( activate )
	{
		Activate( true );
	}
	else
	{
		Deactivate( true );
	}
}

Bool CAreaEnvironmentComponent::Activate( bool force /*=false*/ )
{	
	CEnvironmentManager *manager = m_world ? m_world->GetEnvironmentManager() : NULL;
	if ( manager != nullptr &&  m_environmentDefinition != nullptr )
	{
		if ( !manager->AreEnvChangesDisabled() || force )
		{
			m_id = manager->ActivateAreaEnvironment( m_environmentDefinition.Get(), this, m_priority, m_blendingScale, m_blendInTime, m_id );
			LOG_ENGINE( TXT("[ENVIRONMENT TRIGGERING TEST] activated %i"), m_id );
		}
	}
	else
	{
		m_id = -1;
	}
	return -1 != m_id;
}

Bool CAreaEnvironmentComponent::IsActive() const
{
	CEnvironmentManager *manager = m_world ? m_world->GetEnvironmentManager() : NULL;
	return manager && manager->IsAreaEnvironmentActive( m_id );
}

void CAreaEnvironmentComponent::Deactivate( bool force /*=false*/ )
{
	CEnvironmentManager *manager = m_world ? m_world->GetEnvironmentManager() : NULL;
	if ( manager )
	{
		if ( !manager->AreEnvChangesDisabled() || force )
		{
			manager->DeactivateEnvironment( m_id, m_blendOutTime );
			LOG_ENGINE( TXT("[ENVIRONMENT TRIGGERING TEST] deactivated %i"), m_id );
		}
	}
}

void CAreaEnvironmentComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	m_world = world;
}

void CAreaEnvironmentComponent::OnDetached( CWorld* world )
{
	Deactivate();
	m_world = NULL;

	TBaseClass::OnDetached( world );
}

RED_DEFINE_STATIC_NAME( priority );

void CAreaEnvironmentComponent::OnPropertyPostChange( IProperty* property )
{	
	TBaseClass::OnPropertyPostChange( property );
	NotifyPropertiesImplicitChange();

#ifndef NO_EDITOR
	// If the priority changes, re-activate the environment
	if ( property->GetName() == CNAME( priority ) && IsActive() )
	{
		Deactivate();
		Activate();
	}
#endif
}

void CAreaEnvironmentComponent::DrawArea( CRenderFrame* frame, const TAreaPoints& worldPoints )
{
	TBaseClass::DrawArea( frame, worldPoints );

	// Do not render lines when rendering hit proxies
	if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
		return;
	}
	if ( m_worldPoints.Empty() )
	{
		return;
	}

	// Draw points
	for ( auto it=m_points.Begin(); it != m_points.End(); ++it )
	{
		const SAreaEnvironmentPoint& point = *it;

		// Small sphere for the point's position
		frame->AddDebugSphere( it->m_position + GetWorldPositionRef(), 0.03f, Matrix::IDENTITY, Color::CYAN );

		// Create scale matrix
		Matrix scaleMatrix = Matrix::IDENTITY;
		scaleMatrix.SetScale33( Vector( point.m_scaleX, point.m_scaleY, point.m_scaleZ ) );

		// Sphere for the inner radius
		if ( point.m_innerRadius > 0.001f )
		{
			frame->AddDebugSphere( it->m_position + GetWorldPositionRef(), point.m_innerRadius, scaleMatrix, Color::LIGHT_CYAN );
		}

		// Sphere for the outer radius
		if ( point.m_outerRadius > 0.001f )
		{
			frame->AddDebugSphere( it->m_position + GetWorldPositionRef(), point.m_outerRadius, scaleMatrix, Color::CYAN );
		}

		// Normal arrow
		if ( point.m_blend == AEPB_CameraAngleAndDistance )
		{
			Matrix localToWorldForPoint = scaleMatrix;
			localToWorldForPoint.SetTranslation( GetWorldPositionRef() + point.m_position );
			Vector forward;
			point.m_direction.ToAngleVectors( &forward, NULL, NULL );
			frame->AddDebugArrow( localToWorldForPoint, forward, point.m_outerRadius * 0.85f, Color::RED, IsSelected() );
		}
	}

	// Height vector
	Vector height = GetLocalToWorld().TransformVector( Vector( 0.f, 0.f, m_height ) );
	const Bool drawHeight = m_height > 0.0f;

	// Get color
	Color color = CalcLineColor();
	if ( !IsSelected() )
	{
		color.R /= 2;
		color.G /= 2;
		color.B /= 2;
	}

	// Draw lines for the distance from edges (TODO: draw proper connected lines at some point)
	{
		if ( !IsEnabled() )
		{
			color = Color::RED; // Draw lines RED, if area is disabled
		}
		else
		{
			color.A = 255;
		}

		TDynArray< DebugVertex > vertices;

		// Draw contour
		vertices.Resize( worldPoints.Size() * 8 );
		DebugVertex* write = vertices.TypedData();
		for ( Uint32 i=0; i<worldPoints.Size(); i++ )
		{
			Vector A = worldPoints[ i ];
			Vector B = worldPoints[ (i+1)%worldPoints.Size() ];
			Vector C = A + height;
			Vector D = B + height;
			Vector pushVector = Vector::Cross( height.Normalized3(), ( B - A ).Normalized3() ) * m_blendingDistance;

			// Line segment
			(write++)->Set( A + pushVector, color );
			(write++)->Set( B + pushVector, color );

			// Upper line
			if ( drawHeight )
			{
				// Up vector
				(write++)->Set( A + pushVector, color );
				(write++)->Set( C + pushVector, color );

				// Upper segment
				(write++)->Set( C + pushVector, color );
				(write++)->Set( D + pushVector, color );

				// Other side up vector
				(write++)->Set( B + pushVector, color );
				(write++)->Set( D + pushVector, color );
			}
		}

		// Draw lines
		const Uint32 numVertices = PtrDiffToUint32( (void*)(write - &vertices[0]) );
		ASSERT( numVertices <= vertices.Size() );
		frame->AddDebugLines( vertices.TypedData(), numVertices, false );
	}
}

void CAreaEnvironmentComponent::EnteredArea( CComponent* component )
{
	TBaseClass::EnteredArea( component );
	// ignore triggers from components, we only care about the camera
	if ( !component )
	{
		Activate();
	}
}

void CAreaEnvironmentComponent::ExitedArea( CComponent* component )
{
	// ignore triggers from components, we only care about the camera
	if ( !component )
	{
		Deactivate();
	}

	TBaseClass::ExitedArea( component );
}

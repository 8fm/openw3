/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "collisionMesh.h"
#include "../physics/physicsEngine.h"
#include "../physics/compiledCollision.h"
#include "collisionShape.h"
#include "renderer.h"
#include "renderFragment.h"
#include "renderFrame.h"
#include "collisionCache.h"
#include "collisionShape.h"

IMPLEMENT_ENGINE_CLASS( CCollisionMesh );

///////////////////////////////////////////////

// Maximum distance to render the collision mesh fragment. Just measured to the mesh's origin, so not exact
// if the collision shapes are off-center.
#define FRAGMENT_DIST_FROM_CAMERA	100.0f

///////////////////////////////////////////////


CCollisionMesh::CCollisionMesh()
	: m_debugMesh( NULL )
	, m_occlusionAttenuation( -1.0f )
	, m_occlusionDiagonalLimit( -1.0f )
	, m_swimmingRotationAxis( 0 )
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	// Listen for CollisionShapeChanged events, to catch changes to materials and such. Just needed for keeping the
	// debug mesh updated.
	SEvents::GetInstance().RegisterListener( CNAME( OnCollisionShapeChanged ), this );
#endif
}

CCollisionMesh::~CCollisionMesh()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().UnregisterListener( CNAME( OnCollisionShapeChanged ), this );
#endif

	SAFE_RELEASE( m_debugMesh );
}

void CCollisionMesh::GetAdditionalInfo( TDynArray< String >& info ) const
{
	// Pass to base class
	TBaseClass::GetAdditionalInfo( info );
}

void CCollisionMesh::GenerateFragments( CRenderFrame* frame, const RenderContext& context ) const
{
	// Check if we're too far from the camera.
	Float distanceFromCamera = frame->GetFrameInfo().m_camera.GetPosition().DistanceTo( context.m_localToWorld.TransformPoint( Vector::ZEROS ) );
	if ( distanceFromCamera > FRAGMENT_DIST_FROM_CAMERA )
		return;

	if ( m_shapes.Size() )
	{
		// Generate debug mesh if needed
		if ( !m_debugMesh )
		{
			TDynArray< DebugVertex > vertices;
			TDynArray< Uint32 > indices;

			// Generate drawing data from shapes
			for ( Uint32 i=0; i<m_shapes.Size(); i++ )
			{
				ICollisionShape* shape = m_shapes[i];
				if ( shape )
				{
					shape->GenerateDebugMesh( vertices, indices );
				}
			}

			// Upload to card
			if ( indices.Size() > 0 )
			{
				m_debugMesh = GRender->UploadDebugMesh( vertices, indices );
			}
		}

		// Generate rendering fragment
		if ( m_debugMesh )
		{
			if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
			{
				new ( frame ) CRenderFragmentDebugMesh( frame, context.m_localToWorld, m_debugMesh, context.m_hitProxyID );
			}
			else
			{
				// When drawing solid, we also want to draw the wireframe. This helps to show the actual shape, so it's not just a blob.
				if ( context.m_solid )
				{
					new ( frame ) CRenderFragmentDebugMesh( frame, context.m_localToWorld, m_debugMesh, true );
				}
				new ( frame ) CRenderFragmentDebugMesh( frame, context.m_localToWorld, m_debugMesh, false, true );
			}
		}
	}
}


ICollisionShape* CCollisionMesh::AddShape( ICollisionShape* shape )
{
	m_shapes.PushBack( shape );
#ifndef NO_EDITOR
	InvalidateCollision();
#endif
	return shape;
}

void CCollisionMesh::RemoveAll()
{
	for ( Uint32 i = 0; i < m_shapes.Size(); ++i )
	{
		m_shapes[i]->Discard();
	}
	m_shapes.Clear();

#ifndef NO_EDITOR
	InvalidateCollision();
#endif
}

void CCollisionMesh::RemoveShape( Int32 index )
{
	ICollisionShape* shape = m_shapes[index];
	m_shapes.RemoveAt( ( size_t )index );
	shape->Discard();

#ifndef NO_EDITOR
	InvalidateCollision();
#endif
}

void CCollisionMesh::Append( const CCollisionMesh& other, const Matrix& m )
{
	for ( ICollisionShape* shape : other.m_shapes )
	{
		ICollisionShape* clone = SafeCast< ICollisionShape >( shape->Clone( this ) );
		clone->GetPose() = Matrix::Mul( m, clone->GetPose() );
		AddShape( clone );
	}
}

#ifndef NO_EDITOR
void CCollisionMesh::InvalidateCollision()
{
	CResource* parentResource = Cast< CResource >( GetParent() );
	if ( parentResource )
	{
		GCollisionCache->InvalidateCollision( parentResource->GetDepotPath() );
	}

	// Clear out debug mesh, so it'll get re-created.
	SAFE_RELEASE( m_debugMesh );

	EDITOR_QUEUE_EVENT( CNAME( OnCollisionMeshChanged ), CreateEventData( this ) );
}

TDynArray< Float > CCollisionMesh::GetDensityScalers() const
{
	TDynArray< Float > result;
	for( Uint32 i = 0; i != m_shapes.Size(); ++i )
	{
		result.PushBack( m_shapes[ i ]->GetDensityScaler() );
	}
	return result;
}

void CCollisionMesh::FillDensityScalers( const TDynArray< Float >& scalers )
{
	for( Uint32 i = 0; i != m_shapes.Size(); ++i )
	{
		if( scalers.Size() > i )
		{
			m_shapes[ i ]->SetDensityScaler( scalers[ i ] );
		}
	}
}

#endif // NO_EDITOR

#if !defined(NO_EDITOR_EVENT_SYSTEM)
void CCollisionMesh::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
#ifndef NO_EDITOR
	if ( name == CNAME( OnCollisionShapeChanged ) )
	{
		ICollisionShape* shape = GetEventData< ICollisionShape* >( data );
		// If this is one of our shapes, invalidate ourselves.
		if ( m_shapes.Exist( shape ) )
		{
			InvalidateCollision();
		}
	}
#endif
}
#endif // NO_EDITOR_EVENT_SYSTEM

CompiledCollisionPtr CCollisionMesh::CompileCollision( CObject* parent ) const
{
#ifndef USE_PHYSX
	return CompiledCollisionPtr();
#else
	CompiledCollisionPtr compiledMesh( new CCompiledCollision() );

	compiledMesh->SetOcclusionAttenuation( m_occlusionAttenuation );
	compiledMesh->SetOcclusionDiagonalLimit( m_occlusionDiagonalLimit );
	compiledMesh->SetSwimmingRotationAxis( m_swimmingRotationAxis );

	Uint32 compiledCount = 0;

	for ( Uint32 i = 0; i < m_shapes.Size(); i++ )
	{		
		ICollisionShape* collisionShape = m_shapes[i];
		if ( !collisionShape ) continue;

		SCachedGeometry& geometry = compiledMesh->InsertGeometry();
		if( collisionShape->CompileShape( &geometry, parent ) )
		{
			++compiledCount;
		}
	}

	if ( !compiledCount )
	{
		return CompiledCollisionPtr();
	}

	return compiledMesh;
#endif
}


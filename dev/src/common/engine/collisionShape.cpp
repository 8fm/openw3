/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "collisionShape.h"
#include "../physics/physicsEngine.h"
#include "collisionMesh.h"
#include "collisionCache.h"


IMPLEMENT_ENGINE_CLASS( ICollisionShape );

ICollisionShape::ICollisionShape( CCollisionMesh* mesh, void* geometry, const Matrix& pose )
	: m_pose( pose ), m_densityScaler( 1.0f )
{
    // Parent to given object
    SetParent( mesh );

    // Initialize internal container
    if ( geometry )
    {
        m_geometry = geometry;
    }
}

void ICollisionShape::Invalidate()
{
	EDITOR_DISPATCH_EVENT( CNAME( OnCollisionShapeChanged ), CreateEventData( this ) );
}

const CResource* ICollisionShape::GetCollisionShapeParentResource( const CObject* object ) const
{
	if ( nullptr == object )
	{
		return nullptr;
	}

	const CObject* resource = object->GetParent();
	return Cast< const CResource > ( resource );
}

void ICollisionShape::GetShapeForNavigation( const Matrix& localToWorld, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices ) const
{
	return GetShape( localToWorld, vertices, indices );
}

void ICollisionShape::GenerateDebugMesh( TDynArray< DebugVertex >& vertices, TDynArray< Uint32 >& indices ) const
{
	TDynArray< Vector > tempVertices;
	TDynArray< Uint32 > tempIndices;

	GetShape( Matrix::IDENTITY, tempVertices, tempIndices );
	
	// Each triangle from GetShape() will create three vertices and three indices in the output.
	vertices.Reserve( vertices.Size() + tempIndices.Size() );
	indices.Reserve( indices.Size() + tempIndices.Size() );

	for ( Uint32 i = 0; i < tempIndices.Size(); i += 3 )
	{
		Uint32 color = GetDebugTriangleColor( i / 3 );

		// Generate vertices
		Uint32 baseIndex = vertices.Size();
		new ( vertices ) DebugVertex( tempVertices[tempIndices[i + 0]], color );
		new ( vertices ) DebugVertex( tempVertices[tempIndices[i + 1]], color );
		new ( vertices ) DebugVertex( tempVertices[tempIndices[i + 2]], color );

		// Generate indices
		indices.PushBack( baseIndex + 2 );
		indices.PushBack( baseIndex + 1 );
		indices.PushBack( baseIndex + 0 );
	}
}

/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "crowdArea.h"
#include "crowdManager.h"
#include "crowdEntryPoint.h"
#include "crowdAreaComponent.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/utils.h"

IMPLEMENT_ENGINE_CLASS( CCrowdArea );

void CCrowdArea::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	if ( !GGame->IsActive() )
	{
		return;
	}

	m_spawned = false;

	CCrowdManager* crowd = GCommonGame->GetSystem< CCrowdManager > ();
	if ( crowd )
	{
		crowd->RegisterArea( this );

		// For now, i'm removing it from here
		// I don't think this should be needed at all
		// ( CCrowdManager handles obstacles globally )
		// FindObstacleEdgesInArea( world, crowd );
	}
	else
	{
		ASSERT( false, TXT("WTF?") );
	}

	#ifdef DEBUG_CROWD
		for ( ComponentIterator< CAreaComponent > it( this ); it; ++it )
		{
			if ( false == ( *it )->IsA< CCrowdAreaComponent > () )
			{
				RED_LOG( Crowd, TXT("Error: %s contains component %s which is not a CCrowdAreaComponent."), GetFriendlyName().AsChar(), ( *it )->GetFriendlyName().AsChar() );
				if ( !( *it )->IsA< CCrowdAreaComponent > () )
				{
					DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT("Crowd"), TXT("Error: %s contains component %s which is not a CCrowdAreaComponent."), GetFriendlyName().AsChar(), ( *it )->GetFriendlyName().AsChar() );  
				}
			}
		}
	#endif
}

void CCrowdArea::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );																				   

	if ( !GGame->IsActive() )
	{
		return;
	}
}

void CCrowdArea::FindObstacleEdgesInArea( CWorld* world, CCrowdManager* crowd )
{	
	// TODO:
	// 1. Is this even needed at all?
	// 2. CCroiwdArea is not a Box2, it can have any shape, so if answer to (1) is yes, then it needs to be supported here.

	// obstacles from area edges
	static const Int16 MAX_EDGES = 4; 
	const CCrowdAreaComponent* areaComponent = FindComponent< const CCrowdAreaComponent >( );
	Box2 bBox = GetBoundingBox2( );

	Vector2 points[ MAX_EDGES ];
	Vector2 edges[ MAX_EDGES ];

	points[ 0 ] = Vector2( bBox.Min );
	points[ 1 ] = Vector2( bBox.Max.X, bBox.Min.Y );
	points[ 2 ] = Vector2( bBox.Max );
	points[ 3 ] = Vector2( bBox.Min.X, bBox.Max.Y );

	edges[ 0 ] = points[ 1 ] - points[ 0 ];
	edges[ 1 ] = points[ 2 ] - points[ 1 ];
	edges[ 2 ] = points[ 3 ] - points[ 2 ];
	edges[ 3 ] = points[ 0 ] - points[ 3 ];
	
	TObstacleIndex firstIndex = crowd->GetNumObstacles();
	TObstacleIndex lastIndex = firstIndex - 1;

	for ( Int16 i = 0; i < MAX_EDGES; ++i )
	{
		Int16 counter = static_cast< Int16 > ( ( edges[ i ].Mag() - 0.001f ) / SCrowdObstacleEdge::MAX_EDGE_LEN );
		Vector2 initPos = points[ i ];
		Vector2 normalized = edges[ i ].Normalized( );

		for ( Int16 j = 0; j < counter; ++j )
		{
			SCrowdObstacleEdge& obstacleEdge = crowd->NewObstacle();
			obstacleEdge.m_begin = initPos + normalized * Float( j ) * SCrowdObstacleEdge::MAX_EDGE_LEN;		
			obstacleEdge.m_end = initPos + normalized * Float( j + 1 ) * SCrowdObstacleEdge::MAX_EDGE_LEN;
		}	

		SCrowdObstacleEdge& obstacleEdge = crowd->NewObstacle();
		obstacleEdge.m_begin = initPos + normalized * Float( counter ) * SCrowdObstacleEdge::MAX_EDGE_LEN;		
		obstacleEdge.m_end = initPos + edges[ i ];

		lastIndex += counter + 1;
	}

	for ( TObstacleIndex i = firstIndex; i <= lastIndex; ++i )
	{
		crowd->GetObstacle( i ).m_nextEdge = i + 1;
		crowd->GetObstacle( i ).m_prevEdge = i - 1;
	}

	crowd->GetObstacle( firstIndex ).m_prevEdge = lastIndex;
	crowd->GetObstacle( lastIndex ).m_nextEdge = firstIndex;
}

void CCrowdArea::AddIfInside( CCrowdEntryPoint* entry )
{
	Box b = GetBoundingBox();
	if ( b.Contains( entry->GetWorldPosition() ) )
	{
		m_entries.PushBackUnique( entry );
	}
}

Box CCrowdArea::GetBoundingBox() const
{
	Box retBox( Vector::ZEROS, 0.f );

	for ( ComponentIterator< CCrowdAreaComponent > it( this ); it; ++it )
	{
		const CCrowdAreaComponent* areaComponent = *it;
		if ( areaComponent )
		{
			Box box = areaComponent->GetBoundingBox();
			if ( retBox.IsEmpty() )
			{
				retBox = box;
			}
			else
			{
				retBox.AddBox( box );
			}
		}
	}
	return retBox;
}

Box2 CCrowdArea::GetBoundingBox2() const
{
	const Box box = GetBoundingBox();
	return Box2( box.Min.AsVector2(), box.Max.AsVector2() ); 
}

CCrowdEntryPoint* CCrowdArea::RandomEntry() const 
{
	if ( m_entries.Empty() )
	{
		return nullptr;
	}

	Uint32 randomEntry = GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_entries.Size() );
	return m_entries[ randomEntry ].Get();
}

CCrowdEntryPoint* CCrowdArea::RandomEntryDifferent( CCrowdEntryPoint* differentThanThis ) const
{
	if ( m_entries.Size() < 2 )
	{
		return nullptr;
	}

	Uint32 randomEntry = GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_entries.Size() );
	CCrowdEntryPoint* retEntry = m_entries[ randomEntry ].Get();
	if ( retEntry == differentThanThis )
	{
		if ( randomEntry > 0 )
		{
			--randomEntry;
		}
		else
		{
			randomEntry++;
		}
		retEntry = m_entries[ randomEntry ].Get();
	}
	return retEntry;
}

Bool CCrowdArea::ContainsPosition2( const Vector2& pos ) const
{
	// temp impl
	// TODO: optimize

	Vector point( pos );
	for ( ComponentIterator< CCrowdAreaComponent > it( this ); it; ++it )
	{
		point.Z = ( *it )->GetWorldPositionRef().Z;
		if ( ( *it )->TestPointOverlap( point ) )
		{
			return true;
		}
	}

	return false;
}

Vector2 CCrowdArea::RandomPositionInside2() const
{
	Vector2 vec;
	Box2 boundingBox = GetBoundingBox2();

	vec.X = GEngine->GetRandomNumberGenerator().Get< Float >( boundingBox.Min.X , boundingBox.Max.X );
	vec.Y = GEngine->GetRandomNumberGenerator().Get< Float >( boundingBox.Min.Y , boundingBox.Max.Y );

	return vec;
}

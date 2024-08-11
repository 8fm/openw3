#include "build.h"
#include "moveGlobalPathPlanner.h"
#include "moveNavigationPath.h"
#include "movePathIterator.h"
#include "../engine/renderFrame.h"


/////////////////////////////////////////////////////////////////////////////////////////

CMoveGlobalPathPlanner::CMoveGlobalPathPlanner()
: m_idsPool( 0 )
{
}

CMoveGlobalPathPlanner::~CMoveGlobalPathPlanner()
{
	
}

void CMoveGlobalPathPlanner::Tick( Float timeDelta )
{
	UpdatePointLocks();
	UpdatePathfindingQueries();
	UpdateIterators();
}

void CMoveGlobalPathPlanner::RemoveQuery( const IPathPlannerCallback& callback )
{
}

void CMoveGlobalPathPlanner::UpdatePathfindingQueries()
{
  
}

void CMoveGlobalPathPlanner::UpdateIterators()
{
	for ( IteratorsMap::iterator it = m_iterators.Begin(); it != m_iterators.End(); ++it )
	{
		it->m_second->Update();
	}
}

void CMoveGlobalPathPlanner::RegisterIterator( const INavigable& agent, CMovePathIterator& iterator )
{
	IteratorsMap::iterator it = m_iterators.Find( &agent );
	if ( it == m_iterators.End() )
	{
		m_iterators.Insert( &agent, &iterator );
	}
	else
	{
		it->m_second = &iterator;
	}
}

void CMoveGlobalPathPlanner::UnregisterIterator( const INavigable& agent )
{
	if ( m_iterators.Find( &agent ) != m_iterators.End() )
	{
		m_iterators.Erase( &agent );
	}
}

const CMovePathIterator* CMoveGlobalPathPlanner::GetIteratorFor( const INavigable& agent ) const
{
	IteratorsMap::const_iterator it = m_iterators.Find( &agent );
	if ( it != m_iterators.End() )
	{
		return it->m_second;
	}
	else
	{
		return NULL;
	}
}

// ------------------------------------------------------------------------
// Debug
// ------------------------------------------------------------------------

void CMoveGlobalPathPlanner::GenerateDebugFragments( CRenderFrame* frame )
{
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Locomotion ) ) 
	{
		for ( IteratorsMap::const_iterator it = m_iterators.Begin(); it != m_iterators.End(); ++it )
		{
			it->m_second->OnGenerateDebugFragments( frame );
		}
	}

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Paths ) ) 
	{
		for ( QueryFailures::const_iterator it = m_queryFailures.Begin(); it != m_queryFailures.End(); ++it )
		{
			const SQueryFailure& failure = *it;
			frame->AddDebugLineWithArrow( failure.m_start, failure.m_end, 1.0f, 0.5f, 0.5f, Color( 255, 100, 100 ), true );
		}
	}
}

// ------------------------------------------------------------------------
// Point locks management
// ------------------------------------------------------------------------

Int32 CMoveGlobalPathPlanner::AquireLockId( Uint32 key, Float dist )
{
	LockPointsMap::iterator it = m_lockPoints.Find( key );
	if ( it == m_lockPoints.End() )
	{
		m_lockPoints.Insert( key, TDynArray< SLock >() );
		it = m_lockPoints.Find( key );
	}

	Uint32 lockId = m_idsPool++;
	it->m_second.PushBack( SLock( lockId, dist ) );
	return lockId;
}

void CMoveGlobalPathPlanner::ReleaseLockId( Uint32 key, Int32 lockId )
{
	LockPointsMap::iterator it = m_lockPoints.Find( key );

	ASSERT( it != m_lockPoints.End() );
	if ( it != m_lockPoints.End() )
	{
		TDynArray< SLock >& bucket = it->m_second;
		ASSERT( !bucket.Empty() );

		for ( TDynArray< SLock >::iterator bIt = bucket.Begin(); bIt != bucket.End(); ++bIt )
		{
			if ( bIt->m_lockId == lockId )
			{
				bucket.Erase( bIt );
				break;
			}
		}

		if ( bucket.Empty() )
		{
			m_lockPoints.Erase( key );
		}
	}
}

Float CMoveGlobalPathPlanner::GetCurrentLock( Uint32 key, Int32 lockId, Float dist )
{
	LockPointsMap::iterator it = m_lockPoints.Find( key );

	if ( it != m_lockPoints.End() )
	{
		TDynArray< SLock >& bucket = it->m_second;
		ASSERT( !bucket.Empty() );

		Float smallestDist = FLT_MAX;
		SLock* closestLock = NULL;
		for ( TDynArray< SLock >::iterator bIt = bucket.Begin(); bIt != bucket.End(); ++bIt )
		{
			if ( bIt->m_lockId == lockId )
			{
				// update the lock's distance
				bIt->m_distanceToGoal = dist;
			}

			// is this the closest lock?
			if ( bIt->m_distanceToGoal < smallestDist )
			{
				smallestDist = bIt->m_distanceToGoal;
				closestLock = &( *bIt );
			}
		}


		// get the agent's priority with respect to the distance to the lock point
		if ( closestLock )
		{
			return ( closestLock->m_lockId == lockId ) ? 1.0f : 0.0f;
		}
		else
		{
			return 1.0f;
		}
	}
	else
	{
		ASSERT( false && TXT( "Can't query a lock without creating one first" ) );
		return 0.0f;
	}
}

void CMoveGlobalPathPlanner::UpdatePointLocks()
{
	for ( LockPointsMap::iterator it = m_lockPoints.Begin(); it != m_lockPoints.End(); ++it )
	{
		TDynArray< SLock >& bucket = it->m_second;
		qsort( bucket.TypedData(), bucket.Size(), sizeof( SLock ), &SLock::CompareFunc );
	}
}

///////////////////////////////////////////////////////////////////////////////

CPathPointLock::CPathPointLock( CMoveGlobalPathPlanner& host, const Vector& target )
: m_host( host )
{
	Int32 X = (Int32)target.X;
	Int32 Y = (Int32)target.Y;
	Int32 Z = (Int32)target.Z;
	m_target = Vector( (Float)X, (Float)Y, (Float)Z );// we're gonna lock cubes of the size (0.5m, 0.5m, 0.5m)
	m_key = GetHash( X, Y, Z );
	m_lockId = host.AquireLockId( m_key, FLT_MAX );
}

CPathPointLock::~CPathPointLock()
{
	m_host.ReleaseLockId( m_key, m_lockId );
}

Float CPathPointLock::GetMultiplier( const Vector& currPos, Float agentRadius ) const
{
	// update the lock
	Float dist = m_target.DistanceTo( currPos );
	Float lockVal = m_host.GetCurrentLock( m_key, m_lockId, dist );

	if ( lockVal > 0.0f )
	{
		return 1.0f;
	}
	else
	{
		Float minDist = agentRadius * 3.0f;
		Float maxDist = minDist * 2.0f;
		return ( dist > minDist ) ? ( ( dist < maxDist ) ? 0.0f : 1.0f ) : -1.0f;
	}
}

void CPathPointLock::GenerateDebugFragments( const Vector& queryPos, CRenderFrame* frame )
{
	frame->AddDebugLineWithArrow( queryPos, m_target, 1.0f, 1.0f, 1.0f, Color::RED, true );
	frame->AddDebugText( queryPos, String::Printf( TXT("lockId = %d, key = %d" ), m_lockId, m_key ), true );
}

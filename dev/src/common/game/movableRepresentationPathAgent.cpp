#include "build.h"
#include "movableRepresentationPathAgent.h"

#include "../engine/pathlibWorld.h"
#include "../engine/renderFrame.h"
#include "../engine/renderVertices.h"
#include "../physics/physicsWorldUtils.h"
#include "../physics/physicsWorld.h"

namespace
{
	const Float s_lodBudgetDelta[] = { 0.6f, 0.8f, 1.3f, 1.7f, 2.0f, 2.3f };
}

CPathAgent::CPathAgent( CPathLibWorld* world, CPhysicsWorld* physicsWorld, CMovingAgentComponent& host, Float personalSpace )
	: PathLib::CAgent( world, personalSpace, CLASS_CPathAgent )
	, m_physicsWorld( physicsWorld )
	, m_host( host )
	, m_snapHeight( true )
	, m_animatedMovement( false )
	, m_forceHeightSnaping( false )
	, m_forceInstantZCorrection( false )
#ifdef PATHAGENT_DEBUG_Z_CORRECTION
	, m_didZCorrection( false )
#endif
	, m_forceZCorrection( false )
	, m_forceNavigationZCorrection( false )
{
}

void CPathAgent::OnInit( const Vector& position, const EulerAngles& orientation )
{
	m_lastPos = m_position = position.AsVector3();
	m_orientation = orientation;
	m_forceNavigationZCorrection = true; // will recalculate Z at the first time the agent moves
	m_lastPosRepresentationZ = m_representationZ = position.Z;
}

void CPathAgent::OnActivate( const Vector& position, const EulerAngles& orientation )
{
	m_lastPos = m_position = position.AsVector3();
	m_lastPosRepresentationZ = m_representationZ = position.Z;
	m_orientation = orientation;
	m_forceZCorrection = true; // only smooth z correction on activation (no recalculation) to avoid shots

	m_segmentStart					= position;
	m_segmentEnd					= position;
	m_segmentLen					= 0.f;
}

void CPathAgent::OnDeactivate()
{
	m_lastPos    = m_position;
	m_lastPosRepresentationZ = m_representationZ;

	m_segmentStart					= m_position;
	m_segmentEnd					= m_position;
	m_segmentLen   = 0.f;
}

void CPathAgent::DoMove( const Vector& deltaPosition )
{
	if ( !deltaPosition.AsVector2().IsZero()		          // Don't update the position if the delta in xy is 0 OR
		|| ( m_animatedMovement && deltaPosition.Z != 0.f ) ) // z is changed when we are on animation - cuz normally 
															  // we cannot change z without changing xy - optimalization.
	{
		Vector3 realDelta( deltaPosition.AsVector3() );
		Bool checkIsOnNavdata = true;
		if ( m_host.IsSnapedToNavigableSpace() )
		{
			if ( !StayOnNavdata( realDelta ) )
			{
				return;
			}
			checkIsOnNavdata = false;
		}
		Vector3 destination = m_position + realDelta;
		if ( !m_animatedMovement && m_isOnNavdata )
		{
			if ( !m_world->ComputeHeightFrom( m_currentArea, destination.AsVector2(), m_position, destination.Z, true ) )
			{
				Float zMin = destination.Z - PathLib::GEOMETRY_AND_NAVMESH_MAX_DISTANCE - 1.f;
				Float zMax = destination.Z + PathLib::GEOMETRY_AND_NAVMESH_MAX_DISTANCE + 1.f;
				Bool ok = m_world->ComputeHeight( destination.AsVector2(), zMin, zMax, destination.Z, m_currentArea, true );
				ASSERT( ok, TXT("Problem with PathAgent height computation!") );
			}
		}
		MoveToPosition( destination );
		
		if ( checkIsOnNavdata )
		{
			CheckIfIsOnNavdata();
		}
	}
}

// DeBuG CODE
//void CPathAgent::MoveToPosition( const Vector3& pos )
//{
//	if ( m_isOnNavdata && m_host.IsSnapedToNavigableSpace() && !m_animatedMovement )
//	{
//		if ( !TestLocation( m_world, pos, m_personalSpace, PathLib::CT_IGNORE_METAOBSTACLE | PathLib::CT_FORCE_BASEPOS_ZTEST ) )
//		{
//			PATHLIB_LOG( TXT("HDBGI: Total zjeba") );
//			return;
//		}
//	}
//	PathLib::CAgent::MoveToPosition( pos );
//}

void CPathAgent::OnMove( const Vector& deltaPosition, const EulerAngles& deltaOrientation )
{
	PC_SCOPE( CPathAgent_OnMove );

	m_orientation.Yaw += deltaOrientation.Yaw;

	m_lastPos = m_position;
	m_lastPosRepresentationZ = m_representationZ;

	if ( m_forceNavigationZCorrection )
	{
		if ( m_snapHeight && m_world->GetStreamedWorldBox().Contains2D( m_position ) )
		{
			m_forceNavigationZCorrection = false;
			if ( CheckIfIsOnNavdata()  )
			{
				if ( m_world->ComputeHeight( m_position.AsVector2(), m_position.Z - 1.f, m_position.Z + 1.f, m_position.Z, m_currentArea, true ) )
				{
					m_forceInstantZCorrection = true;
				}
				else if ( m_world->ComputeHeight( m_position.AsVector2(), m_position.Z - 2.f, m_position.Z + 2.f, m_position.Z, m_currentArea, true ) )
				{
					m_forceInstantZCorrection = true;
				}
			}
		}
	}

	DoMove( deltaPosition );
}

Bool CPathAgent::Update2_PostSeparation( Float timeDelta, Bool continousMovement )
{
	Bool res = false;
	if ( m_forceInstantZCorrection )
	{
		m_forceInstantZCorrection = false;
		m_forceZCorrection = true; // re-evaluate interpolated Z data next time the agent moves
		Vector3 pos = m_position;
		res = DoZCorrection( pos );
		m_representationZ = pos.Z;

		if ( !m_snapHeight )
		{
			m_position.Z = pos.Z;
		}
	}
	else if ( m_snapHeight && continousMovement )
	{
		if ( CActor* act = Cast< CActor >( m_host.GetEntity() ) )
		{
			Vector3 delta = m_position - m_lastPos;

			if ( !delta.AsVector2().IsAlmostZero() )
			{
				Float distFromSegStartSq = ( m_position.AsVector2() - m_segmentStart.AsVector2() ).SquareMag();

				Bool walkedEnough = distFromSegStartSq > m_segmentLen*m_segmentLen;
				// need to compute a new segment also when an agent decided to dramatically change its direction
				Bool turnedBack = ( m_segmentEnd.AsVector2() - m_segmentStart.AsVector2() ).Dot( delta.AsVector2() ) < 0.f;

				if ( walkedEnough || turnedBack || m_forceZCorrection )
				{
					m_forceZCorrection = false;

					Uint32 lod = act->GetLODState().m_desiredLOD->m_index;
					ASSERT( lod <= ARRAY_COUNT_U32( s_lodBudgetDelta ) );

					Float dist     = s_lodBudgetDelta[ lod ];
					// get the previous position (before crossing a threshold) as a segment starting point - to keep the motion fluid 
					// we should be already interpolating within the next segment, otherwise we'd get stuck in the same Z for one frame
					m_segmentStart = m_lastPos; 
					m_segmentStart.Z = m_lastPosRepresentationZ;
					m_segmentEnd   = m_segmentStart + delta.Normalized()*dist;

					// prevent from probing Z outside the navmesh and do first Z-approx. based on navmesh
					m_world->GetClearLineInDirection( m_currentArea, m_segmentStart, m_segmentEnd, m_personalSpace, m_segmentEnd, m_defaultCollisionFlags );
					m_segmentLen = ( m_segmentEnd.AsVector2() - m_segmentStart.AsVector2() ).Mag();
				
					DoZCorrection( m_segmentEnd );

					Float zDist = Abs( m_segmentEnd.Z - m_position.Z );
					if ( m_segmentLen != 0.f && ( zDist / m_segmentLen ) > 4.f )
					{
						// FAILSAFE: too steep - cancel z correction. Prevents from sudden movement spikes which may come from
						// a steering error or sth - it's better to keep the old z than to be thrown into space.
						m_segmentLen = 0.f;
					}
				}
			}
		}

		if ( m_segmentLen > .0f ) 
		{
			// interpolate Z
			Float curDist = ( m_position.AsVector2() - m_segmentStart.AsVector2() ).Mag();
			Float ratio = Clamp( curDist / m_segmentLen, 0.f, 1.f );
			m_representationZ = m_segmentStart.Z + ( m_segmentEnd.Z - m_segmentStart.Z ) * ratio;
			res = true;
		}
	}
	else
	{
		m_representationZ = m_position.Z;
	}

	return res;
}

void CPathAgent::OnSeparate( const Vector& deltaPosition )
{
	DoMove( deltaPosition );
}

Bool CPathAgent::DoZCorrection( Vector3& pos )
{
	PC_SCOPE( CPathAgent_ZCorrection );

	if ( m_physicsWorld->IsAvaible( pos ) ) // make sure that physics data is streamed-in
	{
		SPhysicsContactInfo outInfo;
		Vector from( pos.X, pos.Y, pos.Z + PathLib::GEOMETRY_AND_NAVMESH_MAX_DISTANCE - 0.01f );
		Vector to  ( pos.X, pos.Y, pos.Z - PathLib::GEOMETRY_AND_NAVMESH_MAX_DISTANCE + 0.01f );

 		const static CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) )
															   | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) 
															   | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) ) 
															   | GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );

		if ( m_physicsWorld->RayCastWithSingleResult( from, to, includeMask, 0, outInfo ) == TRV_Hit )
		{
			pos.Z = outInfo.m_position.Z;
#ifdef PATHAGENT_DEBUG_Z_CORRECTION
			m_didZCorrection = true;
#endif
			return true;
		}
	}

	return false;
}

Bool CPathAgent::CheckIfCenterIsOnNavData()
{
	return m_world->TestLocation( m_currentArea, m_position, m_defaultCollisionFlags );
}

void CPathAgent::OnSetPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation, const Bool correctZ )
{
	if ( correctZ )
	{
		m_forceNavigationZCorrection = true;
	}
	
	m_lastPos = m_position = newPosition.AsVector3();
	m_representationZ = m_lastPosRepresentationZ = newPosition.Z;
	m_orientation = newOrientation;

	// disable z interpolation
	m_segmentStart = newPosition;
	m_segmentEnd   = newPosition;
	m_segmentLen   = 0.f;

	CheckIfIsOnNavdata();
}

Vector CPathAgent::GetRepresentationPosition( Bool smooth /*= false */ ) const
{
	return Vector( m_position.X, m_position.Y, m_representationZ );
}

EulerAngles CPathAgent::GetRepresentationOrientation() const
{
	return m_orientation;
}

CName CPathAgent::GetName() const
{
	return CNAME( CPathAgent ); 
}

Bool CPathAgent::IsAlwaysActive() const
{
	return false;
}

Bool CPathAgent::ForceMovementAIAction( IAITree* tree )
{
	CActor* actor = Cast< CActor >( m_host.GetEntity() );
	if ( actor )
	{
		actor->SignalGameplayEvent( CNAME( AI_MovementAction ), tree, IAITree::GetStaticClass() );
	}
	return false;
}
void CPathAgent::DetermineHeightSnapping()
{
	Bool shouldSnap = !m_animatedMovement || m_forceHeightSnaping;
	if ( shouldSnap != m_snapHeight )
	{
		m_snapHeight = shouldSnap; 
		if ( shouldSnap )
		{
			m_forceZCorrection = true;
		}
		else
		{
			// not to get extra Z-hit when switching between animated movement and pathagent one
			m_position.Z = m_representationZ;
		}
	}
}

void CPathAgent::ForceHeightSnapping( Bool force )
{
	m_forceHeightSnaping = force;
	DetermineHeightSnapping();
}

void CPathAgent::SetAnimatedMovement( Bool enable )
{ 
	m_animatedMovement = enable;
	DetermineHeightSnapping();
}

String CPathAgent::GetDebugEntityName() const
{
	if ( CEntityTemplate* templ = Cast< CEntity >( m_host.GetParent() )->GetEntityTemplate() )
	{
		return templ->GetFile()->GetFileName();
	}

	return TXT("");
}

void CPathAgent::GenerateDebugFragments( CRenderFrame* frame )
{
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
	Bool hasFailedPathfinding = false;
#endif
	frame->AddDebugCircle( m_position + Vector( 0.f, 0.f, 0.1f ), m_personalSpace, Matrix::IDENTITY, Color::WHITE, 8 );

	//TDynArray< Vector > collisions;
	//m_world->CollectGeometryAtLocation( GetPosition(), 2.f, PathLib::CT_DEFAULT, collisions );

	//if ( !collisions.Empty() )
	//{
	//	TDynArray< DebugVertex > vertexes;
	//	vertexes.Resize( collisions.Size() );
	//	for ( Uint32 i = 0; i < collisions.Size(); ++i )
	//	{
	//		vertexes[ i ].Set( collisions[ i ], Color::LIGHT_MAGENTA );
	//	}

	//	frame->AddDebugLines( vertexes.TypedData(), vertexes.Size(), true );
	//}

	// pathfollowing debug drawing
	if ( m_state == STATE_PATHFOLLOW )
	{
		// draw current path following target
		frame->AddDebugLine( m_position + Vector( 0, 0, 0.25f ), m_lastFollowPosition + Vector( 0, 0, 0.25f ), Color::GREEN, false );


		Uint32 waypoints = GetWaypointsCount();

		if ( waypoints > 0 )
		{
			Uint32 metalinksCount = GetMetalinksCount();
			Uint32 currMetalinkIndex = 0;

			TDynArray< DebugVertex > verts;
			TDynArray< Uint16 > indices;

			Uint32 waypoints = GetWaypointsCount();
			verts.Resize( waypoints );
			for ( Uint16 i = 0; i < waypoints; ++i )
			{
				Float colorStrengthRatio = Float(i) / Float(waypoints);
				Uint8 color = 180 + Uint8( 70.f * colorStrengthRatio );
				Bool isMetalink = false;
				if ( currMetalinkIndex < metalinksCount )
				{
					if ( GetMetalink( currMetalinkIndex ).m_waypointIndex == i )
					{
						isMetalink = true;
					}
					while ( GetMetalink( currMetalinkIndex ).m_waypointIndex <= i )
					{
						if ( ++currMetalinkIndex == metalinksCount )
						{
							break;
						}
					}
				}
				Color vertexColor = isMetalink
					? Color( 0, 0, color )
					: Color( color, color, 0 );
				verts[ i ] = DebugVertex( GetWaypoint( i ) + Vector( 0,0,0.25f ), vertexColor );

				// Cool debug code, ready to be commented out
				//{
				//	String caption = String::Printf( TXT("%d"), i );
				//	frame->AddDebugText( GetWaypoint( i ), caption, -1, -1, false, Color::YELLOW );
				//}
			}
			indices.Resize( (waypoints-1)*2 );
			for ( Uint16 i = 0; i < waypoints-1; ++i )
			{
				indices[ i*2+0 ] = i;
				indices[ i*2+1 ] = i+1;
			}

			frame->AddDebugIndexedLines( verts.TypedData(), waypoints, indices.TypedData(), (waypoints-1)*2 );

			// Cool debug code, ready to be commented out
			//{
			//	String wpDebug = String::Printf( TXT("WP: %d/%d"), m_currentWaypoint, m_waypoints.Size() );
			//	frame->AddDebugText( m_position, wpDebug,   -25, 2, m_didZCorrection, Color::YELLOW );
			//}
		}
	}
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
	else
	{
		Vector lastFailedPathDestination;
		if ( HasFailedRecently( lastFailedPathDestination.AsVector3() ) )
		{
			hasFailedPathfinding = true;
			lastFailedPathDestination.Z += 0.25f;
			frame->AddDebugCircle( lastFailedPathDestination, m_personalSpace, Matrix::IDENTITY, Color::RED, 8, true );
			frame->AddDebugLine( lastFailedPathDestination-Vector( m_personalSpace, m_personalSpace, 0.f ), lastFailedPathDestination+Vector( m_personalSpace, m_personalSpace, 0.f ), Color::RED, true );
			frame->AddDebugLine( lastFailedPathDestination-Vector( m_personalSpace, -m_personalSpace, 0.f ), lastFailedPathDestination+Vector( m_personalSpace, -m_personalSpace, 0.f ), Color::RED, true );
			frame->AddDebugLine( m_position + Vector( 0, 0, 0.25f ), lastFailedPathDestination, Color::RED, true );
		}
	}
	String pathfindStatus;
	Debug_PathfindOutcome( pathfindStatus );
	if ( IsAsyncTaskRunning() )
	{
		pathfindStatus += TXT("...");
	}
	if ( !pathfindStatus.Empty() )
	{
		frame->AddDebugText( m_position, pathfindStatus, -30, -1, false, hasFailedPathfinding ? Color::RED : Color::BLUE );
	}
#endif

	if ( m_host.HasMoved() )
	{
		Box box( Vector::ZEROS, m_host.GetAvoidanceRadius() );
		box.Min.Z = box.Max.Z = 0.f;
		frame->AddDebugBox( box, m_host.GetLocalToWorld(), Color::BLUE, true, true );
	}

	String repName;
	Color  repColor;
	
	if ( m_host.GetRepresentationStack()->GetActiveRepresentation() == this )
	{
		repName  = TXT("PA");
		repColor = Color::GREEN;
	}
	else if ( m_host.GetActiveRepresentationName().AsString() == TXT("CMRPhysicalCharacter") )
	{
		repName  = TXT("CC");
		repColor = Color::RED;
	}
	else
	{
		repName = TXT("EN");
		repColor = Color::BLUE;
	}

	// Cool debug code, ready to be commented out BUT Its drawing on top of pathfinding status!
	//{
	//	Vector pos = m_host.GetWorldPosition();
	//	String coords = String::Printf( TXT("[%.2f, %.2f, %.2f]"), pos.X, pos.Y, pos.Z );
	//	frame->AddDebugText( m_position, coords,    -50,-1, false, Color::WHITE );
	//}

#ifdef PATHAGENT_DEBUG_Z_CORRECTION
	frame->AddDebugText( m_position, repName,   -50, 0, false, m_didZCorrection							? Color::YELLOW : repColor );
	m_didZCorrection = false;
#else
	frame->AddDebugText( m_position, repName,   -50, 0, false, repColor );
#endif
	frame->AddDebugText( m_position, TXT("MC"), -30, 0, false, m_animatedMovement						? Color::GREEN : Color::GRAY );
	frame->AddDebugText( m_position, TXT("SN"), -10, 0, false, m_host.IsSnapedToNavigableSpace()		? Color::GREEN : Color::GRAY );
	frame->AddDebugText( m_position, TXT("ON"),  10, 0, false, m_isOnNavdata							? Color::GREEN : Color::GRAY );
	frame->AddDebugText( m_position, TXT("CC"),  30, 0, false, m_host.IsCharacterCollisionsEnabled()	? Color::GREEN : Color::GRAY );
	
	frame->AddDebugText( m_position, String::Printf( TXT("%p"), m_host.GetEntity() ), -15, -2, false, Color::WHITE );
}


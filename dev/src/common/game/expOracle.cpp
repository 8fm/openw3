
#include "build.h"
#include "expOracle.h"
#include "expChainExecutor.h"
#include "expSlideExecutor.h"
#include "expBlendExecutor.h"
#include "expBlendChainExecutor.h"
#include "expSingleAnimExecutor.h"
#include "expStatesExecutor.h"
#include "expManualStatesExecutor.h"
#include "expTransitionExecutor.h"
#include "expToPointSliderExecutor.h"
#include "expMountBoatExecutor.h"
#include "vehicle.h"

#include "../../common/core/mathUtils.h"

//todo to be removed when the jump animation will not need a check anymore against the terrain collision
#include "../../common/game/movingPhysicalAgentComponent.h"

#include "../../common/engine/physicsCharacterWrapper.h"
#include "../physics/physicsWorldUtils.h"


#define SMALL_LEDGE_AUTO_TRAVERSE_SMALL_LIMIT 0.70f

#define SMALL_LEDGE_LIMIT 1.50f
#define SMALL_FENCE_LIMIT 0.70f

RED_DEFINE_STATIC_NAME( ForceIdleAndWait );
RED_DEFINE_STATIC_NAME( ForceRunLeftUp );
RED_DEFINE_STATIC_NAME( ForceRunRightUp );
RED_DEFINE_STATIC_NAME( ForceFall );
RED_DEFINE_STATIC_NAME( OnInAirStarted );

RED_DEFINE_STATIC_NAME( r_toe );

RED_DEFINE_STATIC_NAME( fence100_center_long_to_fall );
RED_DEFINE_STATIC_NAME( fence100_center_short_to_fall );

RED_DEFINE_STATIC_NAME( fence120_center_short );
RED_DEFINE_STATIC_NAME( fence120_left_short );
RED_DEFINE_STATIC_NAME( fence120_right_short );
RED_DEFINE_STATIC_NAME( fence120_center_long );
RED_DEFINE_STATIC_NAME( fence120_left_long );
RED_DEFINE_STATIC_NAME( fence120_right_long );

RED_DEFINE_STATIC_NAME( fence280_center_short );
RED_DEFINE_STATIC_NAME( fence280_center_long );

RED_DEFINE_STATIC_NAME( window280_center_short );

RED_DEFINE_STATIC_NAME( ledge_from_air_short );

RED_DEFINE_STATIC_NAME( ledge80_center_short );
RED_DEFINE_STATIC_NAME( ledge80_left_short );
RED_DEFINE_STATIC_NAME( ledge80_right_short );
RED_DEFINE_STATIC_NAME( ledge80_center_long );
RED_DEFINE_STATIC_NAME( ledge80_left_long );
RED_DEFINE_STATIC_NAME( ledge80_right_long );

RED_DEFINE_STATIC_NAME( ledge200_left_short );
RED_DEFINE_STATIC_NAME( ledge200_center_short );
RED_DEFINE_STATIC_NAME( ledge200_right_short );
RED_DEFINE_STATIC_NAME( ledge200_center_long );
RED_DEFINE_STATIC_NAME( ledge200_left_long );
RED_DEFINE_STATIC_NAME( ledge200_right_long );

RED_DEFINE_STATIC_NAME( ledge280_center_up_short );
RED_DEFINE_STATIC_NAME( ledge280_left_up_short );
RED_DEFINE_STATIC_NAME( ledge280_right_up_short );

RED_DEFINE_STATIC_NAME( climb_idle_100 );
RED_DEFINE_STATIC_NAME( climb_idle_200 );
RED_DEFINE_STATIC_NAME( climb_idle_300 );
RED_DEFINE_STATIC_NAME( idle_climb_down_300 );
RED_DEFINE_STATIC_NAME( idle_climb_down_100 );

RED_DEFINE_STATIC_NAME( ledge280_center_down_short );
RED_DEFINE_STATIC_NAME( ledge280_left_down_short );
RED_DEFINE_STATIC_NAME( ledge280_right_down_short );

RED_DEFINE_STATIC_NAME( swim_ledge0_center );
RED_DEFINE_STATIC_NAME( swim_ledge50_center );
RED_DEFINE_STATIC_NAME( swim_ledge50_center_2 );
RED_DEFINE_STATIC_NAME( swim_ledge100_center );

RED_DEFINE_STATIC_NAME( swim_ledge0_left );
RED_DEFINE_STATIC_NAME( swim_ledge50_left );
RED_DEFINE_STATIC_NAME( swim_ledge50_left_2 );
RED_DEFINE_STATIC_NAME( swim_ledge100_left );

RED_DEFINE_STATIC_NAME( swim_ledge0_right );
RED_DEFINE_STATIC_NAME( swim_ledge50_right );
RED_DEFINE_STATIC_NAME( swim_ledge50_right_2 );
RED_DEFINE_STATIC_NAME( swim_ledge100_right );

RED_DEFINE_STATIC_NAME( swim_ledge50_center_idle );
RED_DEFINE_STATIC_NAME( swim_entrance_on_boat );

RED_DEFINE_STATIC_NAME( ladder_left_start_top );
RED_DEFINE_STATIC_NAME( ladder_center_start_top );
RED_DEFINE_STATIC_NAME( ladder_right_start_top );
RED_DEFINE_STATIC_NAME( ladder_center_start_bottom_short );
RED_DEFINE_STATIC_NAME( ladder_left_start_bottom_short );
RED_DEFINE_STATIC_NAME( ladder_right_start_bottom_short );
RED_DEFINE_STATIC_NAME( ladder_center_start_long );
RED_DEFINE_STATIC_NAME( ladder_right_up_40 );
RED_DEFINE_STATIC_NAME( ladder_left_up_40 );
RED_DEFINE_STATIC_NAME( man_swimming_ladder_start_hand_l );

static void GetHeightForLocation( CPhysicsWorld* physicsWorld, Vector const & location, Float & outHeight, Float maxHeight )
{
	CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) );
	CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
	Vector startWS = location;
	Vector startWSLifted = startWS + Vector( 0.0f, 0.0f, 1.0f );
	Vector endWS = startWS + Vector( 0.0f, 0.0f, -maxHeight );
	SPhysicsContactInfo contactInfo;
	if( physicsWorld->RayCastWithSingleResult( startWSLifted, endWS, include, exclude, contactInfo ) == TRV_Hit )
	{
		outHeight = Max( 0.0f, startWS.Z - contactInfo.m_position.Z );
	}
	else
	{
		outHeight = maxHeight;
	}
}

static Bool TraceVerticalCollision( CPhysicsWorld* physicsWorld, Vector const & location, Float const & startDiff, Float const & endDiff )
{
	CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) );
	CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
	Vector startWS = location;
	Vector startWSLifted = startWS + Vector( 0.0f, 0.0f, startDiff );
	Vector endWS = startWS + Vector( 0.0f, 0.0f, endDiff );
	SPhysicsContactInfo contactInfo;
	if( physicsWorld->RayCastWithSingleResult( startWSLifted, endWS, include, exclude, contactInfo ) == TRV_Hit )
	{
		return true;
	}
	else
	{
		return false;
	}
}

static Bool TraceHorizontalLineOfSight( CPhysicsWorld* physicsWorld, Vector const & location, Float const & height, Vector const & target, Float const & radius )
{
	CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) );
	CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
	//Vector startWS			= location;
	Vector startWSLifted	= location + Vector( 0.0f, 0.0f, height );

	Vector endLiftedWS		= Vector( target.X, target.Y, startWSLifted.Z );

	Vector direction		= startWSLifted - endLiftedWS;
	Float distance			= direction.Mag2();
	if( distance > radius )
	{
		endLiftedWS			+= direction / distance * radius;
	}
	SPhysicsContactInfo contactInfo;
	if( physicsWorld->RayCastWithSingleResult( startWSLifted, endLiftedWS, include, exclude, contactInfo ) == TRV_Hit )
	{
		return true;
	}
	else
	{
		return false;
	}
}

SExplorationInitialState::SExplorationInitialState()
	: m_physicsState( CPS_Animated )
{


}

SExplorationInitialState::SExplorationInitialState( const CEntity* entity )
	: m_physicsState( CPS_Animated )
{
	if ( const CActor* actor = Cast<CActor>(entity) )
	{
		if ( const CMovingAgentComponent* mac = actor->GetMovingAgentComponent() )
		{
			m_physicsState = mac->GetCurrentPhysicsState();
		}
	}
}

CName ExecutorSetup::GetEdgeBone() const
{
	return ! m_edgeBone.Empty() ? m_edgeBone : m_desc->GetEdgeBone( m_useLeftSideForEdgeBone );
}

Vector const & ExecutorSetup::GetEdgeOffset() const
{
	return m_useEdgeOffset? m_edgeOffset : m_desc->GetEdgeOffset();
}

Vector const & ExecutorSetup::GetOffsetInModelSpace() const
{
	return m_desc->GetOffsetInModelSpace();
}

Bool ExecutorSetup::AlignTransToEdge() const
{
	return m_useAlignTransToEdge? m_alignTransToEdge : m_desc->AlignTransToEdge();
}

Bool ExecutorSetup::AlignRotToEdge() const
{
	return m_useAlignRotToEdge? m_alignRotToEdge : m_desc->AlignRotToEdge();
}

Float ExecutorSetup::AlignRotToEdgeExceeding() const
{
	return m_useAlignRotToEdge? m_alignRotToEdgeExceeding : m_desc->AlignRotToEdgeExceeding();
}

void EdExplorationDesc::GetDistAndAngleToTest( CEntity const * entity, Float& dist, Float& distMoveTo, Float& coneAngleHalf, Bool ignoreSpeed ) const
{
	dist = m_testDist;
	if ( m_testDistSpeed != 0.0f && ! ignoreSpeed )
	{
		const CMovingAgentComponent* mac = SafeCast<CMovingAgentComponent>( entity->GetRootAnimatedComponent() );
		const Float moveSpeed = mac ? mac->GetVelocity().Mag2() : 0.f;
		dist = moveSpeed >= m_testDistSpeed? m_testDistAboveSpeed : dist;
	}
	distMoveTo = m_testDistMove;
	coneAngleHalf = m_testConeAngleHalf;
}

ExpOracleHardCoded::ExpOracleHardCoded()
	: m_descriptions( GGame->GetGameplayConfig().m_explorationsDesc )
{

}

Bool ExpOracleHardCoded::RequiresMoveTo( const SExplorationQueryToken & token, Vector & outToLoc, Float & outReqDist ) const
{
	if ( token.m_initialState.m_physicsState == CPS_Swimming ||
		 token.m_queryContext.m_forJumping )
	{
		return false;
	}

	const Int32 eId = token.GetExploration()->GetId();

	const IExplorationDesc* desc = GetDesc( eId );
	if ( !desc )
	{
		return false;
	}

	ASSERT( token.GetEntity() );

	Float dist = 0.f;
	Float angle = 0.f;
	Float desAngle = 0.f;
	Bool frontSide= true;
	Vector normal;

	CalcDistAndAngle( token, dist, angle, desAngle, frontSide, outToLoc, normal );

	// TODO outToLoc should be with offset!

	Float eDist, eDistMoveTo, eAngle;
	desc->GetDistAndAngleToTest( token.GetEntity(), eDist, eDistMoveTo, eAngle );
	outReqDist = eDist;

	return dist > eDist 
		&& ( token.GetEntity()->IsPlayer() || dist > 2 ); // hack for board presentation - ciri exploration
}

void ExpOracleHardCoded::FilterExplorations( SExplorationQueryToken & token, const IExplorationList& list, ExpRelativeDirection direction, const CEntity* entity ) const
{
	ASSERT( entity );
	ASSERT( direction != ERD_None );

	// clear info about exploration in token
	token.SetExploration( NULL );

	Bool autoTraverse = token.m_queryContext.m_forAutoTraverseBig || token.m_queryContext.m_forAutoTraverseSmall;
	Bool autoTraverseBig = token.m_queryContext.m_forAutoTraverseBig; // only big
	Bool autoTraverseSmall = token.m_queryContext.m_forAutoTraverseSmall && ! token.m_queryContext.m_forAutoTraverseBig; // only small
	Float angleToPointOnEdgeLimit = token.m_queryContext.m_maxAngleToCheck > 0.0f ? token.m_queryContext.m_maxAngleToCheck : ( autoTraverse? 5.0f : 45.0f );

	Bool inputDirectionActive = false;
	Float inputDirectionYaw = EulerAngles::YawFromXY( token.m_queryContext.m_inputDirectionInWorldSpace.X, token.m_queryContext.m_inputDirectionInWorldSpace.Y );
	
	SExplorationQueryToken testToken = token;

	CPhysicsWorld* physicsWorld = nullptr;
	if ( CLayer* lay = token.GetEntity()->GetLayer() )
	{
		if ( CWorld* world = lay->GetWorld() )
		{
			 world->GetPhysicsWorld( physicsWorld );
		}
	}

	if ( list.Size() > 0 )
	{
		Bool bestMoveTo = true;
		Float bestAngle = 360.0f;
		Float bestDist = FLT_MAX;

		Vector inputDir = Vector::ZEROS;
		if( token.m_queryContext.m_inputDirectionInWorldSpace.SquareMag2() > 0.0f )
		{
			inputDir = token.m_queryContext.m_inputDirectionInWorldSpace / token.m_queryContext.m_inputDirectionInWorldSpace.Mag2();
			inputDirectionActive = true;
		}

#ifdef EXPLORATION_DEBUG
		static TDynArray<CName> dbgNames;
		if( dbgNames.Size() < list.Size() )
		{
			for( Uint32 i = dbgNames.Size(); i < list.Size(); ++i )
			{
				dbgNames.PushBack( CName( TXT("explorationDbg") + i ) );
			}
		}
#endif

		Bool isPlayerAsking = entity->IsPlayer();
		const Uint32 size = list.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const IExploration* e = list[ i ];

			const Int32 eId = e->GetId();

			const IExplorationDesc* desc = GetDesc( eId );
			if ( !desc )
			{
				continue;
			}

			if( isPlayerAsking && eId == ET_Jump )
			{
				continue;
			}

			if ( token.m_queryContext.m_laddersOnly && eId != ET_Ladder )
			{
				// we want only ladders
				continue;
			}

			if ( token.m_queryContext.m_forJumping && eId != ET_Ledge )
			{
				// for jumping other types should be unavailable
				continue;
			}

			testToken.SetExploration( e );

			/*if ( autoTraverseBig )
			{
				if ( eId == ET_Ladder )
				{
					// don't auto traverse ladders
					continue;
				}
			}*/

			if ( autoTraverseSmall )
			{
				if ( eId != ET_Ledge )
				{
					// for auto traverse (small) now we are interested only in ledges
					continue;
				}
			}

			Float eDistModifier = autoTraverse? 0.5f : 1.0f;

			const Vector& entPos = entity->GetWorldPositionRef();
			Float dist = 0.f;
			Float angle = 0.f;
			Float desAngle = 0.f;
			Bool frontSide= true;
			Float expHeight;
			Vector pointOnEdge;
			CalcDistAngleHeightAndLocation( testToken, dist, angle, desAngle, frontSide, pointOnEdge, 0.1f, // 0.1f to differentiate edges sharing ends
				0.5f, expHeight); 

			if ( eId >= ET_Horse_LF && eId < ET_Horse_B )
			{
				Matrix expMat;
				testToken.GetExploration()->GetParentMatWS( expMat );
				Vector relPoint = Vector::ZEROS;
				// yeah, hardcoded
				if ( eId == ET_Horse_LF ) relPoint = Vector(-1.0f,  2.0f, 0.0f ); else
				if ( eId == ET_Horse_L )  relPoint = Vector(-1.0f, -0.4f, 0.0f ); else
				if ( eId == ET_Horse_LB ) relPoint = Vector(-1.0f, -2.0f, 0.0f ); else
				if ( eId == ET_Horse_RF ) relPoint = Vector( 1.0f,  2.0f, 0.0f ); else
				if ( eId == ET_Horse_R )  relPoint = Vector( 1.0f, -0.4f, 0.0f ); else
				if ( eId == ET_Horse_RB ) relPoint = Vector( 1.0f, -2.0f, 0.0f ); else
				if ( eId == ET_Horse_B )  relPoint = Vector( 0.0f, -2.0f, 0.0f );
				if ( relPoint.SquareMag2() != 0.0f )
				{
					relPoint = ( expMat.TransformPoint( relPoint ) - expMat.GetTranslation() ).Normalized2();
				}
				else
				{
					relPoint = ( pointOnEdge - expMat.GetTranslation() ).Normalized2();
				}
				Vector relUser = ( entPos - expMat.GetTranslation() ).Normalized2();
				desAngle = RAD2DEG( MAcos_safe( relUser.Dot2( relPoint ) ) );
			}

			if ( eId == ET_Ledge )
			{
				if ( autoTraverseSmall && expHeight > SMALL_LEDGE_AUTO_TRAVERSE_SMALL_LIMIT )
				{
					// for auto traverse (small) now we are interested only in small ledges
					continue;
				}				
				if ( autoTraverse && expHeight < SMALL_LEDGE_LIMIT )
				{
					// extend distance when auto traversing small ledges
					eDistModifier = 0.7f;
				}
				// don't auto traverse when going down
				if ( expHeight < -0.1f )
				{
					continue;
				}
			}

			angle = Abs( angle );

			float dz = entity->GetWorldPositionRef().Z- pointOnEdge.Z;
			switch ( desc->GetZComparision() )
			{
			case EXPZCMP_DIST:
				if ( Abs( dz ) > desc->GetZComparisionParam() )
					continue;
				break;
			case EXPZCMP_GREATER:
				if ( dz < desc->GetZComparisionParam() )
					continue;
				break;
			case EXPZCMP_LESSER:
				if ( dz > desc->GetZComparisionParam() )
					continue;
				break;
			case EXPZCMP_SIDE_LG:
				if ( ( frontSide && dz > desc->GetZComparisionParam() ) ||
					 ( ! frontSide && dz < desc->GetZComparisionParam()) )
				{
					continue;
				}
				break;
			}

			Bool doMaxDZCheck = true;
			Bool doDistCheck = true;

			if ( token.m_queryContext.m_dontDoZAndDistChecks )
			{
				doMaxDZCheck = false;
				doDistCheck = false;
			}

			// special case hardcoded for ledges from water
			if ( eId == ET_Ledge )
			{
				if ( const CMovingAgentComponent* mac = SafeCast<CMovingAgentComponent>( entity->GetRootAnimatedComponent() ) )
				{
					ECharacterPhysicsState cps = mac->GetCurrentPhysicsState();
					if ( cps == CPS_Swimming )
					{
						// just assume where water level should be
						Float waterLevel = entity->GetWorldPosition().Z + 1.0f;
						Float diff = pointOnEdge.Z - waterLevel;
						if ( Abs( diff ) > 1.5f )
						{
							continue;
						}
						if ( dist > 3.0f )
						{
							continue;
						}
						doMaxDZCheck = false;
						doDistCheck = false;
					}
				}
			}

			if ( isPlayerAsking && doMaxDZCheck )
			{
				// check max z difference
				float maxdz = desc->GetZMaxDifference();
				if ( maxdz > 0.0f && Abs( dz ) > maxdz )
				{
					continue;
				}
			}

			// Check input
			if( inputDir.SquareMag2() > 0.0f )
			{
				//Vector entityDir = pointOnEdge - entPos;
				Vector entityDir;
				e->GetNormal( entityDir );
				entityDir.Normalize2();
				if( Abs( inputDir.Dot2( entityDir ) ) <  Abs( MCos( angleToPointOnEdgeLimit ) ) )
				{
					continue;
				} 
			}

#ifdef EXPLORATION_DEBUG
			Color dbgColor = Color::RED;
			Vector p1, p2;
			e->GetEdgeWS( p1, p2 );

			const Vector& entPos = entity->GetWorldPositionRef();
			Vector pointOnEdge = entPos.NearestPointOnEdge( p1, p2 );
#endif

			if ( !desc->IsDoubleSided( entity->IsPlayer() ) && !frontSide 
				&& eId != ET_Boat_B && eId != ET_Boat_Passenger_B ) //temp hack
			{
#ifdef EXPLORATION_DEBUG
				//Cast<CActor>(entity)->GetVisualDebug()->AddArrow( dbgNames[i], entPos, pointOnEdge, 1.f, 0.2f, 0.2f, dbgColor, true, true, 0.2f );
#endif
				continue;
			}

			Bool ignoreSpeed = false;
			if ( eId == ET_Ledge && dz < -2.40f )
			{
				// for ledges that are way higher up, ignore approaching with greater speed
				ignoreSpeed = true;
			}

			if ( eId == ET_Ledge && physicsWorld )
			{
				// check if there is something on the ledge - we don't want to climb into something else
				Float checkIntoLedgeDist = 0.2f;
				Float radius = 0.3f;
				Float heightOffset = 0.2f;
				Vector ledgeNormal;
				e->GetNormal(ledgeNormal);
				Vector pointToCheck = pointOnEdge - ledgeNormal * checkIntoLedgeDist + Vector( 0.0f, 0.0f, radius + heightOffset );
				Vector pointFromCheck = pointToCheck + ledgeNormal * 2.0f * checkIntoLedgeDist;
				CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) );
				CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
				if ( physicsWorld->SweepTestAnyResult( pointFromCheck, pointToCheck, radius, include, exclude ) == TRV_Hit )
				{
					// if we climb, we will hit something
					continue;
				}
			}

			if ( eId == ET_Ladder )
			{
				Bool above;
				CalcAbove( testToken, above );


				// ED Check for proper side
				{
					Vector ladderNormal;
					e->GetNormal(ladderNormal);

					// Climbing from above we need to approach the other direction
					if( above )
					{
						ladderNormal	= -ladderNormal;
					}
					if ( Vector::Dot2( entPos - pointOnEdge, ladderNormal ) < 0.0f )
					{
						// we're behind ladder
						continue;
					}
				}
				// ED Check for trapdoors
				if ( above )
				{
					if ( physicsWorld )
					{
						Vector ladderNormal;
						e->GetNormal(ladderNormal);
						if ( TraceVerticalCollision( physicsWorld, pointOnEdge + ladderNormal * 0.5f, 1.0f, -1.0f ) ) // move half meter away to check if there is collision on the way to get onto ladder
						{
							// there's trap door or something like that
							continue;
						}
					}
				}
				// ED Check horizontal line of sight to the ladder
				if( physicsWorld && TraceHorizontalLineOfSight( physicsWorld, entPos, 1.0f, pointOnEdge, 0.5f ) )
				{
					continue;			
				}

			}

			if ( eId == ET_Jump )
			{
				if( angle > 90 )
				{
					angle = Abs( angle - 180 );
				}
			}

			Float eDist, eDistMoveTo, eAngle;
			desc->GetDistAndAngleToTest( entity, eDist, eDistMoveTo, eAngle, ignoreSpeed );
						
			if ( autoTraverse )
			{
				eDist *= eDistModifier;
				eDistMoveTo = 0.0f; // never move to for auto traverse
			}

			if ( ( ! doDistCheck || dist < Max( eDist, eDistMoveTo ) ) 
				&& ( angle < eAngle 
					|| eId == ET_Boat_B || eId == ET_Boat_Passenger_B //temp hack
					|| ( dist < 2 && eId == ET_Ledge && !entity->IsPlayer() ) //hack for board demo
					) 
				)
			{
				Bool shouldMoveTo = doDistCheck && dist > eDist;
				if ( ! shouldMoveTo || bestMoveTo ) // prefer when we don't have to move
				{
#ifdef EXPLORATION_DEBUG
					dbgColor = Color::YELLOW;
					Cast<CActor>(entity)->GetVisualDebug()->AddArrow( dbgNames[i], entPos, pointOnEdge, 1.f, 0.2f, 0.2f, dbgColor, true, true, 0.2f );
#endif
					// get angle to point on edge - to get exploration closest to what we want
					Float angleToPointOnEdge = desAngle;
					if ( inputDirectionActive )
					{
						Vector toPointOnEdge = pointOnEdge - entPos;
						angleToPointOnEdge = Abs( EulerAngles::AngleDistance( EulerAngles::YawFromXY( toPointOnEdge.X, toPointOnEdge.Y ), inputDirectionYaw ) );
					}

					Float angleToPointOnEdgeLimitAdd = eId == ET_Ledge || eId == ET_Fence || eId == ET_Fence_OneSided ? 0.0f : 90.0f;

					if ( ! inputDirectionActive || angleToPointOnEdge < angleToPointOnEdgeLimit + angleToPointOnEdgeLimitAdd )
					{
						// choose anim that's closest to our desired direction, unless it is at least 15' to side but a little bit closer
						if( ( angleToPointOnEdge < bestAngle ) ||
							( angleToPointOnEdge < bestAngle + 15.0f && dist < bestDist - 0.3f && (eId < ET_Horse_LF || eId > ET_Horse_B) ) ||
							(eId == ET_Boat_B || eId == ET_Boat_Passenger_B) ) // boat hack ( ignore angle test if boat )
						{
							bestAngle = angleToPointOnEdge;
							bestDist = dist;
							bestMoveTo = shouldMoveTo;
							// store in token
							token.SetExploration( e );
							token.m_pointOnEdge = pointOnEdge;
							token.m_type = (EExplorationType) eId;
							UpdateExtraDataIn( token, expHeight );
							e->GetNormal(token.m_normal);
						}
					}
				}
			}

#ifdef EXPLORATION_DEBUG
			//Cast<CActor>(entity)->GetVisualDebug()->AddArrow( dbgNames[i], entPos, pointOnEdge, 1.f, 0.2f, 0.2f, dbgColor, true, true, 0.2f );
#endif
		}
	}
}

void ExpOracleHardCoded::UpdateExtraDataIn( SExplorationQueryToken & token, Float expHeight ) const
{
	// hardcoded? harcoded!
	token.m_usesHands = true;
	if ( token.GetExploration() )
	{
		const Int32 eId = token.GetExploration()->GetId();
		if ( eId == ET_Ledge )
		{
			token.m_usesHands = expHeight >= SMALL_LEDGE_LIMIT;
		}			
		else if ( eId == ET_Fence || eId == ET_Fence_OneSided )
		{
			token.m_usesHands = expHeight >= SMALL_FENCE_LIMIT;
		}
	}
}

const IExplorationDesc* ExpOracleHardCoded::GetDesc( Int32 id ) const
{
	ASSERT( m_descriptions.SizeInt() > id );
	return m_descriptions.SizeInt() > id ? m_descriptions[ id ] : NULL;
}

void ExpOracleHardCoded::CalcLocation( const SExplorationQueryToken & token, Vector& location, Vector& normal, Float offEnds, Bool getLocationInForwardDirection ) const
{
	ASSERT( token.GetEntity() );

	const IExploration* e = token.GetExploration();

	e->GetNormal( normal );

	Vector p1, p2;
	e->GetEdgeWS( p1, p2 );

	const Vector entPos = token.GetEntity()->GetWorldPositionRef();

	if ( offEnds > 0.0f )
	{
		Vector pDir = (p2 - p1).Normalized3();
		offEnds = Min( offEnds, (p2 - p1).Mag3() * 0.5f );
		p1 += pDir * offEnds;
		p2 -= pDir * offEnds;
	}

	if ( ! getLocationInForwardDirection || token.m_queryContext.m_inputDirectionInWorldSpace.SquareMag2() <= 0.0f )
	{
		location = entPos.NearestPointOnEdge( p1, p2 );
	}
	else
	{
		//Vector rayDir = ( token.GetEntity()->GetWorldForward() + token.m_queryContext.m_inputDirectionInWorldSpace * 2.0f ).Normalized2();
		Vector rayDir = token.m_queryContext.m_inputDirectionInWorldSpace.Normalized2();
		Vector rayOrigin = entPos;

		Float proj = 0.0f;
		if ( !MathUtils::GeometryUtils::TestIntersectionRayLine2D( rayDir, rayOrigin, p1, p2, 0, 1, proj ) )
		{
			proj = Clamp( proj, 0.f, 1.f );
		}

		location = p1 * (1.0f - proj) + p2 * proj;
	}
}

void ExpOracleHardCoded::CalcHeightLocationAndNormal( const SExplorationQueryToken & token, Float distFromEdge, Float& height, Float* outHeightBehind, Vector & pointOnEdge, Vector & normal, Float locationOffEnds ) const
{
	ASSERT( token.GetEntity() );

	const IExploration* e = token.GetExploration();

	CalcLocation( token, pointOnEdge, normal, locationOffEnds, false );

	CalcHeightUsingLocationAndNormal( token, pointOnEdge, normal, distFromEdge, height, outHeightBehind );
}

void ExpOracleHardCoded::CalcHeightUsingLocationAndNormal( const SExplorationQueryToken & token, Vector const & pointOnEdge, Vector const & normal, Float distFromEdge, Float& height, Float* outHeightBehind ) const
{
	height = 0.0f;
	if ( outHeightBehind )
	{
		*outHeightBehind = 0.0f;
	}
	if ( CLayer* lay = token.GetEntity()->GetLayer() )
	{
		if ( CWorld* world = lay->GetWorld() )
		{
			CPhysicsWorld* physicsWorld = nullptr;
			if ( world->GetPhysicsWorld( physicsWorld ) )
			{
				// one meter away should be enough
				GetHeightForLocation( physicsWorld, pointOnEdge + normal * 1.0f, height, 10.0f );
				if ( outHeightBehind )
				{
					GetHeightForLocation( physicsWorld, pointOnEdge - normal * 1.0f, *outHeightBehind, 10.0f );
				}
			}
		}
	}
}

void ExpOracleHardCoded::CalcDistAngleHeightAndLocation( const SExplorationQueryToken & token, Float& dist, Float& angle, Float& desAngle, Bool& frontSide, Vector& location, Float locationOffEnds, Float distFromEdgeForHeight, Float& height ) const
{
	Vector normal;
	CalcHeightLocationAndNormal( token, distFromEdgeForHeight, height, nullptr, location, normal, locationOffEnds );
	const Int32 eId = token.GetExploration()->GetId();
	if ( eId == ET_Ledge || eId == ET_Fence || eId == ET_Fence_OneSided ) // to always choose where we're targeting
	{
		CalcLocation( token, location, normal, locationOffEnds, true );
	}

	// Refine point
	if ( eId == ET_Ladder )
	{
		AdjustPointOnEdgeToLadderStep( token, location );
	}

	// TODO for some explorations, get in front dir
	CalcDistAndAngleUsingLocationAndNormal( token, location, normal, dist, angle, desAngle, frontSide );

}

void ExpOracleHardCoded::CalcDistAndAngle( const SExplorationQueryToken & token, Float& dist, Float& angle, Float& desAngle, Bool& frontSide, Vector& location, Vector& normal, Float locationOffEnds ) const
{
	ASSERT( token.GetEntity() );
	const IExploration* e = token.GetExploration();

	CalcLocation( token, location, normal, locationOffEnds, false );

	CalcDistAndAngleUsingLocationAndNormal( token, location, normal, dist, angle, desAngle, frontSide );
}

void ExpOracleHardCoded::CalcDistAndAngleUsingLocationAndNormal( const SExplorationQueryToken & token, Vector const & location, Vector const & normal, Float& dist, Float& angle, Float& desAngle, Bool& frontSide ) const
{
	ASSERT( token.GetEntity() );
	const IExploration* e = token.GetExploration();

	Vector p1, p2;
	e->GetEdgeWS( p1, p2 );

	const Vector& entPos = token.GetEntity()->GetWorldPositionRef();
	const Vector projected = MathUtils::GeometryUtils::ProjectPointOnLine( entPos, p1, p2 );
	const Vector dirToEdge = (projected - entPos).Normalized2();
	const Vector dirToPointOnEdge = (location - entPos).Normalized2();
	frontSide = normal.Dot2( dirToEdge ) < 0.f;

	//HACK - sometimes npc miss exploration ledge
	const Int32 eId = token.GetExploration()->GetId();
	if ( frontSide && eId == ET_Ledge )
	{
		Vector expPos = ( p1 + p1 ) * 0.5f;
		//if npc is close to the top edge...
		if( Abs( expPos.Z - entPos.Z ) < 0.1f )
		{
			//...and is facing proper direction
			if( normal.Dot2( token.GetEntity()->GetWorldForward() ) )
			{
				frontSide = false;
			}
		}		
	}
	// End Of HACK

	Vector py;
	if ( token.m_queryContext.m_inputDirectionInWorldSpace.SquareMag2() > 0.0f )
	{
		py = token.m_queryContext.m_inputDirectionInWorldSpace;
	}
	else
	{
		py = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraForward();
	}
	desAngle = RAD2DEG( MAcos_safe( dirToPointOnEdge.Dot2( py ) ) );

	py = token.GetEntity()->GetLocalToWorld().GetAxisY();
    angle = RAD2DEG( MAcos_safe( dirToEdge.Dot2( py ) ) );

	Vector dirToEdgeSide = Vector( dirToEdge.Y, -dirToEdge.X, 0.0f );
	angle = angle * (dirToEdgeSide.Dot2( py ) >= 0.0f? 1.0f : -1.0f);

	dist = location.DistanceTo2D( entPos );
}

// We assume the ladder is vertical and all the steps are equidistant. that the edge starts at a step
void ExpOracleHardCoded::AdjustPointOnEdgeToLadderStep( const SExplorationQueryToken & token, Vector & pointOnEdge ) const
{
	const IExploration* e	= token.GetExploration();
	Float stepDistance;	
	
	const IExplorationDesc*  description = GetDesc( e->GetId() );
	description->UseEdgeGranularity( stepDistance );

	Vector p1, p2;
	e->GetEdgeWS( p1, p2 );

	// Find the closest step
	Float	distance	= pointOnEdge.Z - p1.Z;
	Float	steps		= distance / stepDistance;
	Float	exactStep	= MCeil( steps ); // Ceil is better in case the ladder is in the ground MRound

	// Recalc point on ladder
	distance			= p1.Z + exactStep * stepDistance;
	pointOnEdge.Z		= distance;
}

void ExpOracleHardCoded::CalcAbove( const SExplorationQueryToken & token, Bool& above ) const
{
	const IExploration* e = token.GetExploration();

	const Vector& entPos = token.GetEntity()->GetWorldPositionRef();

	Vector p1, p2;
	e->GetEdgeWS( p1, p2 );

	const IExplorationDesc* desc = GetDesc( e->GetId() );
	if( desc && !desc->IsHorizontal() )
	{
		Vector nz = p2 - p1;
		nz.Normalize3();

		Float offS, offE;
		desc->GetEdgePointsOffset( offS, offE );
		Vector endPoint = p2 - (nz * offE);

		Vector pe = endPoint - entPos;
		pe.Normalize3();
		const Float dot3 = pe.Dot3( nz );
		above = dot3 < 0.f;
	}
	else
	{
		//TODO
		above = false;
	}
}

void ExpOracleHardCoded::CalcDistAndSideTo( const SExplorationQueryToken & token, const ExecutorSetup& setup, Float& dist, Float& sideOfApproach, Bool& frontSide, Bool& above ) const
{
	const IExploration* e = token.GetExploration();

	Vector py = setup.m_entity->GetLocalToWorld().GetAxisY();

	Vector location;
	Vector normal;
	CalcLocation( token, location, normal, 0.0f, false );

	Vector p1, p2;
	e->GetEdgeWS( p1, p2 );

	const Vector& entPos = setup.m_entity->GetWorldPositionRef();
	const Vector projected = MathUtils::GeometryUtils::ProjectPointOnLine( entPos, p1, p2 );
	const Vector dirToEdge = (projected - entPos).Normalized2();
	frontSide = normal.Dot2( dirToEdge ) < 0.f;

	Vector nx = Vector::Cross( normal, Vector::EZ );

	const Float dot2 = ( nx ).Dot2( py );
	sideOfApproach = dot2;

	dist = entPos.DistanceToEdge( p1, p2 );

	CalcAbove( token, above );
}

IExpExecutor* ExpOracleHardCoded::CreateTransitionJump( const IExploration* from, const IExploration* to, ExecutorSetup& setup, const IExplorationDesc* descTo ) const
{
	static CName animNameL( TXT("child_run_jump_left_leg") );
	static CName animNameR( TXT("child_run_jump_right_leg") );			
	static CName horseJump( TXT("horse_gallop_jump") );			

	CName jumpAnimName = horseJump;				

	CVehicleComponent* vehicleCmp = setup.m_entity->FindComponent< CVehicleComponent >();
	if( !vehicleCmp || !vehicleCmp->IsHorse() )
	{			
		const CMovingAgentComponent* mac = SafeCast<CMovingAgentComponent>( setup.m_entity->GetRootAnimatedComponent() );									

		if( mac )
		{
			const ISkeletonDataProvider* provider = mac->QuerySkeletonDataProvider();
			if ( provider )
			{
						
				Int32 lfootIndex = provider->FindBoneByName( TXT("l_foot") );
				Int32 rfootIndex = provider->FindBoneByName( TXT("r_foot") );
						
				if( lfootIndex != -1 && rfootIndex != -1 )
				{
					Vector lfootPos =  mac->GetBoneMatrixWorldSpace( lfootIndex ).GetTranslation();
					Vector rfootPos =  mac->GetBoneMatrixWorldSpace( rfootIndex ).GetTranslation();

					Vector enPos = setup.m_entity->GetWorldPosition();
					float lfootDistance = ( enPos - lfootPos ).SquareMag3();
					float rfootDistance = ( enPos - rfootPos ).SquareMag3();

					if( rfootDistance < lfootDistance )
					{
						animNameL = animNameR;
					}
				}
			}
		}
	}
	/*static CName preJumpAnim( TXT("ladder_bottom_left_stop") );
			
	ExpToPointSliderExecutor* preLeft = new ExpToPointSliderExecutor( to, descTo, setup, preJumpAnim, 0.f, 0.1f, 0.5f );
*/
	Float blendIn, blendOut, earlyEndOffset;
	descTo->GetBlendsAndEnd( blendIn, blendOut, earlyEndOffset );
				
	
	ExpSingleAnimExecutor* exe = new ExpSingleAnimExecutor( setup, jumpAnimName, blendIn, blendOut, earlyEndOffset );
				

	CName raiseBehaviorEventAtEnd, callScriptEventAtEnd;
	descTo->GetEventsAtEnd( raiseBehaviorEventAtEnd, callScriptEventAtEnd );
	exe->RaiseBehaviorEventAtEnd( raiseBehaviorEventAtEnd );
	exe->CallScriptEventAtEnd( callScriptEventAtEnd );
	return exe;		
}

IExpExecutor* ExpOracleHardCoded::CreateTransitionLedge_Old( const IExploration* from, const IExploration* to, ExecutorSetup& setup, const IExplorationDesc* descTo ) const
{
	Float blendIn, blendOut, earlyEndOffset;
	descTo->GetBlendsAndEnd( blendIn, blendOut, earlyEndOffset );

	// get angle to edge - we want to decide which of animations should we use
	Float cliffDist = 0.f;
	Float cliffAngle = 0.f;
	Float cliffDesAngle = 0.f;
	Bool cliffFrontSide= true;
	Vector cliffLoc = Vector::ZEROS;
	Float cliffHeight = 0.0f;
	CalcDistAngleHeightAndLocation( setup.m_token, cliffDist, cliffAngle, cliffDesAngle, cliffFrontSide, cliffLoc, 0.0f, 0.5f, cliffHeight );

	Float minDistanceBeyondEdge = 0.25f;
	const CMovingAgentComponent* mac = SafeCast<CMovingAgentComponent>( setup.m_entity->GetRootAnimatedComponent() );
	Float moveSpeed = mac ? mac->GetVelocity().Mag2() : 0.f;

	CName raiseAtEnd, callScriptEventAtEnd;
	descTo->GetEventsAtEnd( raiseAtEnd, callScriptEventAtEnd );

	raiseAtEnd = moveSpeed > 2.5f? CNAME( ForceRunLeftUp ) : CNAME( ForceIdleAndWait );

	ExpPrePostExecutor* mainExe = new ExpPrePostExecutor(to, descTo, setup );

	CName animNameUp;
	CName animNameDown;

	Bool alignTowardsInside = false;

	if ( setup.m_token.m_queryContext.m_forJumping )
	{
		animNameUp = CNAME( ledge_from_air_short );
		animNameDown = CNAME( ledge280_center_down_short ); // TODO - do we need it?
		raiseAtEnd = CNAME( ForceIdleAndWait );
		earlyEndOffset = 0.5f;
	}
	else if ( setup.m_token.m_initialState.m_physicsState == CPS_Swimming )
	{
		// just assume where water level should be
		Float waterLevel = setup.m_entity->GetWorldPosition().Z + 1.0f;
		Float expDiffZ = cliffLoc.Z - waterLevel;

		Int32 animIdx = 0;
		if ( expDiffZ < 0.25f )
		{
			animIdx = 0;
			setup.SetAlignTransToEdge( false );
		}
		else if ( expDiffZ < 0.75f )
		{
			animIdx = 1 + GEngine->GetRandomNumberGenerator().Get< Int32 >( 2 );
		}
		else
		{
			animIdx = 3;
		}
		CName animNamesCU[] = { CNAME( swim_ledge0_center ),
			CNAME( swim_ledge50_center ),
			CNAME( swim_ledge50_center_2 ),
			CNAME( swim_ledge100_center ) };
		CName animNamesLU[] = { CNAME( swim_ledge0_left ),
			CNAME( swim_ledge50_left ),
			CNAME( swim_ledge50_left_2 ),
			CNAME( swim_ledge100_left ) };
		CName animNamesRU[] = { CNAME( swim_ledge0_right ),
			CNAME( swim_ledge50_right ),
			CNAME( swim_ledge50_right_2 ),
			CNAME( swim_ledge100_right ) };
		//animNameUp = Abs(cliffAngle) < 30.0f? animNamesCU[animIdx] : ( cliffAngle > 0.0f? animNamesLU[animIdx] : animNamesRU[animIdx] );
		animNameUp = CNAME( swim_ledge50_center_idle );
		animNameDown = CNAME( ledge280_center_down_short ); // TODO - do we need it?
		blendIn = 0.3f;
		blendOut = 0.0f;
		earlyEndOffset = 0.35f;
		raiseAtEnd = CNAME( ForceIdleAndWait );
	}
	else
	{
		Int32 animIdx = 0;
		if ( cliffHeight < SMALL_LEDGE_LIMIT )
		{
			animIdx = 0;
			setup.SetEdgeBone( CNAME( r_toe ) );
			if ( moveSpeed < 2.5f )
			{
				setup.SetEdgeOffset( Vector( 0.0f, 0.3f, 0.02f, 1.0f ) );
				earlyEndOffset = 0.5f;
			}
			else
			{
				setup.SetEdgeOffset( Vector( 0.0f, 0.5f, 0.02f, 1.0f ) );
				blendIn = 0.1f;
				blendOut = 0.15f;
				earlyEndOffset = 0.15f;
			}
			alignTowardsInside = true;
			setup.SetAlignRotToEdge( true, 40.0f );
			minDistanceBeyondEdge = 0.4f;
		}
		else if ( cliffHeight < 2.40f )
		{
			animIdx = 1;
			if ( moveSpeed < 2.5f )
			{
				setup.SetEdgeOffset( Vector( 0.0f, 0.01f, 0.07f, 1.0f ) );
				earlyEndOffset = 0.5f;
			}
			else
			{
				//setup.SetEdgeOffset( Vector( 0.0f, 0.1f, 0.02f, 1.0f ) );
				blendIn = 0.1f;
				blendOut = 0.15f;
				earlyEndOffset = 0.15f;
			}
			minDistanceBeyondEdge = 0.4f;
		}
		else
		{
			animIdx = 2;
			raiseAtEnd = CNAME( ForceIdleAndWait );
			earlyEndOffset = 0.5f;
		}

		// up
		if ( moveSpeed < 2.5f )
		{
			CName animNamesCU[] = { CNAME( ledge80_center_short ),
				CNAME( ledge200_center_short ),
				CNAME( ledge280_center_up_short ) };
			CName animNamesLU[] = { CNAME( ledge80_left_short ),
				CNAME( ledge200_left_short ),
				CNAME( ledge280_left_up_short ) };
			CName animNamesRU[] = { CNAME( ledge80_right_short ),
				CNAME( ledge200_right_short ),
				CNAME( ledge280_right_up_short ) };
			animNameUp = Abs(cliffAngle) < 70.0f? animNamesCU[animIdx] : ( cliffAngle > 0.0f? animNamesLU[animIdx] : animNamesRU[animIdx] );
		}
		else
		{
			CName animNamesCU[] = { CNAME( ledge80_center_long ),
				CNAME( ledge200_center_long ),
				CNAME( ledge280_center_up_short ) };
			CName animNamesLU[] = { CNAME( ledge80_left_long ),
				CNAME( ledge200_left_long ),
				CNAME( ledge280_left_up_short ) };
			CName animNamesRU[] = { CNAME( ledge80_right_long ),
				CNAME( ledge200_right_long ),
				CNAME( ledge280_right_up_short ) };
			animNameUp = Abs(cliffAngle) < 70.0f? animNamesCU[animIdx] : ( cliffAngle > 0.0f? animNamesLU[animIdx] : animNamesRU[animIdx] );
		}
		RED_LOG(ExplorationOracle, TXT("ledge up %i %s"), animIdx, animNameUp.AsChar());

		// down
		{
			CName animNamesCD[] = { CNAME( ledge280_center_down_short ), // TODO - do we need it?
				CNAME( ledge280_center_down_short ), // TODO - do we need it?
				CNAME( ledge280_center_down_short ) };
			CName animNamesLD[] = { CNAME( ledge280_left_down_short ), // TODO - do we need it?
				CNAME( ledge280_left_down_short ), // TODO - do we need it?
				CNAME( ledge280_left_down_short ) };
			CName animNamesRD[] = { CNAME( ledge280_right_down_short ), // TODO - do we need it?
				CNAME( ledge280_right_down_short ), // TODO - do we need it?
				CNAME( ledge280_right_down_short ) };
			animNameDown = Abs(cliffAngle) < 40.0f? animNamesCD[animIdx] : ( cliffAngle > 0.0f? animNamesLD[animIdx] : animNamesRD[animIdx] );
		}
		RED_LOG(ExplorationOracle, TXT("ledge down %i %s"), animIdx, animNameDown.AsChar());
	}

	ExpSlideExecutor* preExe = new ExpSlideExecutor( to, descTo, setup, animNameUp, blendIn, blendOut, earlyEndOffset, false, true, true, alignTowardsInside );

	ExpSlideExecutor* postExe = new ExpSlideExecutor( to, descTo, setup, animNameDown, blendIn, blendOut, earlyEndOffset, true, true, true );

	preExe->SetMinDistBeyondEdge( minDistanceBeyondEdge );

	mainExe->ConnectPre(preExe);
	mainExe->ConnectPost(postExe);

	preExe->EndWhenBlendingOut();
	postExe->EndWhenBlendingOut();

	preExe->RaiseBehaviorEventAtEnd( raiseAtEnd ); // override what comes from config
	preExe->CallScriptEventAtEnd( callScriptEventAtEnd );
	postExe->RaiseBehaviorEventAtEnd( raiseAtEnd ); // override what comes from config
	postExe->CallScriptEventAtEnd( callScriptEventAtEnd );

	return mainExe;
}

IExpExecutor* ExpOracleHardCoded::CreateTransitionLedge_New( const IExploration* from, const IExploration* to, ExecutorSetup& setup, const IExplorationDesc* descTo ) const
{
	Float blendIn, blendOut, earlyEndOffset;
	descTo->GetBlendsAndEnd( blendIn, blendOut, earlyEndOffset );

	// get angle to edge - we want to decide which of animations should we use
	Float cliffDist = 0.f;
	Float cliffAngle = 0.f;
	Float cliffDesAngle = 0.f;
	Bool cliffFrontSide= true;
	Vector cliffLoc = Vector::ZEROS;
	Float cliffHeight = 0.0f;
	CalcDistAngleHeightAndLocation( setup.m_token, cliffDist, cliffAngle, cliffDesAngle, cliffFrontSide, cliffLoc, 0.0f, 0.5f, cliffHeight );

	Float minDistanceBeyondEdge = 0.25f;
	const CMovingAgentComponent* mac = SafeCast<CMovingAgentComponent>( setup.m_entity->GetRootAnimatedComponent() );
	Float moveSpeed = mac ? mac->GetVelocity().Mag2() : 0.f;

	CName raiseAtEnd, callScriptEventAtEnd;
	descTo->GetEventsAtEnd( raiseAtEnd, callScriptEventAtEnd );

	raiseAtEnd = moveSpeed > 2.5f? CNAME( ForceRunLeftUp ) : CNAME( ForceIdleAndWait );

	ExpPrePostExecutor* mainExe = new ExpPrePostExecutor(to, descTo, setup );

	CName animNameUp;
	CName animNameDown;

	Bool alignTowardsInside = false;

	if ( setup.m_token.m_initialState.m_physicsState == CPS_Swimming )
	{
		// just assume where water level should be
		Float waterLevel = setup.m_entity->GetWorldPosition().Z + 1.0f;
		Float expDiffZ = cliffLoc.Z - waterLevel;
		/*
		Int32 animIdx = 0;
		if ( expDiffZ < 0.25f )
		{
			animIdx = 0;
			setup.SetAlignTransToEdge( false );
		}
		else if ( expDiffZ < 0.75f )
		{
			animIdx = 1 + GEngine->GetRandomNumberGenerator().Get< Int32 >( 2 );
		}
		else
		{
			animIdx = 3;
		}
		CName animNamesCU[] = { CNAME( swim_ledge0_center ),
			CNAME( swim_ledge50_center ),
			CNAME( swim_ledge50_center_2 ),
			CNAME( swim_ledge100_center ) };
		CName animNamesLU[] = { CNAME( swim_ledge0_left ),
			CNAME( swim_ledge50_left ),
			CNAME( swim_ledge50_left_2 ),
			CNAME( swim_ledge100_left ) };
		CName animNamesRU[] = { CNAME( swim_ledge0_right ),
			CNAME( swim_ledge50_right ),
			CNAME( swim_ledge50_right_2 ),
			CNAME( swim_ledge100_right ) };
		//animNameUp = Abs(cliffAngle) < 30.0f? animNamesCU[animIdx] : ( cliffAngle > 0.0f? animNamesLU[animIdx] : animNamesRU[animIdx] );
		*/
		//if( from-> )
		if( setup.m_token.m_type == ET_Boat_Enter_From_Beach )
		{
			animNameUp = CNAME( swim_entrance_on_boat );
		}
		else
		{
			animNameUp = CNAME( swim_ledge50_center_idle );
		}
		//animNameUp = CNAME( swim_entrance_on_boat );

		animNameDown = CNAME( ledge280_center_down_short ); // TODO - do we need it?
		blendIn = 0.3f;
		blendOut = 0.0f;
		earlyEndOffset = 0.35f;
		raiseAtEnd = CNAME( ForceIdleAndWait );
	}
	else 
	{
		Int32 animIdx = 0;
		if ( cliffHeight < SMALL_LEDGE_LIMIT )
		{
			animIdx = 0;
			setup.SetEdgeBone( CNAME( r_toe ) );
			if ( moveSpeed < 2.5f )
			{
				setup.SetEdgeOffset( Vector( 0.0f, 0.3f, 0.02f, 1.0f ) );
				earlyEndOffset = 0.5f;
			}
			else
			{
				setup.SetEdgeOffset( Vector( 0.0f, 0.5f, 0.02f, 1.0f ) );
				blendIn = 0.1f;
				blendOut = 0.15f;
				earlyEndOffset = 0.15f;
			}
			alignTowardsInside = true;
			setup.SetAlignRotToEdge( true, 40.0f );
			minDistanceBeyondEdge = 0.4f;
		}
		else if ( cliffHeight < 2.40f )
		{
			animIdx = 1;
			if ( moveSpeed < 2.5f )
			{
				setup.SetEdgeOffset( Vector( 0.0f, 0.01f, 0.07f, 1.0f ) );
				earlyEndOffset = 0.5f;
			}
			else
			{
				//setup.SetEdgeOffset( Vector( 0.0f, 0.1f, 0.02f, 1.0f ) );
				blendIn = 0.1f;
				blendOut = 0.15f;
				earlyEndOffset = 0.15f;
			}
			minDistanceBeyondEdge = 0.4f;
		}
		else
		{
			animIdx = 2;
			raiseAtEnd = CNAME( ForceIdleAndWait );
			earlyEndOffset = 0.5f;
		}

		// up
		//cliffAngle to remove
		CName animNamesCU[] = { CNAME( climb_idle_100 ),CNAME( climb_idle_200 ),CNAME( climb_idle_300 ) };			
		animNameUp = animNamesCU[animIdx];		
		
		// down
		CName animNamesCD[] = { CNAME( idle_climb_down_100 ), CNAME( idle_climb_down_300 ), CNAME( idle_climb_down_300 ) };			
		animNameDown = animNamesCD[animIdx];
	}

	ExpSlideExecutor* preExe = new ExpSlideExecutor( to, descTo, setup, animNameUp, blendIn, blendOut, earlyEndOffset, false, true, true, alignTowardsInside );

	Bool swapSides = true;	
	if( animNameDown == CNAME( idle_climb_down_100 ) )
	{		
		setup.SetEdgeBone( CName::NONE );
		setup.SetEdgeOffset( Vector( 0.0f, 0.01f, 0.1f, 1.0f ) );
	}

	//															  setup, animName	 , blendIn, blendOut, earlyEndOffset, swapSide, alignWhenCloseToEnd, blockCloseToEnd, alignTowardsInside );
	ExpSlideExecutor* postExe = new ExpSlideExecutor( to, descTo, setup, animNameDown, blendIn, blendOut, earlyEndOffset, swapSides, true              , true           , false );

	preExe->SetMinDistBeyondEdge( minDistanceBeyondEdge );

	mainExe->ConnectPre(preExe);
	mainExe->ConnectPost(postExe);

	preExe->EndWhenBlendingOut();
	postExe->EndWhenBlendingOut();

	preExe->RaiseBehaviorEventAtEnd( raiseAtEnd ); // override what comes from config
	preExe->CallScriptEventAtEnd( callScriptEventAtEnd );
	postExe->RaiseBehaviorEventAtEnd( raiseAtEnd ); // override what comes from config
	postExe->CallScriptEventAtEnd( callScriptEventAtEnd );

	// calc side
	Vector p1, p2, n;
	to->GetEdgeWS( p1, p2 );
	const Vector expBegin	= ( p1 + p2 ) * 0.5f;
	const Vector expEnd( expBegin.X, expBegin.Y, expBegin.Z - cliffHeight );	
	const Vector& userPos = setup.m_entity->GetWorldPositionRef();

	if( setup.m_token.m_initialState.m_physicsState != CPS_Swimming && ( expBegin - userPos ).SquareMag3() < ( expEnd - userPos ).SquareMag3()  )
	{
		//climb down
		mainExe->ForceSide( ExpPrePostExecutor::InternalState::IS_Post );
	}
	else
	{
		//climb up
		mainExe->ForceSide( ExpPrePostExecutor::InternalState::IS_Pre );
	}

	return mainExe;
}

IExpExecutor* ExpOracleHardCoded::CreateTransitionLedge( const IExploration* from, const IExploration* to, ExecutorSetup& setup, const IExplorationDesc* descTo ) const
{
	Bool useNew = true;
	if( useNew )
	{
		return CreateTransitionLedge_New( from, to, setup, descTo );
	}
	else
	{
		return CreateTransitionLedge_Old( from, to, setup, descTo );
	}	
}

IExpExecutor* ExpOracleHardCoded::CreateTransitionLadder( const IExploration* from, const IExploration* to, ExecutorSetup& setup, const IExplorationDesc* descTo, Bool above, Bool frontSide, Float sideOfApproach, Float distTo  ) const
{
	static CName a1( TXT("ladder_left_up_40") );
	static CName a2( TXT("ladder_right_up_40") );
	static CName a3( TXT("ladder_left_down_40") );
	static CName a4( TXT("ladder_right_down_40") );

	Float blendIn, blendOut, earlyEndOffset;
	descTo->GetBlendsAndEnd( blendIn, blendOut, earlyEndOffset );

	ExpManualStepsExecutor* stepsExe = new ExpManualStepsExecutor( from, descTo, setup );
	stepsExe->ConnectSteps( to, setup, a1, a2, a3, a4 );

	
	static CName breakAnim1( TXT("ladder_left_jumpdown") );
	static CName breakAnim2( TXT("ladder_right_jumpdown") );

	ExpBreakTransition* break1Transition = new ExpBreakTransition( setup, breakAnim1, CNAME( OnInAirStarted ), 0.25f );
	ExpBreakTransition* break2Transition = new ExpBreakTransition( setup, breakAnim2, CNAME( OnInAirStarted ), 0.25f );
	break1Transition->EndWhenBlendingOut();
	break2Transition->EndWhenBlendingOut();
	stepsExe->ConnectBreak( break1Transition, break2Transition );
	

	ExpPreLoopPostExecutor* mainExe = new ExpPreLoopPostExecutor( to, descTo, setup );

	static CName preAnim( TXT("ladder_bottom_left_stop") );
	static CName postAnim( TXT("ladder_top_left_stop_long") );

	static CName preAnimR( TXT("ladder_bottom_right_stop") );
	static CName postAnimR( TXT("ladder_top_right_stop_long") );

	mainExe->ConnectLoop( stepsExe );
	
	ExpToPointSliderExecutor* preLeft = new ExpToPointSliderExecutor( to, descTo, setup, preAnim, 0.f, blendOut, 0.5f );
	ExpToPointSliderExecutor* postLeft = new ExpToPointSliderExecutor( to, descTo, setup, postAnim, 0.f, blendOut, 0.2f );

	ExpToPointSliderExecutor* preRight = new ExpToPointSliderExecutor( to, descTo, setup, preAnimR, 0.f, blendOut, 0.5f );
	ExpToPointSliderExecutor* postRight = new ExpToPointSliderExecutor( to, descTo, setup, postAnimR, 0.f, blendOut, 0.2f );
	
	/*
	ExpToPointSliderExecutor* preLeft = new ExpToPointSliderExecutor( to, descTo, setup, preAnim, 0.f, .0f, 0.0f );
	ExpToPointSliderExecutor* postLeft = new ExpToPointSliderExecutor( to, descTo, setup, postAnim, 0.f, .0f, 0.0f );

	ExpToPointSliderExecutor* preRight = new ExpToPointSliderExecutor( to, descTo, setup, preAnimR, 0.f, .0f, 0.0f );
	ExpToPointSliderExecutor* postRight = new ExpToPointSliderExecutor( to, descTo, setup, postAnimR, 0.f, .0f, 0.0f );
	*/
	preLeft->EndWhenBlendingOut();
	postLeft->EndWhenBlendingOut();

	preRight->EndWhenBlendingOut();
	postRight->EndWhenBlendingOut();

	CName raiseBehaviorEventAtEnd, callScriptEventAtEnd;
	descTo->GetEventsAtEnd( raiseBehaviorEventAtEnd, callScriptEventAtEnd );
	preLeft->RaiseBehaviorEventAtEnd( raiseBehaviorEventAtEnd );
	preLeft->CallScriptEventAtEnd( callScriptEventAtEnd );
	postLeft->RaiseBehaviorEventAtEnd( raiseBehaviorEventAtEnd );
	postLeft->CallScriptEventAtEnd( callScriptEventAtEnd );
	preRight->RaiseBehaviorEventAtEnd( raiseBehaviorEventAtEnd );
	preRight->CallScriptEventAtEnd( callScriptEventAtEnd );
	postRight->RaiseBehaviorEventAtEnd( raiseBehaviorEventAtEnd );
	postRight->CallScriptEventAtEnd( callScriptEventAtEnd );

	mainExe->ConnectPreLeft( preLeft );
	mainExe->ConnectPostLeft( postLeft );

	mainExe->ConnectPreRight( preRight );
	mainExe->ConnectPostRight( postRight );

	setup.SetAlignTransToEdge( true );


	if( above ) // TODO get better animations to use from other side && !frontSide )
	{
		CName transAnim;
		CName blendToAnim;
		Int32 startingSide;

		// Npcs have less animations
		if( true )// This is temporarily commented for Ciri, cause she has not proper anims yet !setup.m_token.GetEntity()->IsPlayer() )
		{
			transAnim		= CNAME( ladder_center_start_top );
			blendToAnim		= CNAME( ladder_right_up_40 );
			startingSide	= 0;
		}
		else 
		{
			if( sideOfApproach < -0.4f )
			{
				transAnim		= CNAME( ladder_left_start_top );
				blendToAnim		= CNAME( ladder_left_up_40 );
				startingSide	= 1;
			}
			else if( sideOfApproach < 0.4f )
			{
				transAnim		= CNAME( ladder_center_start_top );
				blendToAnim		= CNAME( ladder_left_up_40 );
				startingSide	= 1;
			}
			else
			{
				transAnim		= CNAME( ladder_right_start_top );
				blendToAnim		= CNAME( ladder_right_up_40 );
				startingSide	= 0;
			}
		}

		ExpBlendChainExecutor* chainer = new ExpBlendChainExecutor( setup, mainExe, transAnim, blendToAnim, 0.0f );
		ExpBaseTransitionExecutor* transition = new ExpBaseTransitionExecutor( to, descTo, setup, transAnim, chainer, true );
		stepsExe->SetStartingSide( startingSide );
		stepsExe->AutoGoInDir( -1.0f );
		return transition;
	}
	else // we are at the bottom
	{
		stepsExe->AutoGoInDir( 1.0f );

		CName transAnim;		
		CName blendToAnim;
		Int32 startingSide;

		// Npcs have less animation
		if( !setup.m_token.GetEntity()->IsPlayer() )
		{
			transAnim		= CNAME( ladder_center_start_bottom_short );
			blendToAnim		= CNAME( ladder_right_up_40 );
			startingSide	= 1;
		}
		else if ( setup.m_token.m_initialState.m_physicsState == CPS_Swimming )
		{
			transAnim		= CNAME( man_swimming_ladder_start_hand_l );
			blendToAnim		= CNAME( ladder_right_up_40 );
			startingSide	= 0;
		}
		else
		{
			static Float distToLadder	= 2.0f;
			Bool onLeftSide				= sideOfApproach < 0.0f;
			Int32 sideIdx				= Abs( sideOfApproach ) < 0.4f? 1 :
				( frontSide ^ onLeftSide ? 2 : 0 ); // right : left - check above

			CName blendToAnims[] = { CNAME( ladder_left_up_40 ),
				CNAME( ladder_left_up_40 ),
				CNAME( ladder_right_up_40 ) };
			CName transAnims[] = { CNAME( ladder_left_start_bottom_short ),
				CNAME( ladder_center_start_bottom_short ), 
				CNAME( ladder_right_start_bottom_short ) };
			Int32 startingSides[] = { 1, 0, 0 };
			transAnim = transAnims[sideIdx];		//CNAME( ladder_center_start_bottom_short );
			blendToAnim = blendToAnims[sideIdx];
			startingSide = startingSides[sideIdx]; //ED: changed from to 0 since transanim is hardcoded
		}


		mainExe->CalculateAnimationZToSwitchToPost();

		ExpBlendChainExecutor* chainer = new ExpBlendChainExecutor( setup, mainExe, transAnim, blendToAnim, 0.0f ); // ED: Set blend from 0.1 to 0
		ExpBaseTransitionExecutor* transition = new ExpBaseTransitionExecutor( to, descTo, setup, transAnim, chainer );
		stepsExe->SetStartingSide( startingSide );
		return transition;
	}

	delete mainExe;
	return nullptr;
}

IExpExecutor* ExpOracleHardCoded::CreateTransition( const IExploration* from, const IExploration* to, ExecutorSetup& setup ) const
{
	const Int32 fromId = from ? from->GetId() : -1;
	const Int32 toId = to->GetId();

	const IExplorationDesc* descTo = GetDesc( toId );
	if ( !descTo )
	{
		return NULL;
	}

	Float distTo, sideOfApproach;
	Bool leftSide, frontSide, above;
	CalcDistAndSideTo( setup.m_token, setup, distTo, sideOfApproach, frontSide, above );
	leftSide = sideOfApproach > 0.0f;

	setup.m_useLeftSideForEdgeBone = leftSide;
	setup.m_desc = descTo;

	if ( fromId == -1 )
	{
		// Create transition from gameplay to exploration
		switch ( toId )
		{
		case ET_Jump: //Used only for npcs
			{			
				return CreateTransitionJump( from, to, setup, descTo );
			}
		case ET_Fence:
		case ET_Fence_OneSided:
			{
				CName animName;

				Float expHeight;
				Float expHeightBehind;
				Vector fenceLoc;
				Vector fenceNormal;
				CalcHeightLocationAndNormal( setup.m_token, 0.5f, expHeight, &expHeightBehind, fenceLoc, fenceNormal, 0.0f );

				Float blendIn, blendOut, earlyEndOffset;
				descTo->GetBlendsAndEnd( blendIn, blendOut, earlyEndOffset );

				const CMovingAgentComponent* mac = SafeCast<CMovingAgentComponent>( setup.m_entity->GetRootAnimatedComponent() );
				Float moveSpeed = mac ? mac->GetVelocity().Mag2() : 0.f;
				Bool willFallAfterwards = expHeightBehind > expHeight + 1.0f;

				ExpBaseExecutor* exe = NULL;

				Int32 animIdx = 0;
				if ( expHeight < SMALL_FENCE_LIMIT ) { animIdx = 0; } else
				if ( expHeight < 1.80f ) { animIdx = 1; } else
										 { animIdx = 2; }
				Bool onLeftSide = sideOfApproach < 0.0f;
				Bool alignTowardsInside = animIdx < 2;
				
				CName raiseBehaviorEventAtEnd, callScriptEventAtEnd;
				descTo->GetEventsAtEnd( raiseBehaviorEventAtEnd, callScriptEventAtEnd );

				raiseBehaviorEventAtEnd = CNAME( ForceIdleAndWait );

				if ( moveSpeed > 2.5f )
				{
					raiseBehaviorEventAtEnd = CNAME( ForceRunLeftUp );
					blendIn *= 0.4f;
					blendOut = 0.15f;
					earlyEndOffset = 0.15f;
				}
				else
				{
					blendOut = 0.1f;
					earlyEndOffset = animIdx == 0? 0.1f : 0.3f; // to immediately leave when blending out has started - to trigger player locomotion action to update player speed
				}

				if ( animIdx == 0 )
				{
					// this is the same as for ledge as we're using ledge anim for very small fence
					setup.SetEdgeBone( CNAME( r_toe ) );
					setup.SetEdgeOffset( Vector( 0.0f, 0.1f, 0.2f, 1.0f ) );
					blendIn = 0.1f;
					blendOut = 0.15f;
					earlyEndOffset = 0.15f;
					alignTowardsInside = true;
					setup.SetAlignRotToEdge( true, 40.0f );
				}
				if ( animIdx == 2 )
				{
					sideOfApproach = 0.0f; // from front only
					setup.SetEdgeOffset( Vector( 0.0f, -0.11f, -0.1f, 1.0f ) );
					setup.SetAlignRotToEdge( true, 0.0f );
				}

				Int32 animIdxOffset = 0;
				if ( willFallAfterwards && animIdx != 0 )
				{
					raiseBehaviorEventAtEnd = CNAME( ForceFall );
					blendOut = 0.15f;
					earlyEndOffset = 0.15f;
					animIdxOffset = 3;
				}

				if ( expHeightBehind < 1.5f && expHeight > 2.1f )
				{
					// jumping into window	
					setup.m_useLeftSideForEdgeBone = false; // use right hand
					animName = CNAME( window280_center_short );
					earlyEndOffset = 0.0f;

					RED_LOG(ExplorationOracle, TXT("fence - window %s"), animName.AsChar());
					exe = new ExpSlideExecutor( to, descTo, setup, animName, blendIn, blendOut, earlyEndOffset, false, false, true, alignTowardsInside );
				}
				else if ( Abs( sideOfApproach ) < 0.4f )
				{
					// center
					setup.m_useLeftSideForEdgeBone = false; // use right hand

					// run or stand
					if ( moveSpeed > 2.5f )
					{
						static CName animNames[] = { CNAME( ledge80_center_long ), // TODO
													 CNAME( fence120_center_long ),
													 CNAME( fence280_center_long ),
													 CNAME( ledge80_center_long ), // TODO
													 CNAME( fence100_center_long_to_fall ), // TODO
													 CNAME( fence100_center_long_to_fall ) }; // TODO
						animName = animNames[animIdx + animIdxOffset];
					}
					else
					{
						static CName animNames[] = { CNAME( ledge80_center_long ), // TODO
													 CNAME( fence120_center_short ),
													 CNAME( fence280_center_short ),
													 CNAME( ledge80_center_long ), // TODO
													 CNAME( fence100_center_short_to_fall ), // TODO
													 CNAME( fence100_center_short_to_fall ) }; // TODO
						animName = animNames[animIdx + animIdxOffset];
					}

					RED_LOG(ExplorationOracle, TXT("fence - centre %i %s"), animIdx, animName.AsChar());
					exe = new ExpSlideExecutor( to, descTo, setup, animName, blendIn, blendOut, earlyEndOffset, false, false, true, alignTowardsInside );
				}
				else if ( frontSide ^ onLeftSide ) // translates into ( front && ! left ) || ( ! front && left ) -> right
				{
					// right! (from right side)
					setup.m_useLeftSideForEdgeBone = false; // use right hand

					if ( animIdx == 1 || animIdx == 3 )
					{
						setup.m_useLeftSideForEdgeBone = true; // use left hand
					}

					// run or stand
					if ( moveSpeed > 2.5f )
					{
						CName animNames[] = { CNAME( ledge80_left_long ), // TODO
											  CNAME( fence120_center_long ),
											  CNAME( fence280_center_long ),
											  CNAME( ledge80_left_long ), // TODO
											  CNAME( fence100_center_long_to_fall ), // TODO
											  CNAME( fence100_center_long_to_fall )}; // TODO
						animName = animNames[animIdx + animIdxOffset];
					}
					else
					{
						CName animNames[] = { CNAME( ledge80_left_long ), // TODO
											  CNAME( fence120_left_short ),
											  CNAME( fence280_center_short ),
											  CNAME( ledge80_left_long ), // TODO
											  CNAME( fence100_center_short_to_fall ), // TODO
											  CNAME( fence100_center_short_to_fall )}; // TODO
						animName = animNames[animIdx + animIdxOffset];
					}

					RED_LOG(ExplorationOracle, TXT("fence - right %i %s"), animIdx, animName.AsChar());
					ExpSlideExecutor* sexe = new ExpSlideExecutor( to, descTo, setup, animName, blendIn, blendOut, earlyEndOffset, false, false, true, alignTowardsInside );
					//sexe->UseYawOffset( -25.0f );
					sexe->UseYawOffset( -20.0f );
					exe = sexe;
				}
				else
				{
					// left (from left side)
					setup.m_useLeftSideForEdgeBone = false; // use right hand

					// run or stand
					if ( moveSpeed > 2.5f )
					{
						CName animNames[] = { CNAME( ledge80_right_long ), // TODO
											  CNAME( fence120_center_long ), // TODO
											  CNAME( fence280_center_long ),
											  CNAME( ledge80_right_long ), // TODO
											  CNAME( fence100_center_long_to_fall ), // TODO
											  CNAME( fence100_center_long_to_fall )}; // TODO
						animName = animNames[animIdx + animIdxOffset];
					}
					else
					{
						CName animNames[] = { CNAME( ledge80_right_long ), // TODO
											  CNAME( fence120_right_short ),
											  CNAME( fence280_center_short ),
											  CNAME( ledge80_right_long ), // TODO
											  CNAME( fence100_center_short_to_fall ), // TODO
											  CNAME( fence100_center_short_to_fall )}; // TODO
						animName = animNames[animIdx + animIdxOffset];
					}

					RED_LOG(ExplorationOracle, TXT("fence - left %i %s"), animIdx, animName.AsChar());
					ExpSlideExecutor* sexe = new ExpSlideExecutor( to, descTo, setup, animName, blendIn, blendOut, earlyEndOffset, false, false, true, alignTowardsInside );
					//sexe->UseYawOffset( 25.0f );
					sexe->UseYawOffset( 20.0f );
					exe = sexe;
				}

				exe->RaiseBehaviorEventAtEnd( raiseBehaviorEventAtEnd );
				exe->CallScriptEventAtEnd( callScriptEventAtEnd );

				return exe;
			}
		case ET_Ledge:
		case ET_Boat_Enter_From_Beach:
            {
                return CreateTransitionLedge( from, to, setup, descTo );
            }
		case ET_Ladder:
			{
				
				return CreateTransitionLadder(from, to, setup, descTo, above, frontSide, sideOfApproach, distTo );
			}
		case ET_Horse_LF:
		case ET_Horse_LB:
        case ET_Horse_L:
        case ET_Horse_RF:
        case ET_Horse_RB:
        case ET_Horse_R:
        case ET_Horse_B:
			{
				static CName animNameLF( TXT("horse_mount_LF") );
				static CName animNameLB( TXT("horse_mount_LB") );
                static CName animNameL( TXT("horse_mount_L") );
                static CName animNameR( TXT("horse_mount_R_01") );
                static CName animNameRF( TXT("horse_mount_RF_01") );
                static CName animNameRB( TXT("horse_mount_RB_01") );
                static CName animNameB( TXT("horse_mount_B_01") );

				CName animName;

                if ( toId == ET_Horse_LF )		animName = animNameLF;
                else if ( toId == ET_Horse_LB )	animName = animNameLB;
                else if ( toId == ET_Horse_L )	animName = animNameL;
                else if ( toId == ET_Horse_RF )	animName = animNameRF;
                else if ( toId == ET_Horse_RB )	animName = animNameRB;
                else if ( toId == ET_Horse_R )	animName = animNameR;
                else if ( toId == ET_Horse_B )	animName = animNameB;

				Float blendIn, blendOut, earlyEndOffset;
				descTo->GetBlendsAndEnd( blendIn, blendOut, earlyEndOffset );

				ExpSlideExecutor* exe = new ExpSlideExecutor( to, descTo, setup, animName, blendIn, blendOut, earlyEndOffset );
				exe->StayInSlotAfterEnd(); // for horse, it should stay at the end
				CName raiseBehaviorEventAtEnd, callScriptEventAtEnd;
				descTo->GetEventsAtEnd( raiseBehaviorEventAtEnd, callScriptEventAtEnd );
				exe->RaiseBehaviorEventAtEnd( raiseBehaviorEventAtEnd );
				exe->CallScriptEventAtEnd( callScriptEventAtEnd );
				return exe;
			}

		case ET_Boat_B:
		case ET_Boat_P:
		case ET_Boat_Passenger_B:
		//case ET_Boat_Enter_From_Beach:
			{
				static CName animNameB( TXT("boat_sit_start_front") );
				static CName animNameP_FromBeach( TXT("boat_beach_enter") );
				static CName animNameP_FromPier( TXT("boat_pier_enter") );
				static CName animNameBPassenger( TXT("boat_passenger_sit_start_front") );

				CName animName;
				if ( toId == ET_Boat_B )
				{
					animName = animNameB;
				}
				else if ( toId == ET_Boat_P )
				{
					animName = animNameP_FromPier;
				}
				else if ( toId == ET_Boat_Enter_From_Beach )
				{
					animName = animNameP_FromBeach;
				}
				else if ( toId == ET_Boat_Passenger_B )
				{
					animName = animNameBPassenger;
				}

				Float blendIn, blendOut, earlyEndOffset;
				descTo->GetBlendsAndEnd( blendIn, blendOut, earlyEndOffset );

				ExpSlideExecutor* exe = nullptr;
				if( ( toId == ET_Boat_B ) || ( toId == ET_Boat_Passenger_B ) )
				{
					exe = new ExpMountBoatExecutor( to, descTo, setup, animName, blendIn, blendOut, earlyEndOffset );
				}
				else
				{
					exe = new ExpSlideExecutor( to, descTo, setup, animName, blendIn, blendOut, earlyEndOffset );
				}

				CName raiseBehaviorEventAtEnd, callScriptEventAtEnd;
				descTo->GetEventsAtEnd( raiseBehaviorEventAtEnd, callScriptEventAtEnd );
				exe->RaiseBehaviorEventAtEnd( raiseBehaviorEventAtEnd );
				exe->CallScriptEventAtEnd( callScriptEventAtEnd );
				return exe;
			}
		}
	}

	return NULL;
}

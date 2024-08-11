#include "build.h"
#include "humbleCrittersAI.h"
#include "humbleCritterAlgorithmData.h"

#include "../../common/game/pointOfInterestSpeciesConfig.h"
#include "../../common/game/boidInstance.h"
#include "../../common/game/boidCone.h"
#include "../../common/core/mathUtils.h"

///////////////////////////////////////////////////////////////
// CHumbleCritterAI
///////////////////////////////////////////////////////////////
CHumbleCritterAI::CHumbleCritterAI()
	: m_algorithmData( NULL )
	, m_stateChanged( false )
	, m_targetActor( INVALID_POI_ID )
	, m_stateTimeout( 0.f )
	, m_randomVector( 0, 0 )
	, m_hunger( 0.f )
	, m_isAlive( false )
	, m_effectorRandom( GEngine->GetRandomNumberGenerator().Get< Float >( -0.5f , 0.5f ) )
	, m_immobile( false )
	, m_velocity( 0.0f, 0.0f )
	, m_onActionPoiID( (Uint32)-1 )
	, m_markedX( -1 )
	, m_markedY( -1 )
{
}


void CHumbleCritterAI::Update( const SSwarmMemberStateData& currState, SSwarmMemberStateData& destState )
{
	// Some vars we'll need all the way down
	const CHumbleCritterLairParams &params		= m_algorithmData->GetHumbleCritterLairParams();
	const Vector currentPosition				= currState.m_position;
	const SwarmEnviromentData* enviroment		= m_algorithmData->GetEnviroment();
	const Float & localTime						= m_algorithmData->GetLocalTime();
	const Float & deltaTime						= m_algorithmData->GetCurrentDelta();

	///////////////////////////////////
	// POI behaviour :
	Vector2 poiGravity( 0.0f, 0.0f ); 
	CPoiJobData * closestPoiJobData							= NULL;
	const CPointOfInterestSpeciesConfig * closestPoiConfig	= NULL;
	CPoiDataByType * closestPoiDataByType					= NULL;
	CPoiJobData * actionPoiJobData							= NULL;
	const CPointOfInterestSpeciesConfig * actionPoiConfig	= NULL;
	CPoiDataByType * actionPoiDataByType					= NULL;

	CPoiJobData_Map& staticPoiDataByType_Map		= m_algorithmData->GetStaticPoiDataByType_Map();
	CPoiJobData_Map::iterator it					= staticPoiDataByType_Map.Begin();
	CPoiJobData_Map::iterator end					= staticPoiDataByType_Map.End();
	
	destState.m_flags &= ~BOID_STATE_FLAG_INSIDE_CIRCLE_POI;
	destState.m_flags &= ~BOID_STATE_FLAG_INSIDE_CONE_POI;
	Float closestPoiDist											= NumericLimits<Float>::Max();
	while ( it != end ) // Looping trough all types of POIs
	{
		const CName& pointOfInterestType								= it->m_first;
		const CPointOfInterestSpeciesConfig *const poiSpeciesConfig		= params.GetPOISpeciesConfigFromType(pointOfInterestType); // Find the poi config specific to the species

		if ( poiSpeciesConfig ) // ignoring the POI if no config
		{
			CPoiDataByType *const poiDataByType		= GetPoiDataByType( pointOfInterestType );
			CPoiJobData_Array & staticPointList		= it->m_second;
			CPoiJobData_Array::iterator listIt		= staticPointList.Begin();
			CPoiJobData_Array::iterator listEnd		= staticPointList.End();
			while ( listIt != listEnd ) // looping through all the poi's of that type
			{
				CPoiJobData &staticPointData	= *listIt;

				if ( staticPointData.m_cpntParams.m_closestOnly == false )
				{
					poiGravity += ComputePoiGravity( &staticPointData, poiSpeciesConfig, currState, destState, params.m_individualRandomization );
				}
				else // Amongst the "Closest Only" POI's only the closest will have an effect on the boid, the rest of them will be ignored
				{
					const Float squaredDist = (staticPointData.GetPositionWithOffset() - currState.m_position).SquareMag3();
					if ( squaredDist < closestPoiDist )
					{
						closestPoiDist			= squaredDist;
						closestPoiJobData		= &staticPointData;
						closestPoiConfig		= poiSpeciesConfig;
						closestPoiDataByType	= poiDataByType;
						// if poi not used for action yet or poi being used for action ( but not wandering )
						if (   poiDataByType 
							&& ( poiDataByType->m_usage == POI_USAGE_NONE || poiDataByType->m_usage == POI_USAGE_ACTION ) )
						{
							actionPoiJobData		= &staticPointData;
							actionPoiConfig			= poiSpeciesConfig;
							actionPoiDataByType		= poiDataByType;
						}
					}
				}
				++listIt;
			}
		}
		++it;
	}
	Vector2 closestOnlyPoiGravity( 0.0f, 0.0f );
	if ( actionPoiJobData )
	{
		closestOnlyPoiGravity	= ComputePoiGravity( actionPoiJobData, actionPoiConfig, currState, destState, params.m_individualRandomization );
	}

	////////////////////////////////////
	// Mutual repulsion
	Vector2 gravityWalls(0.0f, 0.0f);
	Int32 currentFieldX, currentFieldY;
	enviroment->CellAdress( currentPosition.AsVector2(), currentFieldX, currentFieldY );
	const auto* fieldData	= enviroment->IsCorrectCell( currentFieldX, currentFieldY ) ? &enviroment->GetChannel( currentFieldX, currentFieldY ) : NULL;
	Vector2 gravityMutualRep( 0.0f, 0.0f );
	if ( fieldData )
	{
		CREATE_RANDOMIZED_VAR( undesiredDensity, params.m_mutualUndesiredDensity, params.m_individualRandomization );
		Vector2 accumulativeRepultion( 0, 0 );
		for ( Int32 relX = -1; relX <= 1; ++relX )
		{
			for ( Int32 relY = -1; relY <= 1; ++relY )
			{
				if ( relX == 0 && relY == 0 )
				{
					continue;
				}

				if ( !enviroment->IsCorrectCell( currentFieldX + relX, currentFieldY + relY ) )
				{
					continue;
				}

				const Int32 guyzInside = enviroment->GetChannel( currentFieldX + relX, currentFieldY + relY ).m_guyzCounter;
				if ( guyzInside > 0 )
				{
					Float weight = Float( guyzInside ) / undesiredDensity;
					Vector2 addVec( Float(-relX), Float(-relY) );
					if ( relX && relY )
					{
						const Float SQRT2DIV2	= 0.7071067811865475f;
						weight					*= SQRT2DIV2;
					}
					addVec					*= weight;
					accumulativeRepultion	+= addVec;
				}
			}
		}
		Int32 guyzInside = fieldData->m_guyzCounter-1;					// we are probably already counted inside
		if ( guyzInside > 0 )
		{
			Float weight			= Float( guyzInside ) / undesiredDensity;
			Vector2 fieldPlacement	= enviroment->NormalAabbVector( currentPosition.AsVector2() );
			fieldPlacement.X		*= Float( enviroment->GetWidth() );
			fieldPlacement.Y		*= Float( enviroment->GetHeight() );
			fieldPlacement.X		-= floor( fieldPlacement.X ) + 0.5f;
			fieldPlacement.Y		-= floor( fieldPlacement.Y ) + 0.5f;
			// vector [ -0.5..0.5, -0.5..0.5 ]
			accumulativeRepultion	+= fieldPlacement * (weight * 2.0f);		// * 2.f - is normalization factor
		}
		Float lenSq = accumulativeRepultion.SquareMag();
		if ( lenSq > 0.0001f )
		{
			if ( lenSq > 1.f )
			{
				accumulativeRepultion *= 1.f / sqrt( lenSq );
			}
			CREATE_RANDOMIZED_VAR( mutualRepultion, params.m_mutualRepultion, params.m_individualRandomization );
			gravityMutualRep = accumulativeRepultion * mutualRepultion;
		}
	}
	///////////////////////////////////////////////
	// Events 
	if ( m_onActionPoiID != (Uint32)-1 )
	{
		// If WanderTime out is Max then we are ready for a new action state
		if ( actionPoiDataByType && actionPoiDataByType->m_wanderTimeOut == NumericLimits< Float >::Max() && actionPoiJobData->m_uid == m_onActionPoiID )
		{
			if ( (EHumbleCritterState)m_critterState !=  HUMBLE_STATE_ACTION )
			{
				ChangeState( HUMBLE_STATE_ACTION );
			}
		}
	}

	///////////////////////////////////////////////
	//  Setting state specific variables 
	Bool applyActors			= true;
	Bool applyPoi				= true;
	Bool applyClosestPoi		= true;
	Bool applyWalls				= true;
	Bool applyMutualRepulsion	= true;
	Bool applyAttackBehaviour	= true;
	Float randomVectorMult		= 0.0f;
	Float actorRangeMult		= 1.0f;
	Vector2 spawnPointGravity( 0.0f, 0.0f );

	CREATE_RANDOMIZED_VAR( actorsGravity,		params.m_actorsGravity,		params.m_individualRandomization );
	m_currentBoidState = BOID_STATE_INVALID;

	Bool done = false; 
	while ( done == false )
	{
		switch ( (Int32)m_critterState )
		{
		case HUMBLE_STATE_FADEOUT:
			if ( m_stateChanged )
			{
				m_immobile		= false;
				m_stateChanged	= false;
				if ( m_isAlive )
				{
					m_algorithmData->OnBoidDied();
					m_isAlive = false;
				}
				m_stateTimeout = localTime + 3.f;
				UnMark();
			}
			if ( m_stateTimeout < localTime )
			{
				ChangeState( (EHumbleCritterState)CRITTER_STATE_NOT_SPAWNED );
			}
			else
			{
				done = true;
			}
			m_currentBoidState					= BOID_STATE_NOT_SPAWNED;
			destState.m_boidStateUpdateRatio	= 0.0f;  // previous boid state at next sync
			m_immobile							= true;
			applyAttackBehaviour				= false;
			break;

		case CRITTER_STATE_NOT_SPAWNED:
			if ( m_stateChanged )
			{
				m_immobile		= false;
				m_stateChanged	= false;
				if ( m_isAlive )
				{
					m_algorithmData->OnBoidDied();
					m_isAlive = false;
				}
				m_algorithmData->OnBoidDeactivated();
				UnMark();
			}
			m_currentBoidState					= BOID_STATE_NOT_SPAWNED;
			destState.m_boidStateUpdateRatio	= 0.0f;  // previous boid state at next sync
			m_immobile							= true;
			applyAttackBehaviour				= false;

			done = true;
			break;

		case HUMBLE_STATE_WANDER:
			{
				if ( m_stateChanged )
				{
					m_immobile     = false;
					m_stateChanged = false;
					m_stateTimeout = localTime + params.m_wanderTime * GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f , 1.5f );
					RandomizeWanderVector();
				}

				CREATE_RANDOMIZED_VAR( wanderGravity, params.m_wanderGravity, params.m_individualRandomization );
				randomVectorMult = wanderGravity;
				if ( m_stateTimeout < localTime )
				{
					ChangeState( GEngine->GetRandomNumberGenerator().Get< Float >() < 0.5f ? HUMBLE_STATE_IDLE : HUMBLE_STATE_WANDER );
				}
				else
				{
					/// If action state requested a break we just return to action :
					if ( closestPoiDataByType && closestPoiDataByType->m_wanderTimeOut < localTime )
					{
						/// Going to "normal" wander state
						closestPoiDataByType->m_wanderTimeOut	= NumericLimits< Float >::Max();
						closestPoiDataByType->m_usage			= POI_USAGE_NONE;
					}
					else
					{
						done = true;
					}
				}
			}
			break;

		case HUMBLE_STATE_IDLE:
			if ( m_stateChanged )
			{
				m_immobile     = false;
				m_stateChanged = false;
				m_stateTimeout = localTime + params.m_wanderTime * GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f , 1.5f );
			}

			if (   m_stateTimeout < localTime
				&& ( closestPoiDataByType == NULL || closestPoiDataByType->m_wanderTimeOut != NumericLimits< Float >::Max()) )
			{
				ChangeState( GEngine->GetRandomNumberGenerator().Get< Float >() < 0.5f ? HUMBLE_STATE_IDLE : HUMBLE_STATE_WANDER );
			}
			else
			{
				/// If action state requested a break we just return to action :
				if ( closestPoiDataByType && closestPoiDataByType->m_wanderTimeOut < localTime )
				{
					/// Going to "normal" idle state
					closestPoiDataByType->m_wanderTimeOut	= NumericLimits< Float >::Max();
					closestPoiDataByType->m_usage			= POI_USAGE_NONE;
				}
				else
				{
					done = true;
				}
			}
			break;

		case HUMBLE_STATE_PANIC:
			if ( m_stateChanged )
			{
				m_immobile	   = false;
				m_stateChanged = false;
				m_stateTimeout = localTime + 4.f * GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f , 1.5f );
				RandomizeWanderVector();
			}

			randomVectorMult = 1.0f;
			if ( m_stateTimeout < localTime )
			{
				if ( GEngine->GetRandomNumberGenerator().Get< Float >() < params.m_postPanicDeathChance )
				{
					ChangeState( HUMBLE_STATE_DIE_BURN_SHAKE );
				}
				else
				{
					ChangeState( HUMBLE_STATE_IDLE );
				}
			}
			else
			{
				m_currentBoidState		= BOID_STATE_RUN_PANIC;
				actorRangeMult			= params.m_actorRangeMultWhenPanic; // When in panic the boids fear the actors more
				applyAttackBehaviour	= false;
				CREATE_RANDOMIZED_VAR( panicGravity, params.m_panicActorsGravity, params.m_individualRandomization );
				actorsGravity			= panicGravity;
				done					= true;
			}
			break;
		case HUMBLE_STATE_ACTION:
			if ( actionPoiJobData == NULL ) // Poi as disapeared please get out of here
			{
				ChangeState( HUMBLE_STATE_WANDER );
				break;
			}
			if ( m_stateChanged )
			{
				m_immobile						= false;
				m_stateChanged					= false;
				actionPoiDataByType->m_usage	= POI_USAGE_ACTION;
				if ( actionPoiConfig->m_actionTimeOut != -1.0f )
				{
					const Float randVar						= GEngine->GetRandomNumberGenerator().Get< Float >();
					m_stateTimeout							= localTime + actionPoiConfig->m_actionTimeOut - randVar * actionPoiConfig->m_timeVariation * actionPoiConfig->m_actionTimeOut ;
					actionPoiDataByType->m_wanderTimeOut	= m_stateTimeout + actionPoiConfig->m_wanderTimeOut - randVar * actionPoiConfig->m_timeVariation * actionPoiConfig->m_wanderTimeOut;
				}
				else
				{
					actionPoiDataByType->m_wanderTimeOut	= NumericLimits< Float >::Max();
					m_stateTimeout							= localTime + 1.0f;
				}
			}

			if ( m_stateTimeout < localTime )
			{
				ChangeState( HUMBLE_STATE_WANDER );
				actionPoiDataByType->m_usage = POI_USAGE_ACTION_WANDER;
			}
			else
			{
				if ( actionPoiJobData && m_onActionPoiID == actionPoiJobData->m_uid )
				{
					actionPoiJobData->m_useCounter++;
				}
				if ( gravityMutualRep.SquareMag() < 0.1f * 0.1f )
				{
					m_immobile		= true;
				}
				if ( m_immobile )
				{
					if ( actionPoiConfig )
					{
						m_currentBoidState		= actionPoiConfig->m_boidStateIndex != -1 ? (EBoidState)actionPoiConfig->m_boidStateIndex : BOID_STATE_IDLE ;
					}
					else
					{
						m_currentBoidState		= BOID_STATE_IDLE;
					}
				}
				done = true;
			}
			break;
		case HUMBLE_STATE_GO_DESPAWN:
			{
				if ( m_stateChanged )
				{
					m_immobile	   = false;
					m_stateChanged = false;
					m_stateTimeout = localTime + GEngine->GetRandomNumberGenerator().Get< Float >( 10.f , 15.f );
				}
				Float closestSpawnPointSq; 
				const Vector2 closestSpawnPointDist	= FindClosestSpawnPoint( currentPosition.AsVector2(), closestSpawnPointSq );
	
				// if spawnpoint is very close or time runs out
				if ( m_stateTimeout < localTime || closestSpawnPointSq < (1.0f * 1.0f) )
				{
					// despawn!
					destState.m_boidState				= BOID_STATE_NOT_SPAWNED;
					destState.m_boidStateUpdateRatio	= 1.f;
					destState.m_position				= currState.m_position;
					destState.m_position.Z				-= 0.5f;
					destState.m_orientation				= currState.m_orientation;

					ChangeState( HUMBLE_STATE_FADEOUT );
					m_immobile = true;
				}
				else
				{
					applyClosestPoi	= false;
					if ( closestSpawnPointSq != NumericLimits< Float >::Max() )
					{
						spawnPointGravity = closestSpawnPointDist * ( 1.f / sqrt(closestSpawnPointSq) );			// normalize and set ratio
					}

					done = true;
				}
			}
			break;
		case HUMBLE_STATE_DIE_BURN_SHAKE:
			if ( m_stateChanged )
			{
				m_stateChanged						= false;
				destState.m_boidStateUpdateRatio	= GEngine->GetRandomNumberGenerator().Get< Float >();
				if ( m_isAlive )
				{
					m_algorithmData->OnBoidDied();
					m_isAlive = false;
				}
				m_stateTimeout = localTime + 12.f * GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f , 1.5f );
			}
			else
			{
				destState.m_boidStateUpdateRatio	= GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f );
			}
			if ( m_stateTimeout < localTime )
			{
				ChangeState( HUMBLE_STATE_FADEOUT );
			}
			else
			{
				m_immobile				= true;
				m_currentBoidState		= BOID_STATE_DEATH;
				applyAttackBehaviour	= false;
				done					= true;
			}
			break;
		default:
			ASSERT(false, TXT("SwarmAI: Humble state unknown"));
			break;
		}
	}
	
	// if action orders a wander state then closest poi gravity should not apply:
	if ( actionPoiDataByType && actionPoiDataByType->m_wanderTimeOut != NumericLimits< Float >::Max() )
	{
		applyClosestPoi = false;
	}

	////////////////////////////////
	// Actors
	// 
	CREATE_RANDOMIZED_VAR( rangeRandomizer, 1.0f, params.m_individualRandomization );
	rangeRandomizer *= actorRangeMult;

	Vector2 actorGravity( 0.0f , 0.0f );
	Vector2 closestActorDiff( 0.0f , 0.0f );
	Bool actorFound							= false;
	Boids::PointOfInterestId closestActorId = INVALID_POI_ID;
	const Float actorRangeMax				= params.m_actorsRangeMax * rangeRandomizer;
	const Float actorRangeDesired			= params.m_actorsRangeDesired * rangeRandomizer;
	Float closestActorDistSq				= actorRangeMax * actorRangeMax;
	

	const CPoiJobData_Array& dynamicPoints = m_algorithmData->GetActors();
	for ( Uint32 i = 0, n = dynamicPoints.Size(); i != n; ++i )
	{
		Vector2 diff = dynamicPoints[ i ].GetPositionWithOffset().AsVector2() - currentPosition.AsVector2();
		Float distSq = diff.SquareMag();
		if ( distSq < closestActorDistSq )
		{
			closestActorDistSq	= distSq;
			closestActorDiff	= diff;
			closestActorId		= dynamicPoints[ i ].m_uid;
			actorFound			= true;
		}
	}
	m_targetActor = closestActorId;
	if ( actorFound && closestActorDistSq > NumericLimits< Float >::Epsilon() )
	{
		const Float closestDist			= sqrt( closestActorDistSq );
		//  C---Desired-----------------Max
		//  1------1---------------------0
		const Float gravityRatio		= closestDist <= actorRangeDesired ? 1.0f :
										  1.f - (closestDist - actorRangeDesired) / (actorRangeMax - actorRangeDesired);

		actorGravity = closestActorDiff * gravityRatio * actorsGravity / closestDist;

		if ( params.m_hasAttackBehavior && applyAttackBehaviour && gravityRatio > 0.0f && closestDist < params.m_attackRange )
		{
			m_currentBoidState					= BOID_STATE_ATTACK;
			destState.m_boidStateUpdateRatio	= GEngine->GetRandomNumberGenerator().Get< Float >();
			m_algorithmData->BoidIsAttacking( closestActorId );
		}
	}

	////////////////////////////////////
	// Wandering vector 
	const Vector2 randomGravity = m_randomVector * randomVectorMult;
	
	////////////////////////////////////
	// Keep away "walls"
	if ( applyWalls )
	{
		if ( fieldData )
		{
			gravityWalls = fieldData->m_wallPotential;
		}
		else
		{
			// TODO: get back to navigable area
		}
	}

	////////////////////////////////////
	// Movement 
	Vector destPosition( currentPosition );
	EulerAngles destOrientation	= currState.m_orientation;
	Vector2 desiredVelocity( m_velocity );
	if ( m_immobile == false )
	{
		closestOnlyPoiGravity	= applyClosestPoi ? closestOnlyPoiGravity : Vector2( 0.0f, 0.0f );
		poiGravity				= applyPoi ? poiGravity : Vector2( 0.0f, 0.0f );
		const Float poiDot		= fabs( closestOnlyPoiGravity.Dot( poiGravity ) );
		Vector2 genPoiGravity	= (closestOnlyPoiGravity + poiGravity) * (1.0f - poiDot) + poiGravity * poiDot;
		gravityMutualRep		= applyMutualRepulsion ? gravityMutualRep : Vector2( 0.0f, 0.0f );
		Vector2 acceleration	= genPoiGravity + actorGravity + randomGravity + spawnPointGravity + gravityWalls + gravityMutualRep;

		Bool canMove		= false;
		if ( acceleration.IsAlmostZero( NumericLimits< Float >::Epsilon() ) == false )
		{
			// try to apply movement
			m_velocity		= m_velocity + acceleration * deltaTime;

			desiredVelocity = m_velocity;

			if( params.m_walkSideway == true )
			{
				static Float addAngleSide = 90.0f;
				Vector2 sideVector = EulerAngles::YawToVector2(currState.m_orientation.Yaw + addAngleSide);	// Get right side vector
				Float velocityProjection = m_velocity.Dot( sideVector );	// Project velocity vector on side vector;
				m_velocity = sideVector * velocityProjection;
			}

			// Capping with max velocity :
			const Bool & doubleSpeed	= m_currentBoidState >= 0 ? params.m_boidStateArray[ m_currentBoidState ].m_doubleSpeed : false;	
			const Float maxVelocity		= doubleSpeed ? params.m_maxVelocity * 7.0f : params.m_maxVelocity;
			const Float velocitySqMag	= m_velocity.SquareMag();
			if ( velocitySqMag > maxVelocity * maxVelocity )
			{
				m_velocity = m_velocity / sqrt( velocitySqMag ) * maxVelocity;
			}

			destPosition	= destPosition + m_velocity * deltaTime;
			if ( TestLocation( destPosition.AsVector2() ) )
			{
				Int32 fieldX, fieldY;
				enviroment->CellAdress( destPosition.AsVector2(), fieldX, fieldY );
				const auto* fieldData = &enviroment->GetChannel( fieldX, fieldY );

				canMove = true;
				UpdateMark( fieldX, fieldY );

				if ( m_currentBoidState == BOID_STATE_INVALID )
				{
					m_currentBoidState = BOID_STATE_MOVE;
				}

				// calculate Z coordinate
				destPosition.Z = ComputeZ( destPosition.AsVector2() );
			}
		}
		destPosition = canMove ? destPosition : currentPosition;

		// Correct velocity if boid is walking sideway
		if( params.m_walkSideway == true )
		{
			// apply orientation
			if ( canMove )
			{
				Float desiredYaw = EulerAngles::YawFromXY( desiredVelocity.X, desiredVelocity.Y );

				if( fabs( EulerAngles::AngleDistance( currState.m_orientation.Yaw, desiredYaw + 90.0f ) ) < fabs( EulerAngles::AngleDistance( currState.m_orientation.Yaw, desiredYaw - 90.0f ) ) )
				{
					desiredYaw += 90.0f;
				}
				else
				{
					desiredYaw -= 90.0f;
				}

				Float computedYaw	= currState.m_orientation.Yaw;
				Turn( &params, computedYaw, desiredYaw );
				destOrientation		= EulerAngles( 0.f, 0.f, computedYaw );
			}
			else
			{
				destOrientation = currState.m_orientation;
			}
		}
		else
		{
			// apply orientation
			if ( canMove )
			{
				Float desiredYaw	= EulerAngles::YawFromXY( m_velocity.X, m_velocity.Y );
				Float computedYaw	= currState.m_orientation.Yaw;
				Turn( &params, computedYaw, desiredYaw );
				destOrientation		= EulerAngles( 0.f, 0.f, computedYaw );
			}
			else
			{
				destOrientation = currState.m_orientation;
			}
		}
	}
	else
	{
		m_velocity = Vector2( 0.0f, 0.0f );
	}

	// If no humble::state nor movement set boid::State then set to idle :
	if ( m_currentBoidState == BOID_STATE_INVALID )
	{
		m_currentBoidState = BOID_STATE_IDLE;
	}
	ASSERT( (EHumbleCritterState)m_critterState == HUMBLE_STATE_FADEOUT || m_critterState == CRITTER_STATE_NOT_SPAWNED || destPosition.Z != 0.0f );
	destState.m_position				= destPosition;
	destState.m_orientation				= destOrientation;
	destState.m_boidState				= m_currentBoidState;


	// resetting flags :
	m_onActionPoiID			= (Uint32)-1;
}



void CHumbleCritterAI::Spawn( const Vector2& position )
{
	if ( m_critterState == CRITTER_STATE_NOT_SPAWNED )
	{
		m_algorithmData->OnBoidActivated();
		Mark( position );
	}
	if ( !m_isAlive )
	{
		m_algorithmData->OnBoidBorn();
		m_isAlive = true;
	}
	ChangeState(HUMBLE_STATE_IDLE);
}
Bool CHumbleCritterAI::ApplyFire(Boids::PointOfInterestId poiID)
{
	EHumbleCritterState outputState;
	Bool isDead = false;
	switch ( (Int32)m_critterState )
	{
	case HUMBLE_STATE_DIE_BURN_SHAKE:
		return true;  // dead
	case HUMBLE_STATE_PANIC:
		return false; // not dead
	default:
		isDead = GEngine->GetRandomNumberGenerator().Get< Float >() > m_algorithmData->GetHumbleCritterLairParams().m_burnResistance;
		outputState = isDead ? HUMBLE_STATE_DIE_BURN_SHAKE : HUMBLE_STATE_PANIC;
	}
	ChangeState( outputState );
	return isDead;
}

void CHumbleCritterAI::ApplyAction(Boids::PointOfInterestId poiID)
{
	m_onActionPoiID = poiID;
}

void CHumbleCritterAI::GoDespawn()
{
	switch ( (Int32)m_critterState )
	{
	case CRITTER_STATE_NOT_SPAWNED:
	case HUMBLE_STATE_FADEOUT:
	case HUMBLE_STATE_DIE_BURN_SHAKE:
	case HUMBLE_STATE_PANIC:
	case HUMBLE_STATE_GO_DESPAWN:
		break;

	case HUMBLE_STATE_IDLE:
	case HUMBLE_STATE_WANDER:
	case HUMBLE_STATE_ACTION:
		ChangeState( HUMBLE_STATE_GO_DESPAWN );
		break;
	}		
}
void CHumbleCritterAI::CancelDespawn()
{
	if ( (EHumbleCritterState)m_critterState == HUMBLE_STATE_GO_DESPAWN )
	{
		ChangeState( HUMBLE_STATE_IDLE );
	}
}
Bool CHumbleCritterAI::Turn( const CHumbleCritterLairParams *const params, Float& inoutYaw, Float desiredYaw, Float uberMultiplier )
{
	CREATE_RANDOMIZED_VAR( turnRatio, params->m_turnRatio, params->m_individualRandomization );

	Float orientationChange = turnRatio * m_algorithmData->GetCurrentDelta() * uberMultiplier;
	Float oriDiff			= MathUtils::GeometryUtils::ClampDegrees( desiredYaw - inoutYaw );

	if ( fabs( oriDiff ) <= orientationChange )
	{
		inoutYaw = desiredYaw;
		return true;
	}
	else
	{
		inoutYaw += oriDiff < 0.f ? -orientationChange : orientationChange;
		inoutYaw = EulerAngles::NormalizeAngle( inoutYaw );
		return false;
	}
}
Bool CHumbleCritterAI::TestLocation( const Vector2& dest ) const
{
	const SwarmEnviromentData* enviroment = m_algorithmData->GetEnviroment();

	Int32 fieldX, fieldY;
	enviroment->CellAdress( dest, fieldX, fieldY );
	if ( !enviroment->IsCorrectCell( fieldX, fieldY ) )
	{
		return false;
	}
	const auto* fieldData = &enviroment->GetChannel( fieldX, fieldY );
	return ( fieldData->m_flags & CDF_BLOCKED ) == 0;
}
Float CHumbleCritterAI::ComputeZ( const Vector2& pos ) const
{
	const SwarmEnviromentData* enviroment = m_algorithmData->GetEnviroment();

	Vector2 normVec = enviroment->NormalAabbVector( pos );
	normVec.X *= Float( enviroment->GetWidth() );
	normVec.Y *= Float( enviroment->GetHeight() );

	Int32 coordXLow = Int32( normVec.X - 0.5f );
	Int32 coordYLow = Int32( normVec.Y - 0.5f );
	Int32 coordXHigh = Int32( normVec.X + 0.5f );
	Int32 coordYHigh = Int32( normVec.Y + 0.5f );
	coordXLow = Max( 0, coordXLow );
	coordYLow = Max( 0, coordYLow );
	coordXHigh = Min( enviroment->GetWidth(), coordXHigh );
	coordYHigh = Min( enviroment->GetHeight(), coordYHigh );

	// TODO: Check if algorithm produces smooth output on quad edges
	Float xRatio = normVec.X - 0.5f;
	xRatio = xRatio - floor( xRatio );

	Float yRatio = normVec.Y - 0.5f;
	yRatio = yRatio - floor( yRatio );

	Float zComponentLL = enviroment->GetChannel( coordXLow, coordYLow ).m_z;
	Float zComponentHL = enviroment->GetChannel( coordXHigh, coordYLow ).m_z;
	Float zComponentLH = enviroment->GetChannel( coordXLow, coordYHigh ).m_z;
	Float zComponentHH = enviroment->GetChannel( coordXHigh, coordYHigh ).m_z;

	Float zComponentL = zComponentLL + ( zComponentLH - zComponentLL ) * yRatio;
	Float zComponentH = zComponentHL + ( zComponentHH - zComponentHL ) * yRatio;

	return zComponentL + ( zComponentH - zComponentL ) * xRatio;
}

void CHumbleCritterAI::RandomizeWanderVector()
{
	m_randomVector = EulerAngles::YawToVector2( GEngine->GetRandomNumberGenerator().Get< Float >( 360.0f ) );
}

// 
void CHumbleCritterAI::ChangeState( EHumbleCritterState newState )
{
	m_stateChanged = true;
	if ( (EHumbleCritterState)m_critterState == newState )
	{
		return;
	}
	
	m_critterState = (ECritterState)newState;
}

void CHumbleCritterAI::Mark( const Vector2& position )
{
	UnMark();
	const SwarmEnviromentData* envData =m_algorithmData->GetEnviroment();
	envData->CellAdress( position, m_markedX, m_markedY );
	if ( !envData->IsCorrectCell( m_markedX, m_markedY ) )
	{
		m_markedX = -1;
		return;
	}
	++( envData->GetChannel( m_markedX, m_markedY ) ).m_guyzCounter;

}
void CHumbleCritterAI::UnMark()
{
	if ( m_markedX >= 0 )
	{
		const SwarmEnviromentData* envData =m_algorithmData->GetEnviroment();
		--( envData->GetChannel( m_markedX, m_markedY ) ).m_guyzCounter;
		ASSERT( envData->GetChannel( m_markedX, m_markedY ).m_guyzCounter >= 0 );
		m_markedX = -1;
	}
}
void CHumbleCritterAI::UpdateMark( Int32 fieldX, Int32 fieldY )
{
	if ( fieldX != m_markedX || fieldY != m_markedY )
	{
		UnMark();
		m_markedX = fieldX;
		m_markedY = fieldY;
		const SwarmEnviromentData* envData =m_algorithmData->GetEnviroment();
		++( envData->GetChannel( m_markedX, m_markedY ) ).m_guyzCounter;
	}
}

Vector2 CHumbleCritterAI::FindClosestSpawnPoint( const Vector2& currentPosition, Float & closestSpawnPointSq )const
{
	closestSpawnPointSq = NumericLimits< Float >::Max();
	Vector2 closestSpawnPointDist(0.0f, 0.0f);
	const auto& spawnPoitsList = m_algorithmData->GetSpawnpoints();
	for ( auto it = spawnPoitsList.Begin(), end = spawnPoitsList.End(); it != end; ++it )
	{
		Vector2 dist = (*it).GetPositionWithOffset().AsVector2() - currentPosition;
		Float distSq = dist.SquareMag();
		if ( distSq < closestSpawnPointSq )
		{
			closestSpawnPointSq		= distSq;
			closestSpawnPointDist	= dist;
		}
	}
	return closestSpawnPointDist;
}


Vector2 CHumbleCritterAI::ComputePoiGravity( const CPoiJobData *const poiJobData, const CPointOfInterestSpeciesConfig *const pointOfInterestSpeciesConfig, const SSwarmMemberStateData &currState, SSwarmMemberStateData &destState, Float randomisation )const
{
	const CPointOfInterestParams &	pointOfInterestParams	= poiJobData->m_cpntParams;
	Vector2 POIInput( 0, 0 );
	if ( pointOfInterestParams.m_enabled )
	{
		if (pointOfInterestParams.m_shapeType == CNAME( Circle ))
		{
			const Vector2 input = ComputeGravityForCircle( pointOfInterestSpeciesConfig, currState.m_position, *poiJobData, randomisation );
			if ( input.IsZero() == false )
			{
				destState.m_flags |= BOID_STATE_FLAG_INSIDE_CIRCLE_POI;
			}
			POIInput += input;
		}
		else
		{
			const Vector2 input = ComputeGravityForCone( pointOfInterestSpeciesConfig, currState.m_position, *poiJobData, randomisation );
			if ( input.IsZero() == false )
			{
				destState.m_flags |= BOID_STATE_FLAG_INSIDE_CONE_POI;
			}
			POIInput += input;
		}
	}
	return POIInput;
}

Vector2 CHumbleCritterAI::ComputeGravityForCircle( const CPointOfInterestSpeciesConfig *const poiSpeciesConfig, const Vector &boidPosition, const CPoiJobData &poiJobData, Float randomisation )const
{
	Vector2 Input( 0.0f, 0.0f );
	const Vector2 distVector	= poiJobData.GetPositionWithOffset().AsVector2() - boidPosition.AsVector2();
	const Float sqDist			= distVector.SquareMag();

	if ( sqDist > NumericLimits<Float>::Epsilon() )
	{
		CREATE_RANDOMIZED_VAR( gravityMult, 1.0f, randomisation );
		const Float	gravityRangeMax				= poiJobData.m_cpntParams.m_gravityRangeMax * gravityMult;
		const Float	gravityRangeMin				= poiJobData.m_cpntParams.m_gravityRangeMin * gravityMult;
		if ( poiJobData.m_cpntParams.m_gravityRangeMax != -1.0f )
		{
			if ( sqDist < gravityRangeMax * gravityRangeMax )
			{
				const Float dist			= sqrt( sqDist );
				// If gravity < 0
				// C = Center, mD = minDist, MD = maxDist
				//
				//  mult	=    1        1                 0
				//  Pos		=    C ----- mD --------------- MD
				// else
				// C = Center, mD = minDist, MD = maxDist
				//
				//  mult	=    0      0|1                 0
				//  Pos		=    C ----- mD --------------- MD
				Float mult					= poiSpeciesConfig->m_gravity > 0.0f ? 0.0f : 1.0f ;
				if ( dist > NumericLimits< Float >::Epsilon() )
				{
					const Float distRatio		= dist / gravityRangeMin;
					if ( distRatio > 1.0f )
					{
						const Float normedGravityrangeMax	= gravityRangeMax/ gravityRangeMin; 
						mult								= 1.f - (distRatio - 1.0f) / (normedGravityrangeMax - 1.0f);
					}
				}
				// normalising dist vector :
				const Vector2 normedDistVector2D	= distVector / dist;
				CREATE_RANDOMIZED_VAR( gravity , poiSpeciesConfig->m_gravity, randomisation );
				Input							+= normedDistVector2D * mult * gravity;
			}
		}
		else
		{
			if ( sqDist >= gravityRangeMin * gravityRangeMin )
			{
				// No matter how far the boid is gravity will affect it the same :
				CREATE_RANDOMIZED_VAR( randGravity , poiSpeciesConfig->m_gravity, randomisation );
				// normalising dist vector :
				const Float dist					= sqrt( sqDist );
				const Vector2 normedDistVector2D	= distVector / dist;
				Input += normedDistVector2D *  randGravity ;
			}
		}
	}

	return Input;
}

Vector2 CHumbleCritterAI::ComputeGravityForCone( const CPointOfInterestSpeciesConfig *const poiSpeciesConfig, const Vector &boidPosition, const CPoiJobData &poiJobData, Float randomisation )const
{
	Vector2 Input( 0.0f, 0.0f );
	CREATE_RANDOMIZED_VAR( gravityMult, 1.0f, randomisation );
	const CBoidCone minRangeCone( poiJobData.GetPositionWithOffset(), poiJobData.m_forwardVect, poiJobData.m_rightVect, poiJobData.m_tanHalfMinConeOpeningAngle, poiJobData.m_cosHalfMinConeOpeningAngle, poiJobData.m_cpntParams.m_gravityRangeMin );
	if (poiJobData.m_cpntParams.m_gravityRangeMax != -1.0f)
	{
		const CBoidCone maxRangeCone( poiJobData.GetPositionWithOffset(), poiJobData.m_forwardVect, poiJobData.m_rightVect, poiJobData.m_tanHalfMaxConeOpeningAngle, poiJobData.m_cosHalfMaxConeOpeningAngle, poiJobData.m_cpntParams.m_gravityRangeMax );
		if ( maxRangeCone.IsPointInCone( boidPosition ) )
		{
			Vector2 normedDistToMinConeVect, normedDistToMaxConeVect;
			Vector2 minConePushVector, maxConePushVector;
			const Float distToMinCone = fabs( minRangeCone.ComputeDistanceToCone( boidPosition, normedDistToMinConeVect, minConePushVector ) );
			const Float distToMaxCone = fabs( maxRangeCone.ComputeDistanceToCone( boidPosition, normedDistToMaxConeVect, maxConePushVector ) );
			

			Float mult = poiSpeciesConfig->m_gravity > 0.0f ? 0.0f : 1.0f ;
			const Vector2 pushVector = poiSpeciesConfig->m_gravity > 0.0f ? Vector2( 0.0f, 0.0f ) : -minConePushVector;
			if ( minRangeCone.IsPointInCone( boidPosition ) == false && (distToMaxCone + distToMinCone) > NumericLimits< Float >::Epsilon() )
			{
				const Float distRatio				= distToMinCone / (distToMaxCone + distToMinCone); 
				mult								= 1.0f - distRatio;
			}
			CREATE_RANDOMIZED_VAR( randGravity , poiSpeciesConfig->m_gravity, randomisation );
			Input		+= (normedDistToMinConeVect + pushVector) * mult * randGravity;
		}
	}
	else
	{
		Vector2 normedDistToMinConeVect, pushVect;
		const Float distToMinCone	= minRangeCone.ComputeDistanceToCone(boidPosition,  normedDistToMinConeVect, pushVect );
		if ( distToMinCone >= 0.0f )
		{
			// the more the boid is far from the center the less the poi affects it
			CREATE_RANDOMIZED_VAR( randGravity , poiSpeciesConfig->m_gravity, randomisation );
			Input						+= -normedDistToMinConeVect * randGravity ;
		}
	}

	return Input;
}

void CHumbleCritterAI::ApplyEffector( const CPoiJobData &staticPointData, const CPointOfInterestSpeciesConfig*const poiConfig )
{
	// Hard coded effectors
	if ( staticPointData.m_cpntParams.m_type == CNAME( Fire ) )
	{
		ApplyFire( staticPointData.m_uid );
	}
	else if ( poiConfig && poiConfig->m_action ) // generic action effector
	{
		ApplyAction( staticPointData.m_uid );
	}
	m_currentEffectorType = staticPointData.m_cpntParams.m_type;
}
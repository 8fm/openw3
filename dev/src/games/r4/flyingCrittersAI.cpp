#include "build.h"
#include "flyingCrittersAI.h"
#include "flyingCritterAlgorithmData.h"
#include "flyingSwarmGroup.h"
#include "swarmCellMap.h"

#include "../../common/game/pointOfInterestSpeciesConfig.h"
#include "../../common/game/boidInstance.h"
#include "../../common/game/boidCone.h"

///////////////////////////////////////////////////////////////
// CFlyingCritterAI
///////////////////////////////////////////////////////////////

CFlyingCritterAI::CFlyingCritterAI()
	: m_algorithmData( NULL )
	, m_stateChanged( false )
	, m_targetActor( INVALID_POI_ID )
	, m_stateTimeout( 0.f )
	, m_randomSeed( GEngine->GetRandomNumberGenerator().Get< Uint32 >() )
	, m_isAlive( false )
	, m_effectorRandom( GEngine->GetRandomNumberGenerator().Get< Float >( -0.5f , 0.5f ) )
	, m_velocity( 0.0f, 0.0f, 0.0f )
	, m_randomForce( 0.0f, 0.0f, 0.0f)
	, m_randomDirectionTimeOut(0.0f)
	, m_lastTooCloseDistanceVect( 0.0f, 0.0f, 0.0f )
	, m_hasRandomDir( (Bool)-1 )
	, m_groundZ( 0.0f )
	, m_previousPosition( 0.0f, 0.0f, 0.0f )
	, m_hasDespawned( false )
{
}

void CFlyingCritterAI::UpdateFlying( const SSwarmMemberStateData& currState, SSwarmMemberStateData& destState, const CFlyingSwarmGroup *const flyingGroup, Bool poiStateChanged, const Vector3 &inTooCloseForce, const Vector3 &inTooFarForce, const Vector3 &centerOfCloud, CFlyingCrittersPoi_Map & flyingCrittersPoi_Map, const CPoiJobData_Array *const despawnPoiArray, const CSwarmCellMap *const cellMap )
{
	PC_SCOPE( CFlyingCritterAI_UpdateFlying );
	ASSERT( flyingGroup->m_groupState );
	const CGroupState *const groupState = flyingGroup->m_groupState;
	//Intialising random direction :
	if ( m_hasRandomDir == (Bool)-1 )
	{
		m_hasRandomDir = GEngine->GetRandomNumberGenerator().Get< Float >() < groupState->m_randomDirRatio ? true : false;
	}

	// Some vars we'll need all the way down
	const CFlyingCritterLairParams &params							= m_algorithmData->GetLairParams();
	const Vector currentPosition									= currState.m_position;
	const Float & localTime											= m_algorithmData->GetLocalTime();
	const Float & deltaTime											= m_algorithmData->GetCurrentDelta();

	m_currentBoidState			= BOID_STATE_INVALID;
	Vector2 spawnPointGravity( 0.0f, 0.0f );
	Bool applyFlyForces			= false;
	Bool applyPoiForce			= false;
	Bool applyThinnessForce		= false;
	Vector3 despawnForce( 0.0f, 0.0f, 0.0f );

	static Float despawnMultiplier		= 100.0f;
	static Float maxRoll				= 35.0f;
	static Float minRollDot				= 0.8f;

	Vector3 tooCloseForce	= inTooCloseForce;
	Vector3 tooFarForce		= inTooFarForce;

	// randomize variables from scipt :
	CREATE_RANDOMIZED_VAR( maxVelocity,				groupState->m_maxVelocity,				groupState->m_velocityVariation );
	CREATE_RANDOMIZED_VAR( minVelocity,				groupState->m_minVelocity,				groupState->m_velocityVariation );
	CREATE_RANDOMIZED_VAR( tooCloseMultiplier,		groupState->m_tooCloseMultiplier,		groupState->m_tooCloseVariation );
	CREATE_RANDOMIZED_VAR( tooFarMultiplier,		groupState->m_tooFarMultiplier,			groupState->m_tooFarVariation );
	CREATE_RANDOMIZED_VAR( randomDirMultiplier,		groupState->m_randomDirMultiplier,		groupState->m_genericVariation );
	CREATE_RANDOMIZED_VAR( thinnessMultiplier,		groupState->m_thinnessMultiplier,		groupState->m_genericVariation );
	CREATE_RANDOMIZED_VAR( frictionMultiplier,		groupState->m_frictionMultiplier,		groupState->m_genericVariation );
	CREATE_RANDOMIZED_VAR( collisionMultiplier,		groupState->m_collisionMultiplier,		groupState->m_genericVariation );


	const Float minDistToCenterOfCloud		= 0.5f;
	const Float maxDistToCenterOfCloud		= 2.5f;

	if ( m_hasRandomDir == false )
	{
		randomDirMultiplier = 0.0f;
	}
	// resseting flags :
	m_hasDespawned = false;

	const CPoiJobData *  despawnPoiJobData = NULL;		// Set when despawning, used to check if despawn is complete at the end of the frame.

	// Updating parameters depending of state :
	Bool done = false; 
	while ( done == false )
	{
		switch ( (Int32)m_critterState )
		{
		case CRITTER_STATE_NOT_SPAWNED:
			if ( m_stateChanged )
			{
				m_stateChanged	= false;
				if ( m_isAlive )
				{
					m_algorithmData->OnBoidDied();
					m_isAlive = false;
				}
				m_algorithmData->OnBoidDeactivated();
				destState.m_boidStateUpdateRatio	= 1.f;	// next boid state at next sync
			}
			else
			{
				destState.m_boidStateUpdateRatio	= 0.0f;  // previous boid state at next sync
			}
			m_currentBoidState	= BOID_STATE_NOT_SPAWNED;
			applyFlyForces		= false;
			applyThinnessForce	= false;
			applyPoiForce		= false;

			done = true;
			break;

		case FLYING_STATE_IDLE:
			if ( m_stateChanged )
			{
				m_stateChanged = false;
			}
			m_currentBoidState	= BOID_STATE_MOVE;
			applyFlyForces		= true;
			applyThinnessForce	= true;
			applyPoiForce		= true;
			done = true;
			break;

		case FLYING_STATE_BURN:
			{
				if ( m_stateChanged )
				{
					m_stateChanged = false;
					m_stateTimeout = localTime + 3.0f;
				}

				if ( m_stateTimeout < localTime )
				{
					ChangeState( (EFlyingCritterState)CRITTER_STATE_NOT_SPAWNED );
				}
				else
				{
					m_currentBoidState	= BOID_STATE_DEATH;
					applyFlyForces		= true;
					applyThinnessForce	= false;
					applyPoiForce		= false;
					m_hasRandomDir		= true;
					done = true;
				}
				break;
			}
		case FLYING_STATE_GO_DESPAWN:
			{
				if ( m_stateChanged )
				{
					m_stateChanged = false;
					m_stateTimeout = localTime + GEngine->GetRandomNumberGenerator().Get< Float >( 10.f , 15.f );
				}
				Float closestSpawnPointSq; 
				despawnPoiJobData	= FindClosestPoi( despawnPoiArray, currentPosition, closestSpawnPointSq );

				if ( despawnPoiJobData )
				{
					// Working out despawn gravity :
					despawnForce = (despawnPoiJobData->GetPositionWithOffset() - currentPosition).Normalized3();
				}
				else
				{
					// depsawning immediately 
					m_stateTimeout = 0;
				}
				// if spawnpoint is very close or time runs out
				if ( m_stateTimeout < localTime )
				{
					// despawn!
					m_currentBoidState					= BOID_STATE_NOT_SPAWNED;
					ChangeState( (EFlyingCritterState)CRITTER_STATE_NOT_SPAWNED );
					// Has despawned without dying :
					m_hasDespawned						= true;
				}				
			
				m_currentBoidState	= BOID_STATE_MOVE;
				applyFlyForces		= true;
				applyThinnessForce	= false;
				applyPoiForce		= false;
				done = true;
			
			}
			break;
		default:
			ASSERT(false, TXT("SwarmAI: Flying critter state unknown"));
			break;
		}
	}

	/////////////////////////////////////////////////
	// Random direction
	// Changing the random direction from time to time :
	if ( m_randomDirectionTimeOut < localTime )
	{
		m_randomForce = Vector3( GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ) );
		RemoveOppositionFromVect( m_randomForce, m_velocity );
		m_randomForce.Normalize();
		
		m_randomDirectionTimeOut	= localTime + GEngine->GetRandomNumberGenerator().Get< Float >( 5.0f );
	}

	//////////////////////////////////////////////////
	// Displacement
	Float roll = 0.0f;
	Vector destPosition( currentPosition );
	m_currentEffectorType = CName::NONE; // will be set by updatePOi
	EulerAngles destOrientation	= currState.m_orientation;
	if ( applyFlyForces )
	{
		Vector3 poiForce(0.0f, 0.0f, 0.0f);
		if ( applyPoiForce )
		{
			CFlyingCrittersPoi_Map::iterator it				= flyingCrittersPoi_Map.Begin();
			CFlyingCrittersPoi_Map::iterator end			= flyingCrittersPoi_Map.End();

			// Looping trough all types of POIs and calculating the force from it
			while ( it != end ) 
			{
				const CName& pointOfInterestType								= it->m_first;
				if ( it->m_second.m_poiConfig ) // this can be null with an error in the xml ( bad POIConfig type_name )
				{
					CPoiJobData_PointerArray & poiJobData_Array	= it->m_second.m_poiJobDataArray;
					CPoiJobData_PointerArray::iterator listIt	= poiJobData_Array.Begin();
					CPoiJobData_PointerArray::iterator listEnd	= poiJobData_Array.End();
					while ( listIt != listEnd ) // looping through all the poi's of that type
					{
						CPoiJobData *const poiJobData	= *listIt;

						poiForce += UpdateCirclePoi( it->m_second.m_poiConfig, currState.m_position, *poiJobData, groupState->m_genericVariation );
						++listIt;
					}
				}
			
				++it;
			}
		}
		
		// Thinness force, attracts the bird toward the group's center on the Z axis
		Vector3 thinnessForce(0.0f, 0.0f, 0.0f);
		if ( applyThinnessForce )
		{
			const Float distToCenterOfCloud		= centerOfCloud.Z - currentPosition.Z;
			const Float absDistToCenterOfCloud	= fabs( distToCenterOfCloud );
			if ( absDistToCenterOfCloud > minDistToCenterOfCloud )
			{
				const Float force	= ( absDistToCenterOfCloud - minDistToCenterOfCloud ) / maxDistToCenterOfCloud;
				thinnessForce.Z		= Abs( force );

				// Making sure the force doesn't oppose poiForce otherwise thinnessForce will stop poi forces.
				if ( thinnessForce.Dot( poiForce ) < 0.0f )
				{
					thinnessForce.Z = 0.0f;
				}
			}
		}
		// [ Step ] Friction force 
		Vector3 frictionForce = m_velocity * -1.0f;

		// [ Step ] Calculating velocity without collision force and collision force modifiers :
		// This permits to feed an idea of this frame's velocity to the collision algo 
		const Vector3 postCollisionAcceleration		= poiForce + tooCloseForce * tooCloseMultiplier + tooFarForce * tooFarMultiplier + m_randomForce * randomDirMultiplier + thinnessForce * thinnessMultiplier + frictionForce * frictionMultiplier + despawnForce * despawnMultiplier;
		const Vector3 postCollisionVelocity			= m_velocity + postCollisionAcceleration * deltaTime;
		
		// [ Step ] Sphere test !
	
		Vector3 collisionForce( 0.0f, 0.0f, 0.0f );	
		const Bool cellIsValid = cellMap ? cellMap->ComputeCollisionForceAtPosition( currentPosition, postCollisionVelocity, SWARM_COLLISION_TEST_RADIUS, collisionForce ) : true;
	
		// Correct position for birds and fishes
		destPosition = currentPosition + postCollisionVelocity;
		if( params.m_livesUnderWater == true && destPosition.Z > flyingGroup->m_waterLevel - params.m_wallsDistance )
		{
			collisionForce.Z -= params.m_waterSurfaceCollisionForce;
		}
		else if( params.m_livesUnderWater == false && destPosition.Z < flyingGroup->m_waterLevel + params.m_wallsDistance )
		{
			collisionForce.Z += params.m_waterSurfaceCollisionForce;
		}

		// [ Step ] collision force modifiers
		// Collision force is very important so anything that
		// opposes it is removed using RemoveVectorComponant
		const Float savedTooCloseForceMag	= tooCloseForce.Mag(); // saved value for next step
		const Float savedTooFarForceMag		= tooFarForce.Mag();	// saved value for next step
		const Float savedThinnessForceMag	= thinnessForce.Mag(); // saved value for next step
		const Float savedFrictionForceMag	= frictionForce.Mag();	// saved value for next step
		const Float savedDespawnForceMag	= despawnForce.Mag(); // saved value for next step
		RemoveOppositionFromVect( tooCloseForce, collisionForce );
		RemoveOppositionFromVect( tooFarForce, collisionForce );
		RemoveOppositionFromVect( m_randomForce, collisionForce );
		RemoveOppositionFromVect( thinnessForce, collisionForce );
		RemoveOppositionFromVect( frictionForce, collisionForce );
		RemoveOppositionFromVect( despawnForce, collisionForce );

		// [ Step ] renormalising corrected forces 
		tooCloseForce	= tooCloseForce.Normalized() * savedTooCloseForceMag;
		tooFarForce		= tooFarForce.Normalized() * savedTooFarForceMag;
		m_randomForce.Normalize();	// This force is always normalized
		thinnessForce	= thinnessForce.Normalized() * savedThinnessForceMag;
		frictionForce	= frictionForce.Normalized() * savedFrictionForceMag;
		despawnForce	= despawnForce.Normalized() * savedDespawnForceMag;

		// [ Step ] Calculating real velocity with collision force
		// Acceleration
		const Vector3 acceleration	= poiForce + tooCloseForce * tooCloseMultiplier + tooFarForce * tooFarMultiplier + m_randomForce * randomDirMultiplier + thinnessForce * thinnessMultiplier + frictionForce * frictionMultiplier + collisionForce * collisionMultiplier + despawnForce * despawnMultiplier;
		const Vector3 previousVel	= m_velocity;
		// Integrating Velocity
		m_velocity					+= acceleration * deltaTime;
		Float velocityMag			= m_velocity.Mag();

		// [ Step ] Clamping velocity to max speed
		if ( velocityMag > maxVelocity )
		{
			m_velocity	= m_velocity / velocityMag * maxVelocity;
			velocityMag = maxVelocity;
		}

		// capping with min velocity
		if ( velocityMag > NumericLimits<Float>::Epsilon() && velocityMag < minVelocity )
		{
			m_velocity	= m_velocity / velocityMag * minVelocity;
			velocityMag	= minVelocity;
		}
		
		// [ Step ] position candidate for fail safe line test
		Vector candidateDisplacementVector		= m_velocity * deltaTime;
		candidateDisplacementVector.W			= candidateDisplacementVector.Mag3();
		const Vector3 nextPositionCandidate		= currentPosition	+ candidateDisplacementVector;
		Vector3 oldVel = m_velocity;

		// [ Step ] Line testing to make sure we are not heading through an obstacle ( can happen because above algo doesn't garanty that )
		if (	cellMap && cellIsValid 
			&&	candidateDisplacementVector.AsVector3().IsAlmostZero() == false 
			&&	collisionMultiplier != 0.0 
			&&	cellMap->LineTest( currentPosition, candidateDisplacementVector ) )
		{
			Vector3 freeCellPosition;
			if ( R4SwarmUtils::FindCellToClearPath( currentPosition, candidateDisplacementVector, cellMap, freeCellPosition ) )
			{
				const Vector3 vectToFreeCell		= freeCellPosition - currentPosition;
				const Float vectToFreeCellSquareMag	= vectToFreeCell.SquareMag();
				Vector3 newVelocity					= vectToFreeCell;
				// If the cell is far enough multiply the old velocity magnitude to the new velocity
				// Of not we must keep the velocity as is otherwise we might end up behind the free cell after an update
				if ( vectToFreeCellSquareMag > velocityMag * velocityMag )
				{
					newVelocity /= Red::Math::MSqrt( vectToFreeCellSquareMag );
					newVelocity *= velocityMag;
				}
				m_velocity = newVelocity;
			}
			else
			{
				m_velocity = Vector3( 0.0f, 0.0f, 0.0f );
			}
		}
		// Integrating position
		destPosition	= currentPosition + m_velocity * deltaTime;

		// Correct position for birds and fishes
		if( params.m_livesUnderWater == true && destPosition.Z > flyingGroup->m_waterLevel - params.m_wallsDistance )
		{
			destPosition.Z = flyingGroup->m_waterLevel - params.m_wallsDistance;
		}
		else if( params.m_livesUnderWater == false && destPosition.Z < flyingGroup->m_waterLevel + params.m_wallsDistance )
		{
			destPosition.Z = flyingGroup->m_waterLevel + params.m_wallsDistance;
		}

		ASSERT( !Red::Math::NumericalUtils::IsNan( destPosition.X ) );
		ASSERT( !Red::Math::NumericalUtils::IsNan( destPosition.Y ) );
		ASSERT( !Red::Math::NumericalUtils::IsNan( destPosition.Z ) );

		// If we are going down : velocity < 0.0f then we must glide 
		const Vector3 normedAcc = acceleration.Normalized();
		if ( normedAcc.Z < -( 1.0f - groupState->m_glidingPercentage ) )
		{
			m_currentBoidState = (EBoidState)FLYING_BOID_STATE_GLIDE;
		}

		// Computing roll depending on how velocity changes on the XY plane:
		const Vector2 normedVel			= m_velocity.AsVector2().Normalized();  // removing Z because Yaw is all about XY
		const Vector2 previousNormedVel = previousVel.AsVector2().Normalized();
		Float rollDot					= normedVel.Dot( previousNormedVel );
		rollDot							= Clamp( rollDot, minRollDot, 1.0f );
		const Float rollRatio			= rollDot * 2.0f - minRollDot * 2.0f; // rollRatio is [0.0, 1.0]
		const Vector3 rollCrossProd		= Vector3( normedVel ).Cross( Vector3( previousNormedVel ) );
		roll							= (1.0f - rollRatio) * maxRoll * ( rollCrossProd.Z >= 0.0f ? 1.0f : -1.0f ); 
	}
	////////////////////////////////////////////
	// setting destState parameters 
	const Vector3 normedVelocity	= m_velocity.Normalized();
	destOrientation.Yaw				= EulerAngles::YawFromXY(normedVelocity.X, normedVelocity.Y);
	destOrientation.Pitch			= 80.0f * normedVelocity.Z;
	destOrientation.Roll			= roll;
	
	m_previousPosition					= currState.m_position;

	destState.m_position				= destPosition;
	destState.m_orientation				= destOrientation;
	destState.m_boidState				= m_currentBoidState;

	// Working out if despawn is complete
	// This needs to be done here otherwise it will be an update late
	if ( despawnPoiJobData )
	{
		const Vector3 previousPosToSpawn			= despawnPoiJobData->GetPositionWithOffset() - currState.m_position;
		const Vector3 PosToSpawn					= despawnPoiJobData->GetPositionWithOffset() - destState.m_position;
		// if spawnpoint is very close or time runs out
		if ( previousPosToSpawn.Dot( PosToSpawn ) < 0.0f )
		{
			// despawn!
			ChangeState( (EFlyingCritterState)CRITTER_STATE_NOT_SPAWNED );
			// Has despawned without dying :
			m_hasDespawned						= true;
			destState.m_boidState				= BOID_STATE_NOT_SPAWNED;
			destState.m_boidStateUpdateRatio	= 1.f;	// next boid state at next sync
			
			Vector3 normedCurrentToDest			= destState.m_position - currState.m_position;
			normedCurrentToDest.Normalize();
			destState.m_position				= currState.m_position + normedCurrentToDest * normedCurrentToDest.Dot( previousPosToSpawn );
		}
	}
}

const CPoiJobData *const CFlyingCritterAI::FindClosestPoi( const CPoiJobData_Array *const poiJobDataArray, const Vector3& currentPosition, Float & closestSpawnPointSq )const
{
	if ( poiJobDataArray == nullptr )
	{
		return nullptr;
	}
	closestSpawnPointSq = NumericLimits< Float >::Max();
	Vector3 closestSpawnPointDist( 0.0f, 0.0f, 0.0f );
	const CPoiJobData * poiJobData = nullptr;
	for ( CPoiJobData_Array::const_iterator it = poiJobDataArray->Begin(), end = poiJobDataArray->End(); it != end; ++it )
	{
		if ( (*it).m_cpntParams.m_enabled )
		{
			Vector3 dist = (*it).GetPositionWithOffset() - currentPosition;
			Float distSq = dist.SquareMag();
			if ( distSq < closestSpawnPointSq )
			{
				poiJobData				= &(*it);
				closestSpawnPointSq		= distSq;
				closestSpawnPointDist	= dist;
			}
		}
	}
	return poiJobData;
}

void CFlyingCritterAI::Spawn( const Vector3& position )
{
	if ( m_critterState == CRITTER_STATE_NOT_SPAWNED )
	{
		m_algorithmData->OnBoidActivated();
	}
	if ( m_isAlive == false )
	{
		m_algorithmData->OnBoidBorn();
		m_isAlive = true;
	}
	ChangeState( FLYING_STATE_IDLE );

	// reseting parameters :
	m_velocity			= Vector3( 0.0f, 0.0f, 0.0f );
	m_previousPosition	= position;
}

void CFlyingCritterAI::ApplyEffector( const CPoiJobData &poiJobData, const CPoiConfigByGroup*const poiConfig )
{
	// Hard coded effectors
	if ( poiJobData.m_cpntParams.m_type == CNAME( Fire ) )
	{
		ApplyFire( poiJobData.m_uid );
	}
	m_currentEffectorType = poiJobData.m_cpntParams.m_type;
}

Bool CFlyingCritterAI::ApplyFire(Boids::PointOfInterestId poiID)
{
	if ( (EFlyingCritterState)m_critterState == FLYING_STATE_BURN )
	{
		return true;
	}
	const Bool isDead = true;
	if ( isDead )
	{
		ChangeState( FLYING_STATE_BURN );
	}
	return isDead;
}

void CFlyingCritterAI::ApplyAction(Boids::PointOfInterestId poiID)
{
}

void CFlyingCritterAI::GoDespawn()
{
	switch ( (Int32)m_critterState )
	{
	case CRITTER_STATE_NOT_SPAWNED:
		break;

	case FLYING_STATE_IDLE:
		ChangeState( FLYING_STATE_GO_DESPAWN );
		break;
	}		
}

void CFlyingCritterAI::CancelDespawn()
{
	if ( (EFlyingCritterState)m_critterState == FLYING_STATE_GO_DESPAWN )
	{
		ChangeState( FLYING_STATE_IDLE );
	}
}

void CFlyingCritterAI::Kill()
{
	ChangeState( (EFlyingCritterState)CRITTER_STATE_NOT_SPAWNED );
}

void CFlyingCritterAI::ChangeState( EFlyingCritterState newState )
{
	m_stateChanged = true;
	if ( (EFlyingCritterState)m_critterState == newState )
	{
		return;
	}
	
	m_critterState = (ECritterState)newState;
}

Vector3 CFlyingCritterAI::UpdateCirclePoi( const CPoiConfigByGroup *const poiConfig, const Vector &currentBoidPosition, CPoiJobData &poiJobData, Float randomisation )
{
	Vector3 Input( 0.0f, 0.0f, 0.0f );
	if ( poiConfig->HasGravity() && poiJobData.m_cpntParams.m_enabled )
	{
		// Target force :
		const Vector3 targetDistVect	= poiJobData.GetPositionWithOffset() - currentBoidPosition;
		const Float targetDist			= targetDistVect.Mag();
		CREATE_RANDOMIZED_VAR( distanceMult, 1.0f, randomisation );
		Float gravity	= 0.0f;
		gravity			= poiConfig->GetGravityFromDistance( targetDist, distanceMult );	 
	
		CREATE_RANDOMIZED_VAR( randGravity , gravity, randomisation );

		
		if ( targetDist > NumericLimits<Float>::Epsilon() )
		{
			const Vector3 normedTargetDist 	= targetDistVect / targetDist;
			Input							= normedTargetDist * randGravity;
		}
	}
	
	if ( m_critterState != CRITTER_STATE_NOT_SPAWNED )
	{
		CREATE_RANDOMIZED_VAR( effectorRadius , poiJobData.m_cpntParams.m_effectorRadius, randomisation );
		const Float radius		= effectorRadius *  poiJobData.m_cpntParams.m_scale;

		Sphere sphere( poiJobData.GetPositionWithOffset(), radius);
		Vector enterPoint, exitPoint;
		if ( sphere.IntersectEdge( m_previousPosition, currentBoidPosition, enterPoint, exitPoint ) > 0 )
		{
			poiJobData.m_reachCounter++;
			if ( poiJobData.m_applyEffector )
			{
				ApplyEffector( poiJobData, poiConfig );
			}
		}
	}

	return Input;
}



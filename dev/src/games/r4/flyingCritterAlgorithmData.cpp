#include "build.h"
#include "flyingCritterAlgorithmData.h"
#include "flyingCrittersAI.h"
#include "flyingCrittersLairEntity.h"
#include "flyingSwarmGroup.h"
#include "r4SwarmSound.h"
#include "swarmCellMap.h"

#include "../../common/game/boidInstance.h"
#include "../../common/game/boidCone.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/renderFrame.h"

#define LINE_TEST_DEBUG_COLOR		Color::CYAN
#define SPHERE_TEST_DEBUG_COLOR		Color::DARK_RED
#define SPAWN_POI_DEBUG_COLOR		Color::BLACK
#define ALTITUDE_TEST_DEBUG_COLOR	Color::DARK_BLUE
#define FREE_SPOT_TEST_DEBUG_COLOR	Color::BROWN

///////////////////////////////////////////////////////////////
// CFlyingCrittersAlgorithmData
///////////////////////////////////////////////////////////////
CFlyingCrittersAlgorithmData::CFlyingCrittersAlgorithmData( CFlyingCrittersLairEntity* lair, const CFlyingCritterLairParams & params, CFlyingSwarmScriptInput * scriptInput )
	: CSwarmAlgorithmData( lair )
	, m_breakCounter( lair->m_breakCounter )
	, m_activeBoids( 0 )
	, m_isLairDefeated( false )
	, m_despawningAll( true )
	, m_despawnAllPending( false )
	, m_despawnAllTime( 0.f )
	, m_nextPoiId( 0x80000000 )
	, m_nextBoidIndexToSpawn( 0 )
	, m_fireInConePoiID_A( (Uint32)-1 )
	, m_fireInConePoiID_B( (Uint32)-1 )
	, m_lairBoundingBox( )
	, m_scriptInput( scriptInput )
	, m_cellMap( nullptr )
	, m_maxDist_IdleTarget( 0.0f )
	, m_minDist_IdleTarget( 0.0f )
	, m_groupForceMult_IdleTarget( 0.0f )
	, m_maxDistToAltitude_IdleTarget( 0.0f )
	, m_altitudeForceMult_IdleTarget( 0.0f )
	, m_collisionForceMult_IdleTarget( 0.0f )
	, m_maxVel_IdleTarget( 0.0f )
	, m_randomForceMult_IdleTarget( 0.0f )
	, m_jobTime( 0.0f )
{
	m_params			= new CFlyingCritterLairParams( params );
	m_critterAiArray	= new CBaseCritterAI* [ m_boidsLimit ];
	for ( Uint32 i = 0; i < m_boidsLimit; ++i )
	{
		m_critterAiArray[ i ] = new CFlyingCritterAI();
	}
}
CFlyingCrittersAlgorithmData::~CFlyingCrittersAlgorithmData()
{
	delete m_params;

	for ( Uint32 i = 0; i < m_boidsLimit; ++i )
	{
		delete m_critterAiArray[ i ];
	}
	delete [] m_critterAiArray;
}

void CFlyingCrittersAlgorithmData::Initialize( CSwarmLairEntity& lair )
{
	Super::Initialize( lair );

	CFlyingCrittersLairEntity& crittersLair		= static_cast< CFlyingCrittersLairEntity& >( lair );
	m_playerList								= &m_dynamicPoiJobDataMap.Find( IBoidLairEntity::CPlayerDynamicPointsAcceptor::GetInstance()->GetFilterTag() )->m_second;

	
	m_boidStateCountArray.Resize( m_params->m_boidStateArray.Size() );
	for ( Uint32 i = 0; i < m_boidStateCountArray.Size(); ++i )
	{
		m_boidStateCountArray[ i ] = 0;
	}

	CalculateEnviromentData( crittersLair );

	for ( Uint32 i = 0; i < m_boidsLimit; ++i )
	{
		CFlyingCritterAI *const critterAi = static_cast< CFlyingCritterAI * > (m_critterAiArray[ i ]);
		critterAi->SetWorld( this );

		CBoidInstance* instance = lair.GetBoidInstance( i );
		if ( instance )
		{
			if ( instance->GetBoidState() != BOID_STATE_NOT_SPAWNED )
			{
				critterAi->Spawn( instance->GetPosition() );
				if ( m_totalSpawnLimit > 0 )
				{
					--m_totalSpawnLimit;
				}
			}
		}
	}
	m_lairBoundingBox			= crittersLair.GetBoundingBox();

	const Box & areaBox					= m_lairBoundingBox;
	const Vector3 areaBoxSize			= areaBox.CalcSize();
	const Vector3 areaBoxPosition		= areaBox.CalcCenter();
	const Float halfAreaHeight			= areaBoxSize.Z * 0.5f;
	const Float greatestLairSide		= Max( areaBoxSize.X, areaBoxSize.Y );

	// groups should be able to stretch through the entire zone :
	m_maxDist_IdleTarget			= greatestLairSide * 0.75f;
	// distance should be fairly small to make birds in a small zone not fight too much
	// This parameter should depend on groupCount Lair size and boid count in each group
	m_minDist_IdleTarget			= 8.0f;
	m_groupForceMult_IdleTarget		= 10.0f;
	
	// If a group is above that distance to the prefered altitude
	// then a force is applied to come back to it
	m_maxDistToAltitude_IdleTarget	= 2.0f;
	m_altitudeForceMult_IdleTarget	= 10.0;

	// collisionForceMult must be bellow groupForceMult & altitudeForceMult so that 
	// the idle target might snap out of collision if spawn point is in collision
	m_collisionForceMult_IdleTarget	= 24.0f;

	// smaller that collision force so that it doesn't go through obstacles
	m_randomForceMult_IdleTarget	= 10.0f;

	m_maxVel_IdleTarget				= 8.0f;
}



void CFlyingCrittersAlgorithmData::CalculateEnviromentData( CFlyingCrittersLairEntity& lair )
{
	
}


Int32 SelectRandomValidPOIIndex( const CPoiJobData_Array & poiArray )
{
	Int32 validPoiCounter	= 0;
	Int32 selectedIndex		= -1;
	for ( Uint32 i = 0, n = poiArray.Size(); i < n; ++i )
	{
		if ( poiArray[ i ].m_cpntParams.m_enabled )
		{
			validPoiCounter++;
			const Float rand	= GEngine->GetRandomNumberGenerator().Get< Float >( 1.0f );
			const Float proba	=  1.0f / validPoiCounter;
			if ( rand < proba )
			{
				selectedIndex = i;
			}
		}
	}
	return selectedIndex;
}
Int32 SelectNextValidPOIIndex( Int32 currentIndex, const CPoiJobData_Array & poiArray )
{
	Int32 selectedIndex	= currentIndex + 1;
	selectedIndex		= selectedIndex % poiArray.Size();

	while ( selectedIndex != currentIndex )
	{	
		if ( poiArray[ selectedIndex ].m_cpntParams.m_enabled )
		{
			return selectedIndex;
		}
		++selectedIndex;
		selectedIndex = selectedIndex % poiArray.Size();
	}
	
	return selectedIndex;
}
 
void CFlyingCrittersAlgorithmData::UpdateMovementAndAnimation( const TSwarmStatesList currentStateList, TSwarmStatesList targetStateList )
{
	// We must wait for cell map to be loaded
	const CSwarmCellMap *const cellMap = m_cellMap.Get();
	
	const EngineTime startTime = EngineTime::GetNow();
	const CFlyingCritterLairParams & params		= *static_cast< const CFlyingCritterLairParams* >( m_params );
	// Update script requests :
	// Add group
	for ( Uint32 i = 0; i < m_scriptInput->m_createFlyingGroupRequestArray.Size(); ++i )
	{
		const CCreateFlyingGroupRequest & request = m_scriptInput->m_createFlyingGroupRequestArray[ i ];
		CreateGroup( &request );
	}
	m_scriptInput->m_createFlyingGroupRequestArray.Clear();

	// Move boid 
	for ( Uint32 i = 0; i < m_scriptInput->m_moveBoidToGroupRequestArray.Size(); ++i )
	{
		const CMoveBoidToGroupRequest & request = m_scriptInput->m_moveBoidToGroupRequestArray[ i ];
		MoveBoidToGroup( &request );
	}
	m_scriptInput->m_moveBoidToGroupRequestArray.Clear();

	// Remove group ( must be done after move otherwise birds might be killed before they can be moved )
	for ( Uint32 i = 0; i < m_scriptInput->m_removeGroupRequestArray.Size(); ++i )
	{
		const CFlyingGroupId & groupId	= m_scriptInput->m_removeGroupRequestArray[ i ];
		RemoveGroup( &groupId );
	}
	m_scriptInput->m_removeGroupRequestArray.Clear();

	UpdateIdleTargets( );

	// Init update counters
	for ( Uint32 i = 0; i < m_boidStateCountArray.Size(); ++i )
	{
		m_boidStateCountArray[ i ] = 0;
	}
	m_boidCountInEffector = 0;

	// Main update 
	// First iterate through all groups :
	for ( Uint32 groupIndex = 0; groupIndex < m_scriptInput->m_groupList.Size(); ++groupIndex )
	{
		CFlyingSwarmGroup &flyingGroup		= m_scriptInput->m_groupList[ groupIndex ] ;
		
		// Do we have a state change request :
		Bool poiStateChanged	= false;
		if ( flyingGroup.m_changeGroupState != CName::NONE )
		{
			poiStateChanged							= true;
			const Uint32 newStateIndex				= params.GetGroupStateIndexFromName( flyingGroup.m_changeGroupState );
			if ( newStateIndex != (Uint32)-1 )
			{
				flyingGroup.m_currentGroupState			= flyingGroup.m_changeGroupState;
				flyingGroup.m_currentGroupStateIndex	= newStateIndex;
			}
			flyingGroup.m_changeGroupState			= CName::NONE;
		}

		if ( flyingGroup.m_currentGroupStateIndex == (Uint32)-1 )
		{
			continue;
		}

		// Init group vars 
		Vector3 newCenterOfCloud(0.0f, 0.0f, 0.0f);
		Uint32 flyingBoidCount									= 0;
		Uint32 aliveBoidCount									= 0;
		flyingGroup.m_groupState								= params.m_groupStateArray[ flyingGroup.m_currentGroupStateIndex ];
		const CPoiJobData_Array *const despawnPoiJobDataArray	= GetPoiArrayFromType( flyingGroup.m_despawnPoiType );
		const CPoiConfigByGroup_Map *const poiConfigByGroupMap	= flyingGroup.m_currentGroupStateIndex < (Int32)params.m_groupStateToPoiConfig.Size() ? &params.m_groupStateToPoiConfig[ flyingGroup.m_currentGroupStateIndex ] : NULL;
		if ( poiConfigByGroupMap == NULL )
		{
			return;
		}

		// Then we must fill the Poi array :
		// first all poi needs to be iterated and checked if they are elligible
		CPoiJobData_Map::iterator it		= m_staticPoiJobData_Map.Begin();
		CPoiJobData_Map::iterator end		= m_staticPoiJobData_Map.End();

		CFlyingCrittersPoi_Map flyingCrittersPoi_Map;	// Map of poi's that will be passed all boids of the group 
		flyingCrittersPoi_Map.Resize( m_staticPoiJobData_Map.Size() );
		Uint32 i = 0;
		while ( it != end ) // Looping trough all types of POIs
		{
			// for each group state finding the poiConfig depending on poi type  :
			const CName& pointOfInterestType							= it->m_first;
			flyingCrittersPoi_Map[ i ].m_first							= pointOfInterestType;
			CPoiConfigByGroup_Map::const_iterator poiConfigByGroupIt	= poiConfigByGroupMap->Find( pointOfInterestType );
			const CPoiConfigByGroup *const poiConfigByGroup				= poiConfigByGroupIt != poiConfigByGroupMap->End() ? poiConfigByGroupIt->m_second : NULL;
			if ( poiConfigByGroup )
			{
				// New for each poi job data work out if elligible :
				Float closestPoiSqDist					= NumericLimits< Float >::Max();
				CPoiJobData * closestPoiJobData			= NULL;

				flyingCrittersPoi_Map[ i ].m_second.m_poiConfig	= poiConfigByGroup;
				
				CPoiJobData_Array & staticPointList		= it->m_second;
				CPoiJobData_Array::iterator listIt		= staticPointList.Begin();
				CPoiJobData_Array::iterator listEnd		= staticPointList.End();
				while ( listIt != listEnd ) // looping through all the poi's of that type
				{
					CPoiJobData &staticPoiJobData	= *listIt;
					if ( poiConfigByGroup->m_closestOnly == false )
					{	
						flyingCrittersPoi_Map[ i ].m_second.m_poiJobDataArray.PushBack( &staticPoiJobData );
					}
					else
					{
						const Float sqDist = ( staticPoiJobData.GetPositionWithOffset() - flyingGroup.m_groupCenter ).SquareMag3();
						if ( sqDist < closestPoiSqDist )
						{
							closestPoiJobData		= &staticPoiJobData;
						}
					}
					++listIt;
				}

				// Adding closest poi to the corresponding field in flyingCrittersPoi_Map
				if ( closestPoiJobData )
				{
					flyingCrittersPoi_Map[ i ].m_second.m_poiJobDataArray.PushBack( closestPoiJobData );
				}
				
			}
			++it;
			i++;
		}
		// sorting the map so that it can works as a map :
		flyingCrittersPoi_Map.Sort();

		// adding player poi 
		if ( m_playerList->Empty() == false )
		{
			CPoiConfigByGroup_Map::const_iterator playerPoiConfigIt		= poiConfigByGroupMap->Find( CNAME( Player ) );
			const CPoiConfigByGroup *const playerPoiConfig				= playerPoiConfigIt != poiConfigByGroupMap->End() ? playerPoiConfigIt->m_second : NULL;
			if ( playerPoiConfig )
			{
				CPoiJobData & poiData				= *m_playerList->Begin();
				CFlyingCrittersPoi_Map::iterator it =  flyingCrittersPoi_Map.Find( CNAME( Player ) );
				if ( it != flyingCrittersPoi_Map.End() )
				{
					it->m_second = CPoiConfigAndList( playerPoiConfig );
					it->m_second.m_poiJobDataArray.PushBack( &poiData );
				}
			}
		}

		// adding movingIdleTarget field to flyingCrittersPoi_Map
		CPoiConfigByGroup_Map::const_iterator movingIdlePoiConfigIt	= poiConfigByGroupMap->Find( CNAME( MovingIdleTarget ) );
		const CPoiConfigByGroup *const movingIdlePoiConfig			= movingIdlePoiConfigIt != poiConfigByGroupMap->End() ? movingIdlePoiConfigIt->m_second : NULL;
		if ( movingIdlePoiConfig )
		{
			CFlyingCrittersPoi_Map::iterator it =  flyingCrittersPoi_Map.Find( CNAME( MovingIdleTarget ) );
			if ( it != flyingCrittersPoi_Map.End() )
			{
				it->m_second = CPoiConfigAndList( movingIdlePoiConfig );

				CIdleTarget &idleTarget			=  flyingGroup.m_idleTarget;
				
				it->m_second.m_poiJobDataArray.PushBack( &idleTarget.m_poiJobData );
				
			}
		}

		
		// Now iterating through all boids in the group and from the index retrieve the 
		// boid's data
		const Uint32 boidCountInGroup	= flyingGroup.m_boidIndexArray.Size();
		for ( Uint32 boidIndexInGroup = 0; boidIndexInGroup < boidCountInGroup; ++boidIndexInGroup )
		{
			const Uint32 & boidIndex					= flyingGroup.m_boidIndexArray[ boidIndexInGroup ];
			CFlyingCritterAI *const flyingCritterAI		= static_cast< CFlyingCritterAI * > (m_critterAiArray[ boidIndex ]);
			const SSwarmMemberStateData & currentState	= currentStateList[ boidIndex ];
			SSwarmMemberStateData & targetState			= targetStateList[ boidIndex ];
			if ( m_lair->IsJobOutOfTime() )
			{
				if ( m_lair->IsJobTerminationRequest() )
				{
					return;
				}
				do 
				{
					targetState = currentState;
					++boidIndexInGroup;
				} while ( boidIndexInGroup < boidCountInGroup );
				return;
			}
			// Setting up neighbours :
			CNeighbourId_Array & neighbourIdArray = flyingCritterAI->GetNeighbourIdArray();
			Vector3 tooCloseForce( 0.0f, 0.0f, 0.0f );
			Vector3 tooFarForce( 0.0f, 0.0f, 0.0f );

			if ( flyingGroup.m_groupState->m_neighbourCount > MAX_SWARM_NEIGHBOUR_COUNT )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( SwarmAI ), TXT("Too many neighbour in group State %s"), flyingGroup.m_currentGroupState.AsString().AsChar() );
			}
			const Uint32 neighbourCount = Min( flyingGroup.m_groupState->m_neighbourCount, MAX_SWARM_NEIGHBOUR_COUNT );

			if ( poiStateChanged )
			{
				neighbourIdArray.Resize( neighbourCount );
				for ( Uint32 j = 0; j < neighbourIdArray.Size(); ++j )
				{
					neighbourIdArray[ j ] = (Uint32) -1;
				}
			}
			const Uint32 randomNeighbourCount = (Uint32)(flyingGroup.m_groupState->m_randomNeighboursRatio * neighbourIdArray.Size());
			for ( Uint32 j = 0; j < neighbourIdArray.Size(); ++j )
			{
				Uint32 & neighbourIndexInGroup							= neighbourIdArray[j];
				if ( neighbourIndexInGroup == (Uint32)-1 )
				{
					if ( j < randomNeighbourCount )
					{
						neighbourIndexInGroup = GEngine->GetRandomNumberGenerator().Get< Uint32 >( boidCountInGroup );
					}
					else
					{
						neighbourIndexInGroup = boidIndexInGroup + j + 1;
						neighbourIndexInGroup = neighbourIndexInGroup < boidCountInGroup ? neighbourIndexInGroup :  j;
					}
				}
				// calculating tooCloseForce and tooFarForce
				if ( neighbourIndexInGroup != (Uint32)-1 && neighbourIndexInGroup < boidCountInGroup )
				{
					const Uint32 neighbourIndex							= flyingGroup.m_boidIndexArray[ neighbourIndexInGroup ];
					CFlyingCritterAI *const neighbourFlyingCritterAI	= static_cast< CFlyingCritterAI * > (m_critterAiArray[ neighbourIndex ]);

					if ( neighbourFlyingCritterAI->GetCritterState() != CRITTER_STATE_NOT_SPAWNED && neighbourIndex != boidIndex )
					{
						const SSwarmMemberStateData & neighbourState		= currentStateList[ neighbourIndex ];

						const Vector3 distanceVect							= neighbourState.m_position - currentState.m_position;
						const Float distance								= distanceVect.Mag();
						const Vector3 normedDistanceVect					= ( distance > NumericLimits< Float >::Epsilon() ) ? ( distanceVect / distance ) : ( flyingCritterAI->m_lastTooCloseDistanceVect );

						if ( distance <= flyingGroup.m_groupState->m_cozyFlyDist )
						{
							const Float ratio					= distance / flyingGroup.m_groupState->m_cozyFlyDist;
							const Vector2 normedDistanceVect2	= normedDistanceVect.AsVector2().Normalized();
							tooCloseForce						-= Vector3( normedDistanceVect2 * (1.0f - ratio) );
						}
						else
						{
							tooFarForce		+=	normedDistanceVect * (distance - flyingGroup.m_groupState->m_cozyFlyDist);
						}
					}	
					else
					{
						neighbourIndexInGroup = (Uint32)-1;
					}
				}			
			}

			// just to make sure the force does not get out of hand
			if ( tooFarForce.SquareMag() > 1.0f * 1.0f ) 
			{
				tooFarForce.Normalize();
			}

			if ( tooCloseForce.SquareMag() > 1.0f * 1.0f ) 
			{
				tooCloseForce.Normalize();
			}
			flyingCritterAI->m_lastTooCloseDistanceVect = tooCloseForce;
			flyingGroup.m_waterLevel = GGame->GetActiveWorld()->GetWaterLevel( flyingGroup.m_groupCenter, 2 );
			flyingCritterAI->UpdateFlying( currentState, targetState, &flyingGroup, poiStateChanged, tooCloseForce, tooFarForce, flyingGroup.m_groupCenter, flyingCrittersPoi_Map, despawnPoiJobDataArray, cellMap );

			const EFlyingCritterState & critterState = (EFlyingCritterState)flyingCritterAI->GetCritterState();
			if ( critterState == FLYING_STATE_IDLE )
			{
				newCenterOfCloud += targetState.m_position;
				flyingBoidCount++;
			}

			if ( (ECritterState)critterState != CRITTER_STATE_NOT_SPAWNED )
			{
				aliveBoidCount++;
			}

			if ( flyingCritterAI->GetHasDespawned() )
			{
				++m_totalSpawnLimit;
			}

			// Put that in ::CritterAiPostUpdate ?
			if ( targetState.m_boidState >= 0 )
			{
				m_boidStateCountArray[ targetState.m_boidState ] ++;
			}

			if ( flyingCritterAI->GetCurrentEffectorType() != CName::NONE )
			{
				m_boidCountInEffector++;
			}
		}
		// compute center of cloud
		if ( flyingBoidCount > 0 )
		{
			flyingGroup.m_groupCenter = newCenterOfCloud / (Float)flyingBoidCount;
		}
		flyingGroup.m_boidCount = aliveBoidCount;
	}

	// Updating spawning requests :

	for ( Uint32 groupIndex = 0; groupIndex < m_scriptInput->m_groupList.Size() ; ++groupIndex )
	{
		CFlyingSwarmGroup & flyingGroup				= m_scriptInput->m_groupList[ groupIndex ];
		if ( flyingGroup.m_toSpawnCount != 0 && m_lairDisabledFromScript == false )
		{
			const CPoiJobData_Array *const poiArray		= GetPoiArrayFromType( flyingGroup.m_spawnPoiType );
			if ( poiArray == NULL || poiArray->Size() == 0 )
			{
				continue;
			}
			Int32 poiIndex =  SelectRandomValidPOIIndex( *poiArray );
			if ( poiIndex != -1 )
			{
				while ( flyingGroup.m_toSpawnCount != 0 && m_spawnAccumulator >= 1.0f )
				{
					// Check if limit has been reached :
					if ( (Uint32)m_activeBoids >= m_boidsLimit )
					{
						flyingGroup.m_toSpawnCount = 0;
						break;
					}
					Uint32 boidIndexInGroup			= (Uint32)-1;
					Uint32 boidIndex				= (Uint32)-1;
					// Find a boid that is not dead :
					for ( Uint32 j = 0; j < flyingGroup.m_boidIndexArray.Size(); ++j )
					{
						boidIndex								= flyingGroup.m_boidIndexArray[ j ];
						CFlyingCritterAI *const flyingCritterAI	= static_cast< CFlyingCritterAI * > (m_critterAiArray[ boidIndex ]);
						if ( flyingCritterAI->GetCritterState() == CRITTER_STATE_NOT_SPAWNED )
						{
							boidIndexInGroup = j;
							break;
						}
					}

					// No boid available !
					if ( boidIndexInGroup == (Uint32)-1 )
					{
						if ( m_nextBoidIndexToSpawn >= m_boidsLimit )
						{
							// happens when scripts order more spawn than 
							flyingGroup.m_toSpawnCount = 0;
							RED_LOG_ERROR( RED_LOG_CHANNEL( SwarmAI ), TXT("Spawn boid error: maximum boid count reached !\n") );
							break;
						}
						boidIndex			=  m_nextBoidIndexToSpawn++;
						flyingGroup.m_boidIndexArray.PushBack( boidIndex );
						boidIndexInGroup	= flyingGroup.m_boidIndexArray.Size() - 1;
						ASSERT( static_cast< CFlyingCritterAI * > (m_critterAiArray[ boidIndex ])->GetCritterState() == CRITTER_STATE_NOT_SPAWNED );
					}
					// Finding poi :
					const EulerAngles orient( GEngine->GetRandomNumberGenerator().Get< Float >( 360.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( 360.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( 360.0f ) );
					const CPoiJobData& poiJobData			= (*poiArray)[ poiIndex ];
					Vector3  randVect( GEngine->GetRandomNumberGenerator().Get< Float >(), GEngine->GetRandomNumberGenerator().Get< Float >(), GEngine->GetRandomNumberGenerator().Get< Float >() );
					randVect.Normalize();
					Vector3 spawnPosition					= poiJobData.GetPositionWithOffset() + orient.TransformVector( randVect ) * GEngine->GetRandomNumberGenerator().Get< Float >( poiJobData.m_cpntParams.m_effectorRadius * poiJobData.m_cpntParams.m_scale );
					Vector3 freePosition					= spawnPosition;

					if ( cellMap && cellMap->GetNextFreeCellPosition_AxisAligned( spawnPosition, freePosition, Vector3( 0.0f, 0.0f, 1.0f ) ) == false )
					{
						// if fail spawn here it will be bad point but do we have any other options ?
						freePosition = spawnPosition;
					}

					spawnPosition = freePosition;
					// Initialising Boid :
					targetStateList	[ boidIndex ].m_position				= spawnPosition;
					targetStateList	[ boidIndex ].m_orientation				= orient;
					targetStateList	[ boidIndex ].m_boidStateUpdateRatio	= GEngine->GetRandomNumberGenerator().Get< Float >();
					targetStateList	[ boidIndex ].m_boidState				= BOID_STATE_NOT_SPAWNED;		// will be there in next frame
					currentStateList[ boidIndex ].m_position				= spawnPosition;
					currentStateList[ boidIndex ].m_orientation				= orient;
					currentStateList[ boidIndex ].m_boidStateUpdateRatio	= GEngine->GetRandomNumberGenerator().Get< Float >();
					currentStateList[ boidIndex ].m_boidState				= BOID_STATE_NOT_SPAWNED;		// will be there in next frame

					CFlyingCritterAI *const flyingCritterAI	= static_cast< CFlyingCritterAI * > (m_critterAiArray[ boidIndex ]);
					flyingCritterAI->Spawn( spawnPosition );
				
					// Book keeping
					if ( m_totalSpawnLimit > 0 )
					{
						--m_totalSpawnLimit;
					}
					m_spawnAccumulator			-= 1.0f;
					poiIndex					= SelectNextValidPOIIndex( poiIndex, *poiArray );
					//poiIndex					+= 1;
					//poiIndex					= poiIndex == poiArray->Size() ? 0 : poiIndex;
					flyingGroup.m_toSpawnCount	-= 1;
				}
					// resseting group cohesion so that it may welcome newcomers
				if ( flyingGroup.m_currentGroupStateIndex != (Uint32)-1 )
				{
					flyingGroup.m_changeGroupState	= params.m_groupStateArray[ flyingGroup.m_currentGroupStateIndex ]->m_stateName;
				}
			}
		}
		if ( flyingGroup.m_toDespawnCount != 0 )
		{
			while ( flyingGroup.m_toDespawnCount != 0 )
			{
				Uint32 boidIndexInGroup			= (Uint32)-1;
				Uint32 boidIndex				= (Uint32)-1;
				// Find a boid that is not dead :
				for ( Uint32 j = 0; j < flyingGroup.m_boidIndexArray.Size(); ++j )
				{
					boidIndex								= flyingGroup.m_boidIndexArray[ j ];
					CFlyingCritterAI *const flyingCritterAI	= static_cast< CFlyingCritterAI * > (m_critterAiArray[ boidIndex ]);
					const EFlyingCritterState & state		= (EFlyingCritterState)flyingCritterAI->GetCritterState();
					if ( state != FLYING_STATE_GO_DESPAWN && (ECritterState)state != CRITTER_STATE_NOT_SPAWNED )
					{
						boidIndexInGroup = j;
						break;
					}
				}
				// No boid available !
				if ( boidIndexInGroup == (Uint32)-1 )
				{
					flyingGroup.m_toDespawnCount	-= 1;
					continue;
				}
				CFlyingCritterAI *const flyingCritterAI	= static_cast< CFlyingCritterAI * > (m_critterAiArray[ boidIndex ]);
				flyingCritterAI->GoDespawn( );
				
				// Book keeping
				m_totalSpawnLimit				+= 1;
				flyingGroup.m_toDespawnCount	-= 1;
			}
			// reseting group cohesion so that it may reform itself
			if ( flyingGroup.m_currentGroupStateIndex != (Uint32)-1 )
			{
				flyingGroup.m_changeGroupState	= params.m_groupStateArray[ flyingGroup.m_currentGroupStateIndex ]->m_stateName;
			}
		}
	}
	const EngineTime endTime = EngineTime::GetNow();
	EngineTime jobTime = endTime - startTime;
	m_jobTime = (Float)jobTime;
}



void CFlyingCrittersAlgorithmData::UpdateSound( const TSwarmStatesList currentStateList, TSwarmStatesList targetStateList )
{
	for ( Uint32 groupIndex = 0; groupIndex < m_scriptInput->m_groupList.Size(); ++groupIndex )
	{ 
		if ( m_soundJobDataCollectionArray.Size() <= groupIndex )
		{
			m_soundJobDataCollectionArray.PushBack( CSwarmSoundJobDataCollection( *m_params ) );
		}
		CSwarmSoundJobDataCollection & jobDataCollection = m_soundJobDataCollectionArray[ groupIndex ];
		CFlyingSwarmGroup & group = m_scriptInput->m_groupList[ groupIndex ];
		for ( Uint32 i = 0; i < jobDataCollection.m_jobDataArray.Size(); ++i )
		{
			CSwarmSoundJobData & soundJobData = jobDataCollection.m_jobDataArray[ i ];
			CalculateSound( targetStateList, &soundJobData, group );
		}
	}
}

void CFlyingCrittersAlgorithmData::CalculateSound( TSwarmStatesList crittersStates, CSwarmSoundJobData *const  soundJobData, CFlyingSwarmGroup & group )
{
	const CSwarmLairParams *const params			= static_cast< const CSwarmLairParams * >( m_params );
	const CR4SwarmSoundConfig *const soundConfig	= static_cast< const CR4SwarmSoundConfig*const >( m_params->m_soundConfigArray[ soundJobData->m_soundIndex ] );

	if ( soundConfig->FilterGroup( group ) && soundConfig->FilterLair( *this ) )
	{
		Vector centerOfMass( 0,0,0,0 );
		Float mass = 0.f;
		TStaticArray< Vector2, 512 > members;
		for ( Uint32 i = 0, n = group.m_boidIndexArray.Size(); i < n; ++i )
		{
			// check if filter match
			const Uint32 & index					= group.m_boidIndexArray[ i ];
			const CBaseCritterAI *const critterAI	= m_critterAiArray[ index ];

			if ( critterAI && critterAI->GetCritterState() != CRITTER_STATE_NOT_SPAWNED &&  soundConfig->FilterBoid( *critterAI ) )
			{
				// distance check
				const Float distSq = (m_cameraPosition.AsVector3() - crittersStates[ index ].m_position.AsVector3()).SquareMag();
				if ( distSq < params->m_maxSoundDistance * params->m_maxSoundDistance )
				{
					// correct center of mass
					Float newMass	= mass + 1.f;
					centerOfMass	= centerOfMass * ( mass / newMass ) + crittersStates[ index ].m_position * ( 1.f / newMass );
					mass			= newMass;
					if ( members.Size() < members.Capacity() )
					{
						// collect position for radius calculation
						members.PushBack( crittersStates[ index ].m_position.AsVector2() );
					}
				}
			}
		}
		// calculate radius
		Float radiusSq = 0.f;
		for ( Uint32 i = 0; i < members.Size(); ++i )
		{
			const Float distSq = (m_cameraPosition.AsVector2() - members[ i ]).SquareMag();
			if ( distSq > radiusSq )
			{
				radiusSq = distSq;
			}
		}
		
		const Vector distVector			= centerOfMass - m_cameraPosition;
		Vector normedDistVector			= distVector;
		const Float dist				= normedDistVector.Normalize3();
		const  EulerAngles distOrient	= normedDistVector.ToEulerAngles();
		const  EulerAngles cameraOrient = m_cameraForward.ToEulerAngles();

		soundJobData->m_center		= centerOfMass;
		soundJobData->m_intensity	= mass;
		soundJobData->m_radius		= sqrt( radiusSq );
		soundJobData->m_distance	= dist;

		soundJobData->m_yaw			= cameraOrient.Yaw - distOrient.Yaw;
		soundJobData->m_pitch		= cameraOrient.Pitch - distOrient.Pitch;

		// keeping yaw and pitch between [ -180.0, 180.0 ]
		if ( soundJobData->m_yaw > 180.0f )
		{
			soundJobData->m_yaw = soundJobData->m_yaw - 360.0f;
		}
		if ( soundJobData->m_yaw < -180.0f )
		{
			soundJobData->m_yaw = soundJobData->m_yaw + 360.0f;
		}
		if ( soundJobData->m_pitch > 180.0f )
		{
			soundJobData->m_pitch = soundJobData->m_pitch - 360.0f;
		}
		if ( soundJobData->m_pitch < -180.0f )
		{
			soundJobData->m_pitch = soundJobData->m_pitch + 360.0f;
		}
	}
	else
	{
		soundJobData->m_intensity	= 0.0f;
	}
}
Boids::PointOfInterestId CFlyingCrittersAlgorithmData::HandleFireInConePoi( CPoiJobData & poiJobData, CPoiJobData *const oldFirePointData  )
{
	// If poi exists we simply update it
	if ( oldFirePointData )
	{
		const Boids::PointOfInterestId id	= oldFirePointData->m_uid;
		*oldFirePointData					= poiJobData;
		oldFirePointData->m_uid				= id;
	}
	else
	{
		return AddTemporaryPoi( poiJobData.m_cpntParams.m_type , poiJobData, 2.0f );
	}
	return (Uint32)-1;
}

void CFlyingCrittersAlgorithmData::PreUpdateSynchronization( Float deltaTime )
{
	const CFlyingCritterLairParams & params		= *static_cast< const CFlyingCritterLairParams* >( m_params );
	Super::PreUpdateSynchronization( deltaTime );

	if ( m_playerList->Empty() || m_lairDisabledFromScript )
	{
		if ( m_despawnAllPending == false )
		{
			if ( m_lairDisabledFromScript == false )
			{
				m_despawnAllTime = m_localTime + 5.f;
			}
			else
			{
				m_despawnAllTime = m_localTime - 1.0f; // deactivate immediatelly
			}
			m_despawnAllPending = true;
		}
		if ( m_despawningAll == false )
		{
			if ( m_localTime > m_despawnAllTime )
			{
				m_despawningAll = true;
				for ( Uint32 i = 0; i < m_scriptInput->m_groupList.Size(); ++i )
				{
					CFlyingSwarmGroup &group				= m_scriptInput->m_groupList[ i ];
					group.m_toDespawnCount					= group.m_boidCount;
					group.m_despawnedInDeactivationCount	= group.m_boidCount + group.m_toSpawnCount; // if you deactivate during spawning toSpawnCount must be accounted for
					group.m_toSpawnCount					= 0;
				}
			}
		}
	}
	else
	{
		if ( m_despawningAll )
		{
			// waiting for despawn to finish before respawning :
			Bool allowRespawn = true;
			for ( Uint32 i = 0; i < m_scriptInput->m_groupList.Size(); ++i )
			{
				CFlyingSwarmGroup &group				= m_scriptInput->m_groupList[ i ];
				if ( group.m_boidCount != 0 )
				{
					allowRespawn = false;
				}
			}
			if ( allowRespawn )
			{
				m_despawningAll = false;
		
				for ( Uint32 i = 0; i < m_scriptInput->m_groupList.Size(); ++i )
				{
					CFlyingSwarmGroup &group	= m_scriptInput->m_groupList[ i ];
					group.m_toSpawnCount		= group.m_despawnedInDeactivationCount;
				}
			}
		}
		m_despawnAllPending = false;
	}

	CFlyingCrittersLairEntity* lair									= static_cast< CFlyingCrittersLairEntity* >( const_cast< CSwarmLairEntity* >( m_lair ) );

	// Updating old fire in cone poi :
	Int32 fireInConePoiIndex_A = -1;
	Int32 fireInConePoiIndex_B = -1;
	for ( Uint32 i = 0; i < m_temporaryPois.Size(); ++i )
	{
		if ( m_temporaryPois[ i ].m_poiId == m_fireInConePoiID_A )
		{
			fireInConePoiIndex_A = i;
		}
		else if ( m_temporaryPois[ i ].m_poiId == m_fireInConePoiID_B )
		{
			fireInConePoiIndex_B = i;
		}
	}

	// if spell stops temp poi will subside for a while ( area marked in the psychology of the swarm )
	// but effector must not apply because the spell has stopped
	CPoiJobData * oldFirePointDataA = NULL;
	CPoiJobData * oldFirePointDataB = NULL;
	if ( fireInConePoiIndex_A != -1 )
	{
		oldFirePointDataA					= GetStaticPoi( m_temporaryPois[ fireInConePoiIndex_A ].m_poiType, m_temporaryPois[ fireInConePoiIndex_A ].m_poiId );
		oldFirePointDataA->m_applyEffector	= false;
	}
	if ( fireInConePoiIndex_B != -1 )
	{
		oldFirePointDataB					= GetStaticPoi( m_temporaryPois[ fireInConePoiIndex_B ].m_poiType, m_temporaryPois[ fireInConePoiIndex_B ].m_poiId );
		oldFirePointDataB->m_applyEffector	= false;
	}


	// Adding POI's for Spells (IGNI etc.)
	const CFlyingCrittersLairEntity::SFireInConeInfo& fireInCone	= lair->GetFireInConeInfo();
	if ( fireInCone.m_isPending )
	{
		EulerAngles orient( 0.0f ,  0.0f, EulerAngles::YawFromXY( fireInCone.m_dir.X, fireInCone.m_dir.Y ) );

		Vector3 offset( 0.0f, 0.0f, 1.0f );
		Vector forward;
		orient.ToAngleVectors( &forward, NULL, NULL );

		const Float radiusA =  fireInCone.m_range * 0.10f * 2;
		const Float radiusB =  fireInCone.m_range * 0.25f * 2;

		CPoiJobData newFirePointDataA;
		newFirePointDataA.m_position								= fireInCone.m_origin + forward * (fireInCone.m_range * 0.5f - radiusA) + offset;
		newFirePointDataA.m_orientation								= orient;
		newFirePointDataA.m_useCounter								= 0;
		newFirePointDataA.m_applyEffector							= true;
		newFirePointDataA.m_cpntParams.m_type						= CNAME( Fire );
		newFirePointDataA.m_cpntParams.m_scale						= radiusA;
		newFirePointDataA.m_cpntParams.m_effectorRadius				= 1.0f;
		newFirePointDataA.m_cpntParams.m_shapeType					= CNAME( Circle );		

		newFirePointDataA.PrecomputeValues();
		const Boids::PointOfInterestId newIdA	= HandleFireInConePoi( newFirePointDataA, oldFirePointDataA );
		m_fireInConePoiID_A						= newIdA != (Uint32)-1 ? newIdA : m_fireInConePoiID_A;
		
		CPoiJobData newFirePointDataB;
		newFirePointDataB.m_position								= fireInCone.m_origin + forward * (fireInCone.m_range * 0.5f + radiusB) + offset;
		newFirePointDataB.m_orientation								= orient;
		newFirePointDataB.m_useCounter								= 0;
		newFirePointDataB.m_applyEffector							= true;
		newFirePointDataB.m_cpntParams.m_type						= CNAME( Fire );
		newFirePointDataB.m_cpntParams.m_scale						= radiusB;
		newFirePointDataB.m_cpntParams.m_effectorRadius				= 1.0f;
		newFirePointDataB.m_cpntParams.m_shapeType					= CNAME( Circle );		

		newFirePointDataB.PrecomputeValues();
		const Boids::PointOfInterestId newIdB	= HandleFireInConePoi( newFirePointDataB, oldFirePointDataB );
		m_fireInConePoiID_B						= newIdB != (Uint32)-1 ? newIdB : m_fireInConePoiID_B;

		lair->DisposePendingFireInConeInfo();
	}	

	// script calls:
	lair->OnScriptTick( *m_scriptInput, m_despawningAll == false, params.m_updateTime );
}

void CFlyingCrittersAlgorithmData::PostUpdateSynchronization(  )
{
	const CFlyingCritterLairParams & params	= *static_cast< const CFlyingCritterLairParams* >( m_params );
	Super::PostUpdateSynchronization( );
	CFlyingCrittersLairEntity* lair			= static_cast< CFlyingCrittersLairEntity* >( const_cast< CSwarmLairEntity* >( m_lair ) );

	// POI behaviour :
	CPoiJobData_Map::iterator it		= m_staticPoiJobData_Map.Begin();
	CPoiJobData_Map::iterator end		= m_staticPoiJobData_Map.End();
	
	while ( it != end )
	{
		const CName& pointOfInterestType								= it->m_first;
		
		CPoiJobData_Array & staticPointList		= it->m_second;
		CPoiJobData_Array::iterator listIt			= staticPointList.Begin();
		CPoiJobData_Array::iterator listEnd		= staticPointList.End();
		while ( listIt != listEnd )
		{
			CPoiJobData & poiJobData	= *listIt;
			poiJobData.PostUpdateSynchronization( lair, params.m_updateTime );
			++listIt;
		}
		++it;
	}

	CPoiJobData_Map::iterator dynMapIt	= m_dynamicPoiJobDataMap.Begin();
	CPoiJobData_Map::iterator dynMapEnd	= m_dynamicPoiJobDataMap.End();
	while ( dynMapIt != dynMapEnd )
	{
		const CName& pointOfInterestType	= dynMapIt->m_first;
		CPoiJobData_Array & pointList	= dynMapIt->m_second;

		CPoiJobData_Array::iterator listIt		= pointList.Begin();
		CPoiJobData_Array::iterator listEnd		= pointList.End();
		while ( listIt != listEnd )
		{
			
			CPoiJobData & poiJobData		= *listIt;
			poiJobData.PostUpdateSynchronization( lair, params.m_updateTime );
			++listIt;
		}
		++ dynMapIt;
	}

	// Remove timed out temporary POI :
	if ( !m_temporaryPois.Empty() )
	{
		for ( Int32 i = m_temporaryPois.Size() - 1; i >= 0; --i )
		{
			if ( m_temporaryPois[ i ].m_fadeoutTime < GetLocalTime() )
			{
				RemoveStaticPoi( m_temporaryPois[ i ].m_poiType, m_temporaryPois[ i ].m_poiId );
				m_temporaryPois.EraseFast( m_temporaryPois.Begin() + i );
			}
		}
	}

	if ( m_totalSpawnLimit >= 0 && Uint32(m_aliveBoids + m_totalSpawnLimit) <= m_breakCounter )
	{
		if ( !m_isLairDefeated )
		{
			m_isLairDefeated = true;
			lair->OnDefeated();
		}
	}
}

Boids::PointOfInterestId CFlyingCrittersAlgorithmData::AddTemporaryPoi( Boids::PointOfInterestType poiType, CPoiJobData& poiData, Float fadeoutDelay )
{
	poiData.m_uid = m_nextPoiId++;
	AddStaticPoi( poiData );
	TemporaryPoi tempPoiData;
	tempPoiData.m_poiType		= poiType;
	tempPoiData.m_poiId			= poiData.m_uid;
	tempPoiData.m_fadeoutTime	= GetLocalTime() + fadeoutDelay;
	if ( m_temporaryPois.Size() < m_temporaryPois.Capacity() )
	{
		m_temporaryPois.PushBack( tempPoiData );
	}
	else
	{
		Uint32 oldestIndex	= 0;
		Float oldestFade	= m_temporaryPois[ 0 ].m_fadeoutTime;
		for( Uint32 i = 1; i < m_temporaryPois.Size(); ++i )
		{
			if ( m_temporaryPois[ i ].m_fadeoutTime < oldestFade )
			{
				oldestIndex = i;
				oldestFade = m_temporaryPois[ i ].m_fadeoutTime;
			}
		}
		RemoveStaticPoi( m_temporaryPois[ oldestIndex ].m_poiType, m_temporaryPois[ oldestIndex ].m_poiId );
		m_temporaryPois[ oldestIndex ] = tempPoiData;
	}
	return poiData.m_uid;
}

void CFlyingCrittersAlgorithmData::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	for ( Uint32 i = 0; i < m_scriptInput->m_groupList.Size(); ++i )
	{
		const CIdleTarget & idleTarget	= m_scriptInput->m_groupList[ i ].m_idleTarget;
		const Vector idleTargetPosition	= idleTarget.m_poiJobData.GetPositionWithOffset();

		frame->AddDebugSphere( idleTargetPosition, 1.25f,  Matrix::IDENTITY, Color::RED, false );
		frame->AddDebug3DArrow( idleTargetPosition, idleTarget.m_collisionForce, 10.0f, 0.04f, 0.16f, 0.1f, Color::DARK_BLUE );
	}
	const Vector3 areaBoxPosition	= m_lairBoundingBox.CalcCenter();
	CSwarmCellMap *const cellMap	= m_cellMap.Get();
	if ( cellMap )
	{
		cellMap->DebugDisplay( frame );
		frame->AddDebugText( areaBoxPosition + Vector3( 0.0f, 0.0f, -1.0f ), String::Printf( TXT("CellMap Size = %.3f KByte"), cellMap->GetSizeInKByte() ), true, Color::RED  );
	}
	frame->AddDebugText( areaBoxPosition, String::Printf( TXT("Job time = %.1f ms"), m_jobTime * 1000.0f ), true, Color::RED );
}

void CFlyingCrittersAlgorithmData::ComputeAltitudeForce( Vector3 & altitudeForce, const Vector3 &idleTargetPosition, Float randomAltitude )
{
	altitudeForce				= Vector3( 0.0f, 0.0f, 0.0f );
	const Float diffToAltitude	= randomAltitude - idleTargetPosition.Z;
	const Float distToAltitude	= fabs( diffToAltitude );
	if ( distToAltitude > m_maxDistToAltitude_IdleTarget )
	{
		altitudeForce.Z		= ( diffToAltitude / distToAltitude ) * ( distToAltitude - m_maxDistToAltitude_IdleTarget );
		// All force vectors between [ 0, 1 ]
		if ( Abs( altitudeForce.Z ) > 1.0f )
		{
			altitudeForce.Z = altitudeForce.Z / Abs( altitudeForce.Z );
		}
	}
}

void CFlyingCrittersAlgorithmData::UpdateIdleTargets( )
{
	CSwarmCellMap *const cellMap = m_cellMap.Get();
	if ( cellMap )
	{
		cellMap->ClearDebug();
	}
	const CFlyingCritterLairParams & params		= *static_cast< const CFlyingCritterLairParams* >( m_params );
	const Box & areaBox					= m_lairBoundingBox;
	const Vector3 areaBoxSize			= areaBox.CalcSize();
	const Vector3 areaBoxPosition		= areaBox.CalcCenter();
	const Float halfAreaHeight			= areaBoxSize.Z * 0.5f;

	for ( Uint32 i = 0; i < m_scriptInput->m_groupList.Size(); ++i )
	{
		CFlyingSwarmGroup & group		= m_scriptInput->m_groupList[ i ];
		
		if ( group.m_currentGroupStateIndex == (Uint32)-1 ) // happens on the first init frame
		{
			continue;
		}
		CGroupState *const groupState	= params.m_groupStateArray[ group.m_currentGroupStateIndex ];

		CIdleTarget & idleTargetA		= group.m_idleTarget;
		Vector3 & positionA				= idleTargetA.m_poiJobData.m_position.AsVector3();

		Vector3 tooCloseForce( 0.0f, 0.0f, 0.0f );
		Vector3 tooFarForce( 0.0f, 0.0f, 0.0f );

		// Center Force vector
		Vector2 centreForce	= areaBoxPosition.AsVector2() - positionA.AsVector2();
		Float centerForceLenSq = centreForce.SquareMag();
		// All force vectors between [ 0, 1 ]
		if ( centerForceLenSq < 1.0f * 1.0f )
		{
			centreForce *= 1.f / MSqrt( centerForceLenSq );
		}
		// too far, too close :
		for ( Uint32 j = 0; j < m_scriptInput->m_groupList.Size(); ++j )
		{
			if ( i != j )
			{
				// Compute distance between groupd in 3D
				const CIdleTarget & idleTargetB = m_scriptInput->m_groupList[ j ].m_idleTarget;
				const Vector3 & positionB		= idleTargetB.m_poiJobData.GetPositionWithOffset();
				const Vector3 distVect			= positionB - positionA;
				const Float dist				= distVect.Mag();

				// Compute avoidance in 2D because flocks of birds like to stay on the same altitude
				if ( dist < m_minDist_IdleTarget )
				{
					if ( dist > FLT_EPSILON ) // divide by 0 check
					{
						tooCloseForce	-=  ((distVect / dist) * (m_minDist_IdleTarget - dist));
					}
				}
				else if ( dist > m_maxDist_IdleTarget )
				{
					tooFarForce		+= ((distVect / dist) * ( dist - m_maxDist_IdleTarget ));
				}
			}
		}
		tooCloseForce.Z = 0.0f;
		tooFarForce.Z = 0.0f;
		// All force vectors between [ 0, 1 ]
		if ( tooCloseForce.SquareMag() > 1.0f * 1.0f )
		{
			tooCloseForce.Normalize();
		}
		// All force vectors between [ 0, 1 ]
		if ( tooFarForce.SquareMag() > 1.0f * 1.0f )
		{
			tooFarForce.Normalize();
		}

		// From time to time bring the group back to the center of the lair
		if ( idleTargetA.m_changeMultTimer < m_localTime )
		{
			// center force
			idleTargetA.m_centerForceMult	= GEngine->GetRandomNumberGenerator().Get< Float >() < 0.5f ? 0.0f : 10.0f;
			// altitude
			idleTargetA.m_randomAltitude	= areaBoxPosition.Z + GEngine->GetRandomNumberGenerator().Get< Float >( -halfAreaHeight, halfAreaHeight );
			// randomForce
			idleTargetA.m_randomForce		= Vector3( GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ) );
			idleTargetA.m_randomForce.Normalize();

			// update timer
			idleTargetA.m_changeMultTimer	= m_localTime + GEngine->GetRandomNumberGenerator().Get< Float >( 10.0f , 20.0f );
		}

		// [ Step ] Calculating altitude force 
		Vector3 altitudeForce;
		ComputeAltitudeForce( altitudeForce, positionA, idleTargetA.m_randomAltitude );

		// [ Step ] Calculating velocity without collision force and collision force modifiers :
		// This permits to feed an idea of this frame's velocity to the collision algo 
		const Vector3 emulatedAcceleration		= ( tooCloseForce + tooFarForce ) * m_groupForceMult_IdleTarget + altitudeForce * m_altitudeForceMult_IdleTarget + idleTargetA.m_randomForce * m_randomForceMult_IdleTarget;
		const Vector3 emulatedVelocity			= idleTargetA.m_velocity + emulatedAcceleration * m_currentDelta;

		// [ Step ] Sphere test !
		Vector3 collisionForce( 0.0f, 0.0f, 0.0f );
		const Bool currentCellValid = cellMap ? cellMap->ComputeCollisionForceAtPosition( positionA, emulatedVelocity, SWARM_COLLISION_TEST_RADIUS, collisionForce, SPHERE_TEST_DEBUG_COLOR ) : true;

		// [ Step ] collision force modifiers
		// Collision force is very important so anything that
		// opposes it is removed using RemoveVectorComponant
		const Float savedTooCloseForceMag	= tooCloseForce.Mag(); // saved value for next step
		const Float savedTooFarForce		= tooFarForce.Mag();	// saved value for next step
		RemoveOppositionFromVect( tooCloseForce, collisionForce );
		RemoveOppositionFromVect( tooFarForce, collisionForce );
		RemoveOppositionFromVect( idleTargetA.m_randomForce, collisionForce );

		if ( cellMap )
		{
			// AltitudeForce is a special case because it is a frame by frame vector
			const Float altitudeDot = altitudeForce.Dot( collisionForce );
			// if altitude force opposing
			if ( currentCellValid && altitudeDot < 0.0f ) 
			{
				// selecting another no opposing altitude
				if ( altitudeForce.Z < 0.0f ) // if altitudeForce pushes toward to bottom
				{
					Vector3 blockedCellPosition;
					Vector3 lastFreeCellPosition;
					// Check if we have indeed a blocking cell down there
					if ( cellMap->GetNextBlockedCellPosition_AxisAligned( positionA, blockedCellPosition, Vector3( 0.0f, 0.0f, -1.0f ), SWARM_COLLISION_TEST_RADIUS, &lastFreeCellPosition, ALTITUDE_TEST_DEBUG_COLOR ) )
					{
						// Correct only if our current altitude is lower than the last free cell Z
						if ( idleTargetA.m_randomAltitude < lastFreeCellPosition.Z )
						{
							idleTargetA.m_randomAltitude = lastFreeCellPosition.Z;
						}
					}
				}
				else // if altitudeForce pushes toward to top
				{
					Vector3 blockedCellPosition;
					Vector3 lastFreeCellPosition;
					// Check if we have indeed a blocking cell up there
					if ( cellMap->GetNextBlockedCellPosition_AxisAligned( positionA, blockedCellPosition, Vector3( 0.0f, 0.0f, 1.0f ), SWARM_COLLISION_TEST_RADIUS, &lastFreeCellPosition ) )
					{
						// Correct only if our current altitude is higher than the last free cell Z
						if ( idleTargetA.m_randomAltitude > lastFreeCellPosition.Z )
						{
							idleTargetA.m_randomAltitude = lastFreeCellPosition.Z;
						}
					}
				}
				ComputeAltitudeForce( altitudeForce, positionA, idleTargetA.m_randomAltitude );
			}
		}

		// [ Step ] renormalising corrected forces 
		idleTargetA.m_randomForce.Normalize();	// This force is always normalized
		tooCloseForce	= tooCloseForce.Normalized() * savedTooCloseForceMag;
		tooFarForce		= tooFarForce.Normalized() * savedTooFarForce;

		// [ Step ] Calculating real velocity
		idleTargetA.m_velocity			+= ( ( tooCloseForce + tooFarForce ) * m_groupForceMult_IdleTarget + collisionForce * m_collisionForceMult_IdleTarget + altitudeForce * m_altitudeForceMult_IdleTarget + idleTargetA.m_randomForce * m_randomForceMult_IdleTarget ) * m_currentDelta;

		// [ Step ] Clamping velocity to max speed
		Float velocityMag = idleTargetA.m_velocity.Mag();
		if ( velocityMag > m_maxVel_IdleTarget )
		{
			idleTargetA.m_velocity = idleTargetA.m_velocity / velocityMag * m_maxVel_IdleTarget;
			velocityMag = m_maxVel_IdleTarget;
		}

		// [ Step ] position candidate for fail safe line test
		Vector candidateDisplacementVector		= idleTargetA.m_velocity * m_currentDelta;
		candidateDisplacementVector.W			= candidateDisplacementVector.Mag3();
		const Vector3 nextPositionCandidate		= positionA	+ candidateDisplacementVector.AsVector3();
		
		// [ Step ] Line testing to make sure we are not heading through an obstacle ( can happen because above algo doesn't garanty that )
		if ( cellMap && currentCellValid && candidateDisplacementVector.AsVector3().IsAlmostZero() == false && cellMap->LineTest( positionA, candidateDisplacementVector, LINE_TEST_DEBUG_COLOR ) )
		{
			Vector3 freeCellPosition;
			if ( R4SwarmUtils::FindCellToClearPath( positionA, candidateDisplacementVector, cellMap, freeCellPosition ) )
			{
				const Vector3 vectToFreeCell		= freeCellPosition - positionA;
				const Float vectToFreeCellSquareMag	= vectToFreeCell.SquareMag();
				Vector3 newVelocity					= vectToFreeCell;
				// If the cell is far enough multiply the old velocity magnitude to the new velocity
				// Of not we must keep the velocity as is otherwise we might end up behind the free cell after an update
				if ( vectToFreeCellSquareMag > velocityMag * velocityMag )
				{
					newVelocity /= Red::Math::MSqrt( vectToFreeCellSquareMag );
					newVelocity *= velocityMag;
				}
				idleTargetA.m_velocity = newVelocity;
			}
			else
			{
				idleTargetA.m_velocity = Vector3( 0.0f, 0.0f, 0.0f );
			}
		}
		
		Vector3 destPosition = positionA	+ idleTargetA.m_velocity * m_currentDelta;

		// Correct position for birds and fishes
		if( params.m_livesUnderWater == true && destPosition.Z > group.m_waterLevel - params.m_wallsDistance )
		{
			destPosition.Z = group.m_waterLevel - params.m_wallsDistance;
		}
		else if( params.m_livesUnderWater == false && destPosition.Z < group.m_waterLevel + params.m_wallsDistance )
		{
			destPosition.Z = group.m_waterLevel + params.m_wallsDistance;
		}

		// [ Step ] interpolating position
		const Vector3 nextPosition		= destPosition;	
		positionA						= nextPosition;
		idleTargetA.m_collisionForce	= collisionForce;
	}
}

const CPoiJobData_Array *const CFlyingCrittersAlgorithmData::GetPoiArrayFromType( CName poiType )const
{
	if ( poiType == CNAME( Player ) )
	{
		return m_playerList;
	}
	CPoiJobData_Map::const_iterator it = m_staticPoiJobData_Map.Find( poiType );
	if ( it != m_staticPoiJobData_Map.End() )
	{
		return &(it->m_second);
	}
	return NULL;
}

void CFlyingCrittersAlgorithmData::CreateGroup( const CCreateFlyingGroupRequest *const createFlyingGroupRequest )
{
	CSwarmCellMap *const cellMap = m_cellMap.Get();
	m_scriptInput->m_groupList.PushBack( CFlyingSwarmGroup() );
	CFlyingSwarmGroup & flyingSwarmGroup = m_scriptInput->m_groupList[ m_scriptInput->m_groupList.Size() - 1 ];
	
	flyingSwarmGroup.m_boidIndexArray.Reserve( m_boidsLimit ); 

	Uint32 toSpawnCount = createFlyingGroupRequest->m_toSpawnCount;
	if ( createFlyingGroupRequest->m_fromOtherGroup_Id.m_id != (Uint32)-1 )
	{
		toSpawnCount = 0;
		m_scriptInput->MoveBoidToGroup( createFlyingGroupRequest->m_fromOtherGroup_Id, createFlyingGroupRequest->m_toSpawnCount, flyingSwarmGroup.m_groupId );
	}
		
	// Setting the poi state:
	flyingSwarmGroup.m_changeGroupState			= createFlyingGroupRequest->m_groupState;
	flyingSwarmGroup.m_toSpawnCount				= toSpawnCount; 
	flyingSwarmGroup.m_spawnPoiType				= createFlyingGroupRequest->m_spawnPoiType;
	flyingSwarmGroup.m_despawnPoiType			= createFlyingGroupRequest->m_spawnPoiType; // defaulting despawn to spawn

	const Vector3 boxSize		= m_lairBoundingBox.CalcSize();
	const Vector3 lairCentre	= m_lairBoundingBox.CalcCenter();
	flyingSwarmGroup.m_idleTarget.m_poiJobData.m_position	= Vector3( GEngine->GetRandomNumberGenerator().Get< Float >( -0.5f , 0.5f ) * boxSize.X, GEngine->GetRandomNumberGenerator().Get< Float >( -0.5f , 0.5f ) * boxSize.Y, GEngine->GetRandomNumberGenerator().Get< Float >( -0.5f , 0.5f ) * boxSize.Z ) + lairCentre;
	// Updating sound collections 
	m_soundJobDataCollectionArray.PushBack( CSwarmSoundJobDataCollection( *m_params ) );

	// Initialising idle target to be at the position of the spawn point :
	const CPoiJobData_Array *const despawnPoiJobDataArray	= GetPoiArrayFromType( flyingSwarmGroup.m_spawnPoiType );
	if ( despawnPoiJobDataArray && despawnPoiJobDataArray->Size() != 0 )
	{
		const Vector3 spawnPointPosition = (*despawnPoiJobDataArray)[ 0 ].GetPositionWithOffset();
		Vector3 freePosition = spawnPointPosition;
		if ( cellMap && cellMap->GetNextFreeCellPosition_AxisAligned( spawnPointPosition, freePosition, Vector3( 0.0f, 0.0f, 1.0f ), SPAWN_POI_DEBUG_COLOR ) == false )
		{
			// if fail spawn here it will be bad point but do we have any other options ?
			freePosition = spawnPointPosition;
		}
		flyingSwarmGroup.m_idleTarget.m_poiJobData.m_position = freePosition;
	}
}

void CFlyingCrittersAlgorithmData::RemoveGroup( const CFlyingGroupId *const groupID )
{
	for ( Uint32 i = 0; i < m_scriptInput->m_groupList.Size(); ++i )
	{
		CFlyingSwarmGroup & flyingGroup = m_scriptInput->m_groupList[ i ];
		if ( flyingGroup.m_groupId == *groupID )
		{
			if ( flyingGroup.m_boidIndexArray.Size() != 0 )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( SwarmAI ), TXT("Remove swarm group error: destroying a group with alive boids !\n") );
			}
			for ( Uint32 j = 0; j < flyingGroup.m_boidIndexArray.Size(); ++j )
			{
				const Uint32 & boidIndex					= flyingGroup.m_boidIndexArray[ j ];
				CFlyingCritterAI *const flyingCritterAI		= static_cast< CFlyingCritterAI * > (m_critterAiArray[ boidIndex ]);

				if ( flyingCritterAI->GetCritterState() != CRITTER_STATE_NOT_SPAWNED )
				{
					flyingCritterAI->Kill();
				}
			}
			
			m_scriptInput->m_groupList.RemoveAt( i );
			m_soundJobDataCollectionArray.RemoveAt( i );
			return;
		}
	}
	GFeedback->ShowError( TXT("Remove swarm group error: group not found !") );
}

void CFlyingCrittersAlgorithmData::MoveBoidToGroup( const CMoveBoidToGroupRequest *const request )
{
	const CFlyingCritterLairParams & params		= *static_cast< const CFlyingCritterLairParams* >( m_params );
	CFlyingSwarmGroup *const flyingSwarmGroupA	= m_scriptInput->GetGroupFromId( request->m_groupIdA );
	CFlyingSwarmGroup *const flyingSwarmGroupB	= m_scriptInput->GetGroupFromId( request->m_groupIdB );
	ASSERT( flyingSwarmGroupA != flyingSwarmGroupB );

	if ( flyingSwarmGroupA == NULL || flyingSwarmGroupB == NULL )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( SwarmAI ), TXT("Move boid Error: One of the group doesn't exists\n") );
		return;
	}
	if ( flyingSwarmGroupA == flyingSwarmGroupB )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( SwarmAI ), TXT("Move boid Error: The two groups are the same !\n") );
		return;
	}
	if ( request->m_count == 0 )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( SwarmAI ), TXT("Move boid Error: boid count = 0 !\n") );
		return;
	}

	Uint32 toMoveCount = request->m_count;
	// First find a boid that is alive 
	// Beggining from the bottom of the array becaus it is cheaper to remove from the bottom of the array ( I think )
	for ( Int32 i = (Int32)flyingSwarmGroupA->m_boidIndexArray.Size() - 1; i >= 0 ; --i )
	{
		const Uint32 & boidIndexA					= flyingSwarmGroupA->m_boidIndexArray[ i ];
		CFlyingCritterAI *const flyingCritterAiA	= static_cast< CFlyingCritterAI * > (m_critterAiArray[ boidIndexA ]);

		if ( ((EFlyingCritterState)flyingCritterAiA->GetCritterState()) == FLYING_STATE_IDLE )
		{
			flyingSwarmGroupB->m_boidIndexArray.PushBack( boidIndexA );
			flyingSwarmGroupA->m_boidIndexArray.RemoveAt( i );
			--toMoveCount;
				
			if ( toMoveCount == 0 )
			{
				break;
			}
		}
	}

	if ( toMoveCount > 0 )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( SwarmAI ), TXT("Move boid Error: not all boids moved !") );
	}

	if ( flyingSwarmGroupA->m_currentGroupStateIndex != (Uint32)-1 )
	{
		flyingSwarmGroupA->m_changeGroupState	= params.m_groupStateArray[ flyingSwarmGroupA->m_currentGroupStateIndex ]->m_stateName;
	}
	if ( flyingSwarmGroupB->m_currentGroupStateIndex != (Uint32)-1 )
	{
		flyingSwarmGroupB->m_changeGroupState	= params.m_groupStateArray[ flyingSwarmGroupB->m_currentGroupStateIndex ]->m_stateName;
	}
}

void CFlyingCrittersAlgorithmData::FillPoiJobDataFromDynamicPoi( CPoiJobData & poiJobData, const CName& filterTag, CEntity*const entity )
{
	CSwarmAlgorithmData::FillPoiJobDataFromDynamicPoi( poiJobData, filterTag, entity );
	const CFlyingCritterLairParams *const params = static_cast< const CFlyingCritterLairParams * >(m_params);
	CFlyingPoiConfig_Map::const_iterator it = params->m_flyingPoiConfigMap.Find( filterTag );
	if ( it != params->m_flyingPoiConfigMap.End() )
	{
		const CFlyingPoiConfig *const config = it->m_second;
		poiJobData.m_positionOffset	= config->m_positionOffest;
	}
}



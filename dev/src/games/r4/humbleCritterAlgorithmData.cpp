#include "build.h"
#include "humbleCritterAlgorithmData.h"
#include "humbleCrittersAI.h"
#include "humbleCritterSound.h"
#include "../../common/game/boidInstance.h"
#include "../../common/game/boidCone.h"
#include "../../common/engine/pathlibWorld.h"

///////////////////////////////////////////////////////////////
// CHumbleCrittersAlgorithmData
///////////////////////////////////////////////////////////////
CHumbleCrittersAlgorithmData::CHumbleCrittersAlgorithmData( CHumbleCrittersLairEntity* lair, const CHumbleCritterLairParams & params )
	: CSwarmAlgorithmData( lair )
	, m_breakCounter( lair->m_breakCounter )
	, m_activeBoids( 0 )
	, m_isLairDefeated( false )
	, m_isDeactivationTimeUp( false )
	, m_isDeactivationPending( false )
	, m_deactivationTime( 0.f )
	, m_nextPoiId( 0x80000000 )
	, m_fireInConePoiID( (Uint32)-1 )
{
	m_params			= new CHumbleCritterLairParams( params );
	m_critterAiArray	= new CBaseCritterAI* [ m_boidsLimit ];
	for ( Uint32 i = 0; i < m_boidsLimit; ++i )
	{
		m_critterAiArray[ i ] = new CHumbleCritterAI();
	}

	// Need to create the job data that will create the sound collection in postUpdate
	m_soundJobDataCollectionArray.PushBack( CSwarmSoundJobDataCollection( *m_params ) );
}
CHumbleCrittersAlgorithmData::~CHumbleCrittersAlgorithmData()
{
	delete m_params;

	for ( Uint32 i = 0; i < m_boidsLimit; ++i )
	{
		delete m_critterAiArray[ i ];
	}
	delete [] m_critterAiArray;
}

void CHumbleCrittersAlgorithmData::Initialize( CSwarmLairEntity& lair )
{
	const CHumbleCritterLairParams & params		= *static_cast< const CHumbleCritterLairParams* >( m_params );
	Super::Initialize( lair );

	CHumbleCrittersLairEntity& crittersLair = static_cast< CHumbleCrittersLairEntity& >( lair );

	m_actorsList	= &m_dynamicPoiJobDataMap.Find( IBoidLairEntity::CPlayerDynamicPointsAcceptor::GetInstance()->GetFilterTag() )->m_second;

	if ( m_params->m_spawnPointArray.Size() == 0 )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( SwarmAI ), TXT("No spawn point found, did you specify the spawn point PoiConfig in the xml ?\n") );
	}
	else
	{
		CPoiJobData_Map::iterator spawnPointIt =  m_staticPoiJobData_Map.Find( m_params->m_spawnPointArray[ 0 ] );
		if ( spawnPointIt == m_staticPoiJobData_Map.End() )
		{
			RED_LOG_ERROR( RED_LOG_CHANNEL( SwarmAI ), TXT("No spawn point found, did you specify the spawn point PoiConfig in the xml ?\n") );
		}
		else
		{
			m_spawnPoints	= &spawnPointIt->m_second;
		}
	}

	
	m_boidStateCountArray.Resize( params.m_boidStateArray.Size() );
	for ( Uint32 i = 0; i < m_boidStateCountArray.Size(); ++i )
	{
		m_boidStateCountArray[ i ] = 0;
	}

	CalculateEnviromentData( crittersLair );

	for ( Uint32 i = 0; i < m_boidsLimit; ++i )
	{
		CHumbleCritterAI *const humbleCritterAi = static_cast< CHumbleCritterAI*const >(m_critterAiArray[ i ]);
		humbleCritterAi->SetWorld( this );

		CBoidInstance* instance = lair.GetBoidInstance( i );
		if ( instance )
		{
			if ( instance->GetBoidState() != BOID_STATE_NOT_SPAWNED )
			{
				// WTF, should this be despawn ?
				humbleCritterAi->Spawn( instance->GetPosition().AsVector2() );
				if ( m_totalSpawnLimit > 0 )
				{
					--m_totalSpawnLimit;
				}
			}
		}
	}
}

void CHumbleCrittersAlgorithmData::CalculateEnviromentData( CHumbleCrittersLairEntity& lair )
{
	const CHumbleCritterLairParams & params		= *static_cast< const CHumbleCritterLairParams* >( m_params );
	// compute world boundings
	const Box& bbox = lair.GetBoundingBox();
	Vector2 bboxExtends = bbox.Max.AsVector2() - bbox.Min.AsVector2();
	Int32 w = Int32( ceilf( bboxExtends.X * 2.f ) );				
	Int32 h = Int32( ceilf( bboxExtends.Y * 2.f ) );
	m_enviroment = new SwarmEnviromentData( bbox.Min, bbox.Max, w, h );
	m_enviroment->Clear();

	// get pathlib
	CWorld*const world				= lair.GetLayer()->GetWorld();
	CPathLibWorld* pathLibWorld		= world ? world->GetPathLibWorld() : NULL;

	// get area boundings
	CAreaComponent* areaBoundings = lair.GetLairBoundings();

	// fill up enviromental data
	Float celRadius = Max( m_enviroment->GetCelSize().X, m_enviroment->GetCelSize().Y ) / 2.f;

	PathLib::AreaId areaId		= PathLib::INVALID_AREA_ID;
	const Float defaultHeight	= lair.GetWorldPositionRef().Z;

	
	
	
	for ( Int32 x = 0; x < w; ++x )
	{
		for ( Int32 y = 0; y < h; ++y )
		{
			SwarmEnviromentCelData& celData	=	m_enviroment->GetChannel( x, y );
			celData.m_flags					=  CDF_BLOCKED;
			celData.m_wallPotential.Set( 0, 0 );
			celData.m_z						= 0.0f;
		}
	}
	// Retrieving spawn points :
	CSwarmFloodFill floodFill( m_enviroment, pathLibWorld, celRadius, areaBoundings );
	for ( Uint32 i = 0; i < m_spawnPoints->Size(); ++i)
	{
		const Vector3 & spawnPointPosition		= (*m_spawnPoints)[ i ].GetPositionWithOffset();
		//Int32 x, y;
		//m_enviroment->GetCellCoordinatesFromWorldPosition( spawnPointPosition.AsVector2(), x, y );
		floodFill.AddFloodSource( spawnPointPosition );
	}

	floodFill.Process();

	// calculate potential field
	// TODO: Optimize (serpent-like iteration)
	const Vector2& celSize = m_enviroment->GetCelSize();
	Int32 maxPotentialCelsX = Int32( params.m_wallsDistance / celSize.X );
	Int32 maxPotentialCelsY = Int32( params.m_wallsDistance / celSize.Y );
	for ( Int32 x = 0; x < w; ++x )
	{
		for ( Int32 y = 0; y < h; ++y )
		{
			// [STEP] Calculating wall potential
			if ( ( m_enviroment->GetChannel( x, y ).m_flags & CDF_BLOCKED ) == 0 )
			{
				Vector2 fieldPotentialDir( 0, 0 );
				Float closestCel = FLT_MAX;
				// TODO: Optimize!
				for ( Int32 relX = -maxPotentialCelsX; relX <= maxPotentialCelsX; ++relX )
				{
					for ( Int32 relY = -maxPotentialCelsY; relY <= maxPotentialCelsY; ++relY )
					{
						Int32 celX = x + relX;
						Int32 celY = y + relY;
						if ( !m_enviroment->IsCorrectCell( celX, celY ) || ( m_enviroment->GetChannel( celX, celY ).m_flags & CDF_BLOCKED ) )
						{
							Vector2 diff( Float( relX ) * celSize.X, Float( relY ) * celSize.Y );
							Float dist = diff.Mag();
							if ( dist < closestCel )
							{
								closestCel = dist;
							}

							if ( dist < params.m_wallsDistance )
							{
								Float ratio = (1.f-(dist / params.m_wallsDistance)) / dist;

								fieldPotentialDir += diff * (-ratio);
							}
						}
					}
				}
				if ( params.m_wallsDistance <= celSize.X )
				{
					RED_LOG_ERROR(  RED_LOG_CHANNEL( SwarmAI ), TXT("wallsDistance should be > 0.5") );
				}
				if ( closestCel < params.m_wallsDistance )
				{
					Float distRatio = 1.f - closestCel / params.m_wallsDistance;
					m_enviroment->GetChannel( x, y ).m_wallPotential = fieldPotentialDir.Normalized() * params.m_wallsRepulsion * distRatio;
				}
			}

			// [STEP] Blocking steep cells
			SwarmEnviromentCelData & currentCellData = m_enviroment->GetChannel( x, y );
			if ( ( currentCellData.m_flags & CDF_BLOCKED ) == 0 )
			{
				if (	TestCellMaxZ( x + 1, y, currentCellData.m_z ) == false
					||	TestCellMaxZ( x - 1, y, currentCellData.m_z ) == false
					||	TestCellMaxZ( x, y + 1, currentCellData.m_z ) == false
					||	TestCellMaxZ( x, y - 1, currentCellData.m_z ) == false )
				{
					currentCellData.m_flags |= CDF_BLOCKED;
				}
			}
		}
	}
}

Bool CHumbleCrittersAlgorithmData::TestCellMaxZ( Int32 x, Int32 y, Float currentZ )const
{
	if ( m_enviroment->IsCorrectCell( x , y ) == false )
	{
		return true;
	}
	static Float maxZDiffBetweenCells = 0.5f;
	const SwarmEnviromentCelData & neighbourCellData = m_enviroment->GetChannel( x, y );
	if ( Abs( neighbourCellData.m_z - currentZ ) > maxZDiffBetweenCells )
	{
		return false;			
	}
	return true;
}

void CHumbleCrittersAlgorithmData::UpdateMovementAndAnimation( const TSwarmStatesList currentStateArray, TSwarmStatesList targetStateArray )
{
	if ( m_spawnPoints == NULL )
	{
		return;
	}
	const CHumbleCritterLairParams & params		= *static_cast< const CHumbleCritterLairParams* >( m_params );
	// deactivation handling
	const auto& actorsList = GetActors();
	if ( actorsList.Empty() || m_lairDisabledFromScript )
	{
		if ( !m_isDeactivationPending )
		{
			if ( m_lairDisabledFromScript == false )
			{
				m_deactivationTime		= m_localTime + 5.f;
			}
			else
			{
				m_deactivationTime		= m_localTime - 1.0f; // deactivate immediatelly
			}
			m_isDeactivationPending = true;
		}
		if ( !m_isDeactivationTimeUp )
		{
			if ( m_localTime > m_deactivationTime )
			{
				m_isDeactivationTimeUp = true;
			}
		}

		if ( m_isDeactivationTimeUp )
		{
			// TODO: despawn frequency (hardcoded by hand now)
			Uint32 index = m_activeBoids != 0 ? GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_activeBoids ) : 0;
			Uint32 activeFound = 0;
			for ( Uint32 i = 0, boidsCount = m_boidsLimit; i < boidsCount; ++i )
			{
				CHumbleCritterAI *const humbleCritterAi = static_cast< CHumbleCritterAI*const >(m_critterAiArray[ i ]);
				if ( humbleCritterAi->GetCritterState() != CRITTER_STATE_NOT_SPAWNED )
				{
					if ( activeFound == index )
					{
						humbleCritterAi->GoDespawn();
						break;
					}
					++activeFound;
				}
			}
		}
	}
	else
	{
		if ( m_isDeactivationTimeUp )
		{
			for ( Uint32 i = 0, boidsCount = m_boidsLimit; i < boidsCount; ++i )
			{
				CHumbleCritterAI *const humbleCritterAi = static_cast< CHumbleCritterAI*const >(m_critterAiArray[ i ]);
				humbleCritterAi->CancelDespawn();
			}

			m_isDeactivationTimeUp = false;
		}
		m_isDeactivationPending = false;
	}

	// init sound 
	for ( Uint32 i = 0; i < m_boidStateCountArray.Size(); ++i )
	{
		m_boidStateCountArray[ i ] = 0;
	}
	m_boidCountInEffector = 0;

	// main update 
	for ( Uint32 i = 0, boidsCount = m_boidsLimit; i < boidsCount; ++i )
	{
		if ( m_lair->IsJobOutOfTime() )
		{
			if ( m_lair->IsJobTerminationRequest() )
			{
				return;
			}
			do 
			{
				targetStateArray[ i ] = currentStateArray[ i ];
				++i;
			} while ( i < boidsCount );
			return;
		}
		SSwarmMemberStateData & targetState			= targetStateArray[ i ];
		CHumbleCritterAI *const humbleCritterAi		= static_cast< CHumbleCritterAI*const >(m_critterAiArray[ i ]);
		humbleCritterAi->Update( currentStateArray[ i ], targetState );

		// put that in ::CritterAiPostUpdate ?
		if ( targetState.m_boidState >= 0 )
		{
			m_boidStateCountArray[ targetState.m_boidState ] ++;
		}
		if ( humbleCritterAi->GetCurrentEffectorType() != CName::NONE )
		{
			m_boidCountInEffector++;
		}
	}

	// spawning
	if ( m_isDeactivationTimeUp == false && !m_actorsList->Empty() && Uint32(m_activeBoids) < m_boidsLimit && m_spawnAccumulator >= 1.f && !m_spawnPoints->Empty() && !IsSpawnLimitHit() )
	{
		Uint32 spawned = 0;
		Uint32 spawnpoints = m_spawnPoints->Size();
		Uint32 tryBoid = 0;
		Uint32 firstPoint = GEngine->GetRandomNumberGenerator().Get< Uint32 >( spawnpoints );

		do 
		{
			while (static_cast< CHumbleCritterAI*const >(m_critterAiArray[ tryBoid ])->GetCritterState() != CRITTER_STATE_NOT_SPAWNED )
			{
				++tryBoid;
			}
			ASSERT( tryBoid < m_boidsLimit );

			const auto& sp = (*m_spawnPoints)[ (firstPoint + spawned) % spawnpoints ];
			targetStateArray[ tryBoid ].m_position				= sp.GetPositionWithOffset();
			targetStateArray[ tryBoid ].m_orientation			= sp.m_orientation;
			targetStateArray[ tryBoid ].m_boidStateUpdateRatio	= GEngine->GetRandomNumberGenerator().Get< Float >();
			targetStateArray[ tryBoid ].m_boidState				= BOID_STATE_NOT_SPAWNED;		// will be there in next frame

			CHumbleCritterAI *const humbleCritterAi = static_cast< CHumbleCritterAI*const >(m_critterAiArray[ tryBoid ]);

			humbleCritterAi->Spawn( sp.GetPositionWithOffset().AsVector2() );
			if ( m_totalSpawnLimit > 0 )
			{
				--m_totalSpawnLimit;
			}
			m_spawnAccumulator -= 1.f;
			++spawned;
		}
		while ( Uint32(m_activeBoids) < m_boidsLimit && m_spawnAccumulator >= 1.f && spawned < m_spawnPoints->Size() && !IsSpawnLimitHit() );
	}
}

void CHumbleCrittersAlgorithmData::PreUpdateSynchronization( Float deltaTime )
{
	const CHumbleCritterLairParams & params		= *static_cast< const CHumbleCritterLairParams* >( m_params );
	Super::PreUpdateSynchronization( deltaTime );
	CHumbleCrittersLairEntity* lair				= static_cast< CHumbleCrittersLairEntity* >( const_cast< CSwarmLairEntity* >( m_lair ) );

	// Adding POI's for Spells (IGNI etc.)
	Int32 fireInConePoiIndex = -1;
	for ( Uint32 i = 0; i < m_temporaryPois.Size(); ++i )
	{
		if ( m_temporaryPois[ i ].m_poiId == m_fireInConePoiID )
		{
			fireInConePoiIndex = i;
		}
	}
	CPoiJobData * oldFirePointData = NULL;
	if ( fireInConePoiIndex != -1 )
	{
		oldFirePointData	= GetStaticPoi( m_temporaryPois[ fireInConePoiIndex ].m_poiType, m_temporaryPois[ fireInConePoiIndex ].m_poiId );
		ASSERT( oldFirePointData );
	}
	// if speels stops temp poi will subside for a while ( area marked in the psychology of the swarm )
	// but effector must not apply because the spell has stopped
	if ( oldFirePointData )
	{
		oldFirePointData->m_applyEffector = false;
	}

	const CHumbleCrittersLairEntity::SFireInConeInfo& fireInCone	= lair->GetFireInConeInfo();
	if ( fireInCone.m_isPending )
	{
		EulerAngles orient( 0.0f ,  0.0f, EulerAngles::YawFromXY( fireInCone.m_dir.X, fireInCone.m_dir.Y ) );

		CPoiJobData newFirePointData;
		newFirePointData.m_position								= fireInCone.m_origin;
		newFirePointData.m_orientation								= orient;
		newFirePointData.m_useCounter								= 0;
		newFirePointData.m_applyEffector							= true;
		newFirePointData.m_cpntParams.m_type						= CNAME( Fire );
		newFirePointData.m_cpntParams.m_gravityRangeMin				= fireInCone.m_range + 1.0f;
		newFirePointData.m_cpntParams.m_gravityRangeMax				= fireInCone.m_range + 2.0f;
		newFirePointData.m_cpntParams.m_effectorRadius				= fireInCone.m_range;
		newFirePointData.m_cpntParams.m_shapeType					= CNAME( Cone );
		newFirePointData.m_cpntParams.m_coneMinOpeningAngle			= 40.0f;
		newFirePointData.m_cpntParams.m_coneMaxOpeningAngle			= 90.0f;
		newFirePointData.m_cpntParams.m_coneEffectorOpeningAngle	= 20.0f;
		

		newFirePointData.PrecomputeValues();

		// If poi exists we simply update it
		if ( fireInConePoiIndex != -1 )
		{
			const Boids::PointOfInterestId id	= oldFirePointData->m_uid;
			*oldFirePointData					= newFirePointData;
			oldFirePointData->m_uid				= id;
		}
		else
		{
			m_fireInConePoiID = AddTemporaryPoi( newFirePointData.m_cpntParams.m_type , newFirePointData, 2.0f );
		}

		lair->DisposePendingFireInConeInfo();
	}

	// Init sounds :
	m_boidCountInEffector = 0;
	const TSwarmStatesList &stateList	= lair->GetPreviousStateList();
	
	for ( Uint32 i = 0; i < m_boidsLimit; ++i )
	{
		CHumbleCritterAI *const humbleCritterAi			= static_cast< CHumbleCritterAI*const >(m_critterAiArray[ i ]);
		humbleCritterAi->SetCurrentEffectorType( CName::NONE );
		const SSwarmMemberStateData & state				= stateList[i];
		const EHumbleCritterState& boidState			= (EHumbleCritterState)humbleCritterAi->GetCritterState();

		if ( (ECritterState)boidState == CRITTER_STATE_NOT_SPAWNED || boidState == HUMBLE_STATE_DIE_BURN_SHAKE || boidState == HUMBLE_STATE_PANIC || boidState == HUMBLE_STATE_FADEOUT )
		{
			continue;
		}

		// Loop through static  poi
		// POI behaviour :
		CPoiJobData_Map::iterator staticIt				= m_staticPoiJobData_Map.Begin();
		CPoiJobData_Map::iterator staticEnd				= m_staticPoiJobData_Map.End();
	
		while ( staticIt != staticEnd )
		{
			const CName& pointOfInterestType								= staticIt->m_first;
			const CPointOfInterestSpeciesConfig *const poiSpeciesConfig		= params.GetPOISpeciesConfigFromType(pointOfInterestType);
			
			CPoiJobData_Array & staticPointList		= staticIt->m_second;
			CPoiJobData_Array::iterator listIt		= staticPointList.Begin();
			CPoiJobData_Array::iterator listEnd		= staticPointList.End();
			while ( listIt != listEnd ) 
			{
				CPoiJobData &staticPointData	= *listIt;
				if ( staticPointData.m_applyEffector )
				{
					if ( staticPointData.m_cpntParams.m_shapeType == CNAME( Circle ) )
					{
						const Vector3 distVect	= staticPointData.GetPositionWithOffset() - state.m_position.AsVector3();
						const Float squaredDist	= distVect.SquareMag();
						const Float radius		= staticPointData.m_cpntParams.m_effectorRadius + humbleCritterAi->GetEffectorRandom() * staticPointData.m_cpntParams.m_effectorRadius ;
						if ( squaredDist < radius * radius )
						{
							humbleCritterAi->ApplyEffector( staticPointData, poiSpeciesConfig );
						}
					}
					else
					{
						const CBoidCone effectorCone( staticPointData.GetPositionWithOffset(), staticPointData.m_forwardVect, staticPointData.m_rightVect,  staticPointData.m_tanHalfEffectorConeOpeningAngle, staticPointData.m_cosHalfEffectorConeOpeningAngle, staticPointData.m_cpntParams.m_effectorRadius );
						if ( effectorCone.IsPointInCone( state.m_position ) )
						{
							humbleCritterAi->ApplyEffector( staticPointData, poiSpeciesConfig );
						}
					}
				}
				++listIt;
			}
			++staticIt;
		}

		// Loop through dynamic  poi
		// POI behaviour :
		CPoiJobData_Map::iterator dynIt				= m_dynamicPoiJobDataMap.Begin();
		CPoiJobData_Map::iterator dynEnd			= m_dynamicPoiJobDataMap.End();
	
		while ( dynIt != dynEnd )
		{
			const CName& pointOfInterestType								= dynIt->m_first;
			const CPointOfInterestSpeciesConfig *const poiSpeciesConfig		= params.GetPOISpeciesConfigFromType(pointOfInterestType);
			
			CPoiJobData_Array & staticPointList		= dynIt->m_second;
			CPoiJobData_Array::iterator listIt		= staticPointList.Begin();
			CPoiJobData_Array::iterator listEnd		= staticPointList.End();
			while ( listIt != listEnd ) 
			{
				CPoiJobData &pointData	= *listIt;
				if ( pointData.m_applyEffector )
				{
					if ( pointData.m_cpntParams.m_shapeType == CNAME( Circle ) )
					{
						const Vector3 distVect	= pointData.GetPositionWithOffset() - state.m_position.AsVector3();
						const Float squaredDist	= distVect.SquareMag();
						const Float radius		= pointData.m_cpntParams.m_effectorRadius + humbleCritterAi->GetEffectorRandom() * pointData.m_cpntParams.m_effectorRadius ;
						if ( squaredDist < radius * radius )
						{
							humbleCritterAi->ApplyEffector( pointData, poiSpeciesConfig );
						}
					}
					else
					{
						const CBoidCone effectorCone( pointData.GetPositionWithOffset(), pointData.m_forwardVect, pointData.m_rightVect,  pointData.m_tanHalfEffectorConeOpeningAngle, pointData.m_cosHalfEffectorConeOpeningAngle, pointData.m_cpntParams.m_effectorRadius );
						if ( effectorCone.IsPointInCone( state.m_position ) )
						{
							humbleCritterAi->ApplyEffector( pointData, poiSpeciesConfig );
						}
					}
				}
				++listIt;
			}
			++dynIt;
		}
	}

	m_cameraPosition = lair->GetLayer()->GetWorld()->GetCameraPosition();
}
void CHumbleCrittersAlgorithmData::PostUpdateSynchronization(  )
{
	const CHumbleCritterLairParams & params		= *static_cast< const CHumbleCritterLairParams* >( m_params );
	Super::PostUpdateSynchronization( );

	// POI behaviour :
	CPoiJobData_Map::iterator it			= m_staticPoiJobData_Map.Begin();
	CPoiJobData_Map::iterator end		= m_staticPoiJobData_Map.End();
	
	while ( it != end )
	{
		const CName& pointOfInterestType								= it->m_first;
		
		CPoiJobData_Array & staticPointList		= it->m_second;
		CPoiJobData_Array::iterator listIt			= staticPointList.Begin();
		CPoiJobData_Array::iterator listEnd		= staticPointList.End();
		while ( listIt != listEnd )
		{
			CPoiJobData &staticPointData	= *listIt;
			if ( staticPointData.m_useCounter != 0 )
			{
				CEntity  * entity										= staticPointData.m_entityHandle.Get();
				if ( entity )
				{
					const TDynArray< CComponent* > & components				= entity->GetComponents();
					CBoidPointOfInterestComponent * poi = NULL;
					// Find component with given name
					for ( Int32 i = components.Size()-1; i >= 0; i-- )
					{
						CComponent* cur = components[ i ];
						if ( cur->IsA< CBoidPointOfInterestComponent >() )
						{
							poi = static_cast< CBoidPointOfInterestComponent* >( cur );
						}
					}

					if (poi)
					{
						poi->OnUsed( staticPointData.m_useCounter,  params.m_updateTime );
					}
					staticPointData.m_useCounter = 0;
				}
			}
			++listIt;
		}
		++it;
	}

	// Remove timed out temporary POI :
	if ( !m_temporaryPois.Empty() )
	{
		for ( Int32 i = m_temporaryPois.Size()-1; i >= 0; --i )
		{
			if ( m_temporaryPois[ i ].m_fadeoutTime < GetLocalTime() )
			{
				RemoveStaticPoi( m_temporaryPois[ i ].m_poiType, m_temporaryPois[ i ].m_poiId );
				m_temporaryPois.EraseFast( m_temporaryPois.Begin() + i );
			}
		}
	}

	Lair* lair = static_cast< Lair* >( const_cast< CSwarmLairEntity* >( m_lair ) );

	if ( m_isDeactivationTimeUp )
	{
		if ( !m_activeBoids )
		{
			lair->DeactivateLair();
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

	if ( !m_attackingCritters.Empty() )
	{
		// do damage
		for ( Uint32 i = 0, n = Min( m_actorsList->Size(), 64U ); i != n; ++i )
		{
			auto itFind = m_attackingCritters.Find( (*m_actorsList)[ i ].m_uid );
			if ( itFind == m_attackingCritters.End() )
			{
				continue;
			}
			CNode* obj = (*m_actorsList)[ i ].m_entityHandle.Get();

			Float damage = Float( itFind->m_second );
			// damage stuff
			obj->CallEvent( CNAME( OnDamageFromBoids ), damage );
		}
		m_attackingCritters.ClearFast();
	}
}

void CHumbleCrittersAlgorithmData::BoidIsAttacking( Boids::PointOfInterestId target )
{
	auto itFind = m_attackingCritters.Find( target ); 
	if ( itFind == m_attackingCritters.End() )
	{
		m_attackingCritters.Insert( target, 1 );
	}
	else
	{
		++itFind->m_second;
	}
}

Boids::PointOfInterestId CHumbleCrittersAlgorithmData::AddTemporaryPoi( Boids::PointOfInterestType poiType, CPoiJobData& poiData, Float fadeoutDelay )
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



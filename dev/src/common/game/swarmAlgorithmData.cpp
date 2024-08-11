#include "Build.h"
#include "swarmAlgorithmData.h"
#include "baseCrittersAi.h"
#include "../core/algorithms.h"

#define INFINITE_SOUND_DISTANCE 10000.0f
 
///////////////////////////////////////////////////////////////
// CSwarmAlgorithmData
///////////////////////////////////////////////////////////////
CSwarmAlgorithmData::CSwarmAlgorithmData( CSwarmLairEntity* lair )
	: m_lair( lair )
	, m_critterAiArray( NULL )
	, m_boidsLimit( lair->GetBoidsCountLimit() )
	, m_params( NULL )
	, m_dynamicPoiCount( 0 )
	, m_poiUid( 0 )
	, m_totalSpawnLimit( lair->GetTotalLifetimeSpawnLimit() )
	, m_spawnFrequency( lair->GetSpawnFrequency() )
	, m_localTime( 0.f )
	, m_spawnAccumulator( 0.f )
	, m_cameraPosition( 0.0f, 0.0f, 0.0f, 0.0f )
	, m_cameraForward( 0.0f, 0.0f, 0.0f, 0.0f )
	, m_soundJobDataCollectionArray()
	, m_boidStateCountArray()
	, m_lairDisabledFromScript( false )
{
	
}

CSwarmAlgorithmData::~CSwarmAlgorithmData()
{
	
}

void CSwarmAlgorithmData::Initialize( CSwarmLairEntity& lair )
{
	// Copying all static points data and mapping them to names :
	// We need to do that because POI may be deleted during the update :
	const TPointsMap& staticPointsOfInterest	= lair.GetStaticPointsOfInterest();
	const Uint32 staticPointsOfInterestCount	= staticPointsOfInterest.Size();
	m_staticPoiJobData_Map.Resize( staticPointsOfInterestCount );
	for ( Uint32 i = 0; i != staticPointsOfInterestCount; ++i )
	{
		m_staticPoiJobData_Map[ i ].m_first		= staticPointsOfInterest[ i ].m_first;
		CPoiJobData_Array& staticPointList		= m_staticPoiJobData_Map[ i ].m_second;
		const CPoiItem_Array& basePoiList		= staticPointsOfInterest[ i ].m_second;
		staticPointList.Reserve( basePoiList.Size() );
		for ( Uint32 j = 0, poiCount = basePoiList.Size(); j != poiCount; ++j )
		{
			CBoidPointOfInterestComponent* boidPointOfInterestComponent = basePoiList[ j ].m_item.Get();
			if ( boidPointOfInterestComponent )
			{
				CPoiJobData staticPointData( boidPointOfInterestComponent, basePoiList[ j ].m_uid );
				staticPointList.PushBack( staticPointData );
			}
		}
	}

	// generate dynamic point categories
	const auto& dynamicPois = lair.GetDynamicPointsOfInterestTypes();
	for ( Uint32 i = 0, n = dynamicPois.Size(); i != n; ++i )
	{
		m_dynamicPoiJobDataMap.Insert( dynamicPois[i]->GetFilterTag(), CPoiJobData_Array() );
	}
}

void CSwarmAlgorithmData::Update( const TSwarmStatesList currentStateList, TSwarmStatesList targetStateList )
{
	UpdateMovementAndAnimation( currentStateList, targetStateList );
	UpdateSound( currentStateList, targetStateList );
}

void CSwarmAlgorithmData::UpdateMovementAndAnimation( const TSwarmStatesList currentStateList, TSwarmStatesList targetStateList )
{
}

void CSwarmAlgorithmData::UpdateSound(  const TSwarmStatesList currentStateList, TSwarmStatesList targetStateList  )
{
	for ( Uint32 j = 0; j < m_soundJobDataCollectionArray.Size(); ++j )
	{
		CSwarmSoundJobDataCollection & jobDataCollection = m_soundJobDataCollectionArray[ j ];
		for ( Uint32 i = 0; i < jobDataCollection.m_jobDataArray.Size(); ++i )
		{
			CSwarmSoundJobData & soundJobData = jobDataCollection.m_jobDataArray[ i ];
			CalculateSound( targetStateList, &soundJobData );
		}
	}
}
void CSwarmAlgorithmData::CalculateSound( TSwarmStatesList crittersStates, CSwarmSoundJobData *const  soundJobData )
{
	const CSwarmLairParams *const params = static_cast< const CSwarmLairParams * >( m_params );

	const CSwarmSoundConfig *const soundConfig	= m_params->m_soundConfigArray[ soundJobData->m_soundIndex ];
	if ( soundConfig->FilterLair( *this ) )
	{
		Vector centerOfMass( 0,0,0,0 );
		Float mass = 0.f;
		TStaticArray< Vector2, 512 > members;
		for ( Uint32 i = 0, n = m_boidsLimit; i < n; ++i )
		{
			// check if filter match
			const CBaseCritterAI *const critterAI	= m_critterAiArray[ i ];
			if ( critterAI && critterAI->GetCritterState() != CRITTER_STATE_NOT_SPAWNED && soundConfig->FilterBoid( *critterAI ) )
			{
				// distance check
				const Float distSq = (m_cameraPosition.AsVector3() - crittersStates[ i ].m_position.AsVector3()).SquareMag();
				if ( distSq < params->m_maxSoundDistance * params->m_maxSoundDistance )
				{
					// correct center of mass
					Float newMass	= mass + 1.f;
					centerOfMass	= centerOfMass * ( mass / newMass ) + crittersStates[ i ].m_position * ( 1.f / newMass );
					mass			= newMass;
					if ( members.Size() < members.Capacity() )
					{
						// collect position for radius calculation
						members.PushBack( crittersStates[ i ].m_position.AsVector2() );
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
void CSwarmAlgorithmData::PreUpdateSynchronization( Float deltaTime )
{
	CSwarmLairEntity*const lair	= static_cast< CSwarmLairEntity* >( const_cast< CSwarmLairEntity* >( m_lair ) );
	m_currentDelta				= deltaTime;
	
	for (CPoiJobData_Map::iterator itr = m_dynamicPoiJobDataMap.Begin(); itr != m_dynamicPoiJobDataMap.End(); ++itr )
	{
		CPoiJobData_Array& pointsList = itr->m_second;
		for( Int32 i = pointsList.Size()-1; i >= 0; --i )
		{
			CPoiJobData & poiJobData = pointsList[i];
			if ( poiJobData.PreUpdateSynchronization( lair, deltaTime ) == false )
			{
				pointsList.RemoveAt(i);
			}
		}
	}
	for (CPoiJobData_Map::iterator itr = m_staticPoiJobData_Map.Begin(); itr != m_staticPoiJobData_Map.End(); ++itr )
	{
		CPoiJobData_Array& pointsList = itr->m_second;
		for( Int32 i = pointsList.Size()-1; i >= 0; --i )
		{
			CPoiJobData & poiJobData = pointsList[i];
			poiJobData.PreUpdateSynchronization( lair, deltaTime );
		}
	}
	// Copying Data
	m_cameraPosition			= lair->GetLayer()->GetWorld()->GetCameraPosition();
	m_cameraForward				= lair->GetLayer()->GetWorld()->GetCameraForward();
	m_lairDisabledFromScript	= lair->GetLairDisabledFromScript();
}
void CSwarmAlgorithmData::PostUpdateSynchronization( )
{
	// We know the job is not running anymore so that means we can use the lair 
	CSwarmLairEntity*const lair	= static_cast< CSwarmLairEntity* >( const_cast< CSwarmLairEntity* >( m_lair ) );
	m_localTime += m_currentDelta;
	if ( m_spawnAccumulator > 1.f )
	{
		m_spawnAccumulator = 1.f;
	}
	m_spawnAccumulator += m_currentDelta * m_spawnFrequency;

	CWorld* world = lair->GetLayer()->GetWorld();
	CBoidSoundsCollection_PointerArray & swarmSoundCollectionArray = lair->GetSoundCollectionArray();


	TDynArray< Bool >  toRemoveArray;
	toRemoveArray.Resize( swarmSoundCollectionArray.Size() );

	// [1] Copy all sound collections in an array to make we can remove the ones that are not used any more :
	for ( Uint32 i = 0; i < toRemoveArray.Size(); ++i) 
	{
		toRemoveArray[ i ] = true;
	}

	// [2] Go through all job data collections, match with sound collections
	// after that loop we should be left with only the sound collection that we do not use any more
	m_cameraPosition	= world->GetCameraPosition();// with latest camera position
	m_cameraForward		= lair->GetLayer()->GetWorld()->GetCameraForward();
	for ( Uint32 i = 0; i < m_soundJobDataCollectionArray.Size(); ++i )
	{
		CSwarmSoundJobDataCollection & jobDataCollection	= m_soundJobDataCollectionArray[ i ];
		const Uint32 index									= lair->GetSoundCollectionIndexFromId( jobDataCollection.m_soundCollectionId );
		if ( index != (Uint32)-1 )
		{
			// we found both so just update :
			CBoidSoundsCollection *const soundCollection = swarmSoundCollectionArray[ index ];
			soundCollection->Update( m_cameraPosition, &jobDataCollection );
			toRemoveArray[ index ] = false;
		}
		else
		{
			// soundCollection is missing so add one :
			CSoundEmitterComponent *const soundEmitterComponent	= lair->GetSoundEmitterComponent( true );
			soundEmitterComponent->SetMaxDistance( INFINITE_SOUND_DISTANCE );
			CBoidSoundsCollection *const soundCollection	= new CBoidSoundsCollection( soundEmitterComponent, *m_params );
			jobDataCollection.m_soundCollectionId			= soundCollection->GetId();
			swarmSoundCollectionArray.PushBack( soundCollection );

			// update :
			soundCollection->Update( m_cameraPosition, &jobDataCollection ); 
		}
	}

	// [3] going through the rest fadeout, update to apply fadeout and then delete
	for ( Int32 i = toRemoveArray.Size() - 1; i >= 0; --i )
	{
		if ( toRemoveArray[ i ] )
		{
			CBoidSoundsCollection *const soundCollection = swarmSoundCollectionArray[ i ];
			soundCollection->FadeOutAll();
			swarmSoundCollectionArray.Remove( soundCollection );
			delete soundCollection;
		}
	}
}
void CSwarmAlgorithmData::AddDynamicPoi( CEntity* entity, const CName& filterTag )
{
	ASSERT( entity );
	auto it = m_dynamicPoiJobDataMap.Find( filterTag );
	if ( it != m_dynamicPoiJobDataMap.End() )
	{
		CPoiJobData poiJobData;
		FillPoiJobDataFromDynamicPoi( poiJobData, filterTag, entity );

		it->m_second.PushBack( poiJobData );
		++m_dynamicPoiCount;
	}
}

void CSwarmAlgorithmData::FillPoiJobDataFromDynamicPoi( CPoiJobData & poiJobData, const CName& filterTag, CEntity*const entity )
{
	poiJobData.m_entityHandle	= entity;
	poiJobData.m_orientation	= entity->GetWorldRotation();
	poiJobData.m_position		= entity->GetWorldPosition();
	poiJobData.m_uid			= m_poiUid++;

	poiJobData.m_cpntParams.m_shapeType			= CNAME( Circle );
	poiJobData.m_cpntParams.m_gravityRangeMin	= 1.0f;
	poiJobData.m_cpntParams.m_gravityRangeMax	= 3.0f;
	poiJobData.m_cpntParams.m_effectorRadius	= 1.0f;
	poiJobData.m_cpntParams.m_type				= filterTag;
	poiJobData.m_cpntParams.m_useReachCallBack	= true;
}

void CSwarmAlgorithmData::RemoveDynamicPoi( CEntity* entity )
{
	ASSERT( entity );
	for( TArrayMap< CName, CPoiJobData_Array >::iterator itr = m_dynamicPoiJobDataMap.Begin(); itr != m_dynamicPoiJobDataMap.End(); ++itr )
	{
		for( Uint32 i = 0; i < itr->m_second.Size(); ++i )
		{
			if( itr->m_second[i].m_entityHandle.Get() == entity )
			{
				itr->m_second.RemoveAt( i );
				--m_dynamicPoiCount;
				break;
			}
		}
	}
}

void CSwarmAlgorithmData::AddStaticPoi( const CPoiJobData& staticPoint )
{
	auto itFind			= m_staticPoiJobData_Map.Find( staticPoint.m_cpntParams.m_type );
	ASSERT( itFind != m_staticPoiJobData_Map.End() );
	itFind->m_second.PushBack( staticPoint );
}

void CSwarmAlgorithmData::RemoveStaticPoi( Boids::PointOfInterestType poiType, Boids::PointOfInterestId uid )
{
	auto itFind = m_staticPoiJobData_Map.Find( poiType );
	if ( itFind == m_staticPoiJobData_Map.End() )
	{
		return;
	}
	auto& list = itFind->m_second;
	auto predicate = [uid]( const CPoiJobData& jobData ) { return jobData.m_uid == uid; };
	list.Erase( RemoveIf( list.Begin(), list.End(), predicate ), list.End() );
}

CPoiJobData *const CSwarmAlgorithmData::GetStaticPoi( Boids::PointOfInterestType poiType, Boids::PointOfInterestId uid )
{
	auto itFind = m_staticPoiJobData_Map.Find( poiType );
	if ( itFind == m_staticPoiJobData_Map.End() )
	{
		return NULL;
	}
	auto& list = itFind->m_second;
	for ( auto it = list.Begin(), end = list.End(); it != end; ++it )
	{
		if ( (*it).m_uid == uid )
		{
			return &(*it);
		}
	}
	return NULL;
}
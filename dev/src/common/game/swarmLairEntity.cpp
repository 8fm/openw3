#include "build.h"
#include "swarmLairEntity.h"
#include "factsDB.h"
#include "boidInstance.h"
#include "swarmAlgorithmData.h"
#include "swarmUpdateJob.h"
#include "../core/taskManager.h"
#include "../core/feedback.h"
#include "../engine/gameTimeManager.h"
#include "../core/mathUtils.h"
#include "../engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CSwarmLairEntity );


///////////////////////////////////////////////////////////////
// CSwarmLairEntity
///////////////////////////////////////////////////////////////
CSwarmLairEntity::CSwarmLairEntity()
	: m_currentStateIndex( 1 )
	, m_isProcessing( false )
	, m_requestOutOfTime( false )
	, m_requestShouldTerminate( false )
	, m_job( NULL )
	, m_swarmAlgorithmData( NULL )
	, m_isDefeated( false )
	, m_defeatedStateFactValue( 1 )
	, m_dynamicPoiCount( 0 )
	, m_lairDisabledFromScript( false )
	, m_lairDisabledAtStartup( false )
{
	for ( Uint32 i = 0; i < MEMBER_STATES_LISTS; ++i )
	{
		m_memberLists[ i ] = NULL;
	}
}

CSwarmLairEntity::~CSwarmLairEntity()
{
	m_requestShouldTerminate = true;
	m_requestOutOfTime = true;

	while ( m_isProcessing )
	{
		Red::Threads::SleepOnCurrentThread( 25 );
	}

	if ( m_swarmAlgorithmData )
	{
		delete (const_cast< CSwarmAlgorithmData* >( m_swarmAlgorithmData ));
		m_swarmAlgorithmData = NULL;
	}

	for ( Uint32 i = 0; i < MEMBER_STATES_LISTS; ++i )
	{
		delete m_memberLists[ i ];
	}
}

void CSwarmLairEntity::SynchroniseData()
{
	ASSERT( m_swarmAlgorithmData );
	CSwarmAlgorithmData* algData = const_cast< CSwarmAlgorithmData* >( m_swarmAlgorithmData );
	// Delayed requests
	for(Uint32 i = 0; i < m_delayedPois.Size(); ++i )
	{
		CEntity *const entity = m_delayedPois[i].m_entity.Get();

		if( entity )
		{
			if( m_delayedPois[i].m_insert )
			{
				AddDynamicPoiToData( algData, entity, m_delayedPois[i].m_filterTag );
			}
			else
			{
				RemoveDynamicPoiFromData( algData, entity );
			}
		}
		else if ( m_delayedPois[i].m_poiCpnt )
		{
			if( m_delayedPois[i].m_insert )
			{
				AddStaticPoiToData( algData, m_delayedPois[i].m_poiCpnt, m_delayedPois[i].m_id );
			}
			else
			{
				RemoveStaticPoiFromData( algData, m_delayedPois[i].m_poiCpnt, m_delayedPois[i].m_id );
			}
		}
		// Else entity was streamed out ?
	}
	m_delayedPois.ClearFast();

	// Test if all elements valid
	algData->PostUpdateSynchronization();
}

void CSwarmLairEntity::OnTimer(const CName name, Uint32 id, Float timeDelta)
{
	PC_SCOPE( CSwarmLairEntity );

	if ( !m_isActivated )
	{
		return;
	}

	const EngineTime& currentTime = m_time;

	if ( m_isProcessing )
	{
		if ( m_nextUpdateTime < currentTime )
		{
			m_requestOutOfTime = true;
		}
	}
	else
	{
		if( m_forceDataSync )
		{
			m_forceDataSync = false;
			SynchroniseData();
		}

		ASSERT( !m_job );
		if ( m_nextUpdateTime < currentTime )
		{
			IssueNewUpdateJob(currentTime);
		}
	}

	InterpolateBoidStates();

	// Needs to be done after because it needs up to date state to read animations
	TBaseClass::OnTimer( name, id, timeDelta );
}

void CSwarmLairEntity::InterpolateWithBezierCurve(Vector* output, const Float progress, const Vector& positionA, const Vector& controlPointA, const Vector& positionB, const Vector& controlPointB) const
{
	(*output) = positionA * pow(1.0f - progress, 3.0f);								// (1-t)^3 * P0
	(*output) += controlPointA * progress * 3.0f * pow(1.0f - progress, 2.0f);		// 3 * (1-t)^2 * P1
	(*output) += controlPointB * (1.0f - progress) * 3.0f * pow(progress, 2.0f);	// 3 * (1-t) * t^2 * P2
	(*output) += positionB * pow(progress, 3.0f);									// t^3 * P3
}

void CSwarmLairEntity::Initialize()
{
	if ( m_spawnLimit > 0 )
	{
		for ( Uint32 i = 0; i < MEMBER_STATES_LISTS; ++i )
		{
			SSwarmMemberStateData* newList = new SSwarmMemberStateData[ m_spawnLimit ];
			for ( Int32 j = 0; j < m_spawnLimit; ++j )
			{
				if ( m_boidInstances[ j ] && m_boidInstances[ j ]->GetBoidState() != BOID_STATE_NOT_SPAWNED )
				{
					newList[ j ].m_position		= m_boidInstances[ j ]->GetPosition();
					newList[ j ].m_orientation	= m_boidInstances[ j ]->GetOrientation();
					newList[ j ].m_boidState	= m_boidInstances[ j ]->GetBoidState();
				}
				else
				{
					newList[ j ].m_position.Set4( 0, 0, 0, 0 );
					newList[ j ].m_orientation	= EulerAngles( 0, 0, 0 );
					newList[ j ].m_boidState	= BOID_STATE_NOT_SPAWNED;
				}
				newList[ j ].m_flags = 0;
			}
			m_memberLists[ i ] = newList;
		}
	}
}

Bool CSwarmLairEntity::ActivateLair()
{
	if ( !Super::ActivateLair() )
	{
		return false;
	}

	if ( m_memberLists[ 0 ] == NULL )
	{
		Initialize();
	}

	if ( m_swarmAlgorithmData == NULL )
	{
		CSwarmAlgorithmData* data = NewAlgorithmData();
		data->Initialize( *this );
		m_swarmAlgorithmData = data;
	}

	
	return true;
}

void CSwarmLairEntity::DeactivateLair()
{
	m_forceDataSync = false;
	Super::DeactivateLair();

	// each time the lair despawn m_lairDisabledAtStartup needs to be re-applied
	if ( m_lairDisabledAtStartup )
	{
		m_lairDisabledFromScript = true;
	}
}

void CSwarmLairEntity::OnDefeated()
{
	if ( !m_isDefeated )
	{
		m_isDefeated = true;

		if ( !m_defeatedStateFact.Empty() )
		{
			CFactsDB* factsDB = GCommonGame ? GCommonGame->GetSystem< CFactsDB >() : NULL;
			if ( factsDB )
			{
				factsDB->AddFact( m_defeatedStateFact, m_defeatedStateFactValue, GGame->GetEngineTime(), CFactsDB::EXP_NEVER );
			}
		}
	}
}

void CSwarmLairEntity::OnJobFinished( CSwarmUpdateJob* job ) volatile
{
	// release job
	job->Release();

	const_cast< CSwarmLairEntity* >( this )->m_job = NULL;

	if ( m_isActivated )
	{
		m_forceDataSync = true;
	}
	// mark as non-processing
	m_isProcessing = false;
}

void CSwarmLairEntity::CollectStaticPointOfInterest( CBoidPointOfInterestComponent* poi, Boids::PointOfInterestId id )
{
	if ( poi )
	{
		if( m_isProcessing || m_forceDataSync )
		{
			SDelayedPoiRequest data;
			data.m_filterTag	= CName::NONE;
			data.m_entity		= NULL;
			data.m_poiCpnt		= poi;
			data.m_insert		= true;
			data.m_id			= id;

			m_delayedPois.PushBack( data );
		}
		else
		{
			CSwarmAlgorithmData* algData = const_cast<CSwarmAlgorithmData*>( m_swarmAlgorithmData );
			AddStaticPoiToData( algData, poi, id );
		}
	}
}

void CSwarmLairEntity::RemoveStaticPointOfInterest( CBoidPointOfInterestComponent* poi, Boids::PointOfInterestId id )
{
	if ( m_swarmAlgorithmData && poi )
	{
		if( m_isProcessing || m_forceDataSync )
		{
			SDelayedPoiRequest data;
			data.m_entity	= NULL;
			data.m_poiCpnt	= poi;
			data.m_insert	= false;
			data.m_id		= id;

			m_delayedPois.PushBack( data );
		}
		else
		{ 
			CSwarmAlgorithmData* algData = const_cast<CSwarmAlgorithmData*>( m_swarmAlgorithmData );
			if (algData)
			{
				RemoveStaticPoiFromData( algData, poi, id );
			}
		}
	}
}

void CSwarmLairEntity::AddDynamicPoiToData( CSwarmAlgorithmData* data, CEntity* entity, const CName& filterTag )
{
	++m_dynamicPoiCount;
	if( !m_isActivated )
	{
		ActivateLair();

		if( !data )
		{
			data = const_cast<CSwarmAlgorithmData*>( m_swarmAlgorithmData );
		}
	}

	if (data)
	{
		data->AddDynamicPoi( entity, filterTag );
	}
}

void CSwarmLairEntity::RemoveDynamicPoiFromData( CSwarmAlgorithmData* data, CEntity* entity )
{
	ASSERT( m_dynamicPoiCount != 0 );
	--m_dynamicPoiCount;
	if (data)
	{
		data->RemoveDynamicPoi( entity );
	}
}

void CSwarmLairEntity::CollectDynamicPointOfInterest( CEntity* entity, const CName& filterTag )
{
	if ( entity )
	{
		if( m_isProcessing || m_forceDataSync )
		{
			SDelayedPoiRequest data;
			data.m_filterTag	= filterTag;
			data.m_entity		= entity;
			data.m_poiCpnt		= NULL;
			data.m_insert		= true;

			m_delayedPois.PushBack( data );
		}
		else
		{
			CSwarmAlgorithmData* algData = const_cast<CSwarmAlgorithmData*>( m_swarmAlgorithmData );
			AddDynamicPoiToData( algData, entity, filterTag );
		}
	}
}

void CSwarmLairEntity::RemoveDynamicPointOfInterest( CEntity* entity ) 
{
	if ( m_swarmAlgorithmData && entity )
	{
		if( m_isProcessing || m_forceDataSync )
		{
			SDelayedPoiRequest data;
			data.m_entity	= entity;
			data.m_poiCpnt	= NULL;
			data.m_insert	= false;

			m_delayedPois.PushBack( data );
		}
		else
		{ 
			CSwarmAlgorithmData* algData = const_cast<CSwarmAlgorithmData*>( m_swarmAlgorithmData );
			if (algData)
			{
				RemoveDynamicPoiFromData( algData, entity );
			}
		}
	}
}

void CSwarmLairEntity::AddStaticPoiToData( CSwarmAlgorithmData* data, CBoidPointOfInterestComponent *const poi, Boids::PointOfInterestId id )
{
	if (data)
	{
		CPoiJobData staticPointData( poi, id );
		data->AddStaticPoi( staticPointData );
	}
}

void CSwarmLairEntity::RemoveStaticPoiFromData( CSwarmAlgorithmData* data, CBoidPointOfInterestComponent *const poi, Uint32 id )
{
	if (data)
	{
		data->RemoveStaticPoi( poi->GetParameters().m_type, id );
	}
}

void CSwarmLairEntity::DeterminePointsOfInterestTypes()
{
	AddDynamicPointOfInterestAcceptor( CPlayerDynamicPointsAcceptor::GetInstance() );

	for ( CPointOfInterestSpeciesConfig_Map::const_iterator it = m_params->m_poiSpeciesConfigMap.Begin(), end = m_params->m_poiSpeciesConfigMap.End(); it != end; ++it )
	{
		const CName &poiType = it->m_first;
		AddStaticPointOfInterestType( poiType );
	}

}

void CSwarmLairEntity::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	IBoidLairEntity::OnGenerateEditorFragments( frame, flag );

	const CPoiJobData_Map & staticPointMap		= const_cast<CSwarmAlgorithmData *>(m_swarmAlgorithmData)->GetStaticPoiDataByType_Map();
	CPoiJobData_Map::const_iterator staticIt	= staticPointMap.Begin();
	CPoiJobData_Map::const_iterator staticEnd	= staticPointMap.End();
	
	while ( staticIt != staticEnd )
	{
		const CPoiJobData_Array & pointList			= staticIt->m_second;
		CPoiJobData_Array::const_iterator listIt	= pointList.Begin();
		CPoiJobData_Array::const_iterator listEnd	= pointList.End();
		while ( listIt != listEnd )
		{
			const Vector3 offset(0.0f, 0.0f, 0.0f);
			const CPoiJobData &pointData	= *listIt;
			frame->AddDebugCircle(  pointData.GetPositionWithOffset() + offset, pointData.m_cpntParams.m_effectorRadius * pointData.m_cpntParams.m_scale, pointData.m_orientation.ToMatrix(), Color::MAGENTA );
			++listIt;			
		}
		++staticIt;
	}

	const CPoiJobData_Map & dynamicPointMap			= const_cast<CSwarmAlgorithmData *>(m_swarmAlgorithmData)->GetDynamicPoiDataByType_Map();
	CPoiJobData_Map::const_iterator  dynamicIt		= dynamicPointMap.Begin();
	CPoiJobData_Map::const_iterator  dynamicEnd		= dynamicPointMap.End();
	
	while ( dynamicIt != dynamicEnd )
	{
		const CPoiJobData_Array & pointList			= dynamicIt->m_second;
		CPoiJobData_Array::const_iterator listIt	= pointList.Begin();
		CPoiJobData_Array::const_iterator listEnd	= pointList.End();
		while ( listIt != listEnd )
		{
			const Vector3 offset(0.0f, 0.0f, 0.0f);
			const CPoiJobData &pointData	= *listIt;
			frame->AddDebugCircle(  pointData.GetPositionWithOffset() + offset, pointData.m_cpntParams.m_effectorRadius * pointData.m_cpntParams.m_scale, pointData.m_orientation.ToMatrix(), Color::MAGENTA );
			++listIt;			
		}
		++dynamicIt;
	}

	GenerateSwarmVisualDebug( frame, flag );
}


void CSwarmLairEntity::OnAttachFinished( CWorld* world )
{
	IBoidLairEntity::OnAttachFinished( world );
	if ( m_lairDisabledAtStartup )
	{
		m_lairDisabledFromScript = true;
	}

	if ( m_params )
	{
		CSoundEmitterComponent * soundEmitterComponent	= GetSoundEmitterComponent( false );
		if ( soundEmitterComponent != nullptr )
		{
			soundEmitterComponent->AddSoundBankDependency( m_params->m_soundBank );
		}		
	}
}


void CSwarmLairEntity::funcDisable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, disable, true );
	FINISH_PARAMETERS;
	
	if ( disable )
	{
		m_lairDisabledFromScript	= true;
	}
	else
	{
		m_lairDisabledFromScript	= false;
		// Need to reactivate lair because if we called disable(true) before
		// it will be deactivated
		ActivateLair();
	}
}

void CSwarmLairEntity::GenerateSwarmVisualDebug(CRenderFrame* frame, EShowFlags flag)
{
	const Float & updateTime = static_cast<const CSwarmLairParams*>(m_params)->m_updateTime;
	if ( !m_isActivated )
	{
		return;
	}

	EngineTime currentTime = m_time;

	// interpolate boids state
	const TSwarmStatesList& listPrev = m_memberLists[ m_currentStateIndex == 0 ? MEMBER_STATES_LISTS-1 : m_currentStateIndex-1 ];
	const TSwarmStatesList& listCurr = m_memberLists[ m_currentStateIndex ];

	Float frameRatio	= (currentTime - m_lastUpdate) / updateTime;
	frameRatio			= Clamp( frameRatio, 0.f, 1.f );
	for( Int32 i = 0; i < m_spawnLimit; ++i )
	{
		const SSwarmMemberStateData& prevState = listPrev[ i ];
		const SSwarmMemberStateData& currState = listCurr[ i ];
		Vector position	(0.0f, 0.0f, 0.0f);
		EulerAngles orientation(EulerAngles::ZEROS);
		Int32 boidAnimState	= frameRatio > currState.m_boidStateUpdateRatio ? currState.m_boidState : prevState.m_boidState;
		if ( boidAnimState != BOID_STATE_NOT_SPAWNED )
		{
			position = prevState.m_position + (currState.m_position - prevState.m_position) * frameRatio;

			frame->AddDebugLine( prevState.m_position, currState.m_position, Color::RED );
			frame->AddDebugText( position + Vector3(0.0f, 0.0f, 0.4f), ToString( i ), 0, 0 );
			frame->AddDebugText( position + Vector3(0.0f, 0.0f, 0.6f), ToString( frameRatio ), 0, 0 );
			frame->AddDebugLine( position, position + Vector3(0.0f, 0.0f, 1.0f), Color::GREEN );
		}
	}
}

void CSwarmLairEntity::InterpolateBoidStates()
{
	const Float & updateTime = static_cast<const CSwarmLairParams*>(m_params)->m_updateTime;
	const EngineTime& currentTime = m_time;

	// interpolate boids state
	const TSwarmStatesList& listPrev = m_memberLists[ m_currentStateIndex == 0 ? MEMBER_STATES_LISTS-1 : m_currentStateIndex-1 ];
	const TSwarmStatesList& listCurr = m_memberLists[ m_currentStateIndex ];

	Float frameRatio	= (currentTime - m_lastUpdate) / updateTime;
	frameRatio			= Clamp( frameRatio, 0.f, 1.f );
	Uint32 visibleBoids	= 0;
	for( Int32 i = 0; i < m_spawnLimit; ++i )
	{
		const SSwarmMemberStateData& prevState = listPrev[ i ];
		const SSwarmMemberStateData& currState = listCurr[ i ];
		Vector position	(0.0f, 0.0f, 0.0f);
		EulerAngles orientation(EulerAngles::ZEROS);
		Int32 boidAnimState	= frameRatio > currState.m_boidStateUpdateRatio ? currState.m_boidState : prevState.m_boidState;
		if ( boidAnimState == BOID_STATE_NOT_SPAWNED )
		{
			if ( m_boidInstances[ i ] )
			{
				m_boidInstances[ i ]->SetBoidState( BOID_STATE_NOT_SPAWNED, m_loopedAnimPriorityQueue, m_time );
			}
			continue;
		}
		else
		{
			++visibleBoids;

			Float maxControlPointLength = (prevState.m_position - currState.m_position).Mag3() / 4.0f;

			Vector prevControlPoint( 0.0f, maxControlPointLength, 0.0f );
			prevControlPoint = prevState.m_position + prevState.m_orientation.TransformVector( prevControlPoint );

			Vector currControlPoint( 0.0f, maxControlPointLength, 0.0f );
			currControlPoint = currState.m_position - currState.m_orientation.TransformVector( currControlPoint );		// Notice '-' here

			InterpolateWithBezierCurve( &position, frameRatio, prevState.m_position, prevControlPoint, currState.m_position, currControlPoint );

			EulerAngles orientationDiff = currState.m_orientation - prevState.m_orientation;
			orientationDiff.Yaw			= MathUtils::GeometryUtils::ClampDegrees( orientationDiff.Yaw );
			orientationDiff.Pitch		= MathUtils::GeometryUtils::ClampDegrees( orientationDiff.Pitch );
			orientationDiff.Roll		= MathUtils::GeometryUtils::ClampDegrees( orientationDiff.Roll);
			orientation					= prevState.m_orientation + orientationDiff * frameRatio;

			if ( m_boidInstances[ i ] == NULL )
			{
				SpawnInstance( i, position, orientation );
				m_maxActiveBoidIndex = Max( i+1, m_maxActiveBoidIndex );
			}
			else
			{
				m_boidInstances[ i ]->SetBoidState( boidAnimState, m_loopedAnimPriorityQueue, m_time );
			}
		}
		if (m_boidInstances[ i ])
		{
			m_boidInstances[ i ]->OnTick( m_loopedAnimPriorityQueue, m_time, position, orientation );
		}
	}

	// When all boids are despawned and player out of the zone then remove the lair :
	if ( visibleBoids == 0 && m_dynamicPoiCount == 0 )
	{
		DeactivateLair();
	}
}

void CSwarmLairEntity::IssueNewUpdateJob(const EngineTime& currentTime)
{
	const Float & updateTime = static_cast<const CSwarmLairParams*>(m_params)->m_updateTime;

	m_lastUpdate = m_nextUpdateTime;
	m_nextUpdateTime = currentTime + updateTime;
	// start new job
	m_requestOutOfTime = false;
	m_requestShouldTerminate = false;
	if ( ++m_currentStateIndex == MEMBER_STATES_LISTS )
	{
		m_currentStateIndex = 0;
	}
	const_cast< CSwarmAlgorithmData* >( m_swarmAlgorithmData )->PreUpdateSynchronization( updateTime );
	CSwarmUpdateJob* job = NewUpdateJob();
	m_job = job;
	m_isProcessing = true;
	GTaskManager->Issue( *job );
}

////////////////////////////////////////////////////////////////////////////////////////
//							CSwarmLairParams									      //
////////////////////////////////////////////////////////////////////////////////////////
Bool CSwarmLairParams::ParseXmlAttribute(const SCustomNodeAttribute & att)
{
	if ( att.m_attributeName == CNAME( updateTime ) )
	{
		if (att.GetValueAsFloat( m_updateTime ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: updateTime is not defined as a float"));
			return false;
		}
	}
	else if ( att.m_attributeName == CNAME( maxSoundDistance ) )
	{
		if (att.GetValueAsFloat( m_maxSoundDistance ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: maxSoundDistance is not defined as a float"));
			return false;
		}
	}
	else
	{
		return CBoidLairParams::ParseXmlAttribute(att);
	}
	return true;
}

Bool CSwarmLairParams::VirtualCopyTo(CBoidLairParams* const params)const
{
	CSwarmLairParams *const swarmParams = params->As<CSwarmLairParams>();
	if (swarmParams)
	{
		*swarmParams = *this;
		return true;
	}
	return false;
}





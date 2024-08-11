#include "build.h"
#include "flyingCritterslairEntity.h"
#include "flyingCrittersAI.h"
#include "r4BoidSpecies.h"
#include "flyingCritterAlgorithmData.h"
#include "flyingCritterUpdateJob.h"
#include "flyingSwarmGroup.h"
#include "swarmCellMap.h"

#include "../../common/game/swarmAlgorithmData.h"
#include "../../common/game/boidInstance.h"
#include "../../common/game/boidSpecies.h"
#include "../../common/core/feedback.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/gameTimeManager.h"
#include "../../common/engine/renderFrame.h"

///////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CFlyingCrittersLairEntity );

CFlyingCrittersLairEntity::CFlyingCrittersLairEntity()
	: m_breakCounter( 8 )
	, m_cellMapResourceFile()
	, m_cellMapCellSize( 2.0f )
	, m_cellMapSet( false )
{
	m_scriptInput = CreateObject< CFlyingSwarmScriptInput >( this );
}

void CFlyingCrittersLairEntity::OnAttachFinished( CWorld* world )
{
	if ( m_params == NULL && GCommonGame->GetBoidSpecies() )
	{
		const CBoidLairParams*const params = GCommonGame->GetBoidSpecies()->GetParamsByName( m_boidSpeciesName );
		if ( params )
		{
			m_params		= params->As< CFlyingCritterLairParams >();
		}
		if ( m_params == NULL )
		{
			m_params = &CFlyingCritterLairParams::sm_defaultParams;
		}
	}
	CSwarmLairEntity::OnAttachFinished( world );
}

volatile CFlyingCrittersAlgorithmData* CFlyingCrittersLairEntity::GetAlgorithmData() const volatile
{
	return static_cast< volatile CFlyingCrittersAlgorithmData* >( m_swarmAlgorithmData );
}
CSwarmUpdateJob* CFlyingCrittersLairEntity::NewUpdateJob()
{
	return new ( CTask::Root ) CFlyingCrittersUpdateJob( this, m_memberLists[ m_currentStateIndex ], m_memberLists[ m_currentStateIndex+1 == MEMBER_STATES_LISTS ? 0 : m_currentStateIndex+1 ], m_swarmAlgorithmData );
}
CSwarmAlgorithmData* CFlyingCrittersLairEntity::NewAlgorithmData()
{
	ASSERT(m_params->IsValid(), TXT("Boid params m_isValid should be true !"));
	return new CFlyingCrittersAlgorithmData( this,  *static_cast<const CFlyingCritterLairParams *>(m_params), m_scriptInput );
}	
void CFlyingCrittersLairEntity::NoticeFireInCone( const Vector& position, const Vector2& coneDir, Float coneHalfAngle, Float coneRange )
{
	m_fireInConeInfo.m_isPending	= true;
	m_fireInConeInfo.m_origin		= position.AsVector3();
	m_fireInConeInfo.m_dir			= coneDir;
	m_fireInConeInfo.m_halfAngle	= coneHalfAngle;
	m_fireInConeInfo.m_range		= coneRange;
}

void CFlyingCrittersLairEntity::OnTick( Float timeDelta )
{
	// Must be called first because frame ratio is computed there :
	CSwarmLairEntity::OnTick( timeDelta );
}
void CFlyingCrittersLairEntity::SynchroniseData()
{
	CSwarmLairEntity::SynchroniseData();
	CFlyingCrittersAlgorithmData *const algoData = static_cast< CFlyingCrittersAlgorithmData * >(const_cast<CSwarmAlgorithmData *> ( m_swarmAlgorithmData ) );
	Bool isLoaded = true;
	if ( m_cellMapResourceFile.IsLoaded() == false )
	{
		isLoaded = false;
		if ( m_cellMapResourceFile.GetAsync() == BaseSoftHandle::ALR_Loaded )
		{
			isLoaded = true;
		}
	}

	if ( isLoaded && m_cellMapSet == false )
	{
		m_cellMapSet = true;
		algoData->SetCellMap( m_cellMapResourceFile.Get() );
	}
}

void CFlyingCrittersLairEntity::DeactivateLair()
{
	CSwarmLairEntity::DeactivateLair();
	if ( m_cellMapResourceFile.IsLoaded() )
	{
		m_cellMapResourceFile.Release();
	}
	m_cellMapSet = false;
}

void CFlyingCrittersLairEntity::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	IBoidLairEntity::OnGenerateEditorFragments( frame, flag );

	CFlyingCrittersAlgorithmData *const algoData = static_cast<CFlyingCrittersAlgorithmData *>(const_cast<CSwarmAlgorithmData *>( m_swarmAlgorithmData ));

	if ( m_swarmAlgorithmData == nullptr )
	{
		return;
	}
	CSwarmLairEntity::OnGenerateEditorFragments( frame, flag );
		
	const TSwarmStatesList & swarmStatesList = m_memberLists[ m_currentStateIndex ];
	for ( Uint32 i=0; i < (Uint32)m_spawnLimit; ++i )
	{
		const SSwarmMemberStateData & stateData = swarmStatesList[i];
		const CBoidInstance *const boidInstance	= m_boidInstances[ i ];
		if ( boidInstance && boidInstance->GetBoidState() != BOID_STATE_NOT_SPAWNED )
		{
			if ( stateData.m_flags & BOID_STATE_FLAG_INSIDE_CONE_POI )
			{
				frame->AddDebugCircle(  boidInstance->GetPosition(), 0.25f,  boidInstance->GetOrientation().ToMatrix(), Color::RED, 5, true );
			}
		}
	}
		
	algoData->OnGenerateEditorFragments( frame, flag );
}

#ifndef NO_EDITOR
Bool CFlyingCrittersLairEntity::OnGenerateSwarmCollisions()
{
	CEntity *const swarmAreaEntity = m_lairBoundings.Get();
	if ( swarmAreaEntity == nullptr )
	{
		GFeedback->ShowError(TXT("No collisions generated because the lairBoundings property is empty"));
		return false;
	}
	CBoidAreaComponent *const boidAreaComponent = swarmAreaEntity->FindComponent<CBoidAreaComponent>();
	// [Step] First create a resource name if I don't already have one
	CSwarmCellMap * swarmCellMap	= m_cellMapResourceFile.Get();
	String cellMapResourceName;
	if ( swarmCellMap == nullptr )
	{
		Bool resourceNameIsUnique = false;
		CStandardRand randomNumberGenerator(  (Uint32) ( Float ) EngineTime::GetNow() );
		do 
		{
			// Generate the name 
			const Int32 randInt		= randomNumberGenerator.Get<Int32>(); // we need a true random not an always the same after startup random
			cellMapResourceName		= String::Printf( TXT( "%s_%s_%ld"), TXT("SwarmCellMap_"), GetName().AsChar(), randInt ); 
			resourceNameIsUnique	= CSwarmCellMap::FindFileInDepot( cellMapResourceName ) == nullptr;
		}
		// repeat until we find a unique name
		while ( resourceNameIsUnique == false );

		// [Step] Create the resource 
		swarmCellMap	= CSwarmCellMap::Create( );
	}

	// [Step] Generate the cell map
	swarmCellMap->Generate( boidAreaComponent, m_cellMapCellSize );

	// [Step] Serializing the disk file
	if ( cellMapResourceName.Empty() == false ) // the file has just been created we need to SaveAs()
	{
		CDirectory* dir = R4SwarmUtils::GetDataDirectory();
		if ( dir == false )
		{
			GFeedback->ShowError(TXT("No collisions generated because there is a problem with the swarm directory"));
			return false;
		}

		// [Step] Save the resource to disk to create the disk file
		// we will resave later in order serialize the bit field
		if ( swarmCellMap->SaveAs( dir, cellMapResourceName ) == false )
		{
			GFeedback->ShowError(TXT("No collisions generated because the cellMap file could not be saved"));
			return false;
		}
		// [Step] Set the handle
		m_cellMapResourceFile = swarmCellMap;
	}
	else
	{
		if ( swarmCellMap->Save( ) == false )
		{
			GFeedback->ShowError(TXT("No collisions generated because the cellMap file could not be saved"));
			return false;
		}
	}

	return true;
}
#endif // ! NO_EDITOR

#ifndef NO_EDITOR
void CFlyingCrittersLairEntity::EditorPreDeletion()
{
	// [Step] Getting the cell map resource
	CSwarmCellMap *const swarmCellMap = m_cellMapResourceFile.Get();
	if ( swarmCellMap )
	{
		//swarmCellMap->RemoveFromRootSet();
		CDiskFile *const diskFile = swarmCellMap->GetFile();
		diskFile->GetStatus();
		diskFile->Delete( false, false );	
	}
}
#endif
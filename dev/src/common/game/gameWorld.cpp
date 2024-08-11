#include "build.h"
#include "gameWorld.h"


#include "../core/depot.h"
#include "../core/gameSave.h"

#include "../engine/environmentManager.h"
#include "../engine/fxPhysicalForce.h"
#include "../engine/gameTimeManager.h"
#include "../physics/physicsWorld.h"
#include "../engine/physicsCharacterWrapper.h"
#include "../engine/tagManager.h"
#include "../engine/weatherManager.h"
#include "../engine/worldIterators.h"

#include "actorsManager.h"
#include "explorationFinder.h"
#include "expManager.h"
#include "gameplayWindComponent.h"
#include "interestPointComponent.h"
#include "movableRepresentationPhysicalCharacter.h"
#include "spawnPointComponent.h"
#include "wayPointComponent.h"
#include "wayPointsCollectionsSet.h"


#ifndef NO_RUNTIME_WAYPOINT_COOKING
#include "wayPointCookingContext.h"
#endif

#include "../engine/characterControllerManager.h"
#include "../engine/soundSystem.h"
IMPLEMENT_ENGINE_CLASS( CGameWorld );

CGameWorld::CGameWorld()
	: m_expManager( nullptr )
	, m_invisibleAgentsAccum( 0 )
{
}

CGameWorld::~CGameWorld()
{}

void CGameWorld::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// GC only
	if ( file.IsGarbageCollector() )
	{
		for ( StateChangeRequests::iterator it = m_requests.Begin(); it != m_requests.End(); ++it )
		{
			file << it->m_second;
		}
	}
}

void CGameWorld::Init( const WorldInitInfo& initInfo )
{
	m_expManager = new ExpManager();

	CCookedExplorations* cookedExp = m_cookedExplorations.Get();
	if ( cookedExp )
	{
		if ( false == m_expManager->LoadCookedData( cookedExp->m_data ) )
		{
			RED_LOG( ExpManager, TXT("Cooked data in file %s corrupted or empty. Working in non-cooked mode."), cookedExp->GetFriendlyName().AsChar() );
		}
	}

	if ( !initInfo.m_previewWorld && !m_cookedWaypoints.IsValid() )
	{
		CDiskFile* file = GetFile();
		if ( file )
		{
			String cookedDirPath = file->GetDirectory()->GetDepotPath() + TXT("navi_cooked\\");
			String worldName = file->GetFileName().StringBefore( TXT(".w2w"), true );
			// load cooked waypoints data
			String wpFilePath =
				String::Printf( TXT("%s%s.%s")
				, cookedDirPath.AsChar()
				, worldName.AsChar()
				, CWayPointsCollectionsSet::GetFileExtension()
				);
			THandle< CResource > res = GDepot->LoadResource( wpFilePath );
			if ( res.Get() )
			{	
				CWayPointsCollectionsSet* typedRes = Cast< CWayPointsCollectionsSet > ( res.Get() );
				if ( typedRes )
				{
					// we have a cooked explorations file, let's add it to this world
					m_cookedWaypoints = typedRes;
				}
			}
		}
	}

#ifndef NO_RUNTIME_WAYPOINT_COOKING
	m_wayPointCookingContext = new CWayPointCookingContext();
#endif

	TBaseClass::Init( initInfo );
}

void CGameWorld::Shutdown()
{
#ifndef NO_EXPLORATION_FINDER
	SExplorationFinder::GetInstance().Shutdown();
#endif

	delete m_expManager;
	m_expManager = nullptr;

#ifndef NO_RUNTIME_WAYPOINT_COOKING
	delete m_wayPointCookingContext;
	m_wayPointCookingContext = nullptr;
#endif

	TBaseClass::Shutdown();
}

void CGameWorld::OnShutdownAtGameEnd()
{
	ClearChangeRequests();

	if ( m_expManager )
	{
		m_expManager->OnShutdownAtGameEnd();
	}
}

#ifndef NO_RESOURCE_COOKING
void CGameWorld::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

	// check if there is a cooked explorations file prepared in "analyze" step
	CDiskFile* file = GetFile();
	String cookedDirPath = file->GetDirectory()->GetDepotPath() + TXT("navi_cooked\\");
	String worldName = file->GetFileName().StringBefore( TXT(".w2w"), true );

	// load exploration file
	{
		String expFilePath =
			String::Printf( TXT("%s%s.%s")
			, cookedDirPath.AsChar()
			, worldName.AsChar()
			, CCookedExplorations::GetFileExtension()
			);
		THandle< CResource > res = GDepot->LoadResource( expFilePath );
		if ( res.Get() )
		{	
			CCookedExplorations* typedRes = Cast< CCookedExplorations > ( res.Get() );
			if ( typedRes )
			{
				// we have a cooked explorations file, let's add it to this world
				m_cookedExplorations = typedRes;
			}
		}
	}

	// load waypoints file
	{
		String wpFilePath =
			String::Printf( TXT("%s%s.%s")
			, cookedDirPath.AsChar()
			, worldName.AsChar()
			, CWayPointsCollectionsSet::GetFileExtension()
			);
		THandle< CResource > res = GDepot->LoadResource( wpFilePath );
		if ( res.Get() )
		{	
			CWayPointsCollectionsSet* typedRes = Cast< CWayPointsCollectionsSet > ( res.Get() );
			if ( typedRes )
			{
				// we have a cooked explorations file, let's add it to this world
				m_cookedWaypoints = typedRes;
			}
		}
	}
}
#endif


void CGameWorld::AddSpawnPointComponent( CSpawnPointComponent* spawnPoint )
{
	// TEMPSHIT
	// TEMPSHIT

	ASSERT( spawnPoint );
	ASSERT( !m_spawnPoints.Exist( spawnPoint ) );
	m_spawnPoints.PushBackUnique( spawnPoint );
}

void CGameWorld::RemoveSpawnPointComponent( CSpawnPointComponent* spawnPoint )
{
	// TEMPSHIT
	// TEMPSHIT

	ASSERT( spawnPoint );
	ASSERT( m_spawnPoints.Exist( spawnPoint ) );
	m_spawnPoints.Remove( spawnPoint );
}

void CGameWorld::FindSpawnPointComponents( const Box &boundingBox, TDynArray< CSpawnPointComponent* > &spawnPoints )
{
	// TEMPSHIT
	// TEMPSHIT

	// Linear search
	for ( Uint32 i=0; i<m_spawnPoints.Size(); i++ )
	{
		CSpawnPointComponent* sp = m_spawnPoints[i];
		if ( boundingBox.Contains( sp->GetWorldPosition() ) )
		{
			spawnPoints.PushBack( sp );
		}
	}
}

void CGameWorld::AddInterestPoint( CInterestPointComponent *interestPoint )
{
	// PAKSAS TODO: TEMPSHIT
	// PAKSAS TODO: TEMPSHIT

	ASSERT( interestPoint );
	ASSERT( !m_intrestPoints.Exist( interestPoint ) );
	m_intrestPoints.PushBackUnique( interestPoint );
}

void CGameWorld::RemoveInterestPoint( CInterestPointComponent *interestPoint )
{
	// PAKSAS TODO: TEMPSHIT
	// PAKSAS TODO: TEMPSHIT

	ASSERT( interestPoint );
	ASSERT( m_intrestPoints.Exist( interestPoint ) );
	m_intrestPoints.Remove( interestPoint );
}

void CGameWorld::FindInterestPoint( const Box &boundingBox, TDynArray< CInterestPointComponent* > &interestPoints )
{
	// PAKSAS TODO: TEMPSHIT
	// PAKSAS TODO: TEMPSHIT

	// Linear search
	for ( Uint32 i=0; i<m_intrestPoints.Size(); i++ )
	{
		CInterestPointComponent* sp = m_intrestPoints[i];
		if ( boundingBox.Contains( sp->GetWorldPosition() ) )
		{
			interestPoints.PushBack( sp );
		}
	}
}

void CGameWorld::RegisterStateChangeRequest( CName tag, IEntityStateChangeRequest* request )
{
	ASSERT( !tag.Empty() );
	if ( tag.Empty() )
	{
		request->Discard();
		return;
	}

	THashMap< CName, IEntityStateChangeRequest* >::iterator it = m_requests.Find( tag );

	if ( it != m_requests.End() )
	{
		// remove the old request, if there was any
		if ( it->m_second )
		{
			it->m_second->Discard();
			it->m_second = NULL;
		}
	}

	if ( !request )
	{
		// if we didn't specify a new request - remove the entry
		m_requests.Erase( tag );
	}
	else
	{
		// we specified a new request...
		if ( it == m_requests.End() )
		{
			// ...add it, cause it's completely new
			m_requests.Insert( tag, request );
		}
		else
		{
			// ...replace the old one for an existing tag
			it->m_second = request;
		}
		request->SetParent( this );
	}

	// update the state of the entities already registered
	if ( request )
	{
		TDynArray< CEntity* > entities;
		GetTagManager()->CollectTaggedEntities( tag, entities );
		for ( TDynArray< CEntity* >::iterator it = entities.Begin(); it != entities.End(); ++it )
		{
			CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( *it );
			if ( gameplayEntity )
			{
				request->Execute( gameplayEntity );
			}
		}
	}
}

void CGameWorld::ResetStateChangeRequest( CName tag )
{
	ASSERT( !tag.Empty() );
	if ( tag.Empty() )
	{
		return;
	}

	THashMap< CName, IEntityStateChangeRequest* >::iterator it = m_requests.Find( tag );
	if ( it != m_requests.End() )
	{
		// remove the old request, if there was any
		if ( it->m_second )
		{
			it->m_second->Discard();
			it->m_second = NULL;
		}

		m_requests.Erase( it );
	}
}

void CGameWorld::UpdateEntityState( CGameplayEntity* entity )
{
	// update the state of the entity
	const TDynArray< CName >& tags = entity->GetTags().GetTags();
	for ( TDynArray< CName >::const_iterator it = tags.Begin(); it != tags.End(); ++it )
	{
		StateChangeRequests::iterator scReqIt = m_requests.Find( *it );
		if ( scReqIt != m_requests.End() && scReqIt->m_second )
		{
			scReqIt->m_second->Execute( entity );
		}
	}
}

void CGameWorld::ClearChangeRequests()
{
	for ( StateChangeRequests::iterator it = m_requests.Begin(); it != m_requests.End(); ++it )
	{
		if ( it->m_second )
		{
			it->m_second->Discard();
		}
	}
	m_requests.Clear();
}

void CGameWorld::OnStateChangeRequestsDebugPage( TDynArray< StateChangeRequestsDesc >& outDescriptions ) const
{
	for ( StateChangeRequests::const_iterator it = m_requests.Begin(); it != m_requests.End(); ++it )
	{
		if ( it->m_second )
		{
			outDescriptions.PushBack( StateChangeRequestsDesc( it->m_first.AsString() ) );
			outDescriptions.Back().m_requestDetails = it->m_second->OnStateChangeRequestsDebugPage();
		}
	}
}

void CGameWorld::OnTerrainCollisionDataBoundingUpdated( const Box& /*bbox*/ )
{
}

Vector CGameWorld::GetWindAtPointForVisuals( const Vector& point, Bool withTurbulence, Bool withForcefield ) const
{
	PC_SCOPE(GetWindAtPointForVisuals)

	CEnvironmentManager* envMgr = GetEnvironmentManager();

	Vector wind = Vector(1.0f, 0.0f, 0.0f, 0.0f);
	if( envMgr )
	{
		wind = envMgr->GetCurrentWindVector( point );
	}
	
	if( withForcefield )
	{
		for( Uint32 i = 0; i != CForceFieldEntity::m_elements.Size(); ++i )
		{
			CForceFieldEntity* force = CForceFieldEntity::m_elements[ i ].Get();
			if( force )
			{
				CFXExplosionImplosionPhysicalForce* data = Cast< CFXExplosionImplosionPhysicalForce >( force->m_data );
				if( data ) wind += data->Process( force, point, data->GetRadius() );
			}
		}
	}


	if ( withTurbulence )
	{
		Float mul = wind.Mag3()*0.1f;

		Float time = ( Float ) EngineTime::GetNow();

		Float frequency = 0.1f;
		Float amplitude = 1.0f;

		//high frequency sine can be treated like a noise
		Float Noisex = sinf( point.A[0]*100.0f - (time*point.A[1]*frequency) ) * mul;
		Float Noisey = sinf( point.A[1]*100.0f - (time*point.A[0]*frequency) ) * mul;
		Float Noisez = sinf( point.A[2]*100.0f - (time*point.A[0]*frequency) ) * mul;

		Vector delta(Noisex, Noisey, Noisez);
		delta *= amplitude;
		wind += delta;	
	}

	return wind;
}

Vector CGameWorld::GetWindAtPoint( const Vector& point ) const
{
	Vector result( GetGlobalWind() );
	CGameplayStorage *storage = GCommonGame->GetGameplayStorage();

	if ( true ) // for now local winds are switched off, should be removed in the future
	{
		return result;
	}

	// gather the wind emitters
	const Uint32 maxWindEmitters = INT_MAX;											   // FIXME: some reasonable number, perhaps?
	const Box aabb( Vector::ZEROS, 2.f * CGameplayWindComponent::WIND_RANGE_MAX );
	TDynArray< TPointerWrapper< CGameplayEntity > > results;
	STATIC_NODE_FILTER( HasWind, filterWind );
	static const INodeFilter* filters[] = { &filterWind };
	storage->GetClosestToPoint( point, results, aabb, maxWindEmitters, filters, 1 );

	// Sum up the wind vectors
	for ( Uint32 i = 0; i < results.Size(); ++i )
	{
		CGameplayEntity *ent = results[ i ].Get();
		const TDynArray< CComponent* >& components = ent->GetComponents();
		for ( Uint32 k = 0; k < components.Size(); ++k )
		{
			if ( components[ k ]->IsA< CGameplayWindComponent > () )
			{
				result += static_cast< CGameplayWindComponent* > ( components[ k ] )->GetWindAtPoint( point );
			}
		}
	}

	// return the sum
	return result;
}

void CGameWorld::GetWindAtPoint( Uint32 elementsCount, void* inputPos, void* outputPos, size_t stride ) const
{
	if( CEnvironmentManager* envMgr = GetEnvironmentManager() )
	{
		if( CWeatherManager* weatherManager = envMgr->GetWeatherManager() )
		{
			weatherManager->GetCurrentWindVector( elementsCount, inputPos, outputPos, stride );
		}
		
	}

	// really?? this is shit slow
	for( Uint32 i = 0; i != CForceFieldEntity::m_elements.Size(); ++i )
	{
		CForceFieldEntity* force = CForceFieldEntity::m_elements[ i ].Get();
		if( !force ) continue;

		CFXExplosionImplosionPhysicalForce* data = Cast< CFXExplosionImplosionPhysicalForce >( force->m_data );
		if( !data ) continue;

		for( Uint32 j = 0; j != elementsCount; ++j )
		{
			float* X = ( float* ) ( ( char* ) inputPos + stride * j );

			float* output = ( float* ) ( ( char* )outputPos + stride * j );

			Vector wind = data->Process( force, Vector( *X, *( X + 1 ), 0.0f ), data->GetRadius() );
			*output = wind.X;
			*( output + 1 ) = wind.Y;

		}
	}

}

Vector CGameWorld::GetGlobalWind() const
{
	CEnvironmentManager* envMgr = GetEnvironmentManager();
	
	if(envMgr)
	{
		return envMgr->GetCurrentWindVector();
	}
	return Vector::EY;
}

//////////////////////////////////////////////////////////////////////////
//
// finalize movement
void CGameWorld::FinalizeMovement( Float timeDelta )
{
	PC_SCOPE_PIX( FinalizeMovement )

	// finalize movement only if something has actually ticked
	if ( timeDelta == 0.0f )
	{
		return;
	}

#ifdef USE_PHYSX
	// init
	CCharacterControllersManager* mgr = GetCharacterControllerManager().Get();
	if ( !mgr )  return;
	
	// collect agents
	{
		m_agentsThatNeedSeparation.ClearFast();
		m_agentsThatDontNeedSeparation.ClearFast();
		m_agentsThatAreInvisible.ClearFast();

		for ( Uint32 i = 0; i < mgr->GetControllersCount(); ++i )
		{
			if ( CPhysicsCharacterWrapper* wrapper = mgr->GetController( i ) )
			{
				CMovingPhysicalAgentComponent* mac = nullptr;
				if ( wrapper->GetParent( mac ) )
				{
					if ( mac->GetActiveRepresentationName() == RED_NAME( CMREntity ) )
					{ // unpushable stuff - no separation
						m_agentsThatDontNeedSeparation.PushBack( mac );
					}
					else
					{
						CActor* actor = Cast< CActor >( mac->GetEntity() );
						if ( actor && actor->GetLODState().m_timeInvisible > 0.f )
						{
							m_agentsThatAreInvisible.PushBack( mac );
						}
						else
						{
							m_agentsThatNeedSeparation.PushBack( mac );
						}
					}
				}
			}
		}
	}

	// process agents that don't need separation
	{
		PC_SCOPE_PIX( FM_DontNeedSeparation )
		for ( CMovingPhysicalAgentComponent* mac : m_agentsThatDontNeedSeparation )
		{
			//PC_SCOPE( FinalizeMovement_EntityRepresentations );
			mac->FinalizeMovement1_PreSeparation( timeDelta );
			mac->FinalizeMovement2_PostSeparation( timeDelta );
		}
	}

	// process agents that are invisible
	const Float invisibleAgentsPeriod = 1.f / 10.f; // invisible npcs ticked at 10 fps
	m_invisibleAgentsAccum += timeDelta;

	if ( m_invisibleAgentsAccum >= invisibleAgentsPeriod )
	{
		PC_SCOPE_PIX( FM_Invisible )
		for ( CMovingPhysicalAgentComponent* mac : m_agentsThatAreInvisible )
		{
			//PC_SCOPE( FinalizeMovement_EntityRepresentations );
			mac->FinalizeMovement1_PreSeparation( m_invisibleAgentsAccum );
			mac->FinalizeMovement2_PostSeparation( m_invisibleAgentsAccum );
		}

		m_invisibleAgentsAccum = 0.f;
	}

	// 1) pre-separation stage
	{
		PC_SCOPE_PIX( FM_NeedSeparation )
		for ( CMovingPhysicalAgentComponent* mac : m_agentsThatNeedSeparation )
		{
			//PC_SCOPE( FinalizeMovement_PreSeparation );

			// compute movement vector
			mac->FinalizeMovement1_PreSeparation( timeDelta );

			// reset collision data
			mac->ResetCollisionObstaclesData();
			mac->ResetCollisionCharactersData();
		}
	}

    // 2) resolve separation stage
	{
		PC_SCOPE_PIX( FM_ResolveSeparations );

		mgr->ResetIntersectionsCountFromLastFrame();
		mgr->ResetCollisionsCountFromLastFrame();
		m_resolveSeparationContext.Clear();

		for ( CMovingPhysicalAgentComponent* mac : m_agentsThatNeedSeparation )
		{
			//PC_SCOPE( FinalizeMovement_ResolveSeparation );
			Uint32 colliders = mac->ResolveSeparation( m_resolveSeparationContext );
			mgr->AddIntersectionsCountFromLastFrame( colliders );
		}
	}

	// 3) post-separation stage
	{
		PC_SCOPE_PIX( FM_PostSeparations );
		for ( CMovingPhysicalAgentComponent* mac : m_agentsThatNeedSeparation )
		{
			//PC_SCOPE( FinalizeMovement_PostSeparation );
			mac->FinalizeMovement2_PostSeparation( timeDelta );
		}
	}
    
#endif
	// update of local to world in FinalizeMovement may miss situation in which other object is moved - 
	// that object won't have local to world updated
	// for now, I'd suggest update of local to world to be done here to cover situation:
	//	1. object A is moved before B and B before C
	//	2. C has pushed B
	//	3. A is looking at B
	// in such case, object A will receive old location of bone B
	// in most situations this doubles calculations and we could create context to handle this
	// but even then this won't cover cases in which IK modifies characters to greater extent
	// for such cases I'd consider lookats to be done in separate passes and have two passes for constraint graphs
}

// ------------------------------------------------------------------------
// Save game
// ------------------------------------------------------------------------
void CGameWorld::SaveState( IGameSaver* saver )
{
	CGameSaverBlock block0( saver, CNAME(entityStateChangeRequests) );

	Uint32 requestsCount = m_requests.Size();
	saver->WriteValue( CNAME( requestsCount ), requestsCount );

	for ( StateChangeRequests::const_iterator it = m_requests.Begin(); it != m_requests.End(); ++it )
	{
		CGameSaverBlock block1( saver, CNAME(request) );

		// find request's tag
		ASSERT( it->m_second );

		saver->WriteValue( CNAME( tag ), it->m_first );
		it->m_second->SaveState( saver );
	}
}

void CGameWorld::RestoreState( IGameLoader* loader )
{
	CGameSaverBlock block0( loader, CNAME(entityStateChangeRequests) );

	Uint32 requestsCount = loader->ReadValue( CNAME( requestsCount ), (Uint32)0 );
	for ( Uint32 i = 0; i < requestsCount; ++i )
	{
		CGameSaverBlock block1( loader, CNAME(request) );

		CName tag = loader->ReadValue( CNAME( tag ), CName::NONE );
		ASSERT( tag != CName::NONE );

		IEntityStateChangeRequest* request = IEntityStateChangeRequest::RestoreState( loader, this );
		ASSERT( request != NULL );

		RegisterStateChangeRequest( tag, request );
	}
}

//////////////////////////////////////////////////////////////////////////

#ifndef NO_DEBUG_PAGES

Uint32 CGameWorld::GetSpawnPointComponentNum() const
{
	return m_spawnPoints.Size();
}

Uint32 CGameWorld::GetInterestPointComponentNum() const
{
	return m_intrestPoints.Size();
}

#endif

void CGameWorld::RefreshAllWaypoints( const Box& bbox )
{
#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
	for ( WorldAttachedComponentsIterator it( this ); it; ++it )
	{
		CWayPointComponent * waypoint = Cast< CWayPointComponent >( *it );
		if ( waypoint )
		{
			if ( bbox.Contains( waypoint->GetWorldPosition() ) )
			{
				waypoint->CheckLocation( this, false );
			}
		}
	}
#endif
}

#ifndef NO_EDITOR_FRAGMENTS
void CGameWorld::GenerateEditorFragments( CRenderFrame* frame )
{
	TBaseClass::GenerateEditorFragments( frame );

#ifndef NO_EXPLORATION_FINDER
	SExplorationFinder::GetInstance().GenerateEditorFragments( frame );
#endif

	GSoundSystem->GenerateEditorFragments(frame);
}
#endif

void CGameWorld::OnLayerAttached( CLayer* layer )
{
	TBaseClass::OnLayerAttached( layer );

	if ( m_expManager )
	{
		m_expManager->OnLayerAttached( layer );
	}
}

void CGameWorld::OnLayerDetached( CLayer* layer )
{
	TBaseClass::OnLayerDetached( layer );

	if ( m_expManager )
	{
		m_expManager->OnLayerDetached( layer );
	}
}

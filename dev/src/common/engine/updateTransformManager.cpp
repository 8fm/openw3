/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "updateTransformManager.h"
#include "hardAttachment.h"
#include "game.h"
#include "node.h"
#include "component.h"
#include "entity.h"
#include "baseEngine.h"
#include "game.h"
#include "renderCommands.h"

CUpdateTransformManager::CUpdateTransformManager()
	: m_isUpdatingTransforms( false )
{
}

CUpdateTransformManager::~CUpdateTransformManager()
{
}

void CUpdateTransformManager::ScheduleEntity( CEntity* entity )
{
	RED_ASSERT( entity );
	RED_ASSERT( entity->HasFlag( NF_MarkUpdateTransform ) );
	RED_ASSERT( entity->GetTransformParent() == nullptr );

	{
		Red::Threads::CScopedLock< Red::Threads::CLightMutex > scheduleLock( m_lock );

		m_updateSetEntities.Insert( entity );
	}
}

void CUpdateTransformManager::UnscheduleEntity( CEntity* entity )
{
	RED_ASSERT( entity );
	RED_ASSERT( !entity->HasFlag( NF_ScheduledUpdateTransform ) );
	RED_ASSERT( !entity->HasFlag( NF_MarkUpdateTransform ) );

	{
		Red::Threads::CScopedLock< Red::Threads::CLightMutex > scheduleLock( m_lock );

		m_updateSetEntities.Erase( entity );
	}
}

Bool CUpdateTransformManager::HasEntityScheduled( CEntity* entity ) const
{
	return m_updateSetEntities.Exist( entity );
}

void CUpdateTransformManager::UpdateOneEntity( CEntity*& entity )
{
	const Bool shouldBeProcessed = entity->GetTransformParent() == nullptr; // Entity state can be changed during tick
	if ( shouldBeProcessed )
	{
		RED_ASSERT( entity->HasFlag( NF_MarkUpdateTransform ) );

		SUpdateTransformContext context;
		context.m_cameraVisibilityData = m_cameraVisibilityData;

		entity->UpdateTransformEntity( context, false );

		context.CommitChanges();

		RED_ASSERT( !entity->HasFlag( NF_MarkUpdateTransform ) );
		RED_ASSERT( !entity->HasFlag( NF_ScheduledUpdateTransform ) );
	}
}

void CUpdateTransformManager::DispatchSingleThreaded()
{
	PC_SCOPE_PIX( ProcessSingleThreaded );

	for ( CEntity* entity : m_updateListEntities )
	{
		UpdateOneEntity( entity );
	}
}

void CUpdateTransformManager::ClearSchedule()
{
	{
		Red::Threads::CScopedLock< Red::Threads::CLightMutex > scheduleLock( m_lock );

		m_updateSetEntities.ClearFast();
	}
}

void CUpdateTransformManager::PrepareList()
{
	RED_FATAL_ASSERT( SIsMainThread(), "CUpdateTransformManager::PrepareList can only be called from main thread, because of accessing entities." );
	PC_SCOPE_PIX( PrepareList );

	m_updateListEntities.ClearFast();

	{
		Red::Threads::CScopedLock< Red::Threads::CLightMutex > scheduleLock( m_lock );

		m_updateListEntities.Reserve( m_updateSetEntities.Size() + 1 );

		for ( CEntity* entity : m_updateSetEntities )
		{
			if ( !entity->IsDestroyed() ) // todo - remove this hack
			{
				entity->MarkAsDuringUpdateTransform();
				m_updateListEntities.PushBack( entity );
			}
		}

		//m_updateSetPlayer = nullptr;
		m_updateSetEntities.ClearFast();

		// TODO - do we need this?
		m_updateSetEntities.Rehash( 4096 ); // This is to downsize the update set after initial update transforms done at level startup
	}
}

#ifdef DEBUG_TRANS_MGR
#pragma optimize("",off)
#endif

void CUpdateTransformManager::ProcessList( Bool allowThreads, Bool blockMainThread, UpdateTransformContext& context )
{
	PC_SCOPE_PIX( ProcessList );

#ifdef DEBUG_TRANS_MGR
	static Bool CAN_USE_THREADS = false;
#else
	static Bool CAN_USE_THREADS = true;
#endif
	allowThreads = allowThreads && CAN_USE_THREADS;

	const Uint32 listSize = m_updateListEntities.Size();
	if ( listSize != 0 )
	{
		// Process as immediate jobs only if list is large enough
		if ( allowThreads == true && listSize >= BUCKET_SIZE_THREADING_THRESHOLD )
		{
			auto params = CParallelForTaskSingleArray< CEntity*, CUpdateTransformManager >::SParams::Create();
			{
				params->m_array						= m_updateListEntities.TypedData();
				params->m_numElements				= listSize;
				params->m_processFunc				= &CUpdateTransformManager::UpdateOneEntity;
				params->m_processFuncOwner			= this;
				params->m_priority					= TSP_VeryHigh;
				params->SetDebugName				( TXT("UpdateTransform") );
			}
			if ( blockMainThread )
			{
				params->ProcessNow();
				params->Release();
			}
			else
			{
				params->StartProcessing();
				context.m_entitiesTask = params;
			}
		}
		else
		{
			DispatchSingleThreaded();
		}
	}
}

void CUpdateTransformManager::CollectDebugData()
{
#ifdef DEBUG_TRANS_MGR

	THashMap< const CClass*, Int32 > counterEntities;
	THashMap< const CClass*, Int32 > counterEntitiesComponents;

	for ( const CEntity* e : m_updateSetEntities )
	{
		const CClass* c = e->GetClass();

		++(counterEntities.GetRef( c, 0 ));

		for ( BaseComponentIterator it( e ); it; ++it )
		{
			CComponent* cc = *it;
			const CClass* ccclass = cc->GetClass();
			if ( cc->HasScheduledUpdateTransform() )
			{
				++(counterEntitiesComponents.GetRef( ccclass, 0 ));
			}
		}
	}

	TDynArray< InternalData > arrEntities_A;
	TDynArray< InternalData > arrEntities_B;

	for ( auto it = counterEntities.Begin(); it != counterEntities.End(); ++it )
	{
		new ( arrEntities_A ) InternalData( it.Key(),  it.Value() );
	}
	for ( auto it = counterEntitiesComponents.Begin(); it != counterEntitiesComponents.End(); ++it )
	{
		new ( arrEntities_B ) InternalData( it.Key(),  it.Value() );
	}
#endif
}

void CUpdateTransformManager::UpdateTransforms_Start( UpdateTransformContext& context )
{
	GGame->OnUpdateTransformBegin();

	m_isUpdatingTransforms = true;

#ifdef DEBUG_TRANS_MGR
	const Uint32 _numEntities = m_updateSetEntities.Size();
	static Bool DO_THIS = false;
	if ( DO_THIS )
	{
		CollectDebugData();
	}
#endif

	PrepareList();

	m_cameraVisibilityData = context.m_cameraVisibilityData.IsValid() ? &context.m_cameraVisibilityData : nullptr; // Set context data

	const Bool allowThreads = context.m_canUseThreads && (context.m_statCollector == NULL);
	ProcessList( allowThreads, false, context );
}

void CUpdateTransformManager::UpdateTransforms_WaitForFinish( UpdateTransformContext& context )
{
	RED_FATAL_ASSERT( SIsMainThread(), "UpdateTransforms_WaitForFinish can only be called from main thread, because of accessing entities." );
	WaitForFinish( context );

	if ( m_cameraVisibilityData )
	{
		RED_FATAL_ASSERT( context.m_cameraVisibilityData.IsValid(), "" );
	}

	m_cameraVisibilityData = nullptr; // reset context data

	m_isUpdatingTransforms = false;

	for( CEntity* entity : m_updateListEntities )
	{
		entity->ProcessPendingDestroyRequest();
	}

	GGame->OnUpdateTransformFinished( true );
}

void CUpdateTransformManager::WaitForFinish( UpdateTransformContext& context )
{
	if ( context.m_entitiesTask )
	{
		context.m_entitiesTask->FinalizeProcessing();
		context.m_entitiesTask->Release();
		context.m_entitiesTask = nullptr;
	}
}

void CUpdateTransformManager::UpdateTransforms_StartAndWait( UpdateTransformContext& context )
{
	GGame->OnUpdateTransformBegin();

	m_isUpdatingTransforms = true;

#ifdef DEBUG_TRANS_MGR
	const Uint32 _numEntities = m_updateSetEntities.Size();
	static Uint32 thr = 100;
	if ( _numEntities > thr )
	{
		m_numSimple.SetValue( 0 );
	}

	m_numSimple.SetValue( 0 );
	m_numFast.SetValue( 0 );
	m_numRest.SetValue( 0 );
	m_numEntities.SetValue( 0 );
	m_numNodes.SetValue( 0 );

	static Bool DO_THIS = false;
	if ( DO_THIS )
	{
		CollectDebugData();
	}
#endif

	PrepareList();

	m_cameraVisibilityData = context.m_cameraVisibilityData.IsValid() ? &context.m_cameraVisibilityData : nullptr; // Set context data

	const Bool allowThreads = context.m_canUseThreads && (context.m_statCollector == NULL);
	ProcessList( allowThreads, true, context );

#ifdef DEBUG_TRANS_MGR
	const Int32 numSimple = m_numSimple.GetValue();
	const Int32 numFast = m_numFast.GetValue();
	const Int32 numRest = m_numRest.GetValue();
	const Int32 numEntities = m_numEntities.GetValue();
	const Int32 numNodes = m_numNodes.GetValue();
#endif

	m_cameraVisibilityData = nullptr; // reset context data

	m_isUpdatingTransforms = false;

	for( CEntity* entity : m_updateListEntities )
	{
		entity->ProcessPendingDestroyRequest();
	}

	GGame->OnUpdateTransformFinished( false );
}

#ifdef DEBUG_TRANS_MGR
#pragma optimize("",on)
#endif

//////////////////////////////////////////////////////////////////////////

void SMeshSkinningUpdateContext::AddCommand_Relink( IRenderProxy* renderProxy, RenderProxyUpdateInfo updateInfo )
{
	if ( m_relinkCommandCount == UPDATE_CONTEXT_BATCH_SIZE )
	{
		CommitGatheredRelinkCommands();
	}

	RED_FATAL_ASSERT( m_relinkCommandCount < UPDATE_CONTEXT_BATCH_SIZE, "Too many relink commands" );
	RED_FATAL_ASSERT( updateInfo.m_localToWorld != nullptr, "No transform to update" );

	renderProxy->AddRef();

	m_proxies[ m_relinkCommandCount ] = renderProxy;
	m_skinningData[m_relinkCommandCount] = nullptr;

	if ( updateInfo.m_boundingBox != nullptr )
	{
		m_relinkInfos[ m_relinkCommandCount ].m_hasBBox = true;
		m_relinkInfos[ m_relinkCommandCount ].m_bbox = *updateInfo.m_boundingBox;
	}
	else
	{
		m_relinkInfos[ m_relinkCommandCount ].m_hasBBox = false;
	}
	
	m_relinkInfos[ m_relinkCommandCount ].m_transform = *updateInfo.m_localToWorld;

	m_relinkCommandCount++;
}

void SMeshSkinningUpdateContext::AddCommand_SkinningDataAndRelink( IRenderProxy* renderProxy, IRenderSkinningData* renderSkinningData, RenderProxyUpdateInfo updateInfo )
{
	if ( m_relinkCommandCount == UPDATE_CONTEXT_BATCH_SIZE )
	{
		CommitGatheredRelinkCommands();
	}

	RED_FATAL_ASSERT( m_relinkCommandCount < UPDATE_CONTEXT_BATCH_SIZE, "Too many skinning and relink commands" );

	renderProxy->AddRef();
	renderSkinningData->AddRef();

	m_proxies[ m_relinkCommandCount ] = renderProxy;
	m_skinningData[ m_relinkCommandCount ] = renderSkinningData;
	if ( updateInfo.m_boundingBox != nullptr )
	{
		m_relinkInfos[ m_relinkCommandCount ].m_hasBBox = true;
		m_relinkInfos[ m_relinkCommandCount ].m_bbox = *updateInfo.m_boundingBox;
	}
	else
	{
		m_relinkInfos[ m_relinkCommandCount ].m_hasBBox = false;
	}

	m_relinkInfos[ m_relinkCommandCount ].m_transform = *updateInfo.m_localToWorld;

	m_relinkCommandCount++;
}

void SMeshSkinningUpdateContext::AddCommand_ParticlesRelink(IRenderProxy* renderProxy, const RenderProxyUpdateInfo& updateInfo, const Vector3& windAtPosVector, const Vector3& windOnlyAtPosVector )
{
	if ( m_simulationContextUpdateCommandCount == UPDATE_CONTEXT_BATCH_SIZE )
	{
		CommitGatheredUpdateSimulationContextCommands();
	}

	RED_FATAL_ASSERT( m_simulationContextUpdateCommandCount < UPDATE_CONTEXT_BATCH_SIZE, "Too many batched commands" );

	renderProxy->AddRef();

	SUpdateParticlesBatchedCommand& updateParticleCmd = m_particleUpdateInfos[ m_simulationContextUpdateCommandCount++ ];
	updateParticleCmd.m_localToWorld = *updateInfo.m_localToWorld;
	updateParticleCmd.m_renderProxy = renderProxy;
	updateParticleCmd.m_simulationContext.m_windVector = windAtPosVector;
	updateParticleCmd.m_simulationContext.m_windVectorOnly = windOnlyAtPosVector;
	updateParticleCmd.m_relink = true;
}

void SMeshSkinningUpdateContext::AddCommand_UpdateParticlesSimulatationContext( IRenderProxy* renderProxy, const SSimulationContextUpdate& simulationContext )
{
	if ( m_simulationContextUpdateCommandCount == UPDATE_CONTEXT_BATCH_SIZE )
	{
		CommitGatheredUpdateSimulationContextCommands();
	}

	RED_FATAL_ASSERT( m_simulationContextUpdateCommandCount < UPDATE_CONTEXT_BATCH_SIZE, "Too many batched commands" );

	renderProxy->AddRef();

	SUpdateParticlesBatchedCommand& updateParticleCmd = m_particleUpdateInfos[ m_simulationContextUpdateCommandCount++ ];
	updateParticleCmd.m_renderProxy = renderProxy;
	updateParticleCmd.m_simulationContext = simulationContext;
	updateParticleCmd.m_relink = false;
}

void SMeshSkinningUpdateContext::AddCommand_UpdateLightProxyParameter( IRenderProxy* renderProxy, const Vector& data, ERenderLightParameter param )
{
	( new CRenderCommand_UpdateLightProxyParameter( renderProxy, data, RLP_Radius ) )->Commit();
}

void SMeshSkinningUpdateContext::CommitGatheredUpdateSimulationContextCommands()
{
	if (m_simulationContextUpdateCommandCount > 0)
	{
		( new CRenderCommand_BatchUpdateParticlesSimulatationContextAndRelink( m_simulationContextUpdateCommandCount, m_particleUpdateInfos ) )->Commit();
		m_simulationContextUpdateCommandCount = 0;
	}
}

void SMeshSkinningUpdateContext::CommitGatheredRelinkCommands()
{
	if (m_relinkCommandCount > 0)
	{
		( new CRenderCommand_BatchSkinAndRelinkProxies( m_relinkCommandCount, m_proxies, m_skinningData, m_relinkInfos ) )->Commit();
		m_relinkCommandCount = 0;
	}
}

void SMeshSkinningUpdateContext::CommitCommands()
{
	CommitGatheredUpdateSimulationContextCommands();
	CommitGatheredRelinkCommands();
}

//////////////////////////////////////////////////////////////////////////

SUpdateTransformContext::SUpdateTransformContext() 
	: m_guardFlag( false )
	, m_cameraVisibilityData( nullptr )
{
}

SUpdateTransformContext::~SUpdateTransformContext()
{
	RED_FATAL_ASSERT( m_guardFlag, "SUpdateTransformContext critical error" );
}

void SUpdateTransformContext::CommitChanges()
{
	m_skinningContext.CommitCommands();

	m_guardFlag = true;
}

//////////////////////////////////////////////////////////////////////////

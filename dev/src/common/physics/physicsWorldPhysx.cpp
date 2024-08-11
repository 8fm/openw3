#include "build.h"
#include "physicsWorldPhysxImplBatchTrace.h"
#include "physicsContactListener.h"
#ifdef USE_PHYSX
#include "physXEngine.h"
#include "physicsWorldPhysXImpl.h"
#include "../physics/physXCpuDispatcher.h"
#include "../physics/physicsSettings.h"
#include "../core/taskManager.h"
#include "physicsWrapper.h"

using namespace physx;

#ifdef USE_APEX
#include "NxApexSDK.h"
#include "NxApexScene.h"
#include "NxParamUtils.h"
#include "NxModuleDestructible.h"
#endif

#include "PhysicsWrappersDefinition.h"

namespace Config
{
	TConfigVar< Bool >	cvShowStreamingCollisionBoxes( "ShowFlags/Debug/Streaming", "StreamingCollisionBoxes", false );
}

class CPhysxActorsAllocateReleaseTask : public CTask
{
protected:
	Red::Threads::CAtomic< Uint32 >* m_backgroundTasksProcessing;
	TDynArray< void* > m_toRelease;

	void Run() override
	{
		PC_SCOPE_PIX( CPhysxActorsAllocateReleaseTask )

//		if( m_toRelease.Size() ) RED_LOG( RED_LOG_CHANNEL( Physics ), TXT(" releasing %i actors" ), m_toRelease.Size() );
		while( m_toRelease.Size() )
		{
			physx::PxShape* actor = ( physx::PxShape* )m_toRelease.PopBackFast();
			actor->release();
		}
		m_toRelease.ClearFast();
	}

public:
	CPhysxActorsAllocateReleaseTask( Red::Threads::CAtomic< Uint32 >* backgroundTasksProcessing ) : m_backgroundTasksProcessing( backgroundTasksProcessing )
	{
		m_toRelease.Reserve( 16 );
		m_backgroundTasksProcessing->Increment();
	}

	~CPhysxActorsAllocateReleaseTask()
	{
		m_backgroundTasksProcessing->Decrement();
	}

	TDynArray< void* >* GetToReleaseArray() { return &m_toRelease; }

public:
#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override { return TXT( "CPhysxActorsAllocateReleaseTask" ); }
	virtual Uint32		GetDebugColor() const override { return Color::CYAN.ToUint32(); }
#endif

};

class CAsyncFetchTask : public CTask
{
	friend class CPhysicsWorldPhysXImpl;
protected:
	physx::PxScene*	m_scene;
	Bool m_resultFromFetch;

	struct SOnContactData
	{
		PxContactPairHeader m_pairHeader;
		PxContactPair* m_pairs;
		PxU32 m_nbPairs;
	};
	TDynArray< SOnContactData > m_onContactData;

	struct SOnTriggerData
	{
		PxTriggerPair* m_pairs;
		PxU32 m_count;
	};
	TDynArray< SOnTriggerData > m_onTriggerData;

	void Run() override
	{
		PC_SCOPE_PIX( CPhysx_CAsyncFetchTask )

		if( m_scene->userData )
		{
#ifdef USE_APEX
			NxApexScene* apexScene = ( ( NxApexScene* )m_scene->userData );
			m_resultFromFetch = apexScene->fetchResults(true, 0 );
#endif
		}
		else
		{
			m_resultFromFetch = m_scene->fetchResults( true );
		}
	}

	void Push( const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs )
	{
		m_onContactData.ResizeFast( m_onContactData.Size() + 1 );
		SOnContactData& data = m_onContactData.Back();
		Red::MemoryCopy( &data.m_pairHeader, &pairHeader, sizeof( PxContactPairHeader ) );
		data.m_nbPairs = nbPairs;

		data.m_pairs = ( PxContactPair* ) RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_PhysicsEngine, sizeof( PxContactPair ) * nbPairs );
		for( Uint32 i = 0; i != nbPairs; ++i )
		{
			
			PxContactPair & pair = data.m_pairs[ i ];
			Red::MemoryCopy( &pair, &pairs[ i ], sizeof( PxContactPair ) );
			
			if( pair.contactStreamSize )
			{
				// ctremblay according to documentation, "Contact forces can be found starting at the next 16-byte boundary" 
				// so the buffer must respect that condition AND add space for contact storage.
				const Uint32 contactStreamAllocSize = ( ( pair.contactStreamSize + 15 ) & ~15 ) + pair.contactCount * sizeof( PxReal );
				pair.contactStream = ( PxU8* ) RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_PhysicsEngine, contactStreamAllocSize );
				Red::MemoryCopy( const_cast< PxU8* >( pair.contactStream ), pairs[ i ].contactStream, pair.contactStreamSize );
			}
		}
	}

	void Push( PxTriggerPair* pairs, PxU32 count )
	{
		m_onTriggerData.ResizeFast( m_onTriggerData.Size() + 1 );

		SOnTriggerData& data = m_onTriggerData.Back();
		data.m_count = count;

		data.m_pairs = ( PxTriggerPair* ) RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_PhysicsEngine, sizeof( PxTriggerPair ) * count );
		for( Uint32 i = 0; i != count; ++i )
		{
			Red::MemoryCopy( &data.m_pairs[ i ] , &pairs[ i ], sizeof( PxTriggerPair ) );
		}
	}

public:
	CAsyncFetchTask( physx::PxScene* scene ) : m_scene( scene ), m_resultFromFetch( false )
	{}

	~CAsyncFetchTask()
	{
		for( auto i : m_onContactData )
		{
			SOnContactData& data = i;
			for( Uint32 j = 0; j != data.m_nbPairs; ++j )
			{
				PxContactPair& pair = data.m_pairs[ j ];
				RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsEngine, ( void* ) pair.contactStream );
			}
			RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsEngine, data.m_pairs );
		}
		for( auto i : m_onTriggerData )
		{
			SOnTriggerData& data = i;
			RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsEngine, data.m_pairs );
		}
	}


public:
#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override { return TXT( "CAsyncFetchTask" ); }
	virtual Uint32		GetDebugColor() const override { return Color::BLUE.ToUint32(); }
#endif

};




PxFilterFlags PxFilterShader(
	PxFilterObjectAttributes attributes0,
	PxFilterData filterData0, 
	PxFilterObjectAttributes attributes1,
	PxFilterData filterData1,
	PxPairFlags& pairFlags,
	const void* constantBlock,
	PxU32 constantBlockSize)
{
	SPhysicalFilterData& data0 = ( SPhysicalFilterData& ) filterData0;
	SPhysicalFilterData& data1 = ( SPhysicalFilterData& ) filterData1;

	const CPhysicsEngine::CollisionMask collisionType0 = data0.GetCollisionType();
	const CPhysicsEngine::CollisionMask collisionType1 = data1.GetCollisionType();

	const CPhysicsEngine::CollisionMask collisionGroup0 = data0.GetCollisionGroup();
	const CPhysicsEngine::CollisionMask collisionGroup1 = data1.GetCollisionGroup();

	unsigned short flags0;
	unsigned short flags1;

	Bool isTrigger0 = PxFilterObjectIsTrigger(attributes0);
	Bool isTrigger1 = PxFilterObjectIsTrigger(attributes1);
	if( isTrigger0 || isTrigger1 )
	{
		if( isTrigger0 && !( collisionGroup0 & collisionType1 ) )
		{
			return PxFilterFlag::eSUPPRESS;
		}
		else if( isTrigger1 && !( collisionGroup1 & collisionType0 ) )
		{
			return PxFilterFlag::eSUPPRESS;
		}

		flags0 = data0.GetFlags();
		flags1 = data1.GetFlags();

		pairFlags |= PxPairFlag::eTRIGGER_DEFAULT;
	}
	else 
	{
		flags0 = data0.GetFlags();
		if( flags0 & SPhysicalFilterData::EPFDF_CollisionDisabled )
		{
			return PxFilterFlag::eSUPPRESS;
		}

		flags1 = data1.GetFlags();
		if( flags1 & SPhysicalFilterData::EPFDF_CollisionDisabled )
		{
			return PxFilterFlag::eSUPPRESS;
		}

		Bool isKinematic0 = PxFilterObjectIsKinematic(attributes0);
		Bool isKinematic1 = PxFilterObjectIsKinematic(attributes1);

		const static CPhysicsEngine::CollisionMask softContact = *( ( CPhysicsEngine::CollisionMask* ) constantBlock );

		if( ( collisionType0 & softContact ) && ( collisionType1 & softContact ) && ( isKinematic0 || isKinematic1 ) )
		{
			pairFlags |= PxPairFlag::eMODIFY_CONTACTS;
		}
		else if( !( collisionGroup0 & collisionType1 ) && !( collisionGroup1 & collisionType0 ) )
		{
			return PxFilterFlag::eSUPPRESS;
		}

		PxU16 contactDefault;
		if( ( flags0 & SPhysicalFilterData::EPFDF_TrackedKinematic ) || ( flags1 & SPhysicalFilterData::EPFDF_TrackedKinematic ) )
		{
			pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;
		}
		contactDefault = PxPairFlag::eCONTACT_DEFAULT;

#ifdef USE_APEX
		PxU16 halfword = (PxU16) filterData0.word3 | (PxU16) filterData1.word3;
		pairFlags |= PxPairFlags( halfword | contactDefault | PxPairFlag::eNOTIFY_TOUCH_FOUND );
#else
		pairFlags |= PxPairFlags( contactDefault | PxPairFlag::eNOTIFY_TOUCH_FOUND );
#endif

		if( flags0 & SPhysicalFilterData::EPFDF_CountactSoundable || flags1 & SPhysicalFilterData::EPFDF_CountactSoundable )
		{
			pairFlags |= PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND | PxPairFlag::eNOTIFY_CONTACT_POINTS;
		}

		if( flags0 & SPhysicalFilterData::EPFDF_DetailedConntactInfo || flags1 & SPhysicalFilterData::EPFDF_DetailedConntactInfo )
		{
			pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;
		}
	}

	return PxFilterFlag::eDEFAULT;
}

class CContactPointsHelper
{
	const PxContactPair* m_pairs;

	PxVec3 m_sumForce;
	PxVec3 m_sumForcePositive;
	PxVec3 m_biggestForcePosition;

	void Process()
	{
		if( !m_pairs ) return;

		const PxU8* stream = m_pairs->contactStream;

		PxContactStreamIterator iter((PxU8*)stream, m_pairs->contactStreamSize);

		//Contact forces can be found starting at the next 16-byte boundary following the contact stream end
		stream += ((m_pairs->contactStreamSize + 15) & ~15);
		const PxReal* impulses = reinterpret_cast<const PxReal*>(stream);

		PxU32 flippedContacts = (m_pairs->flags & PxContactPairFlag::eINTERNAL_CONTACTS_ARE_FLIPPED);
		PxU32 hasImpulses = (m_pairs->flags & PxContactPairFlag::eINTERNAL_HAS_IMPULSES);
		PxU32 nbContacts = 0;

		PxVec3 biggestForcePosition( 0.0f );
		PxVec3 biggestForceImpulse( 0.0f );

		while(iter.hasNextPatch())
		{
			iter.nextPatch();
			while(iter.hasNextContact())
			{
				iter.nextContact();
				PxReal separation = iter.getSeparation();

				PxVec3 point = iter.getContactPoint();
				PxVec3 normal = iter.getContactNormal();

				PxVec3 impulse = hasImpulses ? normal * impulses[ nbContacts ] : PxVec3( 0.f );

				m_sumForce += impulse;
				if( separation > 0.0f )
				{
					m_sumForcePositive += impulse;
				}

				float impulseMag = impulse.magnitudeSquared();
				if( impulseMag == 0 || impulseMag > biggestForceImpulse.magnitudeSquared() )
				{
					biggestForcePosition = point;
					biggestForceImpulse = impulse;
				}
				nbContacts++;
			}
		}

		m_biggestForcePosition = biggestForcePosition;
		m_pairs = 0;
	}
public:
	CContactPointsHelper( const PxContactPair* pair ) : m_pairs( pair ), m_sumForce( PxVec3( 0.0f ) ), m_sumForcePositive( PxVec3( 0.0f ) ), m_biggestForcePosition( PxVec3( 0.0f ) ) {}


	Vector GetSumForce() { Process(); return TO_VECTOR( m_sumForce ); }
	Vector GetSumForcePositive() { Process(); return TO_VECTOR( m_sumForcePositive ); }
	Vector GetBiggestForcePosition() { Process(); return TO_VECTOR( m_biggestForcePosition ); }

};

CPhysicsWorldPhysXImpl::CPhysicsWorldPhysXImpl( IPhysicsWorldParentProvider* parentProvider, Uint32 areaNumTilesPerEdge, Float areaSize, const Vector2& areaCornerPosition, Uint32 clipSize, Uint32 areaRes )
	: CPhysicsWorld( parentProvider, areaNumTilesPerEdge, areaSize, areaCornerPosition, clipSize, areaRes )
	, m_scene( nullptr )
	, m_backgroundTask( nullptr )
	, m_asyncFetchTask( nullptr )
	, m_resultFromFetch( false )
#ifndef NO_EDITOR
	, m_debugVisualisationBox( Box::EMPTY )
	, m_debugLineBuffer( nullptr )
	, m_debugTriangleBuffer( nullptr )
	, m_debugLineBufferSize( 0 )
	, m_debugTriangleBufferSize( 0 )
#endif
{

}

CPhysicsWorldPhysXImpl::~CPhysicsWorldPhysXImpl()
{
#ifdef USE_APEX
	if( m_apexInterface )
	{
		RED_MEMORY_FREE( MemoryPool_SmallObjects, MC_PhysX, m_apexInterface );
		m_apexInterface = nullptr;
	}
#endif

	if( m_backgroundTask )
	{
		m_backgroundTask->Release();
		m_backgroundTask = nullptr;
	}
#ifndef NO_EDITOR

	if( m_debugLineBuffer )
	{
		RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsDebug, m_debugLineBuffer );
	}

	if( m_debugTriangleBuffer )
	{
		RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsDebug, m_debugTriangleBuffer );
	}
#endif

	while ( m_waitingForTasksProcessing.GetValue() > 0 )
	{
	}

	CPhysXCpuDispatcher* dispacher = ( CPhysXCpuDispatcher* ) m_scene->getCpuDispatcher();
	{
		PC_SCOPE_PHYSICS( CPhysicsWorldPhysXImpl StartNextFrameSimulation ProcessCollectedTask );
		dispacher->ProcessCollectedTask();
	}

#ifdef USE_APEX
	NxApexScene* apexScene = ( NxApexScene* ) m_scene->userData;
	if( apexScene )
	{
		physx::PxU32 errorState = 0;
		apexScene->fetchResults(true, &errorState);
		apexScene->release();
		m_scene->userData = nullptr;
	}
#endif

	// remove scene
	m_scene->fetchResults( true );
	m_scene->release();
	m_scene = nullptr;
}

#ifdef USE_APEX
extern void CreateApexInterface( physx::apex::NxApexPhysX3Interface** );
#endif

bool CPhysicsWorldPhysXImpl::Init( Bool criticalPriority, Bool allowGpu, Bool useMemorySettings )
{
	m_singleTraceCallback.m_hitType = PxSceneQueryHitType::eBLOCK;
	m_multiTraceCallback.m_hitType = PxSceneQueryHitType::eTOUCH;

    PxSceneDesc sceneDesc( GPhysXEngine->GetPxPhysics()->getTolerancesScale() );
	sceneDesc.filterShader = PxFilterShader;

	CPhysicsEngine::CollisionMask mask = GPhysicEngine->GetCollisionTypeBit( CNAME( SoftKinematicContact ) );
	sceneDesc.filterShaderData = &mask;
	sceneDesc.filterShaderDataSize = sizeof( CPhysicsEngine::CollisionMask );

	sceneDesc.cpuDispatcher = GPhysXEngine->GetCpuDispatcher( criticalPriority );
#ifdef USE_PHYSX_GPU
	if( allowGpu && GPhysXEngine->GetCudaContext() )
	{
		sceneDesc.gpuDispatcher = GPhysXEngine->GetCudaContext()->getGpuDispatcher();
	}
#endif
    sceneDesc.gravity = TO_PX_VECTOR( GetGravityVector() );
	sceneDesc.flags	|= PxSceneFlag::eENABLE_ACTIVETRANSFORMS;
	sceneDesc.flags	|= PxSceneFlag::eADAPTIVE_FORCE;
	if( SPhysicsSettings::m_kinematicsContactsEnabled )
	{
		sceneDesc.flags	|= PxSceneFlag::eENABLE_KINEMATIC_PAIRS;
		sceneDesc.flags	|= PxSceneFlag::eENABLE_KINEMATIC_STATIC_PAIRS;
	}
	
	sceneDesc.simulationEventCallback = this;
	sceneDesc.contactModifyCallback = this;

	sceneDesc.dynamicStructure = SPhysicsSettings::m_dynamicStructureIsDynamic ? PxPruningStructure::eDYNAMIC_AABB_TREE : PxPruningStructure::eSTATIC_AABB_TREE;
	sceneDesc.staticStructure = SPhysicsSettings::m_staticsStructureIsDynamic ? PxPruningStructure::eDYNAMIC_AABB_TREE : PxPruningStructure::eSTATIC_AABB_TREE;
	sceneDesc.dynamicTreeRebuildRateHint = SPhysicsSettings::m_dynamicRebuildHint;

	if( useMemorySettings )
	{
		sceneDesc.nbContactDataBlocks = SPhysicsSettings::m_contactBlocksReservation;
		sceneDesc.maxNbContactDataBlocks = SPhysicsSettings::m_maxContactBlocks;

		sceneDesc.limits.maxNbActors = SPhysicsSettings::m_maxActorCount;
		sceneDesc.limits.maxNbStaticShapes = SPhysicsSettings::m_maxStaticShapes;
		sceneDesc.limits.maxNbDynamicShapes = SPhysicsSettings::m_maxDynamicShapes;
		sceneDesc.limits.maxNbConstraints = SPhysicsSettings::m_maxConstraints;

		// Buffers must be multiple of 16KB and 16 alignment
		m_physxBuffer = Red::CreateUniqueBuffer( SPhysicsSettings::m_physxScratchMemory, 16, MC_PhysX );
	}

	
    ASSERT( sceneDesc.isValid() );
    m_scene = GPhysXEngine->GetPxPhysics()->createScene( sceneDesc );
    if ( !m_scene )
    {
        return false;
    }

#ifdef USE_APEX
	NxApexScene* apexScene = 0;
	if( NxGetApexSDK() )
	{
		NxApexSceneDesc apexSceneDesc;
		apexSceneDesc.scene = m_scene;

		CreateApexInterface( &apexSceneDesc.physX3Interface );
		m_apexInterface = apexSceneDesc.physX3Interface;
		NxApexScene* apexScene = NxGetApexSDK()->createScene( apexSceneDesc );
		m_scene->userData = apexScene;

		apexScene->setLODResourceBudget(50000);

		apexScene->allocViewMatrix(physx::ViewMatrixType::LOOK_AT_LH );
		apexScene->allocProjMatrix(physx::ProjMatrixType::USER_CUSTOMIZED );
		apexScene->userData = this;
	}
#else
	m_scene->userData = this;
#endif

#ifdef USE_APEX
#ifdef USE_PHYSX_GPU
	if( CApexDestructionWrapper::IsDestructionGPUSimulationEnabled() )
	{
		NxApexSDK* apexSDK = NxGetApexSDK();
		if( apexSDK && apexScene )
		{
			Uint32 count = apexSDK->getNbModules();
			for( Uint32 i = count; i != 0; --i )
			{
				NxModule* module = apexSDK->getModules()[ i - 1 ];
				if( strcmp( module->getName(), "Destructible" ) != 0 ) continue;
				( ( physx::apex::NxModuleDestructible* ) module )->setGrbSimulationEnabled( *apexScene, true );
			}
		}
	}
#endif
#endif

    return true;
}

void CPhysicsWorldPhysXImpl::StartNextFrameSimulation( Float timeDelta, Float timeScale, Bool blankFrame )
{
	PC_SCOPE_PHYSICS( Physics scene simulate );

	if( timeDelta > SPhysicsSettings::m_simulationDeltaClamp )
	{
		timeDelta = SPhysicsSettings::m_simulationDeltaClamp;
	}

	m_currentDelta = timeDelta;

	TickRemovalWrappers( false );
	PreSimulateWrappers( timeDelta, blankFrame );

	CPhysXCpuDispatcher* dispacher = ( CPhysXCpuDispatcher* ) m_scene->getCpuDispatcher();
	{
		PC_SCOPE_PHYSICS( CPhysicsWorldPhysXImpl StartNextFrameSimulation ProcessCollectedTask );
		dispacher->ProcessCollectedTask();
	}

	if( m_scene->userData && m_viewValid )
	{
#ifdef USE_APEX
		NxApexScene* apexScene = ( ( NxApexScene* )m_scene->userData );
		apexScene->setViewMatrix( TO_PX_MAT( m_viewMatrix ), 0 );
#endif
	}

	if( m_currentDelta > 0.0f && m_positionValid )
	{
		if( m_scene->userData )
		{
#ifdef USE_APEX
			PC_SCOPE_PHYSICS( ApexSceneSimulate );
			NxApexScene* apexScene = ( ( NxApexScene* )m_scene->userData );
			apexScene->simulate( m_currentDelta, true, m_completionTask, m_physxBuffer.Get(), m_physxBuffer.GetSize() );
			if( m_completionTask )
			{
				m_completionTask->addReference();
			}
#endif
		}
		else
		{
			PC_SCOPE_PHYSICS( PhysXSceneSimulate );
			m_scene->simulate( m_currentDelta, m_completionTask, m_physxBuffer.Get(), m_physxBuffer.GetSize() );
			if( m_completionTask )
			{
				m_completionTask->addReference();
			}
		}
	}

	{
		CPhysXCpuDispatcher* dispacher = ( CPhysXCpuDispatcher* ) m_scene->getCpuDispatcher();
		dispacher->StartBeginTask();
	}

	if( m_backgroundTask )
	{
		if( !m_backgroundTask->GetToReleaseArray()->Empty() ) GTaskManager->Issue( *m_backgroundTask, TSP_VeryHigh );
		m_backgroundTask->Release();
		m_backgroundTask = nullptr;
	}
}

void CPhysicsWorldPhysXImpl::FetchCurrentFrameSimulation( Bool async )
{
	{
		PC_SCOPE_PHYSICS( waitingForTasksProcessing );
		if( m_completionTask  )
		{
			if( m_completionTask->getTaskManager() == nullptr )
			{
				m_completionTask->run();
				m_completionTask->release();
			}
		}

		while ( m_waitingForTasksProcessing.GetValue() > 0 )
		{
		}

		if( m_completionTask )
		{
			delete m_completionTask;
			m_completionTask = nullptr;
		}

	}

	if( !async )
	{
		if( m_scene->userData )
		{
#ifdef USE_APEX
			NxApexScene* apexScene = ( ( NxApexScene* )m_scene->userData );
			m_resultFromFetch = apexScene->fetchResults(true, 0 );
#endif
		}
		else
		{
			m_resultFromFetch = m_scene->fetchResults( true );
		}

		return;
	}

	m_asyncFetchTask = new ( CTask::Root ) CAsyncFetchTask( m_scene );
	GTaskManager->Issue( *m_asyncFetchTask, TSP_Critical );
}

void CPhysicsWorldPhysXImpl::CompleteCurrentFrameSimulation()
{
	if( m_asyncFetchTask )
	{
		PC_SCOPE_PHYSICS( waitingForFetchProcessing );
		while ( !m_asyncFetchTask->IsFinished() ) {}
		m_resultFromFetch = m_asyncFetchTask->m_resultFromFetch;
		for( auto i : m_asyncFetchTask->m_onContactData )
		{
			ProcessContact( i.m_pairHeader, i.m_pairs, i.m_nbPairs );
		}
		for( auto i : m_asyncFetchTask->m_onTriggerData )
		{
			ProcessTrigger( i.m_pairs, i.m_count );
		}
		m_asyncFetchTask->Release();
		m_asyncFetchTask = nullptr;
	}

	if( !m_resultFromFetch )
	{
		m_scene->flushSimulation( true );
	}
	else
	{
		PostSimulateWrappers();
	}

#ifndef NO_EDITOR
	if( m_debugVisualisationBox == Box::EMPTY )
	{
		for ( Uint32 i = 0; i < PxVisualizationParameter::eNUM_VALUES; ++i )
		{
			m_scene->setVisualizationParameter( PxVisualizationParameter::Enum( i ), 0.f );
		}
		return;
	}

	// tempshit: this needs to be selectable in filters dialog
	for ( Uint32 i = 0; i < PxVisualizationParameter::eNUM_VALUES; ++i )
	{
		m_scene->setVisualizationParameter( PxVisualizationParameter::Enum( i ), 0.f );
	}

	m_scene->setVisualizationParameter( PxVisualizationParameter::eSCALE, 1.f );
	m_scene->setVisualizationParameter( PxVisualizationParameter::eCOLLISION_SHAPES, 1.f );
	m_scene->setVisualizationParameter( PxVisualizationParameter::eCULL_BOX, 1.f );
	m_scene->setVisualizationParameter( PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 0.1f );
	m_scene->setVisualizationParameter( PxVisualizationParameter::eJOINT_LIMITS, 0.3f );

	PxBounds3 cullBox;
	cullBox.minimum = TO_PX_VECTOR( m_debugVisualisationBox.Min );
	cullBox.maximum = TO_PX_VECTOR( m_debugVisualisationBox.Max );
	m_scene->setVisualizationCullingBox( cullBox );

	// fetch the rendering data from PhysX
	const PxRenderBuffer &rb = m_scene->getRenderBuffer();

	if( m_debugTriangleBuffer )
	{
		RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsDebug, m_debugTriangleBuffer );
		m_debugTriangleBuffer = 0;
	}

	// draw triangles
	m_debugTriangleBufferSize = rb.getNbTriangles();
	const PxDebugTriangle* tris = rb.getTriangles();
	if ( m_debugTriangleBufferSize && tris )
	{
		m_debugTriangleBuffer = ( PxDebugTriangle* ) RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_PhysicsDebug, sizeof( PxDebugTriangle ) * m_debugTriangleBufferSize );
		for( Uint32 i = 0; i != m_debugTriangleBufferSize; i++ )
		{
			const PxDebugTriangle* debugTriangle = tris + i;
			PxDebugTriangle& triangle = m_debugTriangleBuffer[ i ];
			triangle.pos0 = debugTriangle->pos0;
			triangle.pos1 = debugTriangle->pos1;
			triangle.pos2 = debugTriangle->pos2;
			triangle.color0 = debugTriangle->color0;
			triangle.color1 = debugTriangle->color1;
			triangle.color2 = debugTriangle->color2;
		}
	}

	if( m_debugLineBuffer )
	{
		RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsDebug, m_debugLineBuffer );
		m_debugLineBuffer = 0;
	}

	m_debugLineBufferSize = rb.getNbLines();
	const PxDebugLine* lines = rb.getLines();
	if ( m_debugLineBufferSize && lines )
	{
		m_debugLineBuffer = ( PxDebugLine* ) RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_PhysicsDebug, sizeof( PxDebugLine ) * m_debugLineBufferSize );

		for ( Uint32 i = 0; i < m_debugLineBufferSize; i++ )
		{
			const PxDebugLine* debugLine = lines + i;
			PxDebugLine& line = m_debugLineBuffer[ i ];
			line.pos0 = debugLine->pos0;
			line.pos1 = debugLine->pos1;
			line.color0 = debugLine->color0;
			line.color1 = debugLine->color1;
		}
	}
#endif
}

void CPhysicsWorldPhysXImpl::UpdateActiveBodies()
{
	PC_SCOPE_PIX( Physics scene UpdateActiveBodies );

	PxU32 transformsCount;
	const PxActiveTransform* transforms = m_scene->getActiveTransforms( transformsCount );
	for ( PxU32 i = 0; i < transformsCount; ++i )
	{
		const PxActiveTransform& transform = transforms[i];

		if( !transform.actor )
		{
			continue;
		}

		if( !transform.userData )
		{
			continue;
		}

		CPhysicsWrapperInterface* wrapper = reinterpret_cast< CPhysicsWrapperInterface* >( transform.userData );

		if ( wrapper )
		{
			Matrix transformMatrix;
			transformMatrix.BuildFromQuaternion( TO_QUAT( transform.actor2World.q.getConjugate() ) );
			transformMatrix.SetTranslation( TO_VECTOR( transform.actor2World.p ) );

			wrapper->PostSimulationUpdateTransform( transformMatrix, transform.actor->isRigidActor() );
		}
	}
}

void CPhysicsWorldPhysXImpl::TickRemovalWrappers( bool force, TDynArray< void* >* toRemove )
{
	static TDynArray< void* > physxActorsToRemove;
	CPhysicsWorld::TickRemovalWrappers( force, &physxActorsToRemove );

	Uint32 toRemoveSize = physxActorsToRemove.Size();
	if( toRemoveSize )
	{
		PC_SCOPE_PHYSICS(CPhysWorl removingActors )
		m_scene->removeActors( ( physx::PxActor* const* )physxActorsToRemove.TypedData(), toRemoveSize );
		physxActorsToRemove.ClearFast();
	}

}

void CPhysicsWorldPhysXImpl::PreSimulateWrappers( Float timeDelta, Bool blankFrame, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease )
{
	m_stuffAdded = false;
	static TDynArray< void* > physxActorToAdd;
	static TDynArray< void* > physxActorToRemove;

	physxActorToAdd.ClearFast();
	physxActorToRemove.ClearFast();
	TDynArray< void* >* physxActorToRelease = nullptr;
	m_backgroundTask = new ( CTask::Root ) CPhysxActorsAllocateReleaseTask( &m_waitingForTasksProcessing );
	physxActorToRelease = m_backgroundTask->GetToReleaseArray();




	CPhysicsWorld::PreSimulateWrappers( timeDelta, blankFrame, &physxActorToAdd, &physxActorToRemove, physxActorToRelease );


	Uint32 toRemoveSize = physxActorToRemove.Size();
	if( toRemoveSize )
	{
//		RED_LOG( RED_LOG_CHANNEL( Physics ), TXT(" removing %i actors" ), toRemoveSize );
		PC_SCOPE_PHYSICS(CPhysWorl removingActors )
		m_scene->removeActors( ( physx::PxActor* const* )( physxActorToRemove.TypedData() ), toRemoveSize );
		physxActorToRemove.ClearFast();
	}

	Uint32 toAddSize = physxActorToAdd.Size();
	if( toAddSize )
	{
//		RED_LOG( RED_LOG_CHANNEL( Physics ), TXT(" adding %i actors" ), toAddSize );
		PC_SCOPE_PHYSICS(CPhysWorl addActors )
		m_scene->addActors( ( physx::PxActor* const* )( physxActorToAdd.TypedData() ), toAddSize );
		physxActorToAdd.ClearFast();
	}


}

void CPhysicsWorldPhysXImpl::PostSimulateWrappers()
{
	PC_SCOPE_PHYSICS( PostSimulateWrappers );
	UpdateActiveBodies();
	CPhysicsWorld::PostSimulateWrappers();
#ifndef NO_EDITOR
	m_scene->getSimulationStatistics( m_statistics );
#endif
}

void CPhysicsWorldPhysXImpl::onContactModify(physx::PxContactModifyPair* const pairs, PxU32 count)
{
	for( Uint32 i = 0; i != count; ++i )
	{
		const PxRigidActor* actor0 = pairs[ i ].actor[ 0 ];
		if( CPhysicsWrapperInterface* wrapper0 = ( CPhysicsWrapperInterface* ) actor0->userData )
		{
			wrapper0->OnContactModify( &pairs[ i ] );
		}
		const PxRigidActor* actor1 = pairs[ i ].actor[ 1 ];
		if( CPhysicsWrapperInterface* wrapper1 = ( CPhysicsWrapperInterface* ) actor1->userData )
		{
			wrapper1->OnContactModify( &pairs[ i ] );
		}
	}
}

void CPhysicsWorldPhysXImpl::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
	if( pairHeader.flags & ( PxContactPairHeaderFlag::eDELETED_ACTOR_0 | PxContactPairHeaderFlag::eDELETED_ACTOR_1 ) )
	{
		return;
	}

	CPhysicsWrapperInterface* wrapper0 = ( CPhysicsWrapperInterface* ) pairHeader.actors[ 0 ]->userData;
	CPhysicsWrapperInterface* wrapper1 = ( CPhysicsWrapperInterface* ) pairHeader.actors[ 1 ]->userData;

	if( !wrapper0 || !wrapper1 )
		return;

	if( m_asyncFetchTask )
	{
		m_asyncFetchTask->Push( pairHeader, pairs, nbPairs );
	}
	else
	{
		ProcessContact( pairHeader, pairs, nbPairs );
	}
}

void CPhysicsWorldPhysXImpl::ProcessContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
	CPhysicsWrapperInterface* wrapper0 = ( CPhysicsWrapperInterface* ) pairHeader.actors[ 0 ]->userData;
	CPhysicsWrapperInterface* wrapper1 = ( CPhysicsWrapperInterface* ) pairHeader.actors[ 1 ]->userData;

	IScriptable* component0 = nullptr;
	wrapper0->GetParent( component0 );
	IScriptable* component1 = nullptr;
	wrapper1->GetParent( component1 );

	do
	{
		if( pairs->flags & PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH )
		{
			continue;
		}

		physx::PxShape* shape0 = pairs->shapes[ 0 ];
		physx::PxShape* shape1 = pairs->shapes[ 1 ];

		CContactPointsHelper contactPointsHelper( pairs );

		SActorShapeIndex index0 = shape0 ? ( SActorShapeIndex& ) shape0->userData : SActorShapeIndex();
		SActorShapeIndex index1 = shape1 ? ( SActorShapeIndex& ) shape1->userData : SActorShapeIndex();

		if( component0 )
		{
			if( wrapper0->m_callbacks.Size() > CPhysicsWrapperInterface::EPCCT_OnCollision )
			{
				IPhysicalCollisionTriggerCallback* callback0 = wrapper0->m_callbacks[ CPhysicsWrapperInterface::EPCCT_OnCollision ].m_codeReceiverObject;
				if( callback0 )
				{
					SPhysicalCollisionInfo info( wrapper0, index0, wrapper1, index1 );
					if( !wrapper0->GetFlag( PRBW_DetailedConntactInfo ) )
						callback0->onCollision( info ); 
					else
					{
						Vector sumForce = contactPointsHelper.GetSumForce();
						info.m_position = contactPointsHelper.GetBiggestForcePosition();
						sumForce /= m_currentDelta;
						info.m_force = sumForce;
						callback0->onCollision( info ); 
					}
				}										   
			}

			if( wrapper0->m_callbacks.Size() > CPhysicsWrapperInterface::EPSCT_OnCollision )
			{
				CPhysicsWrapperInterface::SCallbackData& scriptCalback0 = wrapper0->m_callbacks[ CPhysicsWrapperInterface::EPSCT_OnCollision ];
				if( scriptCalback0.m_scriptReciverObject.Get() )
				{
					PushScriptEvent( scriptCalback0.m_scriptReciverObject, scriptCalback0.m_scriptReciversOnEventName, component1, index1 );
				}
			}
		}

		if( component1 )
		{
			if( wrapper1->m_callbacks.Size() > CPhysicsWrapperInterface::EPCCT_OnCollision )
			{
				IPhysicalCollisionTriggerCallback* callback1 = wrapper1->m_callbacks[ CPhysicsWrapperInterface::EPCCT_OnCollision ].m_codeReceiverObject;
				if( callback1 )
				{
					SPhysicalCollisionInfo info( wrapper1, index1, wrapper0, index0 );
					if( !wrapper1->GetFlag( PRBW_DetailedConntactInfo ) )
						callback1->onCollision( info ); 
					else
					{
						Vector sumForce = contactPointsHelper.GetSumForce();
						info.m_position = contactPointsHelper.GetBiggestForcePosition();
						sumForce /= m_currentDelta;
						info.m_force = sumForce;
						callback1->onCollision( info ); 
					}
				}	
			}

			if( wrapper1->m_callbacks.Size() > CPhysicsWrapperInterface::EPSCT_OnCollision )
			{
				CPhysicsWrapperInterface::SCallbackData& scriptCalback1 = wrapper1->m_callbacks[ CPhysicsWrapperInterface::EPSCT_OnCollision ];
				if( scriptCalback1.m_scriptReciverObject.Get() )
				{
					PushScriptEvent( scriptCalback1.m_scriptReciverObject, scriptCalback1.m_scriptReciversOnEventName, component0, index0 );
				}
			}
		}

		if( ( pairs->events ) & PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND )
		{

			Double currentContactTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();

			for( CPhysicsContactListener* listener : m_contactListeners )
			{
				Uint32 mask = listener->GetMask();

				if( ( mask & PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND ) == 0 )
				{
					continue;
				}

				if( listener->GetMarkedTime() + listener->GetDefinedTimeInterval() > currentContactTime )
				{
					continue;
				}

				PC_SCOPE_PHYSICS( ContactListener );

				PxRigidBody* actor0 = pairHeader.actors[ 0 ]->isRigidBody();
				PxRigidBody* actor1 = pairHeader.actors[ 1 ]->isRigidBody();

				Vector position;
				Uint16 actorIndex = 0;
				const char* objectMaterialName = 0;
				const char* materialName = 0;

				float distance = 0.0f;

				float mass = 0.0f;
				Float velocityMag = 0;

				if( actor0 && actor0->isRigidDynamic() )
				{
					SPhysicalFilterData data0( shape0->getSimulationFilterData() );
					if( data0.GetFlags() & SPhysicalFilterData::EPFDF_CountactSoundable )
					{
						PxTransform trans = actor0->getGlobalPose();
						Vector pos( trans.p.x, trans.p.y, trans.p.z, 0.0f );

						distance = m_position.DistanceSquaredTo( pos );
						if( distance < listener->GetDefinedDistanceSquared() ) 
						{
							velocityMag = actor0->getLinearVelocity().magnitude();
							if( velocityMag >= listener->GetDefinedMinimalVelocity() )
							{
								actorIndex = index0.m_actorIndex;
								PxMaterial* material0 = shape0->getMaterialFromInternalFaceIndex( index0.m_shapeIndex );
								PxMaterial* material1 = shape1->getMaterialFromInternalFaceIndex( index1.m_shapeIndex );
								if( material0 ) objectMaterialName = ( ( SPhysicalMaterial* ) material0->userData )->m_ansiName;
								if( material1 ) materialName = ( ( SPhysicalMaterial* ) material1->userData )->m_ansiName;
								position = pos;
								mass = actor0->getMass();
							}
						}
					}
				}

				if( velocityMag < listener->GetDefinedMinimalVelocity() && actor1 && actor1->isRigidDynamic() )
				{
					SPhysicalFilterData data1( shape1->getSimulationFilterData() );
					if( data1.GetFlags() & SPhysicalFilterData::EPFDF_CountactSoundable )
					{
						PxTransform trans = actor1->getGlobalPose();
						Vector pos( trans.p.x, trans.p.y, trans.p.z, 0.0f );

						distance = m_position.DistanceSquaredTo( pos );
						if( distance < listener->GetDefinedDistanceSquared() )
						{
							velocityMag = actor1->getLinearVelocity().magnitude();
							if( velocityMag >= listener->GetDefinedMinimalVelocity() )
								actorIndex = index1.m_actorIndex;
							PxMaterial* material0 = shape0->getMaterialFromInternalFaceIndex( index0.m_shapeIndex );
							PxMaterial* material1 = shape1->getMaterialFromInternalFaceIndex( index1.m_shapeIndex );
							if( material1 ) objectMaterialName = ( ( SPhysicalMaterial* )material1->userData )->m_ansiName;
							if( material0 ) materialName = ( ( SPhysicalMaterial* )material0->userData )->m_ansiName;
							position = pos;
							mass = actor1->getMass();
						}
					}
				}

				if( velocityMag >= listener->GetDefinedMinimalVelocity() )
				{
					listener->OnContactThresholdFound( position, objectMaterialName, materialName, mass, velocityMag );

					listener->MarkTime( currentContactTime );
				}
			}

		}
	}
	while( --nbPairs && pairs++ );
}

void CPhysicsWorldPhysXImpl::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
	if( m_asyncFetchTask )
	{
		m_asyncFetchTask->Push( pairs, count );
	}
	else
	{
		ProcessTrigger( pairs, count );
	}
}

void CPhysicsWorldPhysXImpl::ProcessTrigger(PxTriggerPair* pairs, PxU32 count)
{
	do
	{

		physx::PxShape* triggerShape = pairs->triggerShape;
		if( !triggerShape )
			continue;

		PxRigidActor* triggerActor = triggerShape->getActor();
		CPhysicsWrapperInterface* triggerWrapper = ( CPhysicsWrapperInterface* ) triggerActor->userData;
		if( !triggerWrapper )
		{
			if( pairs->flags & ( PxTriggerPairFlag::eDELETED_SHAPE_TRIGGER ) )
			{
				//detached trigger
				int a = 0;
			}

			continue;
		}

		IScriptable* triggerComponent = nullptr;;
		if ( !triggerWrapper->GetParent( triggerComponent ) )
		{
			continue;
		}

		physx::PxShape* otherShape = pairs->otherShape;

		if( !( pairs->flags & ( PxTriggerPairFlag::eDELETED_SHAPE_OTHER ) ) )
		{
			if( !otherShape )
				continue;

			PxRigidActor* otherActor = otherShape->getActor();

			CPhysicsWrapperInterface* otherWrapper = ( CPhysicsWrapperInterface* ) otherActor->userData;
			if( !otherWrapper )
				continue;

			SActorShapeIndex& actorShapeIndex = ( SActorShapeIndex& ) otherShape->userData;

			IScriptable* otherObject = nullptr;
			otherWrapper->GetParent( otherObject, actorShapeIndex.m_actorIndex );

			CPhysicsWrapperInterface::EPhysicalScriptCallbackType scriptCallbackType = CPhysicsWrapperInterface::EPSCT_COUNT;
			if( ( pairs->status & PxPairFlag::eNOTIFY_TOUCH_FOUND ) == PxPairFlag::eNOTIFY_TOUCH_FOUND )
			{
				scriptCallbackType = CPhysicsWrapperInterface::EPSCT_OnTriggerFocusFound;
				if( triggerWrapper->m_callbacks.Size() > ( Uint32 )CPhysicsWrapperInterface::EPCCT_OnTriggerFocusFound )
				{
					CPhysicsWrapperInterface::SCallbackData& callback = triggerWrapper->m_callbacks[ CPhysicsWrapperInterface::EPCCT_OnTriggerFocusFound ];
					if( callback.m_parentObject.Get() == nullptr )
					{
						continue;
					}
					IPhysicalCollisionTriggerCallback* callback0 = callback.m_codeReceiverObject;
					if( callback0 )
					{
						callback0->onTriggerEntered( STriggeringInfo( triggerComponent, triggerWrapper, ( SActorShapeIndex& ) triggerShape->userData, otherObject, otherWrapper, ( SActorShapeIndex& ) otherShape->userData, otherShape ) );
					}
				}
			}
			else if( ( pairs->status & PxPairFlag::eNOTIFY_TOUCH_LOST ) == PxPairFlag::eNOTIFY_TOUCH_LOST )
			{
				scriptCallbackType = CPhysicsWrapperInterface::EPSCT_OnTriggerFocusLost;
				if( triggerWrapper->m_callbacks.Size() > ( Uint32 )CPhysicsWrapperInterface::EPCCT_OnTriggerFocusLost )
				{
					CPhysicsWrapperInterface::SCallbackData& callback = triggerWrapper->m_callbacks[ CPhysicsWrapperInterface::EPCCT_OnTriggerFocusLost ];
					if( callback.m_parentObject.Get() == nullptr )
					{
						continue;
					}
					IPhysicalCollisionTriggerCallback* callback0 = callback.m_codeReceiverObject;
					if( callback0 )
					{
						callback0->onTriggerExited( STriggeringInfo( triggerComponent, triggerWrapper, ( SActorShapeIndex& ) triggerShape->userData, otherObject, otherWrapper, ( SActorShapeIndex& ) otherShape->userData, otherShape ) );
					}
				}
			}

			if( scriptCallbackType != CPhysicsWrapperInterface::EPSCT_COUNT )
			{
				if( triggerWrapper->m_callbacks.Size() > ( Uint32 )scriptCallbackType )
				{
					CPhysicsWrapperInterface::SCallbackData& scriptCallback = triggerWrapper->m_callbacks[ scriptCallbackType ];
					if( !scriptCallback.m_scriptReciversOnEventName.Empty() )
					{
						PushScriptEvent( scriptCallback.m_scriptReciverObject, scriptCallback.m_scriptReciversOnEventName, otherObject, actorShapeIndex );
					}
				}
			}
		}
		else
		{	
			if( triggerWrapper->m_callbacks.Size() > ( Uint32 )CPhysicsWrapperInterface::EPCCT_OnTriggerFocusLost )
			{
				CPhysicsWrapperInterface::SCallbackData& callback = triggerWrapper->m_callbacks[ CPhysicsWrapperInterface::EPCCT_OnTriggerFocusLost ];
				if( callback.m_parentObject.Get() == nullptr )
				{
					continue;
				}
				IPhysicalCollisionTriggerCallback* callback0 = callback.m_codeReceiverObject;
				if( callback0 )
				{
					callback0->onTriggerExited( STriggeringInfo( triggerComponent, triggerWrapper, ( SActorShapeIndex& ) triggerShape->userData, otherShape ) );
				}
			}
		}
	}
	while( --count && pairs++ );

}

void CPhysicsWorldPhysXImpl::onConstraintBreak(PxConstraintInfo*, PxU32)
{
	int a = 0;
}

void CPhysicsWorldPhysXImpl::onWake(physx::PxActor** , PxU32 )
{
	int a = 0;
}

void CPhysicsWorldPhysXImpl::onSleep(physx::PxActor** , PxU32 )
{
	int a = 0;
}

void CPhysicsWorldPhysXImpl::SetWhileSceneProcessFlag( bool flag )
{
	PC_SCOPE_PHYSICS( Physics scene SetWhileSceneProcessFlag );
	m_whileSceneProcess.SetValue( flag ? Red::System::Internal::ThreadId::CurrentThread().id : 0 );
	if( !flag ) return;
	while( m_tracesProcessing.GetValue() ) {}
}

#ifndef NO_EDITOR 
void CPhysicsWorldPhysXImpl::AddPlane( float height )
{
	PxScene* scene = GetPxScene();
	PxPhysics& physics = scene->getPhysics();
	PxTransform pose( TO_PX_VECTOR( Vector(0,0,height-0.1f) ), TO_PX_QUAT( Vector(0, 0, 0, 1) ));
	physx::PxRigidStatic* staticRigidActor = physics.createRigidStatic(pose);
	if ( !staticRigidActor )
	{
		ASSERT( staticRigidActor );
		return;
	}

	PxMaterial* material = GPhysXEngine->GetMaterial( CNAME( default ) );
	PxGeometry* geometry = new PxBoxGeometry(PxVec3(1000.f, 1000.f, 0.1f));
	physx::PxShape* shape = staticRigidActor->createShape( *geometry, *material );

	CPhysicsEngine::CollisionMask terrainCollisionTypeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask terrainCollisionGroupMask = GPhysicEngine->GetCollisionGroupMask( terrainCollisionTypeMask );

	SPhysicalFilterData data( terrainCollisionTypeMask, terrainCollisionGroupMask );
	shape->setSimulationFilterData( data.m_data );
	shape->setQueryFilterData( data.m_data );

	m_scene->addActor(*staticRigidActor);
}
#endif

/*template< class T >
void Dump( T* element, Uint32& index )
{
	CComponent* component = nullptr;
	if( element->GetParent( component ) )
	{
		CEntity* entity = component->GetEntity();
		if( entity )
		{
			String cmpName = component->GetFriendlyName();
			String entName = entity->GetName();
			float distanceFromViewportSquared = component->GetLocalToWorld().GetTranslationRef().DistanceSquaredTo2D( element->GetPhysicsWorld()->GetEyePosition() );
			if( entity->ShouldBeStreamed() )
			{
				Char streamed = component->IsStreamed() ? 'S' : 'U';
				RED_LOG( RED_LOG_CHANNEL( Dump ), TXT(" %i %c %s %f ET: %s" ), index, streamed, cmpName.AsChar(), sqrt( distanceFromViewportSquared ), entName.AsChar() );
			}
			else
			{
				RED_LOG( RED_LOG_CHANNEL( Dump ), TXT(" %i N %s %f ET: %s" ), index, cmpName.AsChar(), sqrt( distanceFromViewportSquared ), entName.AsChar() );
			}
		}
	}
}


template< class T1, class T2 >
void CPhysicsWorldPhysXImpl::DumpPhysicalComponentsNames( TWrappersPool< T1, T2 >* pool, Uint32& index )
{
	T1* wrapper = pool->GetWrapperFirst();
	while ( wrapper ) { Dump( wrapper, index ); wrapper = pool->GetWrapperNext( wrapper ); ++index; }

}

void CPhysicsWorldPhysXImpl::DumpPhysicalComponentsNames( Uint32& index )
{
#ifdef USE_APEX
	DumpPhysicalComponentsNames( GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >(), index );
	DumpPhysicalComponentsNames( GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >(), index );
	DumpPhysicalComponentsNames( GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >(), index );
	DumpPhysicalComponentsNames( GetWrappersPool< CPhysicsJointedRagdollWrapper, SWrapperContext >(), index );
	DumpPhysicalComponentsNames( GetWrappersPool< CPhysicsCharacterWrapper, SWrapperContext >(), index );
	DumpPhysicalComponentsNames( GetWrappersPool< CPhysicsParticleWrapper, SWrapperContext >(), index );
	DumpPhysicalComponentsNames( GetWrappersPool< CApexDestructionWrapper, SWrapperContext >(), index );
	DumpPhysicalComponentsNames( GetWrappersPool< CApexClothWrapper, SWrapperContext >(), index );
#endif //! USE_APEX

}
void CPhysicsWorldPhysXImpl::DumpPhysicalComponentsNamesGlobaly()
{
	RED_LOG( RED_LOG_CHANNEL( Dump ), TXT( "========================================================" ) );
	RED_LOG( RED_LOG_CHANNEL( Dump ), TXT( "Starting dumping components names with physical wrappers" ) );
	RED_LOG( RED_LOG_CHANNEL( Dump ), TXT( "" ) );
	RED_LOG( RED_LOG_CHANNEL( Dump ), TXT( "index lod name cameraDistance" ) );
	RED_LOG( RED_LOG_CHANNEL( Dump ), TXT( "" ) );
	CPhysicsWorldPhysXImpl* currentWorld = static_cast< CPhysicsWorldPhysXImpl* >( m_top );

	Uint32 index = 0;
	while( currentWorld )
	{
		currentWorld->DumpPhysicalComponentsNames( index );
		currentWorld = static_cast< CPhysicsWorldPhysXImpl* >( currentWorld->m_next );
	}
	RED_LOG( RED_LOG_CHANNEL( Dump ), TXT( "" ) );
	RED_LOG( RED_LOG_CHANNEL( Dump ), TXT( "Stoping dumping components names with physical wrappers" ) );
	RED_LOG( RED_LOG_CHANNEL( Dump ), TXT( "========================================================" ) );

}


*/


#ifndef NO_EDITOR 

void CPhysicsWorldPhysXImpl::SetDebuggingVisualizationBox( Box cullingBox )
{
	m_debugVisualisationBox = cullingBox;
}

void*	CPhysicsWorldPhysXImpl::GetDebugTriangles( Uint32& trianglesBufferSize )
{
	trianglesBufferSize = m_debugTriangleBufferSize;
	return m_debugTriangleBuffer;
}

void*	CPhysicsWorldPhysXImpl::GetDebugLines( Uint32& linesBufferSize )
{
	linesBufferSize = m_debugLineBufferSize;
	return m_debugLineBuffer;
}

void	CPhysicsWorldPhysXImpl::GetDebugVisualizationForMaterials(const Vector position, TDynArray< SPhysicsDebugLine >& debugLines )
{
	physx::PxScene* scene = m_scene;
	Uint32 actorsCount = scene->getNbActors( physx::PxActorTypeSelectionFlag::eRIGID_STATIC );
	for( Uint32 i = 0; i != actorsCount; ++i )
	{
		physx::PxActor* actor = 0;
		scene->getActors( physx::PxActorTypeSelectionFlag::eRIGID_STATIC, &actor, 1, i );
		physx::PxRigidStatic* staticActor = actor->isRigidStatic();
		if( !staticActor ) continue;
		Uint32 shapesCount = staticActor->getNbShapes();
		for( Uint32 j = 0; j != shapesCount; ++j )
		{
			physx::PxShape* shape = 0;
			staticActor->getShapes( &shape, 1, j );
			physx::PxHeightFieldGeometry geometry;
			if( !shape->getHeightFieldGeometry( geometry ) ) continue;
			if( !geometry.heightField ) continue;
			float dif = ( ( geometry.rowScale + geometry.columnScale ) / 2.0f ) * 0.33f;
			physx::PxBounds3 bounds = staticActor->getWorldBounds();
			float height = bounds.getDimensions().z + 2.0f;
			physx::PxVec3 pos( 0.0f, 0.0f, bounds.maximum.z + 1.0f );
			physx::PxTransform trans = staticActor->getGlobalPose();
			for( Uint32 y = 0; y != geometry.heightField->getNbRows(); ++y )
			{
				for( Uint32 x = 0; x != geometry.heightField->getNbColumns(); ++x )
				{
					pos.x = x * geometry.columnScale + dif + trans.p.x;
					pos.y = y * geometry.rowScale + dif + trans.p.y;
					if( position.DistanceSquaredTo2D( TO_VECTOR( pos ) ) > 20.0f * 20.0f ) continue;
					physx::PxRaycastHit hit;
					if( physx::PxShapeExt::raycast( *shape, *shape->getActor(), pos, physx::PxVec3( 0.0f, 0.0f, -1.0f ), height,  physx::PxSceneQueryFlag::eIMPACT |  physx::PxSceneQueryFlag::eNORMAL |  physx::PxSceneQueryFlag::ePRECISE_SWEEP |  physx::PxSceneQueryFlag::eMESH_BOTH_SIDES, 1, &hit, true ) )
					{
						physx::PxMaterial* material = shape->getMaterialFromInternalFaceIndex( hit.faceIndex );
						if( material )
						{
							SPhysicalMaterial* mat = ( ( SPhysicalMaterial* ) material->userData );
							Color color = mat->m_debugColor;
							
							debugLines.PushBack( SPhysicsDebugLine( TO_VECTOR( hit.position ), TO_VECTOR( hit.position ) + TO_VECTOR( hit.normal ), color ) );
						}
					}
					pos.x = x * geometry.columnScale + geometry.columnScale - dif + trans.p.x;
					pos.y = y * geometry.rowScale + geometry.rowScale - dif + trans.p.y;
					if( !physx::PxShapeExt::raycast( *shape, *shape->getActor(), pos,  physx::PxVec3( 0.0f, 0.0f, -1.0f ), height,  physx::PxSceneQueryFlag::eIMPACT |  physx::PxSceneQueryFlag::eNORMAL |  physx::PxSceneQueryFlag::ePRECISE_SWEEP |  physx::PxSceneQueryFlag::eMESH_BOTH_SIDES, 1, &hit, true ) )
					{
						continue;
					}

					physx::PxMaterial* material = shape->getMaterialFromInternalFaceIndex( hit.faceIndex );
					if( material )
					{
						Color color = ( ( SPhysicalMaterial* ) material->userData )->m_debugColor;
						debugLines.PushBack( SPhysicsDebugLine( TO_VECTOR( hit.position ), TO_VECTOR( hit.position ) + TO_VECTOR( hit.normal ), color ) );
					}

				}
			}
		}

	}
}
#endif //! NO_EDITOR

#endif
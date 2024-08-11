#include "build.h"

#include "physXEngine.h"
#include "physicsWorldPhysXImpl.h"
#include "physXCpuDispatcher.h"

#ifdef USE_APEX 
#include "NullRenderer.h"
#include "NxModuleDestructible.h"
#include "NxModuleClothing.h"
#include "PxString.h"
#include "NxDestructibleAsset.h"
#include "NxSerializer.h"
#include "NxParamUtils.h"
#include "NxModuleLegacy.h"
#endif
#include "physicsSettings.h"


#ifdef USE_PHYSX
using namespace physx;
CPhysXEngine* GPhysXEngine = NULL;

#ifndef RED_FINAL_BUILD
String CPhysXLogger::m_lastErrorString;
Red::System::Internal::ThreadId CPhysXLogger::m_lastErrorThreadId;
#endif

void CPhysXLogger::reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
{
#ifndef RED_FINAL_BUILD
	//TODO make this smarter
	RED_LOG_ERROR( PhysX, TXT( "%" ) RED_PRIWas, message );
	m_lastErrorString = ANSI_TO_UNICODE( message );
	m_lastErrorThreadId = Red::System::Internal::ThreadId::CurrentThread();
#endif
}

#ifdef USE_APEX
extern NxUserRenderResourceManager* CreateUserRenderResourceManager();
#endif

CPhysXEngine::CPhysXEngine()
	: m_physics( nullptr )
	, m_foundation( nullptr )
	, m_cpuDefaultDispatcher( nullptr )
#ifdef USE_PHYSX_GPU
	, m_cudaContext( nullptr )
#endif
	, m_registry( nullptr )
	, m_stringTable( nullptr )
	, m_cooking( nullptr )
	, m_physxMaterials( nullptr )
#ifdef PHYSICS_PROFILE
	, m_profileZoneManager( nullptr )
#endif
{

}

CPhysXEngine::~CPhysXEngine()
{

}

void* CPhysXEngine::CPhysXAllocator::allocate(size_t size, const char* typeName, const char* filename, int line)
{
	// PhysX Allocations must all be 16 byte aligned
	
	size_t correctSize = (size + 15) & ~(15);
	return RED_MEMORY_ALLOCATE_ALIGNED_HYBRID( MemoryPool_Physics, MC_PhysX, correctSize, 16 );
}

void CPhysXEngine::CPhysXAllocator::deallocate(void* ptr)
{
	RED_MEMORY_FREE_HYBRID( MemoryPool_Physics, MC_PhysX, ptr );
}

Bool CPhysXEngine::Init()
{
	PxTolerancesScale scale;

	m_foundation = PxCreateFoundation( PX_PHYSICS_VERSION, m_allocator, m_errorCallback );

#ifdef PHYSICS_PROFILE
	m_profileZoneManager = &PxProfileZoneManager::createProfileZoneManager( m_foundation );
#endif

	m_physics = PxCreatePhysics( PX_PHYSICS_VERSION, *m_foundation, scale 
#ifdef PHYSICS_PROFILE
		, true
		, m_profileZoneManager
#endif		
		);
	if ( !m_physics )
	{
		return false;
	}
	PxInitExtensions( *m_physics );

	m_cooking = PxCreateCooking( PX_PHYSICS_VERSION, *m_foundation, PxCookingParams(scale) );
	if ( !m_cooking )
	{
		return false;
	}

	if( SPhysicsSettings::m_useCpuDefaultDispacherNbCores )
	{
		m_cpuDefaultDispatcher = PxDefaultCpuDispatcherCreate( SPhysicsSettings::m_useCpuDefaultDispacherNbCores );
	}
	m_cpuDispatcherHighPriority = new CPhysXCpuDispatcher( TSP_VeryHigh );
	m_cpuDispatcherCriticalPriority = new CPhysXCpuDispatcher( TSP_Critical );

	m_stringTable = &PxStringTableExt::createStringTable( PxGetFoundation().getAllocatorCallback() );
	m_registry = PxSerialization::createSerializationRegistry( *m_physics );

#if defined(USE_PHYSX_GPU) && !defined( _DEBUG )
	if( !( SPhysicsSettings::m_dontCreateClothOnGPU && SPhysicsSettings::m_dontCreateDestructionOnGPU ) )
	{
		physx::PxCudaContextManagerDesc ctxMgrDesc;

		bool useInterop = false;
/*		if (useInterop && !hasInputCommand(SCLIDS::NO_INTEROP))
		{
			SampleRenderer::Renderer* renderer = getRenderer();
			SampleRenderer::Renderer::DriverType driverType = renderer->getDriverType();
			ctxMgrDesc.graphicsDevice = renderer->getDevice();

			if (driverType == SampleRenderer::Renderer::DRIVER_DIRECT3D9)
			{
				ctxMgrDesc.interopMode = physx::pxtask::CudaInteropMode::D3D9_INTEROP;
			}
			else if (driverType == SampleRenderer::Renderer::DRIVER_DIRECT3D11)
			{
				ctxMgrDesc.interopMode = physx::pxtask::CudaInteropMode::D3D11_INTEROP;
			}
			else if (driverType == SampleRenderer::Renderer::DRIVER_OPENGL)
			{
				ctxMgrDesc.interopMode = physx::pxtask::CudaInteropMode::OGL_INTEROP;
			}
		}*/

		m_cudaContext = physx::PxCreateCudaContextManager(m_physics->getFoundation(), ctxMgrDesc, m_physics->getProfileZoneManager());
		if( m_cudaContext )
		{
			if( !m_cudaContext->contextIsValid() )
			{
				m_cudaContext->release();
				m_cudaContext = NULL;
			}
			else if ( m_cudaContext->supportsArchSM30() )
			{
				m_cudaContext->setUsingConcurrentStreams(false);
			}
		}
	}
#endif

#ifdef USE_APEX
	/* Fill out the Apex SDK descriptor */
	NxApexSDKDesc apexDesc;

	/* Apex needs an allocator and error stream.  By default it uses those of the PhysX SDK. */

	/* Let Apex know about our PhysX SDK and cooking library */
	apexDesc.physXSDK              = m_physics;
	apexDesc.cooking               = m_cooking;

	/* Our custom render resource manager */
	apexDesc.renderResourceManager = CreateUserRenderResourceManager();
	if( !apexDesc.renderResourceManager ) 
	{
		apexDesc.renderResourceManager = new physx::apex::NullRenderResourceManager();
	}
	/* Finally, create the Apex SDK */
	NxApexCreateError errorCode; 

	NxApexSDK* apexSdk = NxCreateApexSDK(apexDesc, &errorCode);

	if ( apexSdk )
	{
		instantiateModuleDestructible();
		instantiateModuleClothing();
		instantiateModuleLegacy();

		NxModuleDestructible* destructible = static_cast< NxModuleDestructible* >( apexSdk->createModule( "Destructible" ) );
		if( destructible )
		{
			NxParameterized::Interface* params = destructible->getDefaultModuleDesc();
#if defined(USE_PHYSX_GPU) && defined(NDEBUG)
			if( CApexDestructionWrapper::IsDestructionGPUSimulationEnabled() )
			{
				NxParameterized::setParamI32(*params, "gpuRigidBodySettings.gpuDeviceOrdinal", -3);	// This value requests GRBs if a device is found, but fails silently
				NxParameterized::setParamF32(*params, "gpuRigidBodySettings.meshCellSize", 10.0f);
			}
#endif
			destructible->init(*params);
			destructible->setMaxChunkDepthOffset(0);
			destructible->setUseLegacyChunkBoundsTesting( true );

			destructible->setLODEnabled( false );
		}


		NxModuleClothing* clothingModule = static_cast<NxModuleClothing*>(apexSdk->createModule("Clothing"));
		if (clothingModule)
		{
			NxParameterized::Interface* params = clothingModule->getDefaultModuleDesc();
#ifdef PX_WINDOWS
			// Don't use compartments on consoles
//			NxParameterized::setParamU32(*params, "maxNumCompartments", 3);
#endif
			NxParameterized::setParamBool(*params, "parallelizeFetchResults", true );

			NxParameterized::setParamBool(*params, "asyncFetchResults", true);

			clothingModule->init(*params);

			clothingModule->setLODEnabled( false );
			clothingModule->setLODBenefitValue(15.0f);

			Float cost = clothingModule->getLODUnitCost();
			clothingModule->setLODUnitCost(cost / 50.0f);
		}

#ifndef RED_FINAL_BUILD
		NxModule* legacy = static_cast< NxModule* >( apexSdk->createModule( "Legacy" ) );
		if( legacy )
		{
			NxParameterized::Interface* params = legacy->getDefaultModuleDesc();
			legacy->init(*params);
		}
#endif

	}

#endif

	return CPhysicsEngine::Init();
}

void CPhysXEngine::ShutDown()
{
	CPhysicsEngine::ShutDown();

	if( m_physxMaterials )
	{
		m_physxMaterials->release();
		m_physxMaterials = nullptr;
	}
#ifdef USE_APEX
	NxApexSDK* apexSdk = NxGetApexSDK();
	if( apexSdk )
	{
		Uint32 count = apexSdk->getNbModules();
		for( Uint32 i = count; i != 0; --i )
		{
			NxModule* module = apexSdk->getModules()[ i - 1 ];
			apexSdk->releaseModule( module );
		}
		apexSdk->release();
	}
#endif

#ifdef USE_PHYSX_GPU
	if (m_cudaContext)
	{
		m_cudaContext->release();
		m_cudaContext = 0;
	}
#endif

	if( m_cpuDefaultDispatcher ) m_cpuDefaultDispatcher->release();
	delete m_cpuDispatcherHighPriority;
	delete m_cpuDispatcherCriticalPriority;
	m_registry->release();
	m_stringTable->release();
	m_cooking->release();
	PxCloseExtensions();
	m_physics->release();
	m_foundation->release();
}

PxMaterial* CPhysXEngine::GetMaterial( const char* name )
{
	CName materialName( ANSI_TO_UNICODE( name ) );

	for( Uint32 i = 0; i != m_physicalMaterials.Size(); ++i )
	{
		SPhysicalMaterial& material = m_physicalMaterials[ i ];
		if( material.m_name == materialName )
		{
			return static_cast< PxMaterial* >( material.m_middlewareInstance );
		}
	}
	return 0;
}

PxMaterial* CPhysXEngine::GetMaterial( const CName& name )
{
	for( Uint32 i = 0; i != m_physicalMaterials.Size(); ++i )
	{
		SPhysicalMaterial& material = m_physicalMaterials[ i ];
		if( material.m_name == name )
		{
			return ( PxMaterial* ) material.m_middlewareInstance;
		}
	}
	return 0;
}

Uint32 CPhysXEngine::GetMaterialCount()
{
	return m_physicalMaterials.Size();
}

CPhysicsWorld* CPhysXEngine::CreateWorld( IPhysicsWorldParentProvider* parentProvider, Uint32 areaResolution, Float areaSize, Vector2 areaCornerPosition, Uint32 clipMapSize, Uint32 tileRes, Bool useMemorySettings, Bool criticalDispatch, Bool allowGpu )
{
	CPhysicsWorldPhysXImpl* physXWorld = new CPhysicsWorldPhysXImpl( parentProvider, areaResolution, areaSize, areaCornerPosition, clipMapSize, tileRes );
	physXWorld->Init( criticalDispatch, allowGpu, useMemorySettings );    
	physXWorld->AddRef();
	return physXWorld;
}

void CPhysXEngine::DestroyWorld( CPhysicsWorld* world )
{
	CPhysicsEngine::DestroyWorld( world );
	world->FetchCurrentFrameSimulation( false );
	world->CompleteCurrentFrameSimulation();
	world->ReleaseRef();
}

physx::PxCpuDispatcher* CPhysXEngine::GetCpuDispatcher( Bool critical ) const
{
#ifndef RED_FINAL_BUILD
	if( SPhysicsSettings::m_useCpuDefaultDispacherNbCores )
		return m_cpuDefaultDispatcher;
#endif
	return critical ? m_cpuDispatcherCriticalPriority : m_cpuDispatcherHighPriority;
}

void* CPhysXEngine::CreateMaterialInstance( Float dynamicFriction, Float staticFriction, Float restitution, void* engineDefinition )
{
	if( !m_physxMaterials )
	{
		m_physxMaterials = PxCreateCollection();
	}
	m_physxMaterials->add( *m_physics->createMaterial( staticFriction, dynamicFriction, restitution ) );
	PxMaterial* material = m_physxMaterials->getObject( m_physxMaterials->getNbObjects() - 1 ).is< PxMaterial >();
	m_physxMaterials->addId( *material, ( Uint64 )material );
	material->userData = engineDefinition;
	return material;
}

TDynArray< PxMaterial* > CPhysXEngine::GetMaterialsArray()
{
	TDynArray< PxMaterial* > result;
	if( m_physxMaterials )
	{
		Uint32 count = m_physxMaterials->getNbObjects();
		result.Reserve( count );
		for( Uint32 i = 0; i != count; ++i )
		{
			PxMaterial* material = m_physxMaterials->getObject( i ).is< PxMaterial >();
			result.PushBack( material );
		}
	}
	return result;
}

void CPhysXEngine::FlushMaterialInstances()
{
	if( !m_physxMaterials ) return;
	Uint32 count = m_physxMaterials->getNbObjects();
	for( Uint32 i = 0; i != count; ++i )
	{
		PxMaterial* material = m_physxMaterials->getObject( i ).is< PxMaterial >();
		material->release();
	}

	m_physxMaterials->release();
	m_physxMaterials = nullptr;
}

#ifndef NO_EDITOR
void UpdatePhysxSettings()
{
#if defined(USE_APEX) && defined(USE_PHYSX_GPU)
	NxApexSDK* apexSdk = NxGetApexSDK();
	if( apexSdk )
	{
		Uint32 count = apexSdk->getNbModules();
		for( Uint32 i = count; i != 0; --i )
		{
			NxModule* module = apexSdk->getModules()[ i - 1 ];
			const char* name = module->getName();
			if( strcmp( module->getName(), "Destructible" ) == 0 )
			{
				Bool destructionOnGPU = CApexDestructionWrapper::IsDestructionGPUSimulationEnabled();
				NxModuleDestructible* destructible = static_cast< NxModuleDestructible* >( module );

				Uint32 scenesCount = GPhysXEngine->GetPxPhysics()->getNbScenes();
				for( Uint32 j = 0; j != scenesCount; ++j )
				{
					PxScene* scene = 0;
					GPhysXEngine->GetPxPhysics()->getScenes( &scene, 1, j );

					NxApexScene* apexScene = ( NxApexScene* ) scene->userData;
					if( apexScene )
					{
						destructible->setGrbSimulationEnabled( *apexScene, destructionOnGPU );
					}
				}
			}
		}
	}
#endif


	Uint32 minPositionIters = SPhysicsSettings::m_ragdollMinPositionIters;
	Uint32 minVelocityIters = SPhysicsSettings::m_ragdollMinVelocityIters;

	Float actorSleepThreshold = SPhysicsSettings::m_actorSleepThreshold;
	Float actorAngularVelocityLimit = SPhysicsSettings::m_actorAngularVelocityLimit;

	Float contactReportThreshold = SPhysicsSettings::m_contactReportsThreshold;
	Uint32 scenesCount = GPhysXEngine->GetPxPhysics()->getNbScenes();
	for( Uint32 j = 0; j != scenesCount; ++j )
	{
		PxScene* scene = 0;
		GPhysXEngine->GetPxPhysics()->getScenes( &scene, 1, j );
		PxU32 count = scene->getNbActors( physx::PxActorTypeFlag::eRIGID_DYNAMIC );
		for( Uint32 i = 0; i != count; ++i )
		{
			physx::PxActor* actor = 0;
			scene->getActors( physx::PxActorTypeFlag::eRIGID_DYNAMIC, &actor, 1, i );
			physx::PxRigidDynamic* rigidDynamic = actor->isRigidDynamic();
			if( !rigidDynamic ) continue;
			Float mass = rigidDynamic->getMass();
			rigidDynamic->setSleepThreshold( actorSleepThreshold );
			rigidDynamic->setMaxAngularVelocity( actorAngularVelocityLimit );
			rigidDynamic->setContactReportThreshold( contactReportThreshold * mass );
			if( rigidDynamic->getNbConstraints() == 0 ) continue;
			rigidDynamic->setSolverIterationCounts( minPositionIters, minVelocityIters );
		}
	}
}
#endif

void CPhysXEngine::GetTimings( Float& physicsFetchTime, Float& physicsClothFetchTime, Uint32& clothAttached, Uint32& clothSimulated )
{
	static size_t perfCountersCount = 0;
	static CPerfCounter* physicsFetchTimePerfCounter = 0;
	static CPerfCounter* physicsFetchClothTimePerfCounter = 0;
	static CPerfCounter* finalizeMovementPerfCounter = 0;

	static CPerfCounter* physXCharacterControllerMovePerfCounter = 0;
	const size_t currentPerfCountersCount = CProfiler::GetCountersCount();
	const Double freq = Red::System::Clock::GetInstance().GetTimer().GetFrequency();

	if( perfCountersCount != currentPerfCountersCount )
	{
		perfCountersCount = currentPerfCountersCount;
		if( !physicsFetchTimePerfCounter ) physicsFetchTimePerfCounter = CProfiler::GetCounter( "Physics scene fetch" );
		if( !physicsFetchClothTimePerfCounter ) physicsFetchClothTimePerfCounter = CProfiler::GetCounter( "Physics scene cloth fetch" );
		if( !finalizeMovementPerfCounter ) finalizeMovementPerfCounter = CProfiler::GetCounter( "FinalizeMovement" );
		if( !physXCharacterControllerMovePerfCounter ) physXCharacterControllerMovePerfCounter = CProfiler::GetCounter( "CC_Update2_ApplyMovement_All" );
	}
	if( physicsFetchTimePerfCounter )
	{
		static Uint64 previousTime = 0;
		const Uint64 time = physicsFetchTimePerfCounter->GetTotalTime();
		physicsFetchTime = (float)((time - previousTime)/freq)*1000.0f;
		previousTime = time;
	}
	else physicsFetchTime = -1.0f;
	if( physicsFetchClothTimePerfCounter )
	{
		static Uint64 previousTime = 0;
		const Uint64 time = physicsFetchClothTimePerfCounter->GetTotalTime();
		physicsClothFetchTime = (float)((time - previousTime)/freq)*1000.0f;
		previousTime = time;
	}
	else physicsClothFetchTime = -1.0f;
#ifndef NO_DEBUG_PAGES
	Float simulated = 0;
	PHYSICS_STATISTICS_GET_AND_CLEAR( ClothsSimulated, simulated );
	clothAttached = (Uint32)SPhysicsStatistics::ClothsInstanced;
	clothSimulated = (Uint32)simulated;
#endif
}

void RemoveSerializable( physx::PxBase* ser )
{
	if( ser->getConcreteType() != physx::PxConcreteType::eCONSTRAINT )
	{
		ser->release();
	}
}

#endif

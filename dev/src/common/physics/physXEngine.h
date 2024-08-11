#pragma once

#include "../physics/physicsIncludes.h"
#include "physicsEngine.h"
#include "../core/importer.h"
#include "../physics/physicalCollision.h"

enum EVisibilityResult
{
	EVR_NotTested,
	EVR_NotVisible,
	EVR_PartialyVisible,
	EVR_Visible,
};

struct SPhysicalFilterData 
{

#ifndef USE_PHYSX
	struct  
	{
		Uint32 word0;
		Uint32 word1;
		Uint32 word2;
		Uint32 word3; 
	} m_data;
#else

	physx::PxFilterData m_data;

	SPhysicalFilterData( physx::PxFilterData data )
	{
		m_data.word0 = data.word0;
		m_data.word1 = data.word1;
		m_data.word2 = data.word2;
		m_data.word3 = data.word3;
	}

#endif

	enum EPhysicalFilterDataFlags
	{
		EPFDF_CollisionDisabled			= FLAG( 0 ),
		EPFDF_DetailedConntactInfo		= FLAG( 1 ),
		EPFDF_TrackedKinematic			= FLAG( 2 ),
		EPFDF_CountactSoundable			= FLAG( 3 ),
		EPFDF_SoundOccludable			= FLAG( 4 ),
		EPFDF_VerticalSlidingIteration	= FLAG( 5 ),

		EPFDF_SHAPE_FLAGS = EPFDF_CollisionDisabled | EPFDF_DetailedConntactInfo | EPFDF_TrackedKinematic
	};

	SPhysicalFilterData( CPhysicalCollision& col )
	{
		Construct( col );		
	}

	SPhysicalFilterData( const CPhysicalCollision& col )
	{
		CPhysicalCollision colCopy = col;
		Construct( colCopy );
	}

	SPhysicalFilterData( CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, Uint16 flags = 0 )
	{
		( ( Uint64& )m_data.word0 ) = ( collisionType & 0x0000FFFFFFFFFFFF ) | ( ( Uint64 ) flags ) << 48;
		( ( Uint64& )m_data.word2 ) = collisionGroup & 0x0000FFFFFFFFFFFF;
	}

	CPhysicsEngine::CollisionMask GetCollisionType() { return  ( CPhysicsEngine::CollisionMask ) ( ( ( Uint64& )m_data.word0 ) & 0x0000FFFFFFFFFFFF ); }
	CPhysicsEngine::CollisionMask GetCollisionGroup() { return ( CPhysicsEngine::CollisionMask& )m_data.word2; }
	Uint16 GetFlags() { return ( ( ( Uint64& )m_data.word0 ) & 0xFFFF000000000000 ) >> 48; }
	void SetFlags( unsigned short flags ) { ( ( Uint64& )m_data.word0 ) = ( GetCollisionType() & 0x0000FFFFFFFFFFFF ) | ( ( Uint64 ) flags ) << 48; }

private:
	RED_INLINE void Construct( CPhysicalCollision& col )
	{
		CPhysicsEngine::CollisionMask collisionType;
		CPhysicsEngine::CollisionMask collisionGroup;
		col.RetrieveCollisionMasks( collisionType, collisionGroup );

		( ( Uint64& )m_data.word0 ) = collisionType & 0x0000FFFFFFFFFFFF;
		( ( Uint64& )m_data.word2 ) = collisionGroup & 0x0000FFFFFFFFFFFF;
	}
};

#ifdef USE_PHYSX

void RemoveSerializable( physx::PxBase* ser );

class CPhysXLogger : public physx::PxErrorCallback
{
#ifndef RED_FINAL_BUILD
	static String m_lastErrorString;
	static Red::System::Internal::ThreadId m_lastErrorThreadId;
#endif
public:
	virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line);

#ifndef RED_FINAL_BUILD
	static const String& GetLastErrorString() { return m_lastErrorString; }
	static Red::System::Internal::ThreadId GetLastErrorThreadId() { return m_lastErrorThreadId; }
	static Bool IsLastErrorFromSameThread() { return m_lastErrorThreadId == Red::System::Internal::ThreadId::CurrentThread(); }
	static void ClearLastError() { m_lastErrorString.Clear(); }
#endif
};

/// PhysX engine
class CPhysXEngine : public CPhysicsEngine
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_PhysicsEngine );

private:
	class CPhysXAllocator : public physx::PxAllocatorCallback
	{
	public:
		virtual void* allocate(size_t size, const char* typeName, const char* filename, int line);
		virtual void deallocate(void* ptr);
	};

	CPhysXAllocator m_allocator;

	physx::PxCooking* m_cooking;

	CPhysXLogger m_errorCallback;
	physx::PxDefaultCpuDispatcher* m_cpuDefaultDispatcher;
	class CPhysXCpuDispatcher* m_cpuDispatcherHighPriority;
	class CPhysXCpuDispatcher* m_cpuDispatcherCriticalPriority;
	physx::PxPhysics* m_physics;
	physx::PxFoundation* m_foundation;

	physx::PxSerializationRegistry* m_registry;
	physx::PxStringTable* m_stringTable;
	physx::PxCollection* m_physxMaterials;

#ifdef USE_PHYSX_GPU
	physx::PxCudaContextManager* m_cudaContext;
#endif

#ifdef PHYSICS_PROFILE
	physx::PxProfileZoneManager* m_profileZoneManager;
#endif

public:

	CPhysXEngine();
	~CPhysXEngine();

	Bool 			Init();
	void 			ShutDown();

	physx::PxMaterial*	GetMaterial( const char* name );
	physx::PxMaterial*	GetMaterial( const CName& name = CNAME( default ) );
	Uint32				GetMaterialCount();

	virtual CPhysicsWorld* CreateWorld( IPhysicsWorldParentProvider* parentProvider, Uint32 areaResolution, Float areaSize, Vector2 areaCornerPosition, Uint32 clipMapSize, Uint32 tileRes, Bool useMemorySettings, Bool criticalDispatch, Bool allowGpu ) override;
	virtual void DestroyWorld( CPhysicsWorld* world );

	RED_INLINE physx::PxCooking* GetCooking() const { return m_cooking; }

public:
	physx::PxCpuDispatcher* GetCpuDispatcher( Bool critical ) const;
	RED_INLINE physx::PxPhysics* GetPxPhysics() { return m_physics; }

	RED_INLINE physx::PxSerializationRegistry* GetSerializationRegistry() { return m_registry; }
	RED_INLINE physx::PxStringTable* GetStringTable() { return m_stringTable; }

#ifdef USE_PHYSX_GPU
	physx::PxCudaContextManager* GetCudaContext() const { return m_cudaContext; }
#endif

	void* CreateMaterialInstance( Float dynamicFriction, Float staticFriction, Float restitution, void* engineDefinition );
	TDynArray< physx::PxMaterial* > GetMaterialsArray();
	const physx::PxCollection* GetMaterialsCollection() { return m_physxMaterials; }

	void FlushMaterialInstances();
	void GetTimings( Float& physicsFetchTime, Float& physicsClothFetchTime, Uint32& clothAttached, Uint32& clothSimulated );

};

#ifndef NO_EDITOR
void UpdatePhysxSettings();
#endif

struct RepXImporterParams : public IImporter::ImportOptions::ImportParams
{
	RepXImporterParams()
		: m_isRagdoll( false )
	{
	}
	bool m_isRagdoll;
};

// Physics System instance
extern CPhysXEngine* GPhysXEngine;

#endif

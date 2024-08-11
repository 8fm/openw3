/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/hashset.h"
#include "../core/parallelFor.h"
#include "cameraDirector.h"
#include "meshSkinningAttachment.h"
#include "scaleable.h"
#include "particleComponent.h"

class CEntity;
class UpdateTransformContext;
class IRenderSkinningData;
enum ERenderLightParameter : Uint32;

//////////////////////////////////////////////////////////////////////////

#define UPDATE_CONTEXT_BATCH_SIZE 64

struct RenderProxyRelinkInfo
{
	Box		m_bbox;
	Matrix	m_transform;
	Bool	m_hasBBox;
};

struct SUpdateParticlesBatchedCommand
{
	Bool					  m_relink;
	IRenderProxy*			  m_renderProxy;
	Matrix					  m_localToWorld;
	SSimulationContextUpdate  m_simulationContext;
};

struct SMeshSkinningUpdateContext
{
	Uint8							m_relinkCommandCount;
	Uint8							m_simulationContextUpdateCommandCount;

	IRenderProxy*					m_proxies[UPDATE_CONTEXT_BATCH_SIZE];
	Bool							m_transformOnlies[UPDATE_CONTEXT_BATCH_SIZE];
	IRenderObject*					m_skinningData[UPDATE_CONTEXT_BATCH_SIZE];
	RenderProxyRelinkInfo			m_relinkInfos[UPDATE_CONTEXT_BATCH_SIZE];
	SUpdateParticlesBatchedCommand 	m_particleUpdateInfos[UPDATE_CONTEXT_BATCH_SIZE];

	SMeshSkinningUpdateContext()
		: m_relinkCommandCount( 0 )
		, m_simulationContextUpdateCommandCount( 0 )
	{
		Red::MemoryZero( m_particleUpdateInfos, sizeof(m_particleUpdateInfos) );
	}

	void AddCommand_Relink( IRenderProxy* renderProxy, RenderProxyUpdateInfo updateInfo );
	void AddCommand_SkinningDataAndRelink( IRenderProxy* renderProxy, IRenderSkinningData* renderSkinningData, RenderProxyUpdateInfo updateInfo );
	void AddCommand_ParticlesRelink( IRenderProxy* renderProxy, const RenderProxyUpdateInfo& updateInfo, const Vector3& windAtPosVector, const Vector3& windOnlyAtPosVector );
	void AddCommand_UpdateParticlesSimulatationContext( IRenderProxy* renderProxy, const SSimulationContextUpdate& simulationContext );
	void AddCommand_UpdateLightProxyParameter( IRenderProxy* renderProxy, const Vector& data, ERenderLightParameter param );

	void CommitGatheredUpdateSimulationContextCommands();
	void CommitGatheredRelinkCommands();

	void CommitCommands();
};

struct SUpdateTransformContext
{
	const SCameraVisibilityData*	m_cameraVisibilityData;
	SMeshSkinningUpdateContext		m_skinningContext;
	Bool							m_guardFlag;

	SUpdateTransformContext();
	~SUpdateTransformContext();

	void CommitChanges();
};

//////////////////////////////////////////////////////////////////////////

// Tick group
/// SHOULD ALWAYS MATCH THE EComponentTickMask
enum ETickGroup : CEnum::TValueType
{
	TICK_PrePhysics,
	TICK_PrePhysicsPost,
	TICK_Main,
	TICK_PostPhysics,
	TICK_PostPhysicsPost,
	TICK_PostUpdateTransform,

	// Number of tickgroups
	TICK_Max,
};

BEGIN_ENUM_RTTI( ETickGroup );
ENUM_OPTION( TICK_PrePhysics );
ENUM_OPTION( TICK_PrePhysicsPost );
ENUM_OPTION( TICK_Main );
ENUM_OPTION( TICK_PostPhysics );
ENUM_OPTION( TICK_PostPhysicsPost );
ENUM_OPTION( TICK_PostUpdateTransform );
END_ENUM_RTTI();

//#define DEBUG_TRANS_MGR

/// Update transform stat reporter
class IUpdateTransformStatCollector
{
public:
	virtual ~IUpdateTransformStatCollector() {};

	//! Collect stats for this component
	virtual void CollectStats( CNode* node, ETickGroup tickGroup, Uint32 bucket, Uint64 timeTook )=0;
};

/// Manager for transform hierarchy
class CUpdateTransformManager
{
public:
	typedef CParallelForTaskSingleArray< CEntity*, CUpdateTransformManager >::SParams		TEntityTaskParam;

#ifdef DEBUG_TRANS_MGR
public:
	Red::Threads::CAtomic< Int32 >		m_numSimple;
	Red::Threads::CAtomic< Int32 >		m_numFast;
	Red::Threads::CAtomic< Int32 >		m_numRest;
	Red::Threads::CAtomic< Int32 >		m_numEntities;
	Red::Threads::CAtomic< Int32 >		m_numNodes;

	struct InternalData
	{
		const CClass*	m_class;
		Int32			m_count;

		InternalData() : m_class( nullptr ), m_count( 0 ) {}
		InternalData( const CClass* c, Int32 count ) : m_class( c ), m_count( count ) {}
	};
#endif

	Red::Threads::CLightMutex			m_lock;

public:
	CUpdateTransformManager();
	~CUpdateTransformManager();

	// Schedule transform update for given entity
	void ScheduleEntity( CEntity* entity );

	// Unschedule transform update for given entity
	void UnscheduleEntity( CEntity* entity );

	// Check if entity is scheduled - use it only for debug
	Bool HasEntityScheduled( CEntity* entity ) const;

public:
	// Update transformations
	void UpdateTransforms_StartAndWait( UpdateTransformContext& context );

	void UpdateTransforms_Start( UpdateTransformContext& context );
	void UpdateTransforms_WaitForFinish( UpdateTransformContext& context );

	// Used on game exit
	void ClearSchedule();

	// Update one node
	void UpdateOneEntity( CEntity*& entity );

protected:
	THashSet< CEntity* >	m_updateSetEntities;
	TDynArray< CEntity* >	m_updateListEntities;
	Bool					m_isUpdatingTransforms;

	const SCameraVisibilityData*	m_cameraVisibilityData;

private:
	static const Uint32 BUCKET_SIZE_THREADING_THRESHOLD = 4;

	void PrepareList();
	void ProcessList( Bool allowThreads, Bool blockMainThread, UpdateTransformContext& context );

	void DispatchSingleThreaded();
	void WaitForFinish( UpdateTransformContext& context );

	void CollectDebugData();
};

/// Update transform context
class UpdateTransformContext
{
	friend class CUpdateTransformManager;

	CUpdateTransformManager::TEntityTaskParam*	m_entitiesTask;

public:
	ETickGroup							m_relatedTickGroup;		//!< Related tick group for this update transform
	bool								m_canUseThreads;		//!< Can we use the job threads to distribute work
	IUpdateTransformStatCollector*		m_statCollector;		//!< Stat collector
	SCameraVisibilityData				m_cameraVisibilityData;

public:
	UpdateTransformContext()
		: m_relatedTickGroup( TICK_PostPhysicsPost )
		, m_canUseThreads( false )
		, m_statCollector( nullptr )
		, m_entitiesTask( nullptr )
	{};
};

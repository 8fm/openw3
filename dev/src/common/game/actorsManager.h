#pragma once

#include "binaryStorage.h"
#include "gameplayStorage.h"

class CStrayActorManager;

//! Actor/instance specific LOD information
struct SActorLODInstanceInfo
{
	Bool		m_hasCollisionDataAround;
	Bool		m_hasNavigationDataAround;
	Float		m_distanceSqr;

	RED_FORCE_INLINE SActorLODInstanceInfo()
		: m_hasCollisionDataAround( true )
		, m_hasNavigationDataAround( true )
		, m_distanceSqr( 0.0f )
	{}
};

struct CActorsManagerMemberData
{
	CActor*		m_actor;

	RED_INLINE CActor* Get() const											{ return m_actor; }
	RED_INLINE Bool operator==( const CActorsManagerMemberData& other ) const	{ return m_actor == other.m_actor; }
	RED_INLINE operator CActor* () const										{ return m_actor; }

};

///////////////////////////////////////////////////////////////////////////////
// Storage allowing to quickly find nearby actors.
class CActorsManager : public CQuadTreeStorage< CActor, CActorsManagerMemberData >
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

protected:
	//! LOD description with deadzone (to minimize frequent LOD toggling)
	struct SActorLODConfig_Internal : SActorLODConfig
	{
		// Cached square deadzone range
		Float			m_deadZoneStartSqr;
		Float			m_deadZoneEndSqr;

		void CacheSqrDistances();
	};

	TDynArray< SActorLODConfig_Internal >	m_lods;
	Float									m_invisibilityTimeThreshold; // Time after which an actor is considered "invisible for a long time"
	Vector									m_referencePosition;
	Bool									m_referencePositionValid;
	SActorLODConfig_Internal*				m_forcedLOD;

	CWorld*									m_world;
	CPathLibWorld*							m_pathLibWorld;
	CPhysicsWorld*							m_physicsWorld;

public:
	CActorsManager( const SGameplayConfig* config );
	~CActorsManager();

	void OnGameStart( CWorld* world );
	void OnReloadedConfig( const SGameplayConfig* config );

	void Update( Float deltaTime );

	void Add( CActor* actor );
	void Remove( CActor* actor );

	Bool TestLocation( const Vector& pos, Float radius, CActor* ignoreActor = NULL );
	Bool TestLine( const Vector& posFrom, const Vector& posTo, Float radius, CActor* ignoreActor = NULL, Bool ignoreGhostCharacters = false );
	Bool TestLine( const Vector& posFrom, const Vector& posTo, Float radius, CActor** ignoreActor, Uint32 ignoredActorsCount, Bool ignoreGhostCharacters = false );

	//returns amount of found actors
	Int32 CollectActorsAtLine( const Vector& posFrom, const Vector& posTo, Float radius, CActor** outputArray, Int32 maxElems );

	// overlap test using custom functor
	template < typename Functor >
	void CollectActorsOverlapTest( const Vector& position, Float radius, Functor& functor )
	{
		Float testRadius = radius + MAX_AGENT_RADIUS;

		Box bbox( Box::RESET_STATE );
		bbox.AddPoint( Vector( testRadius, testRadius, testRadius, 0.0f ) );
		bbox.AddPoint( Vector( -testRadius, -testRadius, -testRadius, 0.0f ) );

		TQuery( position, functor, bbox, true, NULL, 0 );
	}

	// mirror of gameplay storage interface
	void GetClosestToNode( const CNode& node, TDynArray< TPointerWrapper< CActor > >& output, const Box& aabb, Uint32 maxElems, const INodeFilter** filters= NULL, const Uint32 numFilters = 0 );
	void GetClosestToEntity( const CActor& actor, TDynArray< TPointerWrapper< CActor > >& output, const Box& aabb, Uint32 maxElems, const INodeFilter** filters = NULL, const Uint32 numFilters = 0 );
	void GetClosestToPoint( const Vector& position, TDynArray< TPointerWrapper< CActor > >& output, const Box& aabb, Uint32 maxElems, const INodeFilter** filters = NULL, const Uint32 numFilters = 0 );
	void GetAll( TDynArray< TPointerWrapper< CActor > >& output, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters );

	// LOD management
	void SetReferencePosition( const Vector& pos ) { m_referencePosition = pos; m_referencePositionValid = true; }
	void UpdateLODForActor( CActor* actor, Float deltaTime );
	void EnableForcedLOD( Bool enable, Uint32 index = 0 );
	void UpdateLODs( Float deltaTime );	// NOTE: Invoked every frame via Update()
private:
	void SetupLODs();
};
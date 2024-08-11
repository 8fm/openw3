/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/refCountPointer.h"

#include "abstractPathRater.h"
#include "pathlibSearchEngine.h"
#include "pathlibConst.h"
#include "pathlibMetalinkComponent.h"


#ifndef NO_EDITOR_FRAGMENTS
#define PATHLIB_AGENT_DEBUG_PATHFINDING 1
#endif


class CPathLibWorld;


namespace PathLib
{

class CAreaDescription;

////////////////////////////////////////////////////////////////////////////
// IReachabilityDataBase
// Common data object for pathfinding and reachability queries
////////////////////////////////////////////////////////////////////////////
class IReachabilityDataBase
{
	friend class IReachabilityTaskBase;
public:
	enum EPathfindFailureReason
	{
		PATHFAIL_NOT_YET_PATHFIND,
		PATHFAIL_OK,
		PATHFAIL_WITH_TOLERANCE,
		PATHFAIL_INVALID_STARTING_POS,
		PATHFAIL_INVALID_DESTINATION_POS,
		PATHFAIL_OUT_OF_NAVDATA,
		PATHFAIL_NO_NAVGRAPH,
		PATHFAIL_NO_STARTING_NODE,
		PATHFAIL_DESTINATION_OUT_OF_NAVDATA,
		PATHFAIL_DESTINATION_NO_NAVGRAPH,
		PATHFAIL_NO_DESTINATION_NODE,
		PATHFAIL_NO_PATH,
		PATHFAIL_FAILED_PLOTPATH,
	};
protected:
	typedef			Red::Threads::CAtomic< Int32 >							RefCount;
	typedef			Red::Threads::CAtomic< Bool >							AtomicBool;

	RefCount		m_refCount;
	Float			m_personalSpace;
	Uint8			m_agentCategory;
	Bool			m_outputPathUseTolerance;
	AreaId			m_currentArea;
	CollisionFlags	m_defaultCollisionFlags;
	NodeFlags		m_forbiddenPathfindFlags;
	Vector3			m_searchOrigin;
	Float			m_searchTolerance;

#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
	Bool					m_failedRecently;
	EPathfindFailureReason	m_failureReason : 8;
	Vector3					m_lastFailedPathfindingDestination;
#endif

	virtual			~IReachabilityDataBase();
public:
	typedef			TRefCountPointer< IReachabilityDataBase >				Ptr;

	IReachabilityDataBase( Float personalSpace = 0.4f );

	void						Initialize( CPathLibWorld* pathlib, Float personalSpace );
	void						Setup( Uint32 agentCategory, Float personalSpace ) { m_agentCategory = agentCategory; m_personalSpace = personalSpace; }

	Bool						TestLine( CPathLibWorld* world, const Vector3& pos1, const Vector3& pos2, CollisionFlags collisionFlags = CT_NO_ENDPOINT_TEST );
	Bool						TestLine( CPathLibWorld* world, const Vector3& pos1, const Vector3& pos2, Float personalSpace, CollisionFlags collisionFlags = CT_NO_ENDPOINT_TEST );
	Bool						TestLocation( CPathLibWorld* world, const Vector3& pos, CollisionFlags collisionFlags = CT_NO_ENDPOINT_TEST );
	Bool						TestLocation( CPathLibWorld* world, const Vector3& pos, Float radius, CollisionFlags collisionFlags = CT_NO_ENDPOINT_TEST );

	Uint32						GetAgentCategory() const					{ return m_agentCategory; }
	Uint32						GetCollisionFlags() const					{ return m_defaultCollisionFlags; }
	NodeFlags					GetForbiddenPathfindFlags() const			{ return m_forbiddenPathfindFlags; }
	Float						GetPersonalSpace() const					{ return m_personalSpace; }
	RED_INLINE AreaId			GetCachedAreaId() const						{ return m_currentArea; }
	AreaId&						CacheAreaId()								{ return m_currentArea; }

	void						AddCollisionFlags( Uint32 flags )			{ m_defaultCollisionFlags |= flags; }
	void						RemoveCollisionFlags( Uint32 flags )		{ m_defaultCollisionFlags &= ~flags; }

	void						AddForbiddenPathfindFlag( ENodeFlags flag )	{ m_forbiddenPathfindFlags |= flag; }
	void						RemoveForbiddenPathfindFlag( ENodeFlags flag ) { m_forbiddenPathfindFlags &= ~flag; }

	void						AddRef()									{ m_refCount.Increment(); }
	void						Release()									{ if (m_refCount.Decrement() <= 0 ) { delete this; } }

#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
	void						Debug_PathfindOutcome( String& outOutcome );
#endif
};

////////////////////////////////////////////////////////////////////////////
// CReachabilityData
// Data object for 'is point reachable' queries
////////////////////////////////////////////////////////////////////////////
class CReachabilityData : public IReachabilityDataBase
{
	friend class CReachabilityTask;
protected:			// private interface used for asynchronous pathfinding task processing
	AtomicBool		m_isAsyncTaskRunning;
	Bool			m_asyncTaskResultPending;
	Bool			m_asyncTaskInvalidated;
	EPathfindResult	m_asyncTaskResult : 8;

	Bool			m_outputSuccess;
	Bool			m_lastOutputSuccessful;
	Vector3			m_outputDestination;
	Vector3			m_lastOutputSource;
	Vector3			m_lastOutputDestination;
	Vector3			m_searchDestination;
	

	~CReachabilityData();
public:
	typedef			TRefCountPointer< CReachabilityData >					Ptr;

	CReachabilityData( Float personalSpace = 0.4f );

	EPathfindResult			QueryReachable( CPathLibWorld* world, const Vector3& startingPos, const Vector3& destinationPos, Float tolerance = 0.f, Vector3* outDestinationPosition = nullptr );

	const Vector3&			GetSearchDestination() const					{ return m_searchDestination; }
	RED_INLINE Bool			IsAsyncTaskRunning() const						{ return m_isAsyncTaskRunning.GetValue(); }
};

////////////////////////////////////////////////////////////////////////////
// CMultiReachabilityData
// Data object for 'are points reachable' queries
////////////////////////////////////////////////////////////////////////////
class CMultiReachabilityData : public IReachabilityDataBase
{
	typedef IReachabilityDataBase Super;
	friend class CMultiReachabilityTask;
public:
	enum EQueryType
	{
		REACH_ANY,
		REACH_ALL,
		REACH_FULL
	};

protected:			// private interface used for asynchronous pathfinding task processing
	struct Destination
	{
		Destination( const Vector& v, CEntity* entity = nullptr )
			: m_dest( v )
			, m_result( PATHRESULT_PENDING )
			, m_originNode( nullptr )
			, m_destinationNode( nullptr )
			, m_entity( entity )												{}

		Vector3			m_dest;
		EPathfindResult	m_result;
		PathCost		m_destDistance;
		// NOTICE: that stuff is only meant to be used only during the search itself
		CSearchNode*	m_originNode;
		CSearchNode*	m_destinationNode;
		THandle< CEntity >	m_entity;	// just keep the reference for target identyfication
	};

	AtomicBool		m_isAsyncTaskRunning;
	Bool			m_asyncTaskResultPending;
	Bool			m_asyncTaskInvalidated;
	EPathfindResult	m_asyncTaskResult : 8;

	EQueryType		m_queryType : 8;
	Bool			m_outputSuccess;
	Bool			m_lastOutputSuccessful;
	Bool			m_usePathfindDistanceLimit;
	Bool			m_computeClosestTargetDistance;
	Bool			m_correctDestinationPositions;
	Float			m_pathfindDistanceLimit;
	Vector3			m_outputDestination;
	Vector3			m_lastOutputSource;
	Int32			m_closestTargetIdx;
	Float			m_closestTargetDistance;
	TDynArray< Destination >	m_searchDestinations;
	

	~CMultiReachabilityData();

	void					Initialize( CPathLibWorld* pathlib, Float personalSpace ) { Super::Initialize( pathlib, personalSpace ); }
	void					SetCollisionFlags( CollisionFlags flags )		{ m_defaultCollisionFlags = flags; }
public:
	typedef			TRefCountPointer< CMultiReachabilityData >				Ptr;

	struct QuerySetupInterface
	{
		friend class CMultiReachabilityData;
	protected:
		CMultiReachabilityData*				m_data;

		QuerySetupInterface( CMultiReachabilityData* data = nullptr )
			: m_data( data )												{}
	public:

		void				SetPersonalSpace( CPathLibWorld* pathlib, Float personalSpace ) const { m_data->Initialize( pathlib, personalSpace ); }
		Bool				IsValid() const									{ return m_data != nullptr; }
		void				ReserveDestinations( Uint32 n ) const			{ m_data->m_searchDestinations.Reserve( n ); }
		Uint32				AddDestination( const Vector3& v ) const		{ m_data->m_searchDestinations.PushBack( Destination( v ) ); return m_data->m_searchDestinations.Size()-1; }
		Uint32				AddDestination( CEntity* entity ) const			{ m_data->m_searchDestinations.PushBack( Destination( entity->GetWorldPositionRef().AsVector3(), entity ) ); return m_data->m_searchDestinations.Size()-1; }
		void				SetCollisionFlags( CollisionFlags flags ) const	{ m_data->SetCollisionFlags( flags ); }
		void				SetPathfindDistanceLimit( Float limit ) const	{ m_data->m_usePathfindDistanceLimit = true; m_data->m_pathfindDistanceLimit = limit; }
		void				RequestClosestTargetComputation() const			{ m_data->m_computeClosestTargetDistance = true; }
		void				CorrectDestinationPositions() const				{ m_data->m_correctDestinationPositions = true; }
	};

	CMultiReachabilityData( Float personalSpace = 0.4f );

	QuerySetupInterface		SetupReachabilityQuery( EQueryType queryType, const Vector3& startingPos, NodeFlags nf = NFG_FORBIDDEN_BY_DEFAULT );

	EPathfindResult			QueryReachable( CPathLibWorld* world );

	RED_INLINE Bool			IsAsyncTaskRunning() const						{ return m_isAsyncTaskRunning.GetValue(); }

	Bool					GetOveralOutcome() const						{ return m_lastOutputSuccessful; }
	EPathfindResult			GetDestinationOutcome( Uint32 pointIndex ) const{ return m_searchDestinations[ pointIndex ].m_result; }
	const THandle< CEntity >& GetDestinationEntity( Uint32 pointIndex ) const { return m_searchDestinations[ pointIndex ].m_entity; }

	Int32					GetClosestTargetIdx() const						{ return m_closestTargetIdx; }
	Float					GetClosestTargetDistance() const				{ return m_closestTargetDistance; }
};

////////////////////////////////////////////////////////////////////////////
// CSearchData object is interface to pathfinding system and storage of
// calculated path.
// Its also possibly proxy object between pathfinding system
// and asyncronous gameplay requests
////////////////////////////////////////////////////////////////////////////
class CSearchData : public IReachabilityDataBase
{
	friend class CTaskPlotPath;
public:
	typedef Uint8 ClassId;													// simple rtti
	enum PathlibAgents
	{
		CLASS_CSearchData													= 0,
		CLASS_CAgent														= FLAG( 0 )
	};
	static const ClassId CLASS_ID = CLASS_CSearchData;

	struct MetalinkInteraction
	{
		IMetalinkSetup::RuntimeData m_metalinkRuntime;
		IMetalinkSetup::Ptr		m_metalinkSetup;
		Uint32					m_waypointIndex;
		Uint32					m_metalinkFlags;
		Vector3					m_position;
		Vector3					m_destination;
	};
	

protected:
	typedef						TDynArray< Vector3 >						Waypoints;
	typedef						TDynArray< MetalinkInteraction >			Metalinks;

protected:					// private interface used for asynchronous pathfinding task processing
	Vector3						m_searchDestination;
	AtomicBool					m_isAsyncTaskRunning;
	Bool						m_asyncTaskResultPending;
	Bool						m_asyncTaskInvalidated;
	EPathfindResult				m_asyncTaskResult : 8;

protected:
	Bool						m_forcePathfindingInTrivialCase;
	Bool						m_findClosestSpotPathfindingEnabled;
	Bool						m_doHeavyPathOptimization;
	Bool						m_outputPathDestinationWasOutsideOfStreamingRange;
	ClassId						m_classMask;
	Float						m_closestSpotPathfindingDistanceLimit;
	IPathRater::Ptr				m_pathRater;
	Waypoints					m_outputWaypoints;
	Metalinks					m_outputMetalinksStack;

	// simplifies path (optimizes node count)
	Bool						SimplifyPath( CPathLibWorld* world, const Vector3* inWaypoints,Uint32 inWaypointsCount,Vector3* outWaypoints, Uint32& outWaypointsCount, Uint32 waypointsCountLimit );
	// version that is much more heavy, but produce 'optimal' walkable path
	void						OptimizePath( CPathLibWorld* world, Vector3* inOutWaypoints,Uint32 inOutWaypointsCount );
	// process output path with simplification&optimization algorithms
	void						PostProcessCollectedPath( CPathLibWorld* world, Uint32& pathToSimplifyBeginIndex );

	virtual ~CSearchData();
public:
	typedef						TRefCountPointer< CSearchData >				Ptr;

	CSearchData( Float personalSpace = 0.4f, ClassId classMask = CLASS_CSearchData );

	Uint32						GetOutputWaypointsCount() const				{ return m_outputWaypoints.Size(); }
	const Vector3&				GetOutputWaypoint( Uint32 waypoint ) const	{ return m_outputWaypoints[ waypoint ]; }
	const Vector3&				GetSearchDestination() const				{ return m_searchDestination; }

	EPathfindResult				PlotPath( CPathLibWorld* world, const Vector3& startingPos, const Vector3& destinationPos, Float tolerance = 0.f );
	Bool						UpdatePathDestination( CPathLibWorld* world, const Vector3& startingPos, const Vector3& newDestination );

	Bool						CollectPath( CPathLibWorld* world, CQueryData& query, Bool includeDestination = true );

	RED_INLINE IPathRater*		GetPathRater() const						{ return m_pathRater.Get(); }
	RED_INLINE void				SetPathRater( IPathRater* rater )			{ m_pathRater = rater; }
	RED_INLINE Bool				IsAsyncTaskRunning() const					{ return m_isAsyncTaskRunning.GetValue(); }

	void						EnableClosestSpotPathfinding( Bool b )		{ m_findClosestSpotPathfindingEnabled = b; }
	void						EnableHeavyPathOptimization( Bool b )		{ m_doHeavyPathOptimization = b; }

	void						SetClostPathfindingDistanceLimit( Float f  ){ m_closestSpotPathfindingDistanceLimit = f; }

	RED_INLINE void				ForcePathfindingInTrivialCases( Bool force ){ m_forcePathfindingInTrivialCase = force; }

	template < class TSubClass >
	RED_INLINE TSubClass*		As()										{ return ( ( m_classMask & TSubClass::CLASS_ID ) == TSubClass::CLASS_ID ) ? static_cast< TSubClass* >( this ) : nullptr; }
};



};			// namespace PathLib


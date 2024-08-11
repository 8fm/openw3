/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/priqueue.h"
#include "../../common/core/loadingJob.h"

////////////////////////////////////////////////////////////////////////
// Volume Path Manager (deals with path finding in the air and under water)
////////////////////////////////////////////////////////////////////////
class CVolumePathManager : public IGameSystem
{
	DECLARE_ENGINE_CLASS( CVolumePathManager, IGameSystem, 0 );

public:
	// int based coordinate
	struct Coordinate
	{
	public:
		Coordinate()
		{}

		Coordinate( Int32 _x, Int32 _y, Int32 _z )
			: x( _x )
			, y( _y )
			, z( _z )
		{
		}

		Uint32 GetHash() const;
		Coordinate GetNeighborCoordinate( Uint32 direction ) const;

		Coordinate& operator+( const Coordinate& c ) { x += c.x; y += c.y; z += c.z; return *this; }
		Coordinate& operator-( const Coordinate& c ) { x -= c.x; y -= c.y; z -= c.z; return *this; }
		Bool operator==( const Coordinate& c ) const { return x == c.x && y == c.y && z == c.z; }
		Bool operator!=( const Coordinate& c ) const { return x != c.x || y != c.y || z != c.z; }
	public:
		Int32		x, y, z;
	};

	enum ENeighbouringFlags
	{
		NODE_AVAILABLE = FLAG(13),

		// Useful flag to check if all neighbors in 
		// m_availableNeighbors is available (or not)
		ALL_DIRECTIONS = FLAG( 0 ) |		// -1, -1, -1
						FLAG( 1 ) |			//  0, -1, -1
						FLAG( 2 ) |			//  1, -1, -1
						FLAG( 3 ) |			// -1,  0, -1
						FLAG( 4 ) |			//  0,  0, -1
						FLAG( 5 ) |			//  1,  0, -1
						FLAG( 6 ) |			// -1,  1, -1
						FLAG( 7 ) |			//  0,  1, -1
						FLAG( 8 ) |			//  1,  1, -1
						FLAG( 9 ) |			// -1, -1,  0
						FLAG( 10 ) |		//  0, -1,  0
						FLAG( 11 ) |		//  1, -1,  0
						FLAG( 12 ) |		// -1,  0,  0
						// SKIPPING FLAG 13 (0, 0, 0)
						FLAG( 14 ) |		//  1,  0,  0
						FLAG( 15 ) |		// -1,  1,  0
						FLAG( 16 ) |		//  0,  1,  0
						FLAG( 17 ) |		//  1,  1,  0
						FLAG( 18 ) |		// -1, -1,  1
						FLAG( 19 ) |		//  0, -1,  1
						FLAG( 20 ) |		//  1, -1,  1
						FLAG( 21 ) |		// -1,  0,  1
						FLAG( 22 ) |		//  0,  0,  1
						FLAG( 23 ) |		//  1,  0,  1
						FLAG( 24 ) |		// -1,  1,  1
						FLAG( 25 ) |		//  0,  1,  1
						FLAG( 26 )			//  1,  1,  1
	};

	// node used to describe a voxel
	struct Node
	{
	public:
		enum 
		{
			NEIGHBOR_X_POS,
			NEIGHBOR_X_NEG,
			NEIGHBOR_Y_POS,
			NEIGHBOR_Y_NEG,
			NEIGHBOR_Z_POS,
			NEIGHBOR_Z_NEG,
			NEIGHBOR_COUNT
		};

	public:
		Coordinate	m_position;								// 12
		Vector3		m_worldPosition;						// 24
		Float		m_distToPlayerSq;						// 28
		Vector3		m_repulsionVector;						// 40
		Bool		m_available;							// 41
		Bool		m_hasRepultionVector;					// 42
		Uint32		m_availableNeighbors;					// 48
	};

	// Data used for path finding
	struct PathNode
	{
		Node*		m_node;									// 8
		PathNode*	m_parent;								// 16
		Bool		m_open;									// 17
		Bool		m_closed;								// 18
		Float		m_distFromStart;						// 24
		Float		m_distToGoal;							// 28
		Float		m_totalCost;							// 32
	};

	class IRequest : public Red::System::NonCopyable
	{
		friend class CVolumePathManager;
	protected:
		CVolumePathManager*		m_pathManager;
		Node*					m_startingNode;
		
	public:
		enum EWaterSetup
		{
			WATER_ABOVE_AND_BELOW,
			WATER_ABOVE,
			WATER_BELOW
		};

		typedef TDynArray<Vector3>& Result;
		
		Vector3					m_start;
		Vector3					m_end;
		EWaterSetup				m_waterSetup;

		TDynArray<Vector3>&		m_result;

		void GenerateResultPath( PathNode* endNode );

		EWaterSetup GetWaterSetup() const										{ return m_waterSetup; }

		IRequest( const Vector3& start, const Vector3& end, Result& result, EWaterSetup waterSetup )
			: m_start( start )
			, m_end( end )
			, m_waterSetup( waterSetup )
			, m_result( result )												{ m_result.ClearFast(); }
		
		virtual Bool BeginSearch();
		virtual Bool EndSearch()												= 0;

		virtual Bool ProcessNode( PathNode* node )								= 0;
		virtual Bool ProcessNeighbour( Node* node ) = 0;
	};

	class CPreciseRequest : public IRequest
	{
		typedef IRequest Super;
	protected:
		Node*					m_endingNode;
		PathNode*				m_resultNode;
	public:
		CPreciseRequest( const Vector3& start, const Vector3& end, Result& result, EWaterSetup waterSetup )
			: Super( start, end, result, waterSetup )										{}

		Bool BeginSearch() override;
		Bool EndSearch() override;

		Bool ProcessNode( PathNode* node ) override;
		Bool ProcessNeighbour( Node* node ) override;
	};

	class CClosestAcceptablePointRequest : public CPreciseRequest
	{
	private:
		typedef CPreciseRequest Super;
	protected:
		Float		m_acceptableDistance;
	public:
		CClosestAcceptablePointRequest( const Vector3& start, const Vector3& end, Result& result, EWaterSetup waterSetup, Float acceptDistance )
			: Super( start, end, result, waterSetup )
			, m_acceptableDistance( acceptDistance )							{}

		Bool ProcessNode( PathNode* node ) override;
		Bool ProcessNeighbour( Node* node ) override;

		Float GetResultDistanceFromDesiredPosition() const						{ return m_acceptableDistance; }
	};

	class CClosestAcceptablePointInBoundingsRequest : public CClosestAcceptablePointRequest
	{
	private:
		typedef CClosestAcceptablePointRequest Super;

	public:
		Box			m_boundings;

		CClosestAcceptablePointInBoundingsRequest( const Vector3& start, const Vector3& end, Result& result, EWaterSetup waterSetup, Float acceptDistance, const Box& box )
			: Super( start, end, result, waterSetup, acceptDistance )
			, m_boundings( box )												{}

		Bool ProcessNeighbour( Node* node ) override;
	};


protected:


	// job for recycling nodes
	class CNodeRecycleJob : public ILoadJob	
	{
	friend class CVolumePathManager;
	protected:
		CNodeRecycleJob( CVolumePathManager* volumeManager, int numNodesToRecycle, const Vector& referenceSpot );
		EJobResult Process() override;
		const TDynArray<Node*>& GetResult()			{ return m_nodesToRecycle; }

	public:
		const Char* GetDebugName() const override	{ return TXT("VolumePath"); }

	protected:
		CVolumePathManager* m_volumeManager;
		Uint32				m_numNodesToRecycle;
		TDynArray<Node*>	m_nodesToRecycle;
		Vector				m_referenceSpot;
	};

	// struct for comparing 2 path nodes (distance to goal)
	struct SPathNodeComperator
	{
		static RED_INLINE Bool Less( const PathNode* a, const PathNode* b )	{ return b->m_totalCost < a->m_totalCost; }
	};

	RED_FORCE_INLINE void ResetPathNodes();
	RED_FORCE_INLINE PathNode* AllocatePathNode( Node* node, Float distFromStart, Float distToGoal, PathNode* parentNode = nullptr );

public:
	CVolumePathManager();
	CVolumePathManager( Uint32 numNodes, Bool enablePathfinding );
	~CVolumePathManager();

	virtual void Initialize() override;
	virtual void Shutdown() override;

	virtual void OnGameStart( const CGameInfo& gameInfo );
	virtual void OnGameEnd( const CGameInfo& gameInfo );

	virtual void Tick( Float timeDelta );

	virtual void OnGenerateDebugFragments( CRenderFrame* frame );

	// get volume path
	Bool GetPath( const Vector3& start, const Vector3& end, TDynArray<Vector3>& result, Float maxHeight = FLT_MAX );
	
	// plot path
	Bool PlotPath( IRequest& request );

	// simplify a path
	Bool SimpifyPath( TDynArray<Vector3>& path, TDynArray<Vector3>& simplePath ) const;

	// check to see if we need pathfinding
	Bool IsPathfindingNeeded( const Vector3& start, const Vector3& end ) const;

	// Get a repulsion vector from surrounding nodes
	Bool GetCollisionRepulsion( const Vector3& position, Vector3& repulsionVector, Uint32& availableNeighbors );

	// is a node blocked or not?
	Bool IsNodeBlocked( const Vector3& position );

	// get 3D direction flag
	static Uint32 GetDirectionFlag( const Coordinate& c );
	static Uint32 GetDirectionFlag( Int32 x, Int32 y, Int32 z );

	// helpers: position to volume coordinate and vice versa
	Coordinate CoordinateFromPosition( const Vector3& position ) const; 
	Vector3 PositionFromCoordinate( const Coordinate& coordinate ) const;
protected:

	// clear all nodes
	void Clear();

	// get cached node (or create a new one)
	Node* RequestNodeAt( const Coordinate& coordinate );

	// recycle nodes far away from player (TODO: make into a Job)
	void RecycleNodes( Uint32 numNodes );

private:
	void funcGetPath( CScriptStackFrame& stack, void* result );
	void funcGetPointAlongPath( CScriptStackFrame& stack, void* result );
	void funcIsPathfindingNeeded( CScriptStackFrame& stack, void* result );

protected:

	Node*										m_nodes;
	PathNode*									m_pathNodes;
	Uint32										m_numNodes;
	Uint32										m_nextPathNode;
	Float										m_nodeSize;
	Bool										m_enablePathfinding;
	THashMap<Uint32, Node*>						m_usedNodes;
	TDynArray<Node*>							m_availableNodes;
	THashMap<Node*, PathNode*>					m_pathNodeLookupMap;

	TDynArray<Vector3>							m_debugPath;
	TDynArray<Vector3>							m_debugSimplePath;
	TPriQueue<PathNode*, SPathNodeComperator>	m_openQueue;

	CNodeRecycleJob*							m_recycleJob;

	ASSIGN_GAME_SYSTEM_ID( GS_VolumePathManager )
};

BEGIN_CLASS_RTTI( CVolumePathManager );
	PARENT_CLASS( IGameSystem );
	NATIVE_FUNCTION( "GetPath", funcGetPath );
	NATIVE_FUNCTION( "GetPointAlongPath", funcGetPointAlongPath );
	NATIVE_FUNCTION( "IsPathfindingNeeded", funcIsPathfindingNeeded );
END_CLASS_RTTI();

#pragma once
#include "../core/mathUtils.h"


//#define EXPLORATION_DEBUG

class IExploration;
class IExpExecutor;
class IExplorationDesc;
class ExpPlayer;
class CEntity;
class CRenderFrame;

enum ECharacterPhysicsState : CEnum::TValueType;

enum EExplorationType
{
	ET_Jump,
	ET_Ladder,
	ET_Horse_LF,
	ET_Horse_LB,
	ET_Horse_L,
	ET_Horse_R,
	ET_Horse_RF,
	ET_Horse_RB,
	ET_Horse_B,
	ET_Boat_B,
	ET_Boat_P,
	ET_Boat_Enter_From_Beach,
	ET_Fence,
	ET_Fence_OneSided,
	ET_Ledge,
	ET_Boat_Passenger_B,

	// backward compatibility
	ET_Fence80 = ET_Fence,
	ET_Fence80_OneSided = ET_Fence_OneSided,
	ET_Fence100 = ET_Fence,
	ET_Fence100_OneSided = ET_Fence_OneSided,
	ET_Fence120 = ET_Fence,
	ET_Fence120_OneSided = ET_Fence_OneSided,
	ET_Fence280 = ET_Fence,
	ET_Fence280_OneSided = ET_Fence_OneSided,
	ET_Wall80 = ET_Fence,
	ET_Wall80_OneSided = ET_Fence_OneSided,
	ET_WallHigh = ET_Fence,
	ET_WallHigh_OneSided = ET_Fence_OneSided,
	ET_Ledge120 = ET_Ledge,
	ET_Ledge280 = ET_Ledge,
	ET_Cliff = ET_Ledge,
	ET_CliffLow = ET_Ledge,
};

BEGIN_ENUM_RTTI( EExplorationType );
	ENUM_OPTION( ET_Jump );
	ENUM_OPTION( ET_Ladder );
	ENUM_OPTION( ET_Horse_LF );
	ENUM_OPTION( ET_Horse_LB );
	ENUM_OPTION( ET_Horse_L );
	ENUM_OPTION( ET_Horse_R );
	ENUM_OPTION( ET_Horse_RF );
	ENUM_OPTION( ET_Horse_RB );
	ENUM_OPTION( ET_Horse_B );
	ENUM_OPTION( ET_Boat_B );
	ENUM_OPTION( ET_Boat_P );
	ENUM_OPTION( ET_Boat_Enter_From_Beach );
	ENUM_OPTION( ET_Fence );
	ENUM_OPTION( ET_Fence_OneSided );
	ENUM_OPTION( ET_Ledge );
	ENUM_OPTION( ET_Boat_Passenger_B );

	// backward compatibility
	ENUM_OPTION( ET_Fence80 );
	ENUM_OPTION( ET_Fence80_OneSided );
	ENUM_OPTION( ET_Fence100 );
	ENUM_OPTION( ET_Fence100_OneSided );
	ENUM_OPTION( ET_Fence120 );
	ENUM_OPTION( ET_Fence120_OneSided );
	ENUM_OPTION( ET_Fence280 );
	ENUM_OPTION( ET_Fence280_OneSided );
	ENUM_OPTION( ET_Wall80 );
	ENUM_OPTION( ET_Wall80_OneSided );
	ENUM_OPTION( ET_WallHigh );
	ENUM_OPTION( ET_WallHigh_OneSided );
	ENUM_OPTION( ET_Ledge120 );
	ENUM_OPTION( ET_Ledge280 );
	ENUM_OPTION( ET_Cliff );
	ENUM_OPTION( ET_CliffLow );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SExplorationInitialState
{
	ECharacterPhysicsState	m_physicsState;

	SExplorationInitialState();
	SExplorationInitialState(const CEntity * entity);
};

//////////////////////////////////////////////////////////////////////////

struct SExplorationQueryContext
{
	DECLARE_RTTI_STRUCT( SExplorationQueryContext );

	Vector		m_inputDirectionInWorldSpace;
	Float		m_maxAngleToCheck;
	Bool		m_forJumping;
	Bool		m_forDynamic;
	Bool		m_dontDoZAndDistChecks;
	Bool		m_laddersOnly;
	Bool		m_forAutoTraverseSmall;
	Bool		m_forAutoTraverseBig;

	SExplorationQueryContext()
		:	m_inputDirectionInWorldSpace( Vector::ZEROS )
		,	m_maxAngleToCheck( 0.0f )
		,	m_forJumping( false )
		,	m_forDynamic ( false )
		,	m_dontDoZAndDistChecks( false )
		,	m_laddersOnly( false )
		,	m_forAutoTraverseSmall( false )
		,	m_forAutoTraverseBig( false )
	{
	}
};

BEGIN_CLASS_RTTI( SExplorationQueryContext );
	PROPERTY( m_inputDirectionInWorldSpace );
	PROPERTY( m_maxAngleToCheck );
	PROPERTY( m_forJumping );
	PROPERTY( m_forDynamic );
	PROPERTY( m_dontDoZAndDistChecks );
	PROPERTY( m_laddersOnly );
	PROPERTY( m_forAutoTraverseSmall );
	PROPERTY( m_forAutoTraverseBig );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SExplorationQueryToken
{
	DECLARE_RTTI_STRUCT( SExplorationQueryToken );

private:
	const CEntity*				m_entity;
	Bool						m_valid;
	const IExploration*			m_exploration;

public:
	EExplorationType			m_type;
	Vector						m_pointOnEdge;
	Vector						m_normal;
	SExplorationInitialState	m_initialState;
	SExplorationQueryContext	m_queryContext;

public: // extra data
	Bool						m_usesHands;

	SExplorationQueryToken()
		:	m_entity( nullptr )
		,	m_exploration( nullptr )
		,	m_usesHands( false )
	{
		UpdateValid();
	}

	SExplorationQueryToken( const CEntity* entity, const SExplorationQueryContext & queryContext = SExplorationQueryContext() )
		:	m_entity( entity )
		,	m_exploration( nullptr )
		,	m_initialState( entity )
		,	m_queryContext( queryContext )
		,	m_usesHands( false )
	{
		 UpdateValid();
	}

	Bool IsValid() const
	{
		return m_valid;
	}

	const CEntity* GetEntity() const { return m_entity; }
	const IExploration* GetExploration() const { return m_exploration; }
	void SetExploration(const IExploration* exploration) { m_exploration = exploration; UpdateValid(); }

private:
	void UpdateValid()
	{
		m_valid = m_exploration != NULL && m_entity != NULL;
	}
};

BEGIN_CLASS_RTTI( SExplorationQueryToken );
	PROPERTY( m_valid );
	PROPERTY( m_type );
	PROPERTY( m_pointOnEdge );
	PROPERTY( m_normal );
	PROPERTY( m_usesHands );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct ExecutorSetup
{
	// Tu powinien byc interface na anim object
	const CEntity*	m_entity;
	const IExplorationDesc* m_desc;
	SExplorationQueryToken m_token;
	Bool		m_useLeftSideForEdgeBone;

private:
	CName		m_edgeBone;

	Bool		m_useEdgeOffset;
	Vector		m_edgeOffset;

	Bool		m_useAlignTransToEdge;
	Bool		m_alignTransToEdge;

	Bool		m_useAlignRotToEdge;
	Bool		m_alignRotToEdge;
	Float		m_alignRotToEdgeExceeding;

public:
	// Player, npc, horse ?

	ExecutorSetup( const CEntity* entity, const SExplorationQueryToken & token ) : m_entity( entity ), m_token( token ), m_useLeftSideForEdgeBone( true ), m_useEdgeOffset( false ), m_useAlignRotToEdge( false ) {}

	void SetEdgeBone( const CName & _edgeBone ) { m_edgeBone = _edgeBone; }
	void SetEdgeOffset( const Vector & _edgeOffset ) { m_edgeOffset = _edgeOffset; m_useEdgeOffset = true; }
	void SetAlignTransToEdge( Bool _alignTransToEdge ) { m_alignTransToEdge = _alignTransToEdge; m_useAlignTransToEdge = true; }
	void SetAlignRotToEdge( Bool _alignRotToEdge, Float _alignRotToEdgeExceeding ) { m_alignRotToEdge = _alignRotToEdge; m_alignRotToEdgeExceeding = _alignRotToEdgeExceeding; m_useAlignRotToEdge = true; }

	CName GetEdgeBone() const;
	Vector const & GetEdgeOffset() const;
	Vector const & GetOffsetInModelSpace() const;
	Bool AlignTransToEdge() const;
	Bool AlignRotToEdge() const;
	Float AlignRotToEdgeExceeding() const;
};

class IExploration
{
public:
	virtual void GetMatWS( Matrix & mat ) const = 0;
	virtual void GetParentMatWS( Matrix& mat ) const = 0;
	virtual void GetEdgeWS( Vector& p1, Vector& p2 ) const = 0;
	virtual void GetNormal( Vector& n ) const = 0; // WS
	virtual Int32 GetId() const = 0;
	virtual CObject* GetObjectForEvents() const = 0;

	Int32 GetTotalNumberOfGrains( Float granularity ) const
	{
		Vector p1, p2;
		GetEdgeWS( p1, p2 );

		Vector	direction		=  ( p2 - p1 );

		// Find total steps
		Float	totalDistance	= direction.Mag3();
		Float	totalSteps		= MFloor( totalDistance / granularity );

		return ( Int32 ) totalSteps;
	}

	Int32 SetPointOnClosestGrain( Vector& pointOnEdge, Float granularity ) const
	{
		Vector p1, p2;
		GetEdgeWS( p1, p2 );

		Vector	direction		=  ( p2 - p1 );

		// Find total steps
		Float	totalDistance	= direction.Mag3();
		direction				/= totalDistance;
		Float	totalSteps		= MFloor( totalDistance / granularity ); // Floor is better to never take a step out of the actual reach

		// Find point on edge
		pointOnEdge			= MathUtils::GeometryUtils::ProjectPointOnLine( pointOnEdge, p1, p2 );

		// Find the closest step
		Float	distance	= pointOnEdge.DistanceTo( p1 );
		Float	steps		= distance / granularity;
		Float	exactStep	= MRound( distance / granularity ); // Use MRound to get the closest, useful for loop steps //Ceil is better in case the ladder is in the ground MRound

		// Limit it
		Float	targetStep	= Min( totalSteps, exactStep );

		// Recalc point on ladder
		distance			= targetStep * granularity;
		pointOnEdge			= p1 + direction * distance;

		return ( Int32 ) exactStep;
	}
};

typedef TStaticArray< const IExploration*, 64 > IExplorationList;

enum ExpZComparision
{
	EXPZCMP_ANY, // None z - dist filtering
	EXPZCMP_LESSER, // entity.z - exploration.z is lesser than z param
	EXPZCMP_GREATER, // entity.z - exploration.z is greater than z param
	EXPZCMP_DIST, // |entity.z-exploration.z| is lesser than z param
	EXPZCMP_SIDE_LG, // dependent on side, less in front, greater on back
};

BEGIN_ENUM_RTTI( ExpZComparision );
	ENUM_OPTION( EXPZCMP_ANY );
	ENUM_OPTION( EXPZCMP_LESSER );
	ENUM_OPTION( EXPZCMP_GREATER );
	ENUM_OPTION( EXPZCMP_DIST );
	ENUM_OPTION( EXPZCMP_SIDE_LG );
END_ENUM_RTTI();

enum ExpDoubleSided
{
	EXPDS_SINGLE, // single sided
	EXPDS_DOUBLE, // double sided
	EXPDS_DOUBLE_FOR_AI, // double for ai only
	EXPDS_DOUBLE_FOR_PLAYER, // double for player only
};

BEGIN_ENUM_RTTI( ExpDoubleSided );
	ENUM_OPTION( EXPDS_SINGLE );
	ENUM_OPTION( EXPDS_DOUBLE );
	ENUM_OPTION( EXPDS_DOUBLE_FOR_AI );
	ENUM_OPTION( EXPDS_DOUBLE_FOR_PLAYER );
END_ENUM_RTTI();

class IExplorationDesc
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

public:
	virtual ~IExplorationDesc(){}
	virtual Bool IsHorizontal() const = 0;
	virtual Bool IsDoubleSided( Bool forPlayer ) const = 0;
	virtual const CName& GetEdgeBone( Bool leftSide ) const = 0;
	virtual const Vector& GetEdgeOffset() const = 0;
	virtual const Vector& GetOffsetInModelSpace() const = 0;
	virtual Float GetEdgeYawOffset() const = 0;
	virtual void UseEdgeOffsetAxis( Bool& x, Bool& y, Bool& z ) const = 0;
	virtual void UseMotionOffsetAxis( Bool& x, Bool& y, Bool& z ) const = 0;
	virtual void GetEdgePointsOffset( Float& p1, Float& p2 ) const = 0; // in [m]
	virtual Bool UseEdgeGranularity( Float& gran ) const = 0; // in [m]
	virtual Bool AlignTransToEdge() const = 0;
	virtual Bool AlignRotToEdge() const = 0;
	virtual Float AlignRotToEdgeExceeding() const = 0;
	virtual void GetBlendsAndEnd( Float& in, Float& out, Float& earlyEndOffset ) const = 0;
	virtual void GetDistAndAngleToTest( CEntity const * entity, Float& dist, Float& distMoveTo, Float& coneAngleHalf, Bool ignoreSpeed = false ) const = 0;
	virtual ExpZComparision GetZComparision() const = 0;
	virtual Float GetZComparisionParam() const = 0;
	virtual Float GetZMaxDifference() const = 0;
	virtual void GetEventsAtEnd( CName& raiseBehaviorEventAtEnd, CName& callScriptEventAtEnd ) const = 0;
};

enum ExpRelativeDirection
{
	ERD_None,
	ERD_Left,
	ERD_Right,
	ERD_Up,
	ERD_Down,
	ERD_Front,
	ERD_Back,
	ERD_Last,
};

struct ExpNNQueryData
{
	const IExploration*		m_from;
	const IExploration*		m_to;
	ExpRelativeDirection	m_direction;

	ExpNNQueryData() : m_from( NULL ), m_to( NULL ), m_direction( ERD_None ) {}

	void Reset()
	{
		m_from = NULL;
		m_to = NULL;
		m_direction = ERD_None;
	}

	void QueryRequest( const IExploration* from, ExpRelativeDirection direction )
	{
		m_from = from;
		m_to = NULL;
		m_direction = direction;
	}

	Bool QueryRequester( const IExploration* e ) const
	{
		return e == m_from;
	}

	Bool HasQuery() const
	{
		return m_from && m_direction != ERD_None;
	}

	Bool IsReady() const
	{
		return HasQuery() && m_to;
	}
};

struct ExpExecutorContext
{
	Float				m_dt;

	ExpNNQueryData		m_nnQueryLastResult;

	//... Gameplay commands
};

enum EExplorationNotificationType
{
	ENT_AnimationStarted,
	ENT_AnimationFinished,
	ENT_Event,
	ENT_SlideFinished,

	ENT_Dead,
	ENT_Ragdoll,
	ENT_JumpEnd,
};

struct ExplorationNotificationData
{
	CName m_name;
};

typedef TPair< EExplorationNotificationType, ExplorationNotificationData > ExplorationNotification;

struct ExpExecutorUpdateResult
{
	ExpNNQueryData							m_nnQueryRequest;
	IExpExecutor*							m_nextExe;

	TDynArray< ExplorationNotification >	m_notifications;

	Bool			m_dead;
	Bool			m_ragdoll;
	Bool			m_finished;
	Bool			m_jumpEnd;

	Float			m_timeRest;
	Int32			m_finishPoint;
	Int32			m_side;


	ExpExecutorUpdateResult() : m_dead( false ), m_ragdoll( false ), m_finished( false ), m_jumpEnd( false ), m_timeRest( 0.f ), m_nextExe( NULL ), m_finishPoint( 0 ), m_side( 0 ) {}

	void AddNotification( EExplorationNotificationType type )
	{
		ExplorationNotification* notification = new ( m_notifications ) ExplorationNotification;
		notification->m_first = type;
		notification->m_second.m_name = CName::NONE;
	}

	void AddNotification( EExplorationNotificationType type, CName data )
	{
		ExplorationNotification* notification = new ( m_notifications ) ExplorationNotification;
		notification->m_first = type;
		notification->m_second.m_name = data;
	}
};

class IExpExecutor
{
public:
	virtual ~IExpExecutor() {}

	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result ) = 0;

	virtual void GenerateDebugFragments( CRenderFrame* ) {}

	virtual void OnActionStoped(){}
};

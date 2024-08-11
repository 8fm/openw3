#pragma once


namespace PathLib
{

static const Float GEOMETRY_AND_NAVMESH_MAX_DISTANCE = 1.0f;
static const Float DEFAULT_AGENT_HEIGHT = 1.8f;
static const Uint32 MAX_ACTOR_CATEGORIES = 4;

typedef Uint16 NodeFlags;
typedef Uint16 NavLinkCost;
typedef Uint32 SupLinkCost;
typedef Uint32 PathCost;
typedef Uint16 GraphChunkId;
typedef Uint32 CoherentRegion;
typedef Uint16 AreaRegionId;
typedef Uint32 ExternalDepenentId;
typedef Uint16 AreaId;
typedef Uint8 CategoriesBitMask;
typedef Uint16 CollisionFlags;
typedef Uint32 LinkBufferIndex;

enum ENodeFlags : NodeFlags
{
	NF_DEFAULT								= 0,
	NF_BLOCKED								= FLAG( 0 ),			// node/connection is unusable
	NF_MARKED_FOR_DELETION					= FLAG( 1 ),			// node/connection is marked for deletion by generation/optimization process
	NF_IS_OBSTACLE_DETOUR_NODE				= FLAG( 2 ),			// obstacle detour node
	NF_IS_CUSTOM_LINK						= FLAG( 3 ),			// connection is custom link (and can be safely cast to custom link interface)
	NF_DESTINATION_IS_ID					= FLAG( 4 ),			// connection is in computational form and destination is identified by id NOTICE: Its trashable - can be removed
	NF_CONNECTOR							= FLAG( 5 ),			// node/connection is area connector
	NF_PROCESSED							= FLAG( 6 ),			// generation level flag that marks processed nodes
	NF_IS_IN_NODESET						= FLAG( 7 ),			// node/connection is stored in node set
	NF_IS_GHOST_LINK						= FLAG( 8 ),			// node/connection with is not effected by navigation collisions
	NF_IS_ONE_SIDED							= FLAG( 9 ),			// one sided connection
	NF_ROUGH_TERRAIN						= FLAG( 10 ),			// rought terrain, not used in 'idle' behaviors
	NF_INTERIOR								= FLAG( 11 ),			// marks 'interiors'
	NF_PLAYER_ONLY_PORTAL					= FLAG( 12 ),			// for portals that are useable only by players
	NF_DETACHED								= FLAG( 13 ),			// node is detached - its part of a detached nodeset

	NFG_BREAKS_COHERENT_REGION				= NF_DETACHED | NF_BLOCKED | NF_MARKED_FOR_DELETION | NF_IS_ONE_SIDED | NF_CONNECTOR | NF_PLAYER_ONLY_PORTAL,
	NFG_FORBIDDEN_ALWAYS					= NF_DETACHED | NF_BLOCKED | NF_MARKED_FOR_DELETION,			// bit flag for nodes that are forbidden always
	NFG_FORBIDDEN_BY_DEFAULT				= NFG_FORBIDDEN_ALWAYS | NF_PLAYER_ONLY_PORTAL,
	NFG_PROCESSED							= NF_PROCESSED | NF_MARKED_FOR_DELETION,						// node was modified during current generation procedure
	NFG_SERIALIZABLE						= 0xffff & ~(NF_DETACHED),
};

enum ECollisionTestFlags
{
	CT_DEFAULT								= 0,
	CT_NO_ENDPOINT_TEST						= FLAG( 0 ),													// don't check Z coordinate of final point (in some cases greatly improve performance)
	CT_NO_DYNAMIC_OBSTACLES					= FLAG( 1 ),
	CT_NO_PERSISTANT_OBSTACLES				= FLAG( 2 ),
	CT_NO_OBSTACLES							= CT_NO_DYNAMIC_OBSTACLES | CT_NO_PERSISTANT_OBSTACLES,			// don't check agains obstacles
	CT_MULTIAREA							= FLAG( 3 ),													// pass to neighbour areas
	CT_IGNORE_OTHER_AREAS					= FLAG( 4 ),													// don't pass test to neighbour areas (usefull in tests run from context of other area phantom edge hit event)
	CT_IGNORE_METAOBSTACLE					= FLAG( 5 ),
	CT_FORCE_BASEPOS_ZTEST					= FLAG( 6 ),													// force proper z-test on base test position - even for terrain areas
};


enum EPathfindResult
{
	PATHRESULT_FAILED_OUTOFNAVDATA,
	PATHRESULT_FAILED,
	PATHRESULT_SUCCESS,
	PATHRESULT_PENDING
};

enum EClearLineTestResult
{
	CLEARLINE_INVALID_START_POINT,
	CLEARLINE_SUCCESS,
	CLEARLINE_WAS_HIT
};

enum ENavResType
{
	NavRes_Configuration,
	NavRes_Obstacles,
	NavRes_Graph,
	NavRes_Navmesh,
	NavRes_Terrain,
	NavRes_Invalid
};




static const CoherentRegion INVALID_COHERENT_REGION = 0xffffffff;
static const AreaRegionId INVALID_AREA_REGION = 0xffff;
static const CoherentRegion MASK_SEPARETED_COHERENT_REGION = 0x80000000;		// regions id with that mask are not represented in high-level graph
static const AreaId INVALID_AREA_ID = 0xffff;
static const AreaId AREA_ID_MASK_TERRAIN = 0x8000;
static const Uint32 INVALID_BINARIES_VERSION = 0xffffffff;
static const PathCost MAX_PATH_COST = 0xffffffff;
static const LinkBufferIndex INVALID_LINK_BUFFER_INDEX = 0xffffffff;


// external dependent object
static const ExternalDepenentId EXTERNAL_MASK_PERSISTANTOBSTACLE = 0x80000000;
static const ExternalDepenentId EXTERNAL_MASK_METACONNECTION = 0x40000000;
static const ExternalDepenentId EXTERNAL_MASK_INDEX = 0xffffffff & ~(EXTERNAL_MASK_PERSISTANTOBSTACLE | EXTERNAL_MASK_METACONNECTION);
static const ExternalDepenentId EXTERNAL_INVALID_ID = 0xffffffff;



static const Float LINK_COST_SCALE = 4.f;


static const Float MAX_LINK_DISTANCE =
	Float( NumericLimits< NavLinkCost >::Max() / NavLinkCost(LINK_COST_SCALE) );

static RED_INLINE PathCost CalculatePathCost( Float dist )
{
	return PathCost( dist * LINK_COST_SCALE );
}

static RED_INLINE NavLinkCost CalculateLinkCost( Float dist )
{
	PathCost cost = PathCost( MCeil( dist * LINK_COST_SCALE ) );		// we use ceil to get rid of 0-cost links problem and also to prefer simple paths over complex ones
	if ( cost > NumericLimits< NavLinkCost >::Max() )
	{
		cost = NumericLimits< NavLinkCost >::Max();
	}
	return NavLinkCost( Min< PathCost >( cost, NumericLimits< NavLinkCost >::Max() ) );
}

static RED_INLINE Float ConvertPathCostToDistance( PathCost pathCost )
{
	return Float( pathCost ) * ( 1.f / LINK_COST_SCALE );
}

static RED_INLINE CoherentRegion MakeCoherentRegion( AreaId areaId, AreaRegionId areaRegionId )
{
	return
		areaRegionId == INVALID_AREA_REGION
		? INVALID_COHERENT_REGION
		: ( CoherentRegion( areaId ) << 16 )
		| ( CoherentRegion( areaRegionId ) );
}

static RED_INLINE AreaId AreaIdFromCoherentRegion( CoherentRegion regionId )
{
	return AreaId( regionId >> 16 );
}

static RED_INLINE AreaRegionId AreaRegionIdFromCoherentRegion( CoherentRegion regionId )
{
	return AreaRegionId( regionId & 0xffff );
}

static const Uint32 MAX_OBSTACLE_DETOUR_LEN = 128;

typedef TStaticArray< Vector3, MAX_OBSTACLE_DETOUR_LEN > ObstacleDetour;



};		// namespace PathLib


#include "build.h"
#include "gameplayStorage.h"
#include "gameplayStorageAcceptors.h"
#include "actorsManager.h"
#include "newNpc.h"
#include "../engine/pathlibWorld.h"

#ifdef QUAD_TREE_PARANOID_THREAD_SAFETY_ENABLED

void CQuadTreeStorage_ThreadSafetyAssert()
{
	// This is a separate (external) function, so that we (or QA) can put a breakpoint here and catch issues even in Release

	RED_ASSERT( !TXT("Thread-unsafe quadtree operation") );
}

#endif

///////////////////////////////////////////////////////////////////////////////

CGameplayStorage::SSearchParams::SSearchParams()
	: m_origin( Vector::ZERO_3D_POINT )
	, m_range( 0.0f )
	, m_flags( 0 )
	, m_maxResults( NumericLimits< Int32 >::Max() )
	, m_target( nullptr )
	, m_tag( CName::NONE )
	, m_class( nullptr )
	, m_losPosition( Vector::ZERO_3D_POINT )
	, m_predictPositionInTime( -1.0f )
{}

CGameplayStorage::SSearchParams::~SSearchParams()
{
}

Vector CGameplayStorage::SSearchParams::GetOrigin() const
{
	return m_origin;
}

Box CGameplayStorage::SSearchParams::GetTestBox() const
{
	Box box = Box( Vector::ZEROS, m_range );
	if ( m_flags & FLAGMASK_ZDiff )
	{
		if ( m_flags & FLAG_ZDiff_Range )
		{
			// already ok
		}
		else if ( m_flags & FLAG_ZDiff_5 )
		{
			box.Min.Z = -5.f;
			box.Max.Z = 5.f;
		}
		else
		{
			RED_ASSERT( m_flags & FLAG_ZDiff_3 );
			box.Min.Z = -3.f;
			box.Max.Z = 3.f;
		}
	}
	return box;
}

Bool CGameplayStorage::SSearchParams::ShouldUseZBounds() const
{
	return ( m_flags & FLAGMASK_ZDiff ) > 0;
}

Bool CGameplayStorage::SSearchParams::ShouldTestLineOfSight() const
{
	return ( m_flags & FLAG_TestLineOfSight ) > 0;
}

Bool CGameplayStorage::SSearchParams::IsFull( Uint32 currentResultsNumber ) const
{
	return ( m_maxResults != -1 && static_cast< Int32 >( currentResultsNumber ) >= m_maxResults );
}

const INodeFilter** CGameplayStorage::SSearchParams::GetFilters() const
{
	m_filterList.Init( *this );
	return m_filterList.m_filters;
}

Uint32 CGameplayStorage::SSearchParams::GetFiltersCount() const
{
	m_filterList.Init( *this );
	return m_filterList.m_numFilters;
}

///////////////////////////////////////////////////////////////////////////////

#define BEGIN_IMPLEMENT_NODE_FILTER( NAME )										\
	Bool CGameplayStorage::NODE_FILTER_CLASS_NAME( NAME )::IsFilterConditionFulfilled( const CNode* node ) const

#define IMPLEMENT_STATIC_NODE_FILTER( NAME )									\
	CGameplayStorage::NODE_FILTER_CLASS_NAME( NAME )							\
	CGameplayStorage::CFilterList::s_filter_##NAME;

#define GET_STATIC_NODE_FILTER( NAME )											\
	( &s_filter_##NAME )

#define GET_DYNAMIC_NODE_FILTER( NAME )											\
	reinterpret_cast< NODE_FILTER_CLASS_NAME( NAME )* >( m_filter_##NAME )	

#define ALLOCATE_STATIC_NODE_FILTER( NAME )										\
	PushFilter( GET_STATIC_NODE_FILTER( NAME ) );

#define ALLOCATE_DYNAMIC_NODE_FILTER( NAME )									\
	new (m_filter_##NAME) CNodeFilter_##NAME();								\
	PushFilter( GET_DYNAMIC_NODE_FILTER( NAME ) );


///////////////////////////////////////////////////////////////////////////////
BEGIN_IMPLEMENT_NODE_FILTER( IsNotPlayer )	
{ 
	return !node->IsA< CPlayer > ();
} 

IMPLEMENT_STATIC_NODE_FILTER( IsNotPlayer )

///////////////////////////////////////////////////////////////////////////////
BEGIN_IMPLEMENT_NODE_FILTER( IsNotInInterior )	
{ 
	const CNewNPC* npc = Cast< CNewNPC >( node );
	return !npc || !npc->IsInInterior();		
}

IMPLEMENT_STATIC_NODE_FILTER( IsNotInInterior )

///////////////////////////////////////////////////////////////////////////////
BEGIN_IMPLEMENT_NODE_FILTER( IsAlive )	
{ 
	// we can assume its actor because its actor-only test
	return static_cast< const CActor* > ( node )->IsAlive();
} 

IMPLEMENT_STATIC_NODE_FILTER( IsAlive )

///////////////////////////////////////////////////////////////////////////////
BEGIN_IMPLEMENT_NODE_FILTER( HasWind )	
{ 
	if ( node->IsA< CGameplayEntity > () )
	{
		const CGameplayEntity* gent = static_cast< const CGameplayEntity* > ( node );
		return gent->HasGameplayFlags( FLAG_HasWind ); 
	}
	return false;
} 

IMPLEMENT_STATIC_NODE_FILTER( HasWind )

///////////////////////////////////////////////////////////////////////////////
BEGIN_IMPLEMENT_NODE_FILTER( HasVehicle )	
{ 
	if ( node->IsA< CGameplayEntity > () )
	{
		const CGameplayEntity* gent = static_cast< const CGameplayEntity* > ( node );
		return gent->HasGameplayFlags( FLAG_HasVehicle ); 
	}
	return false;
}

IMPLEMENT_STATIC_NODE_FILTER( HasVehicle )

///////////////////////////////////////////////////////////////////////////////
BEGIN_IMPLEMENT_NODE_FILTER( NotVehicle )	
{ 
	if ( node->IsA< CGameplayEntity > () )
	{
		const CGameplayEntity* gent = static_cast< const CGameplayEntity* > ( node );
		return !gent->HasGameplayFlags( FLAG_HasVehicle ); 
	}
	return false;
}

IMPLEMENT_STATIC_NODE_FILTER( NotVehicle )

///////////////////////////////////////////////////////////////////////////////
BEGIN_IMPLEMENT_NODE_FILTER( HasTag )
{
	return ( node->GetTags().HasTag( m_arg ) );
}
///////////////////////////////////////////////////////////////////////////////
BEGIN_IMPLEMENT_NODE_FILTER( Attitude )
{
	const CActor* actor = Cast< CActor >( node );
	if ( actor != nullptr )
	{
		EAIAttitude attitude = const_cast< CActor* >( actor )->GetAttitude( m_target );
		return ((1 << attitude) & m_flags) != 0;
	}
	return false;
}
///////////////////////////////////////////////////////////////////////////////
BEGIN_IMPLEMENT_NODE_FILTER( SkipTarget )
{
	return node != m_target;
}
///////////////////////////////////////////////////////////////////////////////
BEGIN_IMPLEMENT_NODE_FILTER( IsClass )
{
	return ( node->IsA( m_class ) );
}
///////////////////////////////////////////////////////////////////////////////
BEGIN_IMPLEMENT_NODE_FILTER( PathLibTest )	
{ 
	if ( m_pathLib == nullptr )
	{
		return true;
	}
	
	static Uint32 flags = static_cast< Uint32 >( PathLib::CT_DEFAULT | PathLib::CT_NO_ENDPOINT_TEST );
	return m_pathLib->TestLine( m_areaId, m_origin, node->GetWorldPosition(), flags );
}

///////////////////////////////////////////////////////////////////////////////

// no END_IMPLEMENT_NODE_FILTER here, because you need to specify the argument each time you use the filter, so there's no need to create a static filter here

///////////////////////////////////////////////////////////////////////////////

CGameplayStorage::CGameplayStorage()
{
#ifdef QUAD_TREE_THREAD_SAFETY_ENABLED
	EnableThreadValidation();
#endif

	m_nodeToEntry.Reserve( 60 * 1024 );
	ReserveEntry( 64 * 1024 );
	ReserveNode( 32 * 1024 );
}

void CGameplayStorage::GetClosestToNode( const CNode& node, TDynArray< TPointerWrapper< CGameplayEntity > >& output, const Box& aabb, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters  )
{
	if ( node.IsA< CGameplayEntity >() )
	{
		GetClosestToEntity( static_cast<const CGameplayEntity&>( node ), output, aabb, maxElems, filters, numFilters );
	}
	else
	{
		GetClosestToPoint( node.GetWorldPosition(), output, aabb, maxElems, filters, numFilters );
	}
}

void CGameplayStorage::GetClosestToEntity( const CGameplayEntity& elem, TDynArray< TPointerWrapper< CGameplayEntity > >& output, const Box& aabb, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters  )
{
	PC_SCOPE( ActorsStorage );
	Query( elem, output, aabb, true, maxElems, filters, numFilters );
}

void CGameplayStorage::GetClosestToPoint( const Vector& position, TDynArray< TPointerWrapper< CGameplayEntity > >& output,	const Box& aabb, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters  )
{
	PC_SCOPE( ActorsStorage );
	Query( position, output, aabb, true, maxElems, filters, numFilters );
}

void CGameplayStorage::GetAll( TDynArray< TPointerWrapper< CGameplayEntity > >& output, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters )
{
	PC_SCOPE( ActorsStorage );
	Query( output, maxElems, filters, numFilters );
}

///////////////////////////////////////////////////////////////////////////////
// Scripting support

IMPLEMENT_RTTI_ENUM( EScriptQueryFlags )

CGameplayStorage::CFilterList::CFilterList()
	: m_numFilters( 0 )
	, m_initialized( false )
{
}

void CGameplayStorage::CFilterList::Init( const CGameplayStorage::SSearchParams& params )
{
	if ( m_initialized )
	{
		return;
	}

	Uint32 flags = params.m_flags;
	if ( flags & FLAG_ExcludePlayer )
	{
		ALLOCATE_STATIC_NODE_FILTER( IsNotPlayer );
	}
	if ( flags & FLAG_OnlyAliveActors )
	{
		ALLOCATE_STATIC_NODE_FILTER( IsAlive );
	}
	if ( flags & FLAG_WindEmitters )
	{
		ALLOCATE_STATIC_NODE_FILTER( HasWind );
	}
	if ( flags & FLAG_Vehicles )
	{
		ALLOCATE_STATIC_NODE_FILTER( HasVehicle );
	}
	if ( flags & FLAG_NotVehicles )
	{
		ALLOCATE_STATIC_NODE_FILTER( NotVehicle );
	}
	if ( flags & FLAG_ExcludeTarget )
	{
		ALLOCATE_DYNAMIC_NODE_FILTER( SkipTarget );
		GET_DYNAMIC_NODE_FILTER( SkipTarget )->m_target = params.m_target;
	}
	if ( !params.m_tag.Empty() )
	{
		ALLOCATE_DYNAMIC_NODE_FILTER( HasTag );
		GET_DYNAMIC_NODE_FILTER( HasTag )->m_arg = params.m_tag;
	}
	if ( ( flags & FLAGMASK_Attitude ) && params.m_target != nullptr )
	{
		CActor* actor = Cast< CActor >( params.m_target );
		if ( actor )
		{
			ALLOCATE_DYNAMIC_NODE_FILTER( Attitude );
			GET_DYNAMIC_NODE_FILTER( Attitude )->m_target = actor;
			GET_DYNAMIC_NODE_FILTER( Attitude )->m_flags = 0;
			if ( flags & FLAG_Attitude_Neutral )
			{
				GET_DYNAMIC_NODE_FILTER( Attitude )->m_flags |= 1 << AIA_Neutral;
			}
			if ( flags & FLAG_Attitude_Friendly )
			{
				GET_DYNAMIC_NODE_FILTER( Attitude )->m_flags |= 1 << AIA_Friendly;
			}
			if ( flags & FLAG_Attitude_Hostile )
			{
				GET_DYNAMIC_NODE_FILTER( Attitude )->m_flags |= 1 << AIA_Hostile;
			}
		}
	}
	if ( params.m_class )
	{
		ALLOCATE_DYNAMIC_NODE_FILTER( IsClass );
		GET_DYNAMIC_NODE_FILTER( IsClass )->m_class = params.m_class;
	}
	if ( flags & FLAG_PathLibTest )
	{
		ALLOCATE_DYNAMIC_NODE_FILTER( PathLibTest );
		GET_DYNAMIC_NODE_FILTER( PathLibTest )->m_pathLib = GGame->GetActiveWorld()->GetPathLibWorld();
		GET_DYNAMIC_NODE_FILTER( PathLibTest )->m_origin = params.m_origin;
		GET_DYNAMIC_NODE_FILTER( PathLibTest )->m_areaId = PathLib::INVALID_AREA_ID;
	}

	m_initialized = true;
}

///////////////////////////////////////////////////////////////////////////////
// FindGameplayEntities functions moved to findGameplayEntities.cpp
///////////////////////////////////////////////////////////////////////////////

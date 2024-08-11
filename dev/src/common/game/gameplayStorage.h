/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "binaryStorage.h"
#include "../engine/pathlib.h"

class CActor;

///////////////////////////////////////////////////////////////////////////////
// Some useful macros
// NOTICE: Current implementation don't run destructors on filters so keep in
// mind they must have trivial destructor.
// So one mustn't use strings, dynarrays and such as arguments.
#define NODE_FILTER_CLASS_NAME( NAME ) CNodeFilter_##NAME

#define DECLARE_NODE_FILTER_BEGIN( NAME )										\
	public:																		\
	class NODE_FILTER_CLASS_NAME( NAME ) : public INodeFilter					\
	{																			\
	public:

#define DECLARE_NODE_FILTER_END( NAME )											\
		virtual Bool IsFilterConditionFulfilled( const CNode* node ) const;		\
	};																			

#define DECLARE_NODE_FILTER( NAME )												\
	DECLARE_NODE_FILTER_BEGIN( NAME )											\
	DECLARE_NODE_FILTER_END( NAME )

#define INSTANCE_DYNAMIC_NODE_FILTER( NAME )									\
	Int8	m_filter_##NAME[ sizeof( NODE_FILTER_CLASS_NAME( NAME ) ) ];

#define INSTANCE_STATIC_NODE_FILTER( NAME )										\
	static NODE_FILTER_CLASS_NAME( NAME ) s_filter_##NAME;


///////////////////////////////////////////////////////////////////////////////
// This defines are meant to simplify access to query filters from external
// systems.

#define STATIC_NODE_FILTER( FILTERNAME, VARNAME )								\
	CGameplayStorage::NODE_FILTER_CLASS_NAME( FILTERNAME )&	VARNAME				\
		= CGameplayStorage::CFilterList::s_filter_##FILTERNAME;

#define DYNAMIC_NODE_FILTER( FILTERNAME, VARNAME )								\
	CGameplayStorage::NODE_FILTER_CLASS_NAME( FILTERNAME )	VARNAME;



///////////////////////////////////////////////////////////////////////////////
// Storage allowing to quickly find nearby actors.
class CGameplayStorage : public CQuadTreeStorage< CGameplayEntity, TPointerWrapper< CGameplayEntity > >
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

	DECLARE_NODE_FILTER( IsNotPlayer )
	DECLARE_NODE_FILTER( IsNotInInterior )	
	DECLARE_NODE_FILTER( IsAlive )
	DECLARE_NODE_FILTER( HasWind )
	DECLARE_NODE_FILTER( HasVehicle )
	DECLARE_NODE_FILTER( NotVehicle )
	DECLARE_NODE_FILTER_BEGIN( SkipTarget )
	CGameplayEntity* m_target;
	DECLARE_NODE_FILTER_END( SkipTarget )
	DECLARE_NODE_FILTER_BEGIN( HasTag )
	CName m_arg;
	DECLARE_NODE_FILTER_END( HasTag )
	DECLARE_NODE_FILTER_BEGIN( IsClass )
	CClass* m_class;
	DECLARE_NODE_FILTER_END( IsClass )
	DECLARE_NODE_FILTER_BEGIN( Attitude )
	CActor* m_target;
	Uint32 m_flags;
	DECLARE_NODE_FILTER_END( Attitude )
	DECLARE_NODE_FILTER_BEGIN( PathLibTest )
	CPathLibWorld* m_pathLib;
	Vector m_origin;
	mutable PathLib::AreaId m_areaId;
	DECLARE_NODE_FILTER_END( PathLibTest )

public:

	struct SSearchParams;

	class CFilterList
	{
	public:
		INSTANCE_STATIC_NODE_FILTER( IsNotPlayer )
		INSTANCE_STATIC_NODE_FILTER( IsNotInInterior )
		INSTANCE_STATIC_NODE_FILTER( IsAlive )
		INSTANCE_STATIC_NODE_FILTER( HasWind )
		INSTANCE_STATIC_NODE_FILTER( HasVehicle )
		INSTANCE_STATIC_NODE_FILTER( NotVehicle )
		INSTANCE_DYNAMIC_NODE_FILTER( SkipTarget )
		INSTANCE_DYNAMIC_NODE_FILTER( HasTag )
		INSTANCE_DYNAMIC_NODE_FILTER( IsClass )
		INSTANCE_DYNAMIC_NODE_FILTER( Attitude )
		INSTANCE_DYNAMIC_NODE_FILTER( PathLibTest )
	
	public:
		static const Uint32 MAX_FILTERS = 10;

		CFilterList();
		void Init( const SSearchParams& params );

		const INodeFilter*				m_filters[ MAX_FILTERS ];
		Uint32							m_numFilters;

	private:

		Bool							m_initialized;
		void PushFilter( INodeFilter* filter )									{ m_filters[ m_numFilters++ ] = filter; }
	};

	struct SSearchParams
	{
		Vector				m_origin;
		Float				m_range;
		Uint32				m_flags;
		Int32				m_maxResults;
		CGameplayEntity*	m_target;
		CName				m_tag;
		CClass*				m_class;
		Vector				m_losPosition;				// custom position for line of sight test
		Float				m_predictPositionInTime;	// negative values means don't predict (-1.0f by def.) 

		SSearchParams();
		~SSearchParams();

		Vector GetOrigin() const;
		Box	GetTestBox() const;		
		Bool ShouldUseZBounds() const;
		Bool ShouldTestLineOfSight() const;
		Bool IsFull( Uint32 currentResultsNumber ) const;
		const INodeFilter** GetFilters() const;
		Uint32 GetFiltersCount() const;

	private:

		mutable CFilterList	m_filterList;
	};

	CGameplayStorage();

	// Finds actors closest to the selected node.
	void GetClosestToNode( const CNode& node,	TDynArray< TPointerWrapper< CGameplayEntity > >& output, const Box& aabb, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters );

	// Finds actors closest to the selected actor.
	void GetClosestToEntity( const CGameplayEntity& actor, TDynArray< TPointerWrapper< CGameplayEntity > >& output, const Box& aabb, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters );

	// Finds actors closest to the selected position.
	void GetClosestToPoint( const Vector& position, TDynArray< TPointerWrapper< CGameplayEntity > >& output, const Box& aabb, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters );

	//! Gets all entities on world
	void GetAll( TDynArray< TPointerWrapper< CGameplayEntity > >& output, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters );
};

///////////////////////////////////////////////////////////////////////////////
// FindGameplayEntities functions moved to findGameplayEntities.h
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Scripting support
enum EScriptQueryFlags
{
	FLAG_ExcludePlayer		= FLAG( 0 ),
	FLAG_OnlyActors			= FLAG( 1 ),
	FLAG_OnlyAliveActors	= FLAG( 2 ),
	FLAG_WindEmitters		= FLAG( 3 ),
	FLAG_Vehicles			= FLAG( 4 ),
	FLAG_ExcludeTarget		= FLAG( 5 ),
	FLAG_Attitude_Neutral	= FLAG( 6 ),
	FLAG_Attitude_Friendly	= FLAG( 7 ),
	FLAG_Attitude_Hostile	= FLAG( 8 ),
	FLAG_ZDiff_3			= FLAG( 9 ),
	FLAG_ZDiff_5			= FLAG( 10 ),
	FLAG_ZDiff_Range		= FLAG( 11 ),
	FLAG_PathLibTest		= FLAG( 12 ),
	FLAG_NotVehicles		= FLAG( 13 ),
	FLAG_TestLineOfSight	= FLAG( 14 ),	//!< since line of sight test is quite heavy, it's not implemented as "standard" filter but rather as "post" functor filter

	FLAGMASK_Attitude		= FLAG_Attitude_Hostile | FLAG_Attitude_Neutral | FLAG_Attitude_Friendly,
	FLAGMASK_OnlyActors		= FLAG_OnlyActors | FLAG_OnlyAliveActors | FLAGMASK_Attitude,
	FLAGMASK_ZDiff			= FLAG_ZDiff_3 | FLAG_ZDiff_5 | FLAG_ZDiff_Range
};

BEGIN_ENUM_RTTI( EScriptQueryFlags )
	ENUM_OPTION( FLAG_ExcludePlayer )
	ENUM_OPTION( FLAG_OnlyActors )
	ENUM_OPTION( FLAG_OnlyAliveActors )
	ENUM_OPTION( FLAG_WindEmitters )
	ENUM_OPTION( FLAG_Vehicles )
	ENUM_OPTION( FLAG_ExcludeTarget )
	ENUM_OPTION( FLAG_Attitude_Neutral )
	ENUM_OPTION( FLAG_Attitude_Friendly )
	ENUM_OPTION( FLAG_Attitude_Hostile )
	ENUM_OPTION( FLAG_PathLibTest )
	ENUM_OPTION( FLAG_NotVehicles )
	ENUM_OPTION( FLAG_TestLineOfSight )
END_ENUM_RTTI()

///////////////////////////////////////////////////////////////////////////////

#undef DECLARE_NODE_FILTER
#undef DECLARE_NODE_FILTER_BEGIN
#undef DECLARE_NODE_FILTER_END

#undef INSTANCE_DYNAMIC_NODE_FILTER
#undef INSTANCE_STATIC_NODE_FILTER

#pragma once

#include "pathlibObstacleSpawnContext.h"
#include "pathlibNodeSet.h"
#include "pathlibMetalink.h"
#include "pathlibSpatialQuery.h"

namespace PathLib
{

class CNavGraph;
class CObstaclesMap;
class CObstacle;
class CObstaclesMap;
class CDynamicPregeneratedObstacle;


////////////////////////////////////////////////////////////////////////////
// Pathfinding obstacle. Consist of shape and logic responsible for
// taking part in graph generation process.
class CObstacle
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_PathLib );

	friend class CObstaclesMap;
public:
	typedef ExternalDepenentId Id;
	static const Id INVALID_ID			= EXTERNAL_INVALID_ID;
	static const Id MASK_IS_PERSISTANT	= EXTERNAL_MASK_PERSISTANTOBSTACLE;
	static const Id MASK_INDEX			= EXTERNAL_MASK_INDEX;

	enum EFlags
	{
		EMPTY_FLAGS									= 0,
		IS_PERSISTANT								= FLAG( 0 ),
		IS_MARKED									= FLAG( 1 ),
		IS_COLLISION_DISABLED						= FLAG( 3 ),
		IS_DYNAMIC									= FLAG( 4 ),
		IS_METAOBSTACLE								= FLAG( 5 ),			// Is obstacle disabled using runtime conditions (used by metalinks and some immediate denied areas)
		IS_GROUPED									= FLAG( 6 ),

		FLAGS_SAVEABLE								= IS_METAOBSTACLE | IS_PERSISTANT | IS_DYNAMIC | IS_GROUPED,

	};

protected:
	Id												m_id;
	Uint32											m_flags;
	CObstacleShape*									m_shape;
	TDynArray< AreaId >								m_areasInside;

private:
	// values set by CObstacleMap
	Uint16											m_xmin;					// This attributes
	Uint16											m_ymin;					// are precomputed
	Uint16											m_xmax;					// values, used only
	Uint16											m_ymax;					// by CObstaclesMap.

protected:

	Bool						CollideArea( const Box& localNaviAreaBBox, CNavmeshAreaDescription* naviArea, CAreaDescription* myArea );

	void						UpdateCollisionFlags( CNavGraph* navgraph, CNodeSetProcessingContext& context );

	template < class Functor >
	RED_INLINE Bool			GenerateDetourNodes( Functor& func );
	template < class Functor >
	RED_INLINE Bool			ConnectDetourNodes( Functor& func );
public:
	CObstacle( Uint32 defaultFlags )
		: m_flags( defaultFlags )
		, m_shape( NULL )													{}
	CObstacle( Uint32 defaultFlags, CObstacleShape* shape )
		: m_flags( defaultFlags )
		, m_shape( shape )													{}
	virtual ~CObstacle();

	template < class TQuery >
	Bool						TestFlags( TQuery& query ) const
	{
		if ( !m_areasInside.Empty() )
		{
			if ( query.m_flags & CT_MULTIAREA )
			{
				CMultiAreaSpatialQuery* areaData = TQuery::MultiArea::GetMultiAreaData( query );
				for ( Uint32 i = 0, n = m_areasInside.Size(); i < n; ++i )
				{
					if ( areaData->VisitedArea( m_areasInside[ i ] ) )
					{
						return false;
					}
				}
			}
		}
		return true;
	}

	static CObstacle*			NewFromBuffer( CSimpleBufferReader& reader );
	virtual void				WriteToBuffer( CSimpleBufferWriter& writer ) const;
	virtual	Bool				ReadFromBuffer( CSimpleBufferReader& reader );

	Bool						IsPersistant() const						{ return (m_flags & IS_PERSISTANT) != 0; }
	Bool						IsMetaobstacle() const						{ return (m_flags & IS_METAOBSTACLE ) != 0; }
	Id							GetId() const								{ return m_id; }
	Uint32						GetFlags() const							{ return m_flags; }
	void						AddFlags( Uint32 flags )					{ m_flags |= flags; }
	void						RemoveFlags( Uint32 flags )					{ m_flags &= ~flags; }
	

	virtual void				Initialize( CObstaclesMap* obstaclesMap, const CObstacleSpawnData& spawnData );		// called only once after initial addition to obstacles map (at generation process)
	virtual void				OnAddition( CObstaclesMap* obstaclesMap );											// when added to world
	virtual void				OnRemoval( CObstaclesMap* obstaclesMap );											// when removed from world
	virtual void				PostGraphGeneration( CObstaclesMap* map, CNavGraph* navgraph );						// called when base navgraph is computed
	virtual void				OnGraphClearance( CNavGraph* navgraph );											// called when graph is being cleared
	virtual void				OnShow( CAreaDescription* area, CComponentRuntimeProcessingContext& context );		// gameplay requires obstacle to show
	virtual void				OnHide( CAreaDescription* area, CComponentRuntimeProcessingContext& context );		// gameplay requires obstacle to hide
	virtual void				OnPostLoad( CAreaDescription* area, CComponentRuntimeProcessingContext& context );	// called post obstacle map loading procedure
	virtual void				OnPreUnload( CAreaDescription* area );												// called right before nav data for given area is gonna be dumped
	virtual void				SetMapping( const SComponentMapping& mapping, const SLayerMapping& layerMapping );

	void						MarkCollision( CObstaclesMap* obstacles );											// try to mark itself at obstacle map
	void						UnmarkCollision( CObstaclesMap* obstacles );										// unmark itself from obstacle map

	void						ClearShape();
	void						ChangeShape( CObstacleShape* shape )		{ ClearShape(); m_shape = shape; }
	CObstacleShape*				GetShape() const							{ return m_shape; }

	Bool						MarkArea( const Box& localNaviBBox, CNavmeshAreaDescription* naviArea, CAreaDescription* myArea );
	void						UpdateAreasInside( CAreaDescription* area );

	virtual CDynamicPregeneratedObstacle* AsDynamicPregeneratedObstacle();
};

class CPersistantObstacle : public CObstacle
{
	typedef CObstacle Super;
public:
	CPersistantObstacle()
		: CObstacle( IS_PERSISTANT)											{}
	CPersistantObstacle( Uint32 flags )
		: CObstacle( flags | IS_PERSISTANT )								{}
	CPersistantObstacle( CObstacleShape* shape )
		: CObstacle( IS_PERSISTANT, shape )									{}

};

class CDynamicGroupedObstacle : public CObstacle
{
	typedef CObstacle Super;
public:
	CDynamicGroupedObstacle( Uint32 flags = 0 )
		: CObstacle( IS_GROUPED| IS_DYNAMIC | IS_COLLISION_DISABLED | flags )			{}
	CDynamicGroupedObstacle( CObstacleShape* shape )
		: CObstacle( IS_GROUPED | IS_DYNAMIC | IS_COLLISION_DISABLED, shape )			{}

	void						Initialize( CObstaclesMap* obstaclesMap, const CObstacleSpawnData& spawnData ) override;

	void						OnShow( CAreaDescription* area, CComponentRuntimeProcessingContext& context ) override;		// gameplay requires obstacle to show
	void						OnHide( CAreaDescription* area, CComponentRuntimeProcessingContext& context ) override;		// gameplay requires obstacle to hide

	void						Show( CAreaDescription* area, CComponentRuntimeProcessingContext& context );
	void						Hide( CAreaDescription* area, CComponentRuntimeProcessingContext& context ); 

	void						SetMapping( const SComponentMapping& mapping, const SLayerMapping& layerMapping ) override;
};

class CDynamicIndependentObstacle : public CObstacle, public INodeSetPack
{
	typedef CObstacle Super;
protected:
	Bool						m_isShown;
	SComponentMapping			m_mapping;

	void						Show( CAreaDescription* area, CComponentRuntimeProcessingContext& context );
	void						Hide( CAreaDescription* area, CComponentRuntimeProcessingContext& context ); 
public:
	CDynamicIndependentObstacle( Uint32 defaultFlags )
		: CObstacle( defaultFlags | IS_DYNAMIC | IS_COLLISION_DISABLED )
		, m_isShown( false )												{}
	CDynamicIndependentObstacle( Uint32 defaultFlags, CObstacleShape* shape )
		: CObstacle( defaultFlags | IS_DYNAMIC | IS_COLLISION_DISABLED, shape )
		, m_isShown( false )												{}

	// CObstacle interface
	Bool						IsUpdatingNodeCollisionOnAttach() override;
	void						OnPostLoad( CAreaDescription* area, CComponentRuntimeProcessingContext& context ) override;
	void						OnPreUnload( CAreaDescription* area ) override;
	void						SetMapping( const SComponentMapping& mapping, const SLayerMapping& layerMapping ) override;

	void						OnShow( CAreaDescription* area, CComponentRuntimeProcessingContext& context ) override;
	void						OnHide( CAreaDescription* area, CComponentRuntimeProcessingContext& context ) override;

	Bool						Generate( CNavGraph* navgraph );

	const SComponentMapping&	GetMapping() const							{ return m_mapping; }					// TODO: push it lower

	void						WriteToBuffer( CSimpleBufferWriter& writer ) const override;
	Bool						ReadFromBuffer( CSimpleBufferReader& reader ) override;
};

class CDynamicPregeneratedObstacle : public CDynamicIndependentObstacle
{
	typedef CDynamicIndependentObstacle Super;
public:
	CDynamicPregeneratedObstacle()
		: Super( EMPTY_FLAGS )												{}
	CDynamicPregeneratedObstacle( Uint32 flags )
		: Super( flags )													{}
	CDynamicPregeneratedObstacle( CObstacleShape* shape )
		: Super( EMPTY_FLAGS, shape )										{}

	void						PostGraphGeneration( CObstaclesMap* obstaclesMap, CNavGraph* navgraph ) override;
	void						OnAddition( CObstaclesMap* obstaclesMap ) override;
	void						OnRemoval( CObstaclesMap* obstaclesMap ) override;
	void						OnPostLoad( CAreaDescription* area, CComponentRuntimeProcessingContext& context ) override;

	Bool						IsShown() const								{ return m_isShown; }

	// CNodeSetPack interface
	void						GenerationOnPreNodeSetConnection( CNavGraph* navgraph ) override;
	void						GenerationOnPostNodeSetConnection( CNavGraph* navgraph ) override;

	CDynamicPregeneratedObstacle* AsDynamicPregeneratedObstacle() override;
};

class CDynamicImmediateObstacle : public CDynamicIndependentObstacle
{
	typedef CDynamicIndependentObstacle Super;
protected:
	Bool						m_isGenerated;
public:
	CDynamicImmediateObstacle()
		: Super( IS_METAOBSTACLE )										{}
	CDynamicImmediateObstacle( CObstacleShape* shape )
		: Super( IS_METAOBSTACLE, shape )								{}

	void						OnAddition( CObstaclesMap* obstaclesMap ) override;
	void						OnRemoval( CObstaclesMap* obstaclesMap ) override;
};

};				// namespace PathLib
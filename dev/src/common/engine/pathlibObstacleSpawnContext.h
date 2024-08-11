#pragma once


#include "pathlibGenerationManager.h"

class CDeniedAreaComponent;
class CDestructionSystemComponent;
class CSimpleBufferWriter;
class CSimpleBufferReader;
struct SFoliageInstance;


namespace PathLib
{

class CComponentRuntimeProcessingContext;
class CObstaclesMapper;
class CObstaclesLayerAsyncProcessing;
class CObstacleProcessingJob;
class CObstacleShape;
class IComponent;

enum ENodeProcessingFlags
{
	NODEPROCESSING_IS_OUT_OF_AREA					= FLAG( 0 ),
	NODEPROCESSING_IS_UNACCESSIBLE					= FLAG( 1 ),
	NODEPROCESSING_MARKED_FOR_DELETION				= FLAG( 2 ),
	NODEPROCESSING_OK								= FLAG( 3 ),
	NODEPROCESSING_POSSIBLE_CONNECTOR				= FLAG( 4 ),
};

////////////////////////////////////////////////////////////////////////////////
// Data that maps external data object for PathLib system.
////////////////////////////////////////////////////////////////////////////////
struct SComponentMapping
{
	enum eClear
	{
		CLEAR
	};
	SComponentMapping()															{}
	SComponentMapping( CComponent* component );
	SComponentMapping( eClear )
		: m_entityGuid( CGUID::ZERO )
		, m_componentHash( 0 )													{}

	CGUID			m_entityGuid;
	Uint32			m_componentHash;

	Bool operator==( const SComponentMapping& c ) const							{ return m_entityGuid == c.m_entityGuid && m_componentHash == c.m_componentHash; }
	Bool operator<( const SComponentMapping& c ) const
	{
		return m_entityGuid < c.m_entityGuid ? true
			: m_entityGuid != c.m_entityGuid ? false
			: m_componentHash < c.m_componentHash;
	}

	IComponent* GetComponent( CWorld* world );

	RED_FORCE_INLINE Uint32 CalcHash() const
	{
		return m_entityGuid.CalcHash() ^ m_componentHash;
	}
};

struct SLayerMapping
{
	SLayerMapping()																{}
	SLayerMapping( Uint64 id )
		: m_layerGroupId( id )													{}
	SLayerMapping( CComponent* component );
	SLayerMapping( CLayerGroup* layerGroup );

	Uint64			m_layerGroupId;

	Bool operator==( const SLayerMapping& c ) const								{ return m_layerGroupId == c.m_layerGroupId; }
	Bool operator<( const SLayerMapping& c ) const								{ return m_layerGroupId < c.m_layerGroupId; }

	RED_FORCE_INLINE Uint32 CalcHash() const									{ return GetHash( m_layerGroupId ); }
};


struct CProcessingEvent
{
	struct GeneralProcessingMechanism
	{
		virtual Bool	RuntimeProcessing( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e );
		virtual Bool	ToolAsyncProcessing( CObstaclesLayerAsyncProcessing* processingJob, const CProcessingEvent& e, IGenerationManagerBase::CAsyncTask::CSynchronousSection& section );
	};

	enum EType
	{
		TYPE_UPDATED,
		TYPE_ATTACHED,
		TYPE_REMOVED,
		TYPE_DETACHED,
	};

	CProcessingEvent()														{}
	CProcessingEvent( EType t, CComponent* s )
		: m_type( t )
		, m_component( s )
		, m_componentMapping( s )
		, m_generalProcessingImplementation( NULL )							{}
	CProcessingEvent( EType t, const SComponentMapping& mapping )
		: m_type( t )
		, m_componentMapping( mapping )
		, m_generalProcessingImplementation( NULL )							{}
	EType										m_type;
	THandle< CComponent >						m_component;
	SComponentMapping							m_componentMapping;
	GeneralProcessingMechanism*					m_generalProcessingImplementation;
	Bool CompareEvents( const CProcessingEvent& e ) const					{ return m_componentMapping == e.m_componentMapping; }
};
////////////////////////////////////////////////////////////////////////////////
// All data needed to spawn pathlib obstacle from external data (static mesh or
// denied area).
////////////////////////////////////////////////////////////////////////////////
struct CObstacleShapeGeometryData
{
	enum EType
	{
		T_INDVERTS,
		T_PURE_CONVEX,
		T_CYLINDER,
	}							m_type;
	Box							m_shapeBBox;

	CObstacleShapeGeometryData( EType t )
		: m_type( t )															{}
};

struct CObstacleIndicedVertsGeometryData : public CObstacleShapeGeometryData
{
	TDynArray< Vector >			m_verts;
	TDynArray< Uint32 >			m_indices;
	
	CObstacleIndicedVertsGeometryData()
		: CObstacleShapeGeometryData( T_INDVERTS )								{}
	CObstacleIndicedVertsGeometryData( CObstacleIndicedVertsGeometryData&& data )
		: CObstacleShapeGeometryData( data )
		, m_verts( Move( data.m_verts ) )
		, m_indices( Move( data.m_indices ) )									{}

	CObstacleIndicedVertsGeometryData& operator=( CObstacleIndicedVertsGeometryData&& data )	{ CObstacleShapeGeometryData::operator=( data ); m_verts = Move( data.m_verts ); m_indices = Move( data.m_indices ); return *this; }
};

struct CObstacleConvexOccluderData : public CObstacleShapeGeometryData
{
	TDynArray< Vector2 >		m_occluder;

	CObstacleConvexOccluderData()
		: CObstacleShapeGeometryData( T_PURE_CONVEX )							{}
	CObstacleConvexOccluderData( const CObstacleConvexOccluderData& data )
		: CObstacleShapeGeometryData( data )
		, m_occluder( data.m_occluder )											{}
	CObstacleConvexOccluderData( CObstacleConvexOccluderData&& data )
		: CObstacleShapeGeometryData( data )
		, m_occluder( Move( data.m_occluder ) )									{}

	CObstacleConvexOccluderData& operator=( CObstacleConvexOccluderData&& data )				{ CObstacleShapeGeometryData::operator=( data ); m_occluder = Move( data.m_occluder ); return *this; }
};

struct CObstacleCylinderData : public CObstacleShapeGeometryData
{
	Vector2							m_pos;
	Float							m_radius;

	CObstacleCylinderData()
		: CObstacleShapeGeometryData( T_CYLINDER )								{}
	CObstacleCylinderData( const Vector2& pos, Float minZ, Float maxZ, Float radius )
		: CObstacleShapeGeometryData( T_CYLINDER )
		, m_pos( pos )
		, m_radius( radius )													{ m_shapeBBox.Min = Vector( m_pos.X - m_radius, m_pos.Y - m_radius, minZ ); m_shapeBBox.Max = Vector( m_pos.X + m_radius, m_pos.Y + m_radius, maxZ ); }
};


struct CObstacleSpawnData
{
public:
	TDynArray< CObstacleShapeGeometryData* >	m_shapes;
	EPathLibCollision							m_collisionType;
	Box											m_bbox;
	SComponentMapping							m_mapping;
	SLayerMapping								m_layerMapping;
	Bool										m_isOnHiddeableLayer;
	Bool										m_isLayerBasedGrouping;

	CObstacleSpawnData()														{}
	CObstacleSpawnData( CObstacleSpawnData&& data );
	~CObstacleSpawnData();

	Bool Initialize( CComponent* component, CPathLibWorld& pathlib, CLayerGroup* layerGroup );
	void InitializeTree( const SFoliageInstance & foliageInstance, TDynArray< Sphere >& treeCollisionShapes );

private:
	void Initialize( CStaticMeshComponent* staticMesh, CPathLibWorld& pathlib );
	void Initialize( CDestructionComponent* staticMesh, CPathLibWorld& pathlib );
	void Initialize( CDeniedAreaComponent* deniedArea, CPathLibWorld& pathlib );
	void Initialize( CDestructionSystemComponent* destructionSystem, CPathLibWorld& pathlib );

#ifndef NO_OBSTACLE_MESH_DATA
	Bool TryProcessNavObstacleData(CMesh* mesh, Matrix localToWorld);
#endif
#ifndef NO_RESOURCE_IMPORT
	void TryProcessCollisionCache(CMesh* mesh, Matrix localToWorld);
#endif

};

struct CObstacleSpawnContext : public Red::System::NonCopyable
{
	const CObstacleSpawnData&	m_baseData;
	CObstacleSpawnData			m_optimizedData;	
	Bool						m_dataOptimized;

	CObstacleSpawnContext( const CObstacleSpawnData& data )
		: m_baseData( data )
		, m_dataOptimized( false )												{}

	const CObstacleSpawnData&	Data() const									{ return m_dataOptimized ? m_optimizedData : m_baseData; }
};

struct CMetalinkConfigurationCommon
{
public:
	static const Uint32			MAX_NODES										= 256;

	struct Node
	{
		Vector3						m_pos;
		NodeFlags					m_nodeFlags;
		Uint16						m_processingFlags;							// NOTICE: reserved for runtime processing - trash by default
		AreaId						m_targetArea;								// reserved for interarea metaconnections
	};
	struct Connection 
	{
		Uint16						m_ind[2];
		NavLinkCost					m_linkCost;									// enforced link cost - used only for custom links
		NodeFlags					m_linkFlags;								// enforced link flags - used for custom links
	};
	TDynArray< Node >			m_nodes;
	TDynArray< Connection >		m_connections;
	
	Box							m_bbox;

	Bool						IsEmpty();

protected:																		// NOTICE: making interface protected protects us from situation when this interface would be used directly from outside
	CMetalinkConfigurationCommon()												{}
	~CMetalinkConfigurationCommon();

	void						WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool						ReadFromBuffer( CSimpleBufferReader& reader );
	void						ComputeBBox();

	void						Clear();

};

// external metalink configuration visible from metalink components
struct CMetalinkConfiguration : public CMetalinkConfigurationCommon
{
	typedef CMetalinkConfigurationCommon Super;
public:
	CObstacleSpawnData*			m_internalObstacle;
	Bool						m_injectGraph;

	CMetalinkConfiguration()
		: m_internalObstacle( nullptr )
		, m_injectGraph( false )												{}
	~CMetalinkConfiguration();

	void						Clear();

	void						ComputeBBox();
};

// runtime metalink graph data used for navgraph generation
struct CMetalinkGraph : public CMetalinkConfigurationCommon
{
	typedef CMetalinkConfigurationCommon Super;
public:
	CMetalinkGraph()															{}
	~CMetalinkGraph()															{}

	void						CopyGraphFrom( const CMetalinkConfigurationCommon& data );
};

// metalink info stored with each navmodyfication
struct CMetalinkComputedData : public CMetalinkConfigurationCommon
{
	typedef CMetalinkConfigurationCommon Super;
public:
	CObstacleShape*				m_internalShape;
	Bool						m_injectGraph;

	CMetalinkComputedData()
		: m_internalShape( nullptr )
		, m_injectGraph( false )												{}

	~CMetalinkComputedData();

	void						WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool						ReadFromBuffer( CSimpleBufferReader& reader );

	void						CopyGraphFrom( const CMetalinkConfigurationCommon& data );

	void						Clear();
	void						ComputeBBox();
};

};			// namespace PathLib
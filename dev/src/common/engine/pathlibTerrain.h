#pragma once

#include "../core/intsBitField.h"

#include "pathlibAreaDescription.h"
#include "pathlibAreaRes.h"
#include "pathlibDetailedSurfaceData.h"
#include "pathlibTerrainHeight.h"
#include "pathlibWorld.h"

//#ifdef NO_EDITOR
#define PATHLIB_COMPECT_TERRAIN_DATA
//#endif

#ifdef NO_EDITOR_PATHLIB_SUPPORT
	#define PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
#endif

class CClipMap;

namespace PathLib
{

class CTerrainSurfaceProcessingThread;

#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
class CTerrainHeightComputationContext
{
protected:
	Uint16*					m_tileTexels;
	Float					m_tileSize;
	Uint32					m_tileRes;
	Float					m_lowestElevation;
	Float					m_heightRange;
	Bool					m_isValid;
public:
	CTerrainHeightComputationContext();
	~CTerrainHeightComputationContext();

	// Two functions to check for availability. IsInitialized is used when we want to 
	// decide to use computation context for compute height - from possibly asynchronous
	// call. If computation context is 'invalid'
	Bool		IsInitialized() const													{ return m_tileTexels != NULL; }
	Bool		IsValid() const															{ return m_isValid; }

	Bool		InitializeSync( CWorld* world, Int32 tileX, Int32 tileY );
	void		Clear();
	void		Invalidate()															{ m_isValid = false; }

	Float		ComputeHeight( const Vector2& localPos ) const;
	Float		ComputeTexelNormalizedHeight( Int32 texelX, Int32 texelY ) const;
	Float		GetVertHeight( Uint32 x, Uint32 y ) const;

	void		GetTexelVertsHeight( Int32 texelX, Int32 texelY, Float* texelHeight ) const;
	void		GetTexelHeightLimits( Int32 texelX, Int32 texelY, Float& zMin, Float& zMax ) const;
	void		GetTexelHeightLimits( Int32 minX, Int32 maxX, Int32 minY, Int32 maxY, Float& zMin, Float& zMax ) const;
	void		WaitUntilReady() const													{}
};
#endif		// PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT

class CTerrainMap : public CVersionTracking, public CAreaRes
{
	template< class T > friend struct MipMapTester;
public:
	static const Int16 RES_VERSION		= 6;

	enum eQuadState
	{
		QUAD_FREE						= 0x0,
		QUAD_BLOCK_ALL					= 0x1,
		QUAD_INSTANCE					= 0x2,
		QUAD_OBSTACLE					= 0x3,
	};
	enum eMipMapQuadState
	{
		MIPMAP_FREE						= 0x0,
		MIPMAP_HOMOGENOUS				= 0x1,
		MIPMAP_MIXED					= 0x2,
		MIPMAP_BLOCKED					= 0x3
	};
	enum eForcedState
	{
		FORCED_FREE						= 0x0,
		FORCED_BLOCK					= 0x1
	};
	typedef Uint32						QuadIndex;
	static const QuadIndex INVALID_INDEX = 0xffffffff;
protected:
	static const Uint32 BITS_PER_QUAD = 4;

	typedef TTypedIntsBitField< 1, eQuadState, MC_PathLib >					DetailedMap;
	typedef TTypedIntsBitField< 1, eMipMapQuadState, MC_PathLib >			MipMap;
	
	CTerrainAreaDescription*			m_area;
	const CTerrainInfo*					m_terrainInfo;
	DetailedMap							m_detailedMap;
	MipMap								m_mipMap;
	CTerrainHeight						m_height;
#ifndef PATHLIB_COMPECT_TERRAIN_DATA
	LongBitField						m_forcedData;
	DetailedMap							m_terrainData;
#endif
	Uint32								m_mipMapLevels;
	////////////////////////////////////////////////////////////////////////
	// Walkable test generic functions
	template < class TQuery >
	struct DefaultPredicate : public Red::System::NonCopyable
	{
		enum
		{
			REPORT_SUCCESS = false,
			AUTOFAIL = true
		};

		typedef TQuery QueryType;
		DefaultPredicate( TQuery& query )
			: m_query( query )																	{}
		TQuery& m_query;
		
		RED_INLINE Bool OnSuccess( const Vector2& centralPoint ) const							{ return true; }
		RED_INLINE void OnFail( const Vector2& v1, const Vector2& v2 ) const					{}
		RED_INLINE void OnFailRect( const Vector2& rectMin, const Vector2& rectMax ) const		{}
		RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const	{ return m_query.IntersectRect( rectMin, rectMax ); }
		RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const					{ return m_query.Intersect( v1, v2 ); }
	};
	template < class Predicate >
	RED_INLINE Bool MipMapTest( Predicate& c ) const;
	////////////////////////////////////////////////////////////////////////

	Uint32			CalculateMipMapLevels( QuadIndex tileResoultion ) const;
	void			InitializeMipMap();
	void			UpdateMipMap( eQuadState state, Int32 quadX, Int32 quadY );

	RED_INLINE Uint32		MipMapShift( Uint32 level ) const									{ return m_mipMapLevels - level; }
	static inline Uint32	MipMapIndex( Uint32 level, Uint32 x, Uint32 y );
	eQuadState				MipMapGetHomogenousType( Uint32 level, Uint32 x, Uint32 y ) const;

public:
	CTerrainMap();
	~CTerrainMap();

	Bool			IsInitialized() const;

	void			Initialize();
	Bool			UpdateMap();
	void			CalculateMipMap();
	Bool			Save( const String& depotPath ) const;
	Bool			WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool			ReadFromBuffer( CSimpleBufferReader& reader );
	Bool			IsValid() const;

	QuadIndex		GetQuadIndex( Int32 x, Int32 y ) const;
	void			GetQuadCoords( QuadIndex q, Int32& x, Int32& y ) const;
	eQuadState		GetQuadState( QuadIndex quad ) const;
	Bool			ForceQuadState( QuadIndex index, eForcedState state, Bool autoUpdate );
	Bool			SetTerrainQuadState( QuadIndex index, eQuadState state, Bool updateMipMap = true );
	void			GetQuadCoordsAt( const Vector2& localPos, Int32& x, Int32& y ) const;
	Int32			GetQuadCoordFromScalar( Float scalar ) const;
	void			GetMipMapLocalPosition( Int32 mipmapLevel, Int32 x, Int32 y, Vector2& outLocalPosMin, Vector2& outLocalPosMax ) const;
	void			GetQuadLocalPosition( Int32 x, Int32 y, Vector2& outLocalPosMin, Vector2& outLocalPosMax ) const;
	Vector2			GetQuadCenterLocal( Int32 x, Int32 y ) const;
	Bool			GenerateHeightData();

#ifndef PATHLIB_COMPECT_TERRAIN_DATA
	eQuadState		GetTerrainQuadState( QuadIndex index );
	eQuadState		ComputeQuadState( QuadIndex quad ) const;
	Bool			UpdateQuad( QuadIndex index, Bool updateMipMap = true );
#else
	RED_INLINE eQuadState	GetTerrainQuadState( QuadIndex index )						{ return GetQuadState( index ); }
#endif

	void SetTerrainInfo( const CTerrainInfo* manager );
	RED_INLINE const CTerrainInfo* GetTerrainInfo() const								{ return m_terrainInfo; }
	RED_INLINE CTerrainAreaDescription& GetArea() const									{ return *m_area; }
	const CTerrainHeight& GetHeightContext() const										{ return m_height; }

	Bool			TestLocation(const Vector2& pos) const;																					// check if given point is walkable (on terrain)
	Bool			SpatialQuery( CCircleQueryData& query ) const;																			// check if given position is walkable (on terrain)
	Bool			SpatialQuery( CClosestObstacleCircleQueryData& query ) const;															// return distance to closest wall in given radius (or F_MAX) and sets vPointOut to closest point on this wall
	Bool			SpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const;													// collect closest collision points in radius
	Bool			SpatialQuery( CLineQueryData& query ) const;																			// check if given line segment is walkable
	Bool			SpatialQuery( CWideLineQueryData& query ) const;																		// check if given wide line is walkable
	Bool			SpatialQuery( CClosestObstacleWideLineQueryData& query ) const;															// finds closest collision point to given wide line				
	Bool			SpatialQuery( CClearWideLineInDirectionQueryData& query ) const;														// finds furthest non-collidable point on wide line
	Bool			SpatialQuery( CRectangleQueryData& query ) const;																			
	Bool			SpatialQuery( CCollectGeometryInCirceQueryData& query ) const;
	Bool			SpatialQuery( CCustomTestQueryData& query ) const;																		// custom external queries

	template < class TQuery >
	Bool			TFindWalkablePlace( TQuery* query, Vector2& outPosition ) const;

	Bool			TestLocation(const Vector2& pos, Float radius);																					
	Float			GetClosestFloorInRange(const Vector3& pos, Float radius, Vector3& pointOut);

	Float			ComputeHeight( const Vector2& localPos, Bool smooth = false ) const;

	void			OnPreLoad( CAreaDescription* area );
	void			OnPostLoad( CAreaDescription* area );

	// CAreaRes interface
	Bool			VHasChanged() const override;
	Bool			VSave( const String& depotPath ) const override;
	void			VOnPreLoad( CAreaDescription* area ) override;
	Bool			VLoad( const String& depotPath, CAreaDescription* area ) override;
	void			VOnPostLoad( CAreaDescription* area ) override;
	const Char*		VGetFileExtension() const override;
	ENavResType		VGetResType() const override;

	static const Char*			GetFileExtension()									{ return TXT("navtile"); }
	static ENavResType			GetResType()										{ return NavRes_Terrain; }
};

////////////////////////////////////////////////////////////////////////////
class CTerrainAreaDescription : public CAreaDescription
{
	typedef CAreaDescription Super;
public:
	static const Float				HEIGHT_UNSET;

	enum EInfo
	{
		SPATIALTESTS_IN_LOCAL_SPACE				= true
	};

	CTerrainAreaDescription();
	CTerrainAreaDescription( CPathLibWorld& pathlib, Id id );

	~CTerrainAreaDescription();

	void				VLocalToWorld( Box& v ) const override;
	void				VWorldToLocal( Box& v ) const override;
	void				VLocalToWorld( Vector3& v ) const override;
	void				VWorldToLocal( Vector3& v ) const override;
	void				VLocalToWorld( Vector2& v ) const override;
	void				VWorldToLocal( Vector2& v ) const override;

	void				LocalToWorld( Box& v ) const;
	void				WorldToLocal( Box& v ) const;
	void				LocalToWorld( Vector3& v ) const;
	void				WorldToLocal( Vector3& v ) const;
	void				LocalToWorld( Vector2& v ) const;
	void				WorldToLocal( Vector2& v ) const;

	Bool				VSpatialQuery( CCircleQueryData& query ) const override;
	Bool				VSpatialQuery( CClosestObstacleCircleQueryData& query ) const override;
	Bool				VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const override;
	Bool				VSpatialQuery( CLineQueryData& query ) const override;
	Bool				VSpatialQuery( CWideLineQueryData& query ) const override;
	Bool				VSpatialQuery( CClosestObstacleWideLineQueryData& query ) const override;
	Bool				VSpatialQuery( CClearWideLineInDirectionQueryData& query ) const override;
	Bool				VSpatialQuery( CRectangleQueryData& query ) const override;
	Bool				VSpatialQuery( CCustomTestQueryData& query ) const override;
	Bool				VSpatialQuery( CCollectGeometryInCirceQueryData& query ) const override;

	template < class TQuery >
	Bool				SpatialQuery( TMultiAreaSpatialQuery< TQuery >& query ) const { return TMultiAreaQuery< decltype( this ) >( query ); }
	template < class TQuery >
	Bool				SpatialQuery( TQuery& query ) const;
	template < class TQuery >
	Bool				LocalSpaceSpatialQuery( TQuery& query ) const;

	Bool				VContainsPoint( const Vector3& v ) const override;
	Bool				VComputeHeight( const Vector3& v, Float& z, Bool smooth = false ) override;
	Bool				VComputeHeight( const Vector2& v, Float minZ, Float maxZ, Float& z, Bool smooth = false ) override;
	Bool				VComputeHeightFrom(const Vector2& pos, const Vector3& posFrom, Float& outHeight, Bool smooth = false ) override;
	Bool				VTestLocation(const Vector3& v1, Uint32 collisionFlags = PathLib::CT_DEFAULT) override;
	Bool				VComputeAverageHeight( const Box& bbox, Float& zAverage, Float& zMin, Float& zMax ) override;
	
	Bool				ContainsPoint( const Vector3& v ) const;
	void				ComputeHeight( const Vector2& v, Float& z, Bool smooth = false ) const;
	void				ComputeHeight( const Vector3& v, Float& z, Bool smooth = false ) const		{ return ComputeHeight( v.AsVector2(), z, smooth ); }
	Bool				TestLocation(const Vector3& v1, Uint32 collisionFlags = PathLib::CT_DEFAULT) const;
	
#ifndef NO_EDITOR_PATHLIB_SUPPORT
	void				GenerateAsync( CAreaGenerationJob* job ) override;
	using				CAreaDescription::GenerateAsync;
	void				GenerateSync() override;
	Bool				PreGenerateSync() override;
	void				PostGenerationSyncProcess() override;
	void				Describe( String& description ) override;
	void				ExtractSurfaceData( CDetailedSurfaceData* surfaceData );
	Bool				ApplySurfaceData( CDetailedSurfaceData* surfaceData );
	Bool				ComputeTerrainData( CTerrainSurfaceProcessingThread* task = nullptr );
	Bool				ComputeTerrainData( CDetailedSurfaceData* surfaceData, CTerrainSurfaceProcessingThread* task = nullptr );
	Bool				CorrectNeighbourList() override;
	Bool				GenerateHeightData();
#endif

	void				VPrecomputeObstacleSpawnData( CObstacleSpawnContext& context ) override;

	void				Initialize() override;
	Float				GetMaxNodesDistance() const override;
	void				WriteToBuffer( CSimpleBufferWriter& writer ) override;
	Bool				ReadFromBuffer( CSimpleBufferReader& reader ) override;
	void				SetPathLib( CPathLibWorld* pathlib ) override;
	Bool				IterateAreaResources( ResourceFunctor& functor ) override;

	void				OnPreUnload() override;
	void				OnPostLoad() override;

	AreaId				GetNeighbourId( Int32 xDiff, Int32 yDiff ) const;
	void				GatherPossibleConnectors( AreaId neighbourAreaId, TDynArray< Vector3 >& outLocations ) const;

	void				GetAreaFileList( TDynArray< CDiskFile* >& fileList, Bool onlyChanged = false ) override;

	RED_INLINE CTerrainMap*	GetTerrainMap() const								{ return m_terrain.Get(); }	
	const Vector2&		GetCorner() const										{ return m_tileCorner; }
	void				GetTileCoords( Int32& x, Int32& y ) const;

	template < class TQuery >
	Bool				InternalLocalSpatialTest( TQuery& query ) const			{ return m_terrain.Get()->SpatialQuery( query ); }

	// Navmesh marking functionality
	void				RemarkInstancesAtLocation( CDetailedSurfaceData* surface, const Vector2& localMin, const Vector2& localMax, AreaId ignoreInstance = INVALID_AREA_ID );
	Bool				RemarkInstancesAtLocation( const Vector2& localMin, const Vector2& localMax, AreaId ignoreInstance = INVALID_AREA_ID );											// legacy interface
	Bool				Mark( CDetailedSurfaceData* surface, CNavmeshAreaDescription* navi, Bool markNavmeshConnections = true );

#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	const CTerrainHeightComputationContext& GetHeightData() const				{ return m_heightData; }
	CTerrainHeightComputationContext& GetHeightData()							{ return m_heightData; }
#endif

protected:
	Bool				IsQuadOccupiedByInstance( const Vector2& quadMin, const Vector2& quadMax, CNavmeshAreaDescription* naviArea );
	
#ifndef NO_EDITOR_PATHLIB_SUPPORT
	Bool				ProcessDirtySurface( IGenerationManagerBase::CAsyncTask** outJob ) override;
	Bool				ProcessDirtyCollisionData() override;
	Bool				RemarkInstances() override;
#endif
	void				ComputeBBox();

	CTerrainMapResPtr					m_terrain;
	Vector2								m_tileCorner;
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	CTerrainHeightComputationContext	m_heightData;
#endif
};

//typedef TAreaHandler< CTerrainAreaDescription > CTerrainAreaHandler;

RED_INLINE CTerrainAreaDescription* CAreaDescription::AsTerrainArea()
{
	ASSERT( IsTerrainArea() );
	return static_cast< CTerrainAreaDescription* >( this );
}

RED_INLINE const CTerrainAreaDescription* CAreaDescription::AsTerrainArea() const
{
	ASSERT( IsTerrainArea() );
	return static_cast< const CTerrainAreaDescription* >( this );
}

};		// namespace PathLib

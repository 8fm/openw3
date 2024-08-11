#pragma once

#include "pathlibAreaDescription.h"
#include "pathlibNavmesh.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
class CNavmeshAreaDescription : public CAreaDescription
{
	typedef CAreaDescription Super;
public:
	CNavmeshAreaDescription()
		: CAreaDescription()
		, m_useTransformation( false )										{}
	CNavmeshAreaDescription( CPathLibWorld& world, Id id = INVALID_AREA_ID )
		: CAreaDescription( world, id )
		, m_useTransformation( false )										{}

	Bool				IsUsingTransformation() const						{ return m_useTransformation; }

	Bool				VSpatialQuery( CCircleQueryData& query ) const override;
	Bool				VSpatialQuery( CClosestObstacleCircleQueryData& query ) const override;
	Bool				VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const override;
	Bool				VSpatialQuery( CLineQueryData& query ) const override;
	Bool				VSpatialQuery( CWideLineQueryData& query ) const override;
	Bool				VSpatialQuery( CClosestObstacleWideLineQueryData& query ) const override;
	Bool				VSpatialQuery( CClearWideLineInDirectionQueryData& query ) const override;
	Bool				VSpatialQuery( CCollectGeometryInCirceQueryData& query ) const override;
	Bool				VSpatialQuery( CRectangleQueryData& query ) const override;
	Bool				VSpatialQuery( CCustomTestQueryData& query ) const override;

	template < class TQuery >
	Bool				SpatialQuery( TMultiAreaSpatialQuery< TQuery >& query ) const { return TMultiAreaQuery< decltype( this ) >( query ); }
	template < class TQuery >
	Bool				SpatialQuery( TQuery& query ) const;
	template < class TQuery >
	Bool				LocalSpaceSpatialQuery( TQuery& query ) const;

	Bool				ComputeHeight( const Vector3& v, Float& z );
	Bool				ComputeHeight( const Vector2& v, Float minZ, Float maxZ, Float& z );
	Bool				ComputeHeightFrom( const Vector2& pos, const Vector3& posFrom, Float& outHeight );

	Bool				VContainsPoint( const Vector3& v ) const override;
	Bool				VTestLocation( const Vector3& v1, Uint32 collisionFlags = PathLib::CT_DEFAULT ) override;
	Bool				VComputeHeight( const Vector3& v, Float& z, Bool smooth = false ) override;
	Bool				VComputeHeight( const Vector2& v, Float minZ, Float maxZ, Float& z, Bool smooth = false ) override;
	Bool				VComputeHeightFrom( const Vector2& pos, const Vector3& posFrom, Float& outHeight, Bool smooth = false ) override;
	Bool				VComputeAverageHeight( const Box& bbox, Float& zAverage, Float& zMin, Float& zMax ) override;

	Bool				ContainsPoint( const Vector3& v ) const;
	Bool				TestLocation( const Vector3& v1, Uint32 collisionFlags );

	CNavmesh*			GetNavmesh() const									{ return m_navmesh.Get(); }
	void				SetNavmesh( CNavmesh* navi, const String& externalNavmeshPath = String::EMPTY );
	CNavmeshResPtr&		GetNavmeshHandle()									{ return m_navmesh; }
	const CGUID&		GetOwnerGUID() const								{ return m_ownerComponent; }
	virtual void		NoticeEngineComponent( CNavmeshComponent* component );
	void				WriteToBuffer( CSimpleBufferWriter& writer ) override;
	Bool				ReadFromBuffer( CSimpleBufferReader& reader ) override;


#ifndef NO_EDITOR_PATHLIB_SUPPORT
	void				GenerateAsync( CAreaGenerationJob* job ) override;
	using				CAreaDescription::GenerateAsync;
	void				GenerateSync() override;
	Bool				PreGenerateSync() override;
	void				Describe( String& description ) override;
	Bool				CorrectNeighbourList() override;
#endif

	void				ConnectWithNeighbours() override;
	void				ClearNeighboursConnection();
	Float				GetMaxNodesDistance() const override;

	void				OnPostLoad() override;

	void				OnRemoval() override;
	virtual void		OnNavmeshUpdated();
	Bool				IterateAreaResources( ResourceFunctor& functor ) override;
	

	void				SetInstanceMapBoundings( Int16 minX, Int16 minY, Int16 maxX, Int16 maxY )			{ m_instanceMapBoundings[ 0 ] = minX; m_instanceMapBoundings[ 1 ] = minY; m_instanceMapBoundings[ 2 ] = maxX; m_instanceMapBoundings[ 3 ] = maxY; }
	void				GetInstanceMapBoundings( Int16& minX, Int16& minY, Int16& maxX, Int16& maxY ) const	{ minX = m_instanceMapBoundings[ 0 ]; minY = m_instanceMapBoundings[ 1 ]; maxX = m_instanceMapBoundings[ 2 ]; maxY = m_instanceMapBoundings[ 3 ]; }

	void				GatherNeighbourAreas( TSortedArray< AreaId >& outAreas ) const;
	void				GatherPossibleConnectors( AreaId neighbourId, TDynArray< Vector3 >& outLocations ) const;
	void				DetermineNeighbourAreas();

	AreaId				GetNeighbourAreaId( Uint32 neighbourId ) const		{ return neighbourId < m_neighbourAreas.Size() ? m_neighbourAreas[ neighbourId ] : INVALID_AREA_ID; }

	template < class TQuery >
	Bool				InternalLocalSpatialTest( TQuery& query ) const		{ CNavmesh* navmesh = m_navmesh.Get(); return navmesh ? navmesh->SpatialQuery( query, this ) : false; }
protected:
	CNavmeshAreaDescription( Bool useTransformation )
		: CAreaDescription()
		, m_useTransformation( useTransformation )							{}
	CNavmeshAreaDescription( CPathLibWorld& world, Id id, Bool useTransformation )
		: CAreaDescription( world, id )
		, m_useTransformation( useTransformation )							{}

	void				Clear() override;

	CNavmeshResPtr						m_navmesh;
	CGUID								m_ownerComponent;
	Bool								m_useTransformation;

	Int16								m_instanceMapBoundings[4];

	TDynArray< AreaId >					m_neighbourAreas;
	//TDynArray< Vector2 >				m_convexBoundings;
};

////////////////////////////////////////////////////////////////////////////
class CNavmeshTransformedAreaDescription : public CNavmeshAreaDescription
{
	typedef CNavmeshAreaDescription Super;
public:
	enum EInfo
	{
		SPATIALTESTS_IN_LOCAL_SPACE				= true
	};


	CNavmeshTransformedAreaDescription()
		: CNavmeshAreaDescription( true )
		, m_position( 0, 0, 0 )
		, m_yaw( 0.f )														{}
	CNavmeshTransformedAreaDescription( CPathLibWorld& world, Id id = INVALID_AREA_ID )
		: CNavmeshAreaDescription( world, id, true )
		, m_position( 0, 0, 0 )
		, m_yaw( 0.f )														{}

	void				ComputeTransformation( Matrix& m );

	void				VLocalToWorld( Box& v ) const override;
	void				VWorldToLocal( Box& v ) const override;
	void				VLocalToWorld( Vector3& v ) const override;
	void				VWorldToLocal( Vector3& v ) const override;
	void				VLocalToWorld( Vector2& v ) const override;
	void				VWorldToLocal( Vector2& v ) const override;
	Float				VLocalToWorldZ( Float z ) const override;
	Float				VWorldToLocalZ( Float z ) const override;

	void				LocalToWorld( Box& v ) const;
	void				WorldToLocal( Box& v ) const;
	void				LocalToWorld( Vector3& v ) const;
	void				WorldToLocal( Vector3& v ) const;
	void				LocalToWorld( Vector2& v ) const;
	void				WorldToLocal( Vector2& v ) const;
	Float				LocalToWorldZ( Float z ) const						{ return z + m_position.Z; }
	Float				WorldToLocalZ( Float z ) const						{ return z - m_position.Z; }

	Bool				VSpatialQuery( CCircleQueryData& query ) const override;
	Bool				VSpatialQuery( CClosestObstacleCircleQueryData& query ) const override;
	Bool				VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const override;
	Bool				VSpatialQuery( CLineQueryData& query ) const override;
	Bool				VSpatialQuery( CWideLineQueryData& query ) const override;
	Bool				VSpatialQuery( CClosestObstacleWideLineQueryData& query ) const override;
	Bool				VSpatialQuery( CClearWideLineInDirectionQueryData& query ) const override;
	Bool				VSpatialQuery( CCollectGeometryInCirceQueryData& query ) const override;
	Bool				VSpatialQuery( CRectangleQueryData& query ) const override;
	Bool				VSpatialQuery( CCustomTestQueryData& query ) const override;

	template < class TQuery >
	Bool				SpatialQuery( TMultiAreaSpatialQuery< TQuery >& query ) const { return TMultiAreaQuery< decltype( this ) >( query ); }
	template < class TQuery >
	Bool				SpatialQuery( TQuery& query ) const;

	Bool				ComputeHeight( const Vector3& v, Float& z );
	Bool				ComputeHeight( const Vector2& v, Float minZ, Float maxZ, Float& z );
	Bool				ComputeHeightFrom( const Vector2& pos, const Vector3& posFrom, Float& outHeight );

	Bool				VContainsPoint( const Vector3& v ) const override;
	Bool				VTestLocation(const Vector3& v1, Uint32 collisionFlags = PathLib::CT_DEFAULT) override;
	Bool				VComputeHeight( const Vector3& v, Float& z, Bool smooth = false ) override;
	Bool				VComputeHeight( const Vector2& v, Float minZ, Float maxZ, Float& z, Bool smooth = false ) override;
	Bool				VComputeHeightFrom(const Vector2& pos, const Vector3& posFrom, Float& outHeight, Bool smooth = false ) override;
	Bool				VComputeAverageHeight( const Box& bbox, Float& zAverage, Float& zMin, Float& zMax ) override;

	Bool				ContainsPoint( const Vector3& v ) const;
	Bool				TestLocation( const Vector3& v1, Uint32 collisionFlags );

	void				OnNavmeshUpdated() override;
	void				NoticeEngineComponent( CNavmeshComponent* component ) override;
	void				WriteToBuffer( CSimpleBufferWriter& writer ) override;
	Bool				ReadFromBuffer( CSimpleBufferReader& reader ) override;

protected:
	Vector3								m_position;
	Float								m_yaw;								// radians
	Float								m_yawSin;							// precomputed sine
	Float								m_yawCos;							// precomputed cosine
};


// typedef TAreaHandler< CNavmeshAreaDescription > CNavmeshAreaHandler;
// typedef TAreaHandler< CNavmeshTransformedAreaDescription > CNavmeshTransformedAreaHandler;


RED_INLINE CNavmeshAreaDescription* CAreaDescription::AsNavmeshArea()
{
	ASSERT( IsNavmeshArea() );
	return static_cast< CNavmeshAreaDescription* >( this );
} 
RED_INLINE CNavmeshTransformedAreaDescription* CAreaDescription::AsTransformedNavmeshArea()
{
	ASSERT( IsNavmeshArea() && static_cast< CNavmeshAreaDescription* >( this )->IsUsingTransformation());
	return static_cast< CNavmeshTransformedAreaDescription* >( this );
}

RED_INLINE const CNavmeshAreaDescription* CAreaDescription::AsNavmeshArea() const
{
	ASSERT( IsNavmeshArea() );
	return static_cast< const CNavmeshAreaDescription* >( this );
} 
RED_INLINE const CNavmeshTransformedAreaDescription* CAreaDescription::AsTransformedNavmeshArea() const
{
	ASSERT( IsNavmeshArea() && static_cast< const CNavmeshAreaDescription* >( this )->IsUsingTransformation());
	return static_cast< const CNavmeshTransformedAreaDescription* >( this );
}


};			// namespace PathLib
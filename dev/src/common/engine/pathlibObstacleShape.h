/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "pathlibSpatialQuery.h"
#include "pathlibAreaDescription.h"

class CSimpleBufferWriter;
class CSimpleBufferReader;

namespace PathLib
{

class CObstacleShapePoly;
class CObstacleShapeLineSegment;
class CObstacleShapeCircle;
class CObstacleShapeBox;
class CObstacleShapeComposite;


////////////////////////////////////////////////////////////////////////////
// Basic shape describing pathfinding obstacle. Virtual interface allow us to
// create custom shapes like - circles, composites, transformed.
class CObstacleShape
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_PathLib );

protected:
	static const Float SHAPE_COMPARISION_PRECISION;
	static const Float SHAPE_COMPARISION_PRECISION_SQ;

	Vector3				m_bbox[2];

	virtual Bool VCompareShape( CObstacleShape* shape ) = 0;
public:
	enum EClass
	{
		CLASS_POLY,
		CLASS_COMPOSITE,
		CLASS_LINE,
		CLASS_CIRCLE,
		CLASS_BOX,
		CLASS_COUNT
	};

	CObstacleShape()																{}	// notice empty default constructor
	virtual ~CObstacleShape();

	static CObstacleShape*	NewFromBuffer( CSimpleBufferReader& reader );
	virtual void		WriteToBuffer( CSimpleBufferWriter& writer );
	virtual	Bool		ReadFromBuffer( CSimpleBufferReader& reader );
	virtual EClass		GetClass() const = 0;

	RED_INLINE Bool	TestBoundings( const Vector2& min, const Vector2& max )
	{
		if(m_bbox[0].X > max.X || min.X > m_bbox[1].X) return false;
		if(m_bbox[0].Y > max.Y || min.Y > m_bbox[1].Y) return false;

		return true;
	}

	RED_INLINE Bool	TestBoundings( const Vector3& min, const Vector3& max )
	{
		if(m_bbox[0].X > max.X || min.X > m_bbox[1].X) return false;
		if(m_bbox[0].Y > max.Y || min.Y > m_bbox[1].Y) return false;
		if(m_bbox[0].Z > max.Z || min.Z > m_bbox[1].Z) return false;

		return true;
	}
	RED_INLINE Bool	TestBoundings( const Vector3& point )
	{
		if(m_bbox[0].X > point.X || point.X > m_bbox[1].X) return false;
		if(m_bbox[0].Y > point.Y || point.Y > m_bbox[1].Y) return false;
		if(m_bbox[0].Z > point.Z + DEFAULT_AGENT_HEIGHT || point.Z - DEFAULT_AGENT_HEIGHT > m_bbox[1].Z) return false;

		return true;
	}

	virtual Bool		VTestLocation( const Vector3& v ) const = 0;
	virtual Bool		VSpatialQuery( CCircleQueryData& query, const Vector3* bbox ) const = 0;
	virtual Bool		VSpatialQuery( CClosestObstacleCircleQueryData& query, const Vector3* bbox ) const = 0;
	virtual Bool		VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query, const Vector3* bbox ) const = 0;
	virtual Bool		VSpatialQuery( CLineQueryData& query, const Vector3* bbox ) const = 0;
	virtual Bool		VSpatialQuery( CWideLineQueryData& query, const Vector3* bbox ) const = 0;
	virtual Bool		VSpatialQuery( CClosestObstacleWideLineQueryData& query, const Vector3* bbox ) const = 0;
	virtual Bool		VSpatialQuery( CClearWideLineInDirectionQueryData& query, const Vector3* bbox ) const = 0;
	virtual Bool		VSpatialQuery( CCollectGeometryInCirceQueryData& query, const Vector3* bbox ) const = 0;
	virtual Bool		VSpatialQuery( CRectangleQueryData& query, const Vector3* bbox ) const = 0;
	virtual Bool		VSpatialQuery( CPolyTestQueryData& query, const Vector3* bbox ) const = 0;
	virtual Bool		VSpatialQuery( CCustomTestQueryData& query, const Vector3* bbox ) const = 0;

	virtual void		VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour );

	virtual CObstacleShapePoly*			AsPoly();
	virtual CObstacleShapeLineSegment*	AsLineSegment();
	virtual CObstacleShapeCircle*		AsCircle();
	virtual CObstacleShapeBox*			AsBox();
	virtual CObstacleShapeComposite*	AsComposite();
	

	const Vector3&		GetBBoxMin() const										{ return m_bbox[ 0 ]; }
	const Vector3&		GetBBoxMax() const										{ return m_bbox[ 1 ]; }
	void				InitializeBBox( const Vector3& bbMin, const Vector3& bbMax );

	// 'Contains' function guarantee that on success (true) 'shape' is contained inside 'this'.
	// IMPORTANT: It DO NOT guarantee that 'shape' is NOT contained in 'this' on failure (false).
	Bool				Contains( CObstacleShape* shape );
	Bool				CompareShape( CObstacleShape* shape );

protected:
	Bool				ComputeDetourPoint( CAreaDescription* area, Float personalSpace, const Vector2& basePoint, const Vector2& deltaPosition, Vector3& outDetourPoint );
	Bool				ComputeLineDetour( CAreaDescription* area, Float personalSpace, const Vector2& v1, const Vector2& v2, ObstacleDetour& outDetour );
};

class CObstacleShapeLineSegment : public CObstacleShape
{
	friend class CObstaclesMap;
	typedef CObstacleShape Super;
protected:
	Vector2				m_v1;
	Vector2				m_v2;

	Bool VCompareShape( CObstacleShape* shape ) override;
public:
	enum { POLYCLASS = CLASS_LINE };

	void				WriteToBuffer( CSimpleBufferWriter& writer ) override;
	Bool				ReadFromBuffer( CSimpleBufferReader& reader ) override;
	EClass				GetClass() const override;

	Bool				VTestLocation( const Vector3& v ) const override;
	Bool				VSpatialQuery( CCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClosestObstacleCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CWideLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClosestObstacleWideLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClearWideLineInDirectionQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCollectGeometryInCirceQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CRectangleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CPolyTestQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCustomTestQueryData& query, const Vector3* bbox ) const override;

	void				VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour ) override;

	Vector3				GetVert( Uint32 i ) const;

	CObstacleShapeLineSegment* AsLineSegment() override;
};

class CObstacleShapeCircle : public CObstacleShape
{
	friend class CObstaclesMap;
	typedef CObstacleShape Super;
protected:
	Vector2				m_center;
	Float				m_radius;

	Bool VCompareShape( CObstacleShape* shape ) override;
public:
	enum { POLYCLASS = CLASS_CIRCLE };

	void				WriteToBuffer( CSimpleBufferWriter& writer ) override;
	Bool				ReadFromBuffer( CSimpleBufferReader& reader ) override;
	EClass				GetClass() const override;

	const Vector2&		GetCenter() const											{ return m_center; }
	Float				GetRadius() const											{ return m_radius; }

	Bool				VTestLocation( const Vector3& v ) const override;
	Bool				VSpatialQuery( CCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClosestObstacleCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CWideLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClosestObstacleWideLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClearWideLineInDirectionQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCollectGeometryInCirceQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CRectangleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CPolyTestQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCustomTestQueryData& query, const Vector3* bbox ) const override;

	void				VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour ) override;

	CObstacleShapeCircle*	AsCircle() override;
};

class CObstacleShapeBox : public CObstacleShape
{
	friend class CObstaclesMap;
	typedef CObstacleShape Super;
protected:
	Bool VCompareShape( CObstacleShape* shape ) override;
public:
	enum { POLYCLASS = CLASS_BOX };

	EClass				GetClass() const override;

	Bool				VTestLocation( const Vector3& v ) const override;
	Bool				VSpatialQuery( CCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClosestObstacleCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CWideLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClosestObstacleWideLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClearWideLineInDirectionQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCollectGeometryInCirceQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CRectangleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CPolyTestQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCustomTestQueryData& query, const Vector3* bbox ) const override;

	void				VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour ) override;

	CObstacleShapeBox*	AsBox() override;
};

class CObstacleShapePoly : public CObstacleShape
{
	friend class CObstaclesMap;
	typedef CObstacleShape Super;
protected:
	TDynArray< Vector2 > m_verts;

	Bool VCompareShape( CObstacleShape* shape ) override;
public:
	enum { POLYCLASS = CLASS_POLY };

	void				WriteToBuffer( CSimpleBufferWriter& writer ) override;
	Bool				ReadFromBuffer( CSimpleBufferReader& reader ) override;
	EClass				GetClass() const override;

	Bool				VTestLocation( const Vector3& v ) const override;
	Bool				VSpatialQuery( CCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClosestObstacleCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CWideLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClosestObstacleWideLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClearWideLineInDirectionQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCollectGeometryInCirceQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CRectangleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CPolyTestQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCustomTestQueryData& query, const Vector3* bbox ) const override;

	void				VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour ) override;

	const TDynArray< Vector2 >& GetVerts() const								{ return m_verts; }
	CObstacleShapePoly* AsPoly() override;
};

class CObstacleShapeComposite : public CObstacleShape
{
	friend class CObstaclesMap;
	typedef CObstacleShape Super;
protected:
	TDynArray< CObstacleShape* > m_subShapes;

	Bool VCompareShape( CObstacleShape* shape ) override;
	void ComputeBBox();
public:
	enum { POLYCLASS = CLASS_COMPOSITE };

	~CObstacleShapeComposite();
	void				WriteToBuffer( CSimpleBufferWriter& writer ) override;
	Bool				ReadFromBuffer( CSimpleBufferReader& reader ) override;
	EClass				GetClass() const override;

	template < class TQuery >
	RED_INLINE Bool	TSpatialQuery( TQuery& query, const Vector3* bbox ) const;

	Bool				VTestLocation( const Vector3& v ) const override;
	Bool				VSpatialQuery( CCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClosestObstacleCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CWideLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClosestObstacleWideLineQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CClearWideLineInDirectionQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCollectGeometryInCirceQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CRectangleQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CPolyTestQueryData& query, const Vector3* bbox ) const override;
	Bool				VSpatialQuery( CCustomTestQueryData& query, const Vector3* bbox ) const override;

	void				VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour ) override;

	CObstacleShapeComposite* AsComposite() override;

	const TDynArray< CObstacleShape* >& GetSubShapes() const					{ return m_subShapes; }
};




};			// namespace PathLib
#pragma once

#ifndef NO_OBSTACLE_MESH_DATA

#include "engineTypeRegistry.h"

struct SNavigationObstacleShape
{
	DECLARE_RTTI_STRUCT( SNavigationObstacleShape );

	SNavigationObstacleShape()														{}
	SNavigationObstacleShape( TDynArray< Vector2 >&& verts, const Box& bbox )
		: m_verts( Move( verts ) )
		, m_bbox( bbox )															{}
	SNavigationObstacleShape( const SNavigationObstacleShape& m )
		: m_verts( m.m_verts )
		, m_bbox( m.m_bbox )														{}
	SNavigationObstacleShape( SNavigationObstacleShape&& m )
		: m_verts( Move( m.m_verts ) )
		, m_bbox( m.m_bbox )														{}

	SNavigationObstacleShape& operator=( const SNavigationObstacleShape& other )
		{
			if ( this != &other )
			{
				m_verts = other.m_verts;
				m_bbox = other.m_bbox;
			}
			return *this;
		}

	TDynArray< Vector2 >		m_verts;
	Box							m_bbox;
};

BEGIN_CLASS_RTTI( SNavigationObstacleShape );
	PROPERTY_EDIT( m_verts, TXT("Obstacle vertices") );
	PROPERTY_EDIT( m_bbox, TXT("Bounding box of obstacle") );
END_CLASS_RTTI();



class CNavigationObstacle
{
	DECLARE_RTTI_SIMPLE_CLASS( CNavigationObstacle );
protected:
	TDynArray< SNavigationObstacleShape >	m_shapes;

public:
	CNavigationObstacle()															{}
	~CNavigationObstacle()															{}

	void PushShape( TDynArray< Vector2 >&& verts, const Box& bbox );
	Bool IsEmpty() const															{ return m_shapes.Empty(); }
	
	const TDynArray< SNavigationObstacleShape >& GetShapes() const					{ return m_shapes; }
};

BEGIN_CLASS_RTTI( CNavigationObstacle );
	PROPERTY_INLINED( m_shapes, TXT("Obstacle shapes") );
END_CLASS_RTTI();

#endif
#include "build.h"
#include "navigationObstacle.h"

#ifndef NO_OBSTACLE_MESH_DATA

IMPLEMENT_ENGINE_CLASS( SNavigationObstacleShape );
IMPLEMENT_ENGINE_CLASS( CNavigationObstacle );

void CNavigationObstacle::PushShape( TDynArray< Vector2 >&& verts, const Box& bbox )
{
	m_shapes.PushBack( Move( SNavigationObstacleShape( Move( verts ), bbox ) ) );
}

#endif
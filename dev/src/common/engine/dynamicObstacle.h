/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once


///////////////////////////////////////////////////////////////////////////////

/// Interface of a dynamic obstacle.
class IDynamicObstacle
{
public:
	virtual ~IDynamicObstacle() {}

	// Moves the obstacle to a different location.
	virtual Bool Move( const Vector& newPos ) = 0;
};

///////////////////////////////////////////////////////////////////////////////

class IDynamicObstaclesFactory
{
public:
	virtual ~IDynamicObstaclesFactory() {}

	virtual IDynamicObstacle* CreateDynamicObstacle( const Matrix& localToWorld, const Box& boundingBox) = 0;
};

///////////////////////////////////////////////////////////////////////////////

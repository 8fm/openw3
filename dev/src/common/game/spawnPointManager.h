/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

// Simplifies finding appropriate spawn points for NPCs
namespace SpawnPointManager
{
	Bool IsSpawnPointAppropriate( const Vector &testPoint, Bool isAppear );

	//////////////////////////////////////////////////////////////////////////
	// Tests methods
	//////////////////////////////////////////////////////////////////////////

	// Returns true if player can see 'testPoint'.
	Bool IsPointSeenByPlayer( const Vector &testPoint );

	// Returns true if 'testPoint' lays on a navigation mesh
	Bool IsPointReachable( const Vector &testPoint );

	// Returns true if 'testPoint' is not occupied by an actor
	Bool IsPointFree( const Vector &testPoint );

	//////////////////////////////////////////////////////////////////////////
	// Methods that return single spawn point
	//////////////////////////////////////////////////////////////////////////

	// Returns random point placed in the circle (center, maxRadius)
	Vector GetRandomPointInDisc( const Vector &center, Float maxRadius );

	Vector GetRandomPointInAnnulus( const Vector &center, Float minRadius, Float maxRadius );

	Int32 GetPointsInAnnulus( const Vector &center, Float minRadius, Float maxRadius,
		Float stepRadius, Float stepAngle, TDynArray< Vector > &resultPoints /* out */ );

	Vector GetRandomPointInCircle( const Vector &center, Float radius );

	Vector GetRandomPointInPie( const Vector &center, Float radius, Float yawAngleBeg, Float yawAngleEnd );

	Vector GetPointInCircle( const Vector &center, Float radius, Float angle );

	// Tries to find random reachable point, returns true if succeed.
	// If the one call of this method fails (returns 'false') there is still chance that the next
	// call will succeed.
	Bool GetRandomReachablePoint( const Vector &center, Float maxRadius, Vector &spawnPoint /* out */ );
	Bool GetRandomReachablePoint( const Vector &center, Float minRadius, Float maxRadius, Vector &spawnPoint /* out */ );
	Bool GetRandomReachablePoint( const Vector &center, Float minRadius, Float maxRadius, CAreaComponent *area, Vector &spawnPoint /* out */ );

	// Returns true if reachable, free point can be found within circle (center, maxRadius)
	Bool GetFreeReachablePoint( const Vector &center, Float maxRadius, Vector &spawnPoint /* out */ );

	//////////////////////////////////////////////////////////////////////////
	// Methods that generate multiple spawn points
	//////////////////////////////////////////////////////////////////////////

	void GenerateSpawnPoints( const Vector &center, Float maxRadius, TDynArray< Vector > &spawnPoints /* out */ );
};

void RegisterSpawnPointManagerFunctions();

/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "traceTool.h"

//////
// byDex: original cached CTraceTool with havok phantom was removed because it was SLOWER than this one.
// The reason for that was that every time MAC was moved havok phantom was updated by resyncing
// in the broadphase. Then linear cast was done on that phantom which was O(n) in terms of bodies in the phantom
// because there is no hierarchy of objects collected inside the phantom. Current method is using direct
// linearCast on the world which is faster when done every frame from different position which is the case
// anyway for MOST of the actors ( even the idle animation has some sligh movement :( )
// If we want to return to the phantom method it needs to be made more convervative - e.g. phantom should
// be large enough to compensate for few frames of actor movement and should not be reattached to broadphase
// every frame. Also separate interface like this one is needed anyway for one time queries like when spawning an actor 
// or checking if teleport destination is OK.
/////

const static Float	Z_DETECT_RAY_LENGTH( 3.f );	// length of a whole ray	  
const static Float	Z_DETECT_RAY_UP_LENGTH( 1.5f ); // length of upper part of the ray (part, which is above navmesh)
const static Vector	EXTEND_MARGIN( 0.2f, 0.2f, 0.2f, 0.f ); // aabb margin, for safety reasons
const static Vector	Z_DETECT_RAY_DOWN( 0.f, 0.f, -Z_DETECT_RAY_LENGTH + Z_DETECT_RAY_UP_LENGTH, 0.f ); // ray vector (part below the navmesh)
const static Vector	Z_DETECT_RAY_UP( 0.f, 0.f, Z_DETECT_RAY_UP_LENGTH, 0.f ); // ray vector (part above the navmesh)


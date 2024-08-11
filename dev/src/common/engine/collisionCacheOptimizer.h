/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Optimizer for collision cache - it sorts all of the token in the collision cache by size
/// This allows us to preload the smallest chunks into the memory
class CCollisionCacheOptimizer
{
public:
	CCollisionCacheOptimizer();

	// Optimize collision cache from input file, write optimized collision file as the output file
	Bool OptimizeFile( const String& inputCacheFile, const String& outputCacheFile );
};
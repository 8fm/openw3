#include "build.h"
#include "shortestPathRater.h"

#include "pathlibGraph.h"

PathLib::PathCost CShortestPathRater::CountRealRate( const Vector3& from, const PathLib::CPathLink* pathLink )
{
	return pathLink->GetCost();
}

PathLib::PathCost CShortestPathRater::CountHeurusticRate( const Vector3& from, const Vector3& to )
{
	return PathLib::CalculatePathCost( ( from - to ).Mag() );
}
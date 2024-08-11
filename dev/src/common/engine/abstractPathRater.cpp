#include "build.h"

#include "abstractPathRater.h"

#include "pathlibGraph.h"

void IPathRater::PathFindingStarted()
{

}

void IPathRater::PathFindingEnded()
{

}

PathLib::PathCost IPathRater::CountRealRate( const Vector3& from, const PathLib::CPathLink* pathLink )
{
	return pathLink->GetCost();
}
PathLib::PathCost IPathRater::CountHeurusticRate( const Vector3& from, const Vector3& to )
{
	return PathLib::CalculatePathCost( ( from - to ).Mag() );
}
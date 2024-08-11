#pragma once

#include "build.h"
#include "abstractPathRater.h"

class CShortestPathRater : public IPathRater
{
public:
	PathLib::PathCost CountRealRate( const Vector3& from, const PathLib::CPathLink* pathLink ) override;
	PathLib::PathCost CountHeurusticRate( const Vector3& from, const Vector3& to ) override;
};
/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/pathlibTerrainFoliage.h"


class CTerrainCookerFoliageProcessingThread : public PathLib::CTerrainFoliageProcessingThread
{
	typedef PathLib::CTerrainFoliageProcessingThread Super;
protected:
	void							GetTreeCollisionShapes( const CSRTBaseTree* tree, TDynArray< Sphere >& outShapes ) override;
public:
	CTerrainCookerFoliageProcessingThread( PathLib::CTerrainAreaDescription* area )
		: Super( area )																	{}

	~CTerrainCookerFoliageProcessingThread();
};

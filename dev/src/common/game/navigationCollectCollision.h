/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/pathlibCollectCollisionPointsSpatialQuery.h"

// Important to expose pathlib query proxy data to rtti. That is needed to eg. store them in InstanceVar
struct SNavigationCollectCollisionInCircleData
{
	DECLARE_RTTI_STRUCT( SNavigationCollectCollisionInCircleData )

	PathLib::CCollectCollisionPointsInCircleProxy	m_navProxy;
};

BEGIN_NODEFAULT_CLASS_RTTI( SNavigationCollectCollisionInCircleData )
END_CLASS_RTTI()
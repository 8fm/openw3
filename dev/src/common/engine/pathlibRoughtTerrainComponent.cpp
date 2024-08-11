/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibRoughtTerrainComponent.h"

#include "pathlibConst.h"
#include "pathlibCookerData.h"
#include "pathlibSpecialZoneMap.h"

IMPLEMENT_ENGINE_CLASS( CPathLibRoughtTerrainComponent )


#ifndef NO_EDITOR
void CPathLibRoughtTerrainComponent::OnNavigationCookerInitialization( CWorld* world, CNavigationCookingContext* context )
{
	PathLib::CCookerData* cookerData = context->GetPathlibCookerData();
	if ( cookerData )
	{
		PathLib::NodeFlags clearFlags = m_isRoughtTerrain ? 0 : PathLib::NF_ROUGH_TERRAIN;
		PathLib::NodeFlags forceFlags = m_isRoughtTerrain ? PathLib::NF_ROUGH_TERRAIN : 0;
		cookerData->GetSpecialZones()->Collect( this, clearFlags, forceFlags );
	}
}
#endif

/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "areaComponent.h"

class CPathLibRoughtTerrainComponent : public CAreaComponent
{
	DECLARE_ENGINE_CLASS( CPathLibRoughtTerrainComponent, CAreaComponent, 0 )
protected:
	Bool										m_isRoughtTerrain;

public:
	CPathLibRoughtTerrainComponent()
		: m_isRoughtTerrain( true )															{}

	Bool					IsRoughtTerrain() const											{ return m_isRoughtTerrain; }

#ifndef NO_EDITOR
	void					OnNavigationCookerInitialization( CWorld* world, CNavigationCookingContext* context ) override;
#endif
};

BEGIN_CLASS_RTTI( CPathLibRoughtTerrainComponent )
	PARENT_CLASS( CAreaComponent )
	PROPERTY_EDIT( m_isRoughtTerrain, TXT("If flag is set, navgraph will mark every connection that intersects it as 'rought'. If flag is set to false, navgraph will clear rought terrain marking of every connection that intersects it.") )
END_CLASS_RTTI()

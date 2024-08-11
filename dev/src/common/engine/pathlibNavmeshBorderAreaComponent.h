/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "areaComponent.h"
#include "entityHandle.h"


class CNavmeshBorderAreaComponent : public CAreaComponent
{
	DECLARE_ENGINE_CLASS( CNavmeshBorderAreaComponent, CAreaComponent, 0 );
protected:
	EntityHandle					m_lockedToSpecyficNavmesh;
	Bool							m_isDisabled;
public:
	CNavmeshBorderAreaComponent()
		: m_isDisabled( false )																	{}
	~CNavmeshBorderAreaComponent();


	Bool							IsEffectingNavmesh( CNavmeshComponent* component );
};

BEGIN_CLASS_RTTI( CNavmeshBorderAreaComponent )
	PARENT_CLASS( CAreaComponent )
	PROPERTY_EDIT( m_lockedToSpecyficNavmesh, TXT("If set, it will restrict this border area only to be available from perspective of given component navmesh generation.") )
	PROPERTY_EDIT( m_isDisabled, TXT("If marked this flag will disable this border area from navmesh computations") )
END_CLASS_RTTI()
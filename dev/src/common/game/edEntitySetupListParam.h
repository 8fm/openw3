/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "entityParams.h"

class CEdEntitySetupListParam : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CEdEntitySetupListParam, CGameplayEntityParam, 0 )
protected:
	TDynArray< THandle< IEdEntitySetupEffector > >			m_effectors;
	Bool													m_detachFromTemplate;

public:
	CEdEntitySetupListParam();
	~CEdEntitySetupListParam();

	void					OnSpawn( CEntity* entity );
};


BEGIN_CLASS_RTTI( CEdEntitySetupListParam )
	PARENT_CLASS( CGameplayEntityParam )
	PROPERTY_INLINED( m_effectors, TXT("Entity editor behavior effectors list.") )
	PROPERTY_EDIT( m_detachFromTemplate, TXT("Forcefully detach entity from template." ))
END_CLASS_RTTI()


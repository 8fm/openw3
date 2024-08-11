/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "entityTargetingAction.h"

class CEnableEncounterAction : public IEntityTargetingAction
{
	DECLARE_ENGINE_CLASS( CEnableEncounterAction, IEntityTargetingAction, 0 )
protected:
	Bool						m_enable;

	void						PerformOnEntity( CEntity* entity ) override;
public:
	CEnableEncounterAction()
		: m_enable( false )																{}
};

BEGIN_CLASS_RTTI( CEnableEncounterAction )
	PARENT_CLASS( IEntityTargetingAction )
	PROPERTY_EDIT( m_enable, TXT("Enable/disable encounter") )
END_CLASS_RTTI()
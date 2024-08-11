/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "entityTargetingAction.h"

class CEnabledDeniedAreaAction : public IComponentTargetingAction
{
	DECLARE_ENGINE_CLASS( CEnabledDeniedAreaAction, IComponentTargetingAction, 0 )
protected:
	Bool							m_enable;
	
	void							PerformOnComponent( CComponent* component ) override;
	CClass*							SupportedComponentClass() override;
public:
	CEnabledDeniedAreaAction()
		: m_enable( true )																{}
};


BEGIN_CLASS_RTTI( CEnabledDeniedAreaAction )
	PARENT_CLASS( IComponentTargetingAction )
	PROPERTY_EDIT( m_enable, TXT( "Enable/disable denied area" ) )
END_CLASS_RTTI()


/**
* Copyright ©2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "entityTargetingAction.h"

class CActionPointActivationSwitcher : public IComponentTargetingAction
{
	DECLARE_ENGINE_CLASS( CActionPointActivationSwitcher, IComponentTargetingAction, 0 );

private:
	Bool			m_activate;

private:
	void					PerformOnComponent( CComponent* component ) override;
	CClass*					SupportedComponentClass() override;
public:
	CActionPointActivationSwitcher()
		: m_activate( false )																{}
};

BEGIN_CLASS_RTTI( CActionPointActivationSwitcher );
	PARENT_CLASS( IComponentTargetingAction );
	PROPERTY_EDIT( m_activate			, TXT("Activate or deactivate component") );
END_CLASS_RTTI();
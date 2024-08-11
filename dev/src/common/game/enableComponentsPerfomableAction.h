#pragma once

#include "../../common/game/entityTargetingAction.h"


class CEnableComponentsPerformableAction : public IComponentTargetingAction
{
	DECLARE_ENGINE_CLASS( CEnableComponentsPerformableAction, IComponentTargetingAction, 0 )
protected:
	Bool				m_enable;	
public:
	CEnableComponentsPerformableAction();

	void PerformOnComponent( CComponent* component ) override;
};

BEGIN_CLASS_RTTI( CEnableComponentsPerformableAction	)
	PARENT_CLASS( IComponentTargetingAction )
	PROPERTY_EDIT( m_enable, TXT( "Enable or disable" ) )	
END_CLASS_RTTI()
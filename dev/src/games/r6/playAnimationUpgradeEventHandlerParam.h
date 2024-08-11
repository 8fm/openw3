#pragma once


#include "baseUpgradeEventHandlerParam.h"


class CPlayAnimationUpgradeHandlerParam : public CUpgradeEventHandlerParam
{
	DECLARE_ENGINE_CLASS( CPlayAnimationUpgradeHandlerParam, CUpgradeEventHandlerParam, 0 );

private:
	CName m_bahaviorEventName;
public:
	RED_INLINE CName GetBehaviorEventName(){ return m_bahaviorEventName; }
	RED_INLINE void SetBehaviorEventName( CName val ){ m_bahaviorEventName = val; }

};

BEGIN_CLASS_RTTI( CPlayAnimationUpgradeHandlerParam );
	PARENT_CLASS( CUpgradeEventHandlerParam );
	PROPERTY_EDIT_NAME( m_bahaviorEventName, TXT("i_bahaviorEventName"), TXT("Behavior event, that will be raised") );
END_CLASS_RTTI();

#pragma once


#include "baseUpgradeEventHandlerParam.h"


class CSingleNameUpgradeHandlerParam : public CUpgradeEventHandlerParam
{
	DECLARE_ENGINE_CLASS( CSingleNameUpgradeHandlerParam, CUpgradeEventHandlerParam, 0 );

private:
	CName m_value;
public:
	RED_INLINE CName GetValue(){ return m_value; }
	RED_INLINE void SetValue( CName val ){ m_value = val; }

};

BEGIN_CLASS_RTTI( CSingleNameUpgradeHandlerParam );
PARENT_CLASS( CUpgradeEventHandlerParam );
PROPERTY_EDIT_NAME( m_value, TXT("i_value"), TXT("Value") );
END_CLASS_RTTI();

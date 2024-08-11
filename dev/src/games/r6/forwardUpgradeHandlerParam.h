#pragma once


#include "baseUpgradeEventHandlerParam.h"


class CForwardUpgradeHandlerParam : public CUpgradeEventHandlerParam, public I2dArrayPropertyOwner
{
	DECLARE_ENGINE_CLASS( CForwardUpgradeHandlerParam, CUpgradeEventHandlerParam, 0 );

private:
	CName m_slotName;
	CName m_newEventName;
public:
	RED_INLINE CName GetSlotName(){ return m_slotName; }
	RED_INLINE CName GetNewEventName(){ return m_newEventName; }

	void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties ) override;
};

BEGIN_CLASS_RTTI( CForwardUpgradeHandlerParam );
	PARENT_CLASS( CUpgradeEventHandlerParam );
	PROPERTY_EDIT( m_slotName		, TXT("Slot name")		);	
	PROPERTY_CUSTOM_EDIT( m_newEventName	, TXT("New event name")	, TXT("2daValueSelection")  );
END_CLASS_RTTI();

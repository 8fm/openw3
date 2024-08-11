#pragma once


#include "baseUpgradeEventHendler.h"

class CItemPartDefinitionComponent;

class CForwardEventHandler : public CUpgradeEventHandler
{
	DECLARE_ENGINE_CLASS( CForwardEventHandler, CUpgradeEventHandler, 0 );
private:
	CName m_slotName;
	CName m_newEventName;

public:	
	void HandleEvent( SUpgradeEventHandlerParam& params ) override;
};
	
BEGIN_CLASS_RTTI( CForwardEventHandler )
	PARENT_CLASS( CUpgradeEventHandler );	

	PROPERTY_EDIT( m_slotName		, TXT("Forward destination") );
	PROPERTY_EDIT( m_newEventName	, TXT("New event")			 );
END_CLASS_RTTI();
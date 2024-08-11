#pragma once


#include "baseUpgradeEventHendler.h"

class CItemPartDefinitionComponent;

	
class CBrodecastEventHandler : public CUpgradeEventHandler
{
	DECLARE_ENGINE_CLASS( CBrodecastEventHandler, CUpgradeEventHandler, 0 );
public:	
	void HandleEvent( SUpgradeEventHandlerParam& params ) override;
};

BEGIN_CLASS_RTTI( CBrodecastEventHandler )
	PARENT_CLASS( CUpgradeEventHandler );	
END_CLASS_RTTI();
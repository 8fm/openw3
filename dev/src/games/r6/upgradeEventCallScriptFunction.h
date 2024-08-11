#pragma once

#include "baseUpgradeEventHendler.h"

class CItemPartDefinitionComponent;

class CCallScriptFunctionEventHandler : public CUpgradeEventHandler
{
	DECLARE_ENGINE_CLASS( CCallScriptFunctionEventHandler, CUpgradeEventHandler, 0 );
public:	
	void HandleEvent( SUpgradeEventHandlerParam& params ) override;
};

BEGIN_CLASS_RTTI( CCallScriptFunctionEventHandler )
	PARENT_CLASS( CUpgradeEventHandler );	
END_CLASS_RTTI();
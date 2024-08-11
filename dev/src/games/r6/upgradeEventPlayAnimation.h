#pragma once


#include "baseUpgradeEventHendler.h"

class CItemPartDefinitionComponent;

class CPlayAnimationEventHandler : public CUpgradeEventHandler
{
	DECLARE_ENGINE_CLASS( CPlayAnimationEventHandler, CUpgradeEventHandler, 0 );
public:	
	void HandleEvent( SUpgradeEventHandlerParam& params ) override;
};

BEGIN_CLASS_RTTI( CPlayAnimationEventHandler )
	PARENT_CLASS( CUpgradeEventHandler );	
END_CLASS_RTTI();

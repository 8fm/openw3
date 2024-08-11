#pragma once


#include "baseUpgradeEventHendler.h"

class CItemPartDefinitionComponent;
class CStatsContainerComponent;		

class CFrameStartAutoFireEventHandler : public CUpgradeEventHandler
{
	DECLARE_ENGINE_CLASS( CFrameStartAutoFireEventHandler, CUpgradeEventHandler, 0 );
public:	
	void HandleEvent( SUpgradeEventHandlerParam& params ) override;
};

BEGIN_CLASS_RTTI( CFrameStartAutoFireEventHandler )
	PARENT_CLASS( CUpgradeEventHandler );	
END_CLASS_RTTI();
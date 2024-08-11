#pragma once


#include "baseUpgradeEventHendler.h"

class CItemPartDefinitionComponent;
class CStatsContainerComponent;		

class CFrameReloadEventHandler : public CUpgradeEventHandler
{
	DECLARE_ENGINE_CLASS( CFrameReloadEventHandler, CUpgradeEventHandler, 0 );
public:	
	void HandleEvent( SUpgradeEventHandlerParam& params ) override;
};

BEGIN_CLASS_RTTI( CFrameReloadEventHandler )
	PARENT_CLASS( CUpgradeEventHandler );	
END_CLASS_RTTI();
#pragma once


#include "baseUpgradeEventHendler.h"

class CItemPartDefinitionComponent;
class CStatsContainerComponent;		

class CFrameStopAutoFireEventHandler : public CUpgradeEventHandler
{
	DECLARE_ENGINE_CLASS( CFrameStopAutoFireEventHandler, CUpgradeEventHandler, 0 );
public:	
	void HandleEvent( SUpgradeEventHandlerParam& params ) override;
};

BEGIN_CLASS_RTTI( CFrameStopAutoFireEventHandler )
	PARENT_CLASS( CUpgradeEventHandler );	
END_CLASS_RTTI();
#pragma once


#include "baseUpgradeEventHendler.h"

class CItemPartDefinitionComponent;

class CFrameShootEventHandler : public CUpgradeEventHandler
{
	DECLARE_ENGINE_CLASS( CFrameShootEventHandler, CUpgradeEventHandler, 0 );
private:
	void Reload( SUpgradeEventHandlerParam& params ) const;
	void Shoot( SUpgradeEventHandlerParam& params ) const;
public:	
	void HandleEvent( SUpgradeEventHandlerParam& params ) override;
};

BEGIN_CLASS_RTTI( CFrameShootEventHandler )
	PARENT_CLASS( CUpgradeEventHandler );	
END_CLASS_RTTI();


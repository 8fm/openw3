#include "build.h"

#include "baseUpgradeEventHendler.h"
#include "itemPartDefinitionComponent.h"

IMPLEMENT_ENGINE_CLASS( CScriptedUgpradeEventHandler );
IMPLEMENT_ENGINE_CLASS( CUpgradeEventHandler );
IMPLEMENT_ENGINE_CLASS( SUpgradeEventHandlerParam );

void CScriptedUgpradeEventHandler::HandleEvent( SUpgradeEventHandlerParam& params )
{	
	CallFunction( this, CNAME( Export_HandleEvent ), params );
}
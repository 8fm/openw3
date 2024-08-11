#include "build.h"

#include "upgradeEventCallScriptFunction.h"
#include "itemPartDefinitionComponent.h"
#include "itemPartSlotDefinition.h"

IMPLEMENT_ENGINE_CLASS( CCallScriptFunctionEventHandler );

void CCallScriptFunctionEventHandler::HandleEvent( SUpgradeEventHandlerParam& params ) 
{
	CallFunction( params.m_owner->GetEntity(), params.m_eventName );
}
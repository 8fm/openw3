#include "build.h"

#include "upgradeEventFrameStopAutoFire.h"
#include "firearmFrameDefinitionComponent.h"

IMPLEMENT_ENGINE_CLASS( CFrameStopAutoFireEventHandler );

void CFrameStopAutoFireEventHandler::HandleEvent( SUpgradeEventHandlerParam& params ) 
{
	if( !params.m_owner->IsA< CFirearmFrameDefinitionComponent >() )
	{
		return;
	}		

	CFirearmFrameDefinitionComponent* owner = static_cast< CFirearmFrameDefinitionComponent* >( params.m_owner );		

	owner->StopAutoFire();
}

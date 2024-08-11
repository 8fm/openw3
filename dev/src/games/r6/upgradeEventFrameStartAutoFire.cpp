#include "build.h"

#include "upgradeEventFrameStartAutoFire.h"
#include "firearmFrameDefinitionComponent.h"

IMPLEMENT_ENGINE_CLASS( CFrameStartAutoFireEventHandler );

void CFrameStartAutoFireEventHandler::HandleEvent( SUpgradeEventHandlerParam& params ) 
{
	if( !params.m_owner->IsA< CFirearmFrameDefinitionComponent >() )
	{
		return;
	}	

	if( !params.m_dynamicParams || !params.m_dynamicParams->IsA< CShootUpgradeHandlerParam >() )
	{
		return;
	}

	CFirearmFrameDefinitionComponent* owner = static_cast< CFirearmFrameDefinitionComponent* >( params.m_owner );	
	CShootUpgradeHandlerParam* shootParam = static_cast< CShootUpgradeHandlerParam* >( params.m_dynamicParams  );

	owner->StartAutoFire( shootParam );
}

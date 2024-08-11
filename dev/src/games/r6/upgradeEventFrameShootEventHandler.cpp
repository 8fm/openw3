#include "build.h"

#include "upgradeEventFrameShootEventHandler.h"
#include "itemPartDefinitionComponent.h"
#include "itemPartSlotDefinition.h"
#include "playAnimationUpgradeEventHandlerParam.h"
#include "statsContainerComponent.h"
#include "firearmFrameDefinitionComponent.h"

IMPLEMENT_ENGINE_CLASS( CFrameShootEventHandler );

void CFrameShootEventHandler::HandleEvent( SUpgradeEventHandlerParam& params )
{	
	if( !params.m_owner->IsA< CFirearmFrameDefinitionComponent >() )
	{
		return;
	}

	CFirearmFrameDefinitionComponent* owner = static_cast< CFirearmFrameDefinitionComponent* >( params.m_owner );	
	
	if( owner->ConsumeAmmo( 1 ) )
	{
		Shoot( params );						
	}
	else
	{
		if( !owner->IsReloading() )
		{
			Reload( params );
			owner->StartReloading();
		}			
	}
		
}

void CFrameShootEventHandler::Reload( SUpgradeEventHandlerParam& params ) const
{
	CFirearmFrameDefinitionComponent* owner = static_cast< CFirearmFrameDefinitionComponent* >( params.m_owner );

	// play reload animation
	CPlayAnimationUpgradeHandlerParam* dynamicParm = CreateObject< CPlayAnimationUpgradeHandlerParam >( owner );
	dynamicParm->SetBehaviorEventName( CNAME( Reload ) );
	owner->SendToParent( CNAME( OnPlayAnimation ), dynamicParm );	

	// stop automatic fire 
	owner->SendToParent( CNAME( OnStopShooting ), NULL );
}

void CFrameShootEventHandler::Shoot( SUpgradeEventHandlerParam& params  ) const
{
	CFirearmFrameDefinitionComponent* owner = static_cast< CFirearmFrameDefinitionComponent* >( params.m_owner );
	if( owner->ForwardEvent( CNAME( barrel ), params.m_eventName, params.m_dynamicParams ) )
	{		
		// play animation
		CPlayAnimationUpgradeHandlerParam* dynamicParm = CreateObject< CPlayAnimationUpgradeHandlerParam >( owner );
		dynamicParm->SetBehaviorEventName( CNAME( Shoot ) );
		owner->SendToParent( CNAME( OnPlayAnimation ), dynamicParm );

		// fire "on shot fired" event
		owner->SendToParent( CNAME( OnShotFired ), NULL );
	}
}
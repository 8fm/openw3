#include "build.h"

#include "upgradeEventPlayAnimation.h"
#include "itemPartDefinitionComponent.h"
#include "itemPartSlotDefinition.h"
#include "playAnimationUpgradeEventHandlerParam.h"

IMPLEMENT_ENGINE_CLASS( CPlayAnimationEventHandler );

void CPlayAnimationEventHandler::HandleEvent( SUpgradeEventHandlerParam& params )
{
	CItemPartDefinitionComponent* owner = params.m_owner;
	CUpgradeEventHandlerParam* dynamicParams = params.m_dynamicParams;

	if( dynamicParams && dynamicParams->IsA< CPlayAnimationUpgradeHandlerParam >() )
	{
		CPlayAnimationUpgradeHandlerParam* snParam = static_cast< CPlayAnimationUpgradeHandlerParam* >( dynamicParams );
		owner->GetEntity()->RaiseBehaviorEvent( snParam->GetBehaviorEventName() );
	}	
}
#include "build.h"

#include "upgradeEventBrodecastEvent.h"
#include "itemPartDefinitionComponent.h"
#include "itemPartSlotDefinition.h"

IMPLEMENT_ENGINE_CLASS( CBrodecastEventHandler );

void CBrodecastEventHandler::HandleEvent( SUpgradeEventHandlerParam& params ) 
{
	CName eventName								= params.m_eventName;
	CName childSlotName							= params.m_childSlotName;
	CItemPartDefinitionComponent* owner			= params.m_owner;
	CUpgradeEventHandlerParam*	dynamicParams	= params.m_dynamicParams;

	const TDynArray< CItemPartSlotDefinition* >& slots = owner->GetSlots();

	for( Uint32 i=0; i<slots.Size(); ++i )
	{
		CItemPartDefinitionComponent* part = slots[ i ]->GetPluggenPart();
		if( part )
		{
			part->ProcessEvent( eventName, dynamicParams, childSlotName );
		}
	}
}
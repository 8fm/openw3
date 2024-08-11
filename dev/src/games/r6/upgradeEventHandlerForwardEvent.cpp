#include "build.h"

#include "upgradeEventHandlerForwardEvent.h"
#include "itemPartDefinitionComponent.h"
#include "itemPartSlotDefinition.h"
#include "forwardUpgradeHandlerParam.h"

IMPLEMENT_ENGINE_CLASS( CForwardEventHandler );

void CForwardEventHandler::HandleEvent( SUpgradeEventHandlerParam& params ) 
{
	
	CName eventName								= params.m_eventName;
	CName childSlotName							= params.m_childSlotName;
	CItemPartDefinitionComponent* owner			= params.m_owner;
	
	CUpgradeEventHandlerParam*	dynamicParams	= params.m_dynamicParams;

	if( !m_newEventName )
		m_newEventName = eventName;

	if( m_slotName == CNAME( parent ) )
	{
		owner->SendToParent( m_newEventName );	
		return;
	}

	const TDynArray< CItemPartSlotDefinition* >& slots = owner->GetSlots();

	for( Uint32 i=0; i<slots.Size(); ++i )
	{
		if( slots[ i ]->GetSlotName() == m_slotName )
		{
			CItemPartDefinitionComponent* part = slots[ i ]->GetPluggenPart();
			if( part )
			{
				part->ProcessEvent( m_newEventName, dynamicParams, childSlotName );
			}
			
		}		
	}
}
#include "build.h"

#include "baseUpgradeEventHendler.h"
#include "itemPartDefinitionComponent.h"
#include "itemPartSlotDefinition.h"
#include "statsContainerComponent.h"
#include "baseUpgradeStatsModyfier.h"
#include "baseUpgradeEventHandlerParam.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/meshSkinningAttachment.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/meshTypeComponent.h"


IMPLEMENT_ENGINE_CLASS( CItemPartDefinitionComponent );
IMPLEMENT_ENGINE_CLASS( CUpgradeEventDefinition );

CGatheredResource resItemPartDefsEventNames			( TXT("gameplay\\globals\\upgradeevents.csv"), RGF_Startup );
CGatheredResource resItemPartDefsEventHandlerNames	( TXT("gameplay\\globals\\upgradeeventshandlers.csv"), RGF_Startup );	

void CUpgradeEventDefinition::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	if( property->GetName() == CNAME( eventName ) )
	{
		valueProperties.m_array = resItemPartDefsEventNames.LoadAndGet< C2dArray >();
		valueProperties.m_valueColumnName = TXT("Event name");
	}
	else if( property->GetName() == CNAME( eventHandlerName ) )
	{
		valueProperties.m_array = resItemPartDefsEventHandlerNames.LoadAndGet< C2dArray >();
		valueProperties.m_valueColumnName = TXT("Handler name");
	}	
}

void CItemPartDefinitionComponent::SpawnSlotsContent()
{
	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		SpawnSlotContent( m_slots[i] );
	}
}

void CItemPartDefinitionComponent::AttachEntityToSlot( CItemPartSlotDefinition* slot, CEntity* entity )
{
	if( entity )
	{
		CItemPartDefinitionComponent* partCmp = entity->FindComponent< CItemPartDefinitionComponent >();

		if( partCmp )
		{

			HandleInParentSlots( partCmp );				
			slot->SetPluggedPart( partCmp );
			partCmp->SetParentPart( this );
			partCmp->SetInParentSlotName( slot->GetAttachmentName() );										
		}

		if( slot->GetAttachmentName() == CNAME( rootAnimatedComponent ) )
		{
			CAnimatedComponent* animComponent = GetEntity()->GetRootAnimatedComponent();
			if( animComponent )
			{
				if( partCmp )
				{
					partCmp->AttachToParentRootIfNeeded( animComponent );
				}						
			}
			entity->CreateAttachmentImpl( GetEntity(), CName::NONE );
		}
		else
		{
			const EntitySlot* entitySlot = GetEntity()->GetEntityTemplate()->FindSlotByName( slot->GetAttachmentName(), true );
			if( entitySlot )
			{
				entity->CreateAttachmentImpl( GetEntity(), slot->GetAttachmentName() );
			}					
			else
			{
				m_slotsUsingParentSlots.PushBack( slot );
			}
		}													
	}
}

void CItemPartDefinitionComponent::SpawnSlotContent( CItemPartSlotDefinition* slot )
{
	if( slot->GetEntityToPlug() )
	{
		EntitySpawnInfo info;
		info.m_template = slot->GetEntityToPlug();

		CEntity * entity = GetLayer()->GetWorld()->GetDynamicLayer()->CreateEntitySync( info );
		AttachEntityToSlot( slot, entity );
	}
}

void CItemPartDefinitionComponent::HandleInParentSlots( CItemPartDefinitionComponent* childCmp )
{
	const TDynArray< CItemPartSlotDefinition* >& slotsUsingParetntSlot = childCmp->GetSlotsUsingParentSlots();

	for( Uint32 j=0; j<slotsUsingParetntSlot.Size(); ++j )
	{
		CItemPartSlotDefinition* slotUsingParent = slotsUsingParetntSlot[ j ];
		const EntitySlot* entitySlot = GetEntity()->GetEntityTemplate()->FindSlotByName( slotUsingParent->GetAttachmentName(), true );
		if( entitySlot )
		{//if slot exists, then plug childs child to this slot
			if( slotUsingParent->GetPluggenPart() )
			{
				slotUsingParent->GetPluggenPart()->GetEntity()->CreateAttachmentImpl( GetEntity(), slotUsingParent->GetAttachmentName() );
			}
		}
		else
		{//if slot does not exist, then try plug it to parent slot
			m_slotsUsingParentSlots.PushBack( slotUsingParent );
		}
	}

	childCmp->ClearSlotUsingParentSlots();
}

void CItemPartDefinitionComponent::AttachToParentRootIfNeeded( CAnimatedComponent* parentRoot )
{
	CAnimatedComponent* animComponent = GetEntity()->GetRootAnimatedComponent();
	if( animComponent )
	{//don't attach to parent skeleton if entity has skeleton itself
		return;
	}

	for ( ComponentIterator< CMeshTypeComponent > meshIterator( GetEntity() ); meshIterator; ++meshIterator )
	{
		CMeshTypeComponent* meshComponent =  *meshIterator;
		if ( !meshComponent->GetTransformParent() )
		{
			parentRoot->Attach( meshComponent, ClassID< CMeshSkinningAttachment >() );
		}
	}
	
	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		CItemPartSlotDefinition* slot = m_slots[i];
		if( slot->GetAttachmentName() == CNAME( rootAnimatedComponent ) )
		{
			if( slot->GetPluggenPart() )
			{
				slot->GetPluggenPart()->AttachToParentRootIfNeeded( parentRoot );
			}
		}
	}
}

void CItemPartDefinitionComponent::ApplyStatsModifications( CStatsContainerComponent* stats )
{
	if( !stats )
		return;

	CEntity* ownerEnt = GetEntity();

	for( Uint32 i=0; i<m_statsModifiers.Size(); ++i )
	{
		if( m_statsModifiers[ i ] )
		{
			m_statsModifiers[ i ]->ApplyChanges( stats, ownerEnt );
		}
	}	

	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		CItemPartSlotDefinition* slot = m_slots[ i ];
		if( !slot->IsEmpty() )
		{
			slot->GetPluggenPart()->ApplyStatsModifications( stats );
		}
	}
}

void CItemPartDefinitionComponent::OnAttachFinishedEditor( CWorld* world )
{
	OnAttachFinished( world );
}

void CItemPartDefinitionComponent::OnAttachFinished( CWorld* world )
{	

	TBaseClass::OnAttachFinished( world );	
	SpawnSlotsContent();
	
	if( GGame->IsActive() )
		DoStatsStuff();	
}

void CItemPartDefinitionComponent::OnDetached( CWorld* world )	
{
	DetachSlots( );
	TBaseClass::OnDetached( world );
}

void CItemPartDefinitionComponent::DoStatsStuff()
{
	CStatsContainerComponent* stats = GetEntity()->FindComponent< CStatsContainerComponent >( );
	if( stats )
	{
		stats->ResetStats();
		ApplyStatsModifications( stats );
	}	
}

CItemPartDefinitionComponent* CItemPartDefinitionComponent::GetPartPluggedToSlot( CName slotName )
{
	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		CItemPartSlotDefinition* slot = m_slots[i];
		if( slot->GetSlotName() == slotName )
		{
			return slot->GetPluggenPart();
		}
	}
	return NULL;
}

void CItemPartDefinitionComponent::ProcessEvent( CName eventName, CUpgradeEventHandlerParam* dynamicParam /* = NULL */, CName childSlotName /* = CName::NONE */ )
{

	Bool found = false;
	for( Uint32 i=0; i<m_avaliableEvents.Size(); ++i )
	{
		if( m_avaliableEvents[i]->GetEventName() == eventName )
		{
			CUpgradeEventHandler*  handler = m_avaliableEvents[i]->GetHandlerInstance();
			if( handler!= NULL )
			{
				SUpgradeEventHandlerParam params;
				params.m_eventName		= eventName;
				params.m_childSlotName	= childSlotName;
				params.m_owner			= this;				
				params.m_dynamicParams	= dynamicParam;

				handler->HandleEvent( params );

				found = true;
			}
		}
	}
	if( !found && childSlotName )
	{
		SendToParent( eventName, dynamicParam );
	}
}

void CItemPartDefinitionComponent::SendToParent( CName eventName, CUpgradeEventHandlerParam* dynamicParams /* = NULL */ )
{
	if( m_parentPart.Get() )
	{
		m_parentPart.Get()->ProcessEvent( eventName, dynamicParams, m_inParentSlotName );
	}	
}

Bool CItemPartDefinitionComponent::ForwardEvent( CName childSlotName, CName eventName, CUpgradeEventHandlerParam* dynamicParams /* = NULL */ )
{
	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		CItemPartSlotDefinition* slot = m_slots[i];
		if( slot->GetSlotName() == childSlotName )
		{
			if( !slot->IsEmpty()  )
			{
				CItemPartDefinitionComponent* cmp = slot->GetPluggenPart();
				if( cmp )
				{
					cmp->ProcessEvent( eventName, dynamicParams );//sending to child, not to parent, so I don't send slot name
					return true;
				}
			}
			
		}
	}
	return false;
}

Bool CItemPartDefinitionComponent::PlugEntityToSlotInit( CEntity* eTamplate, CName slotName, Bool recursive/* = true */)
{
	DetachSlotContent( slotName );
	Bool ret = PlugEntityToSlot( eTamplate, slotName, recursive );
	
	if( GGame->IsActive() )
		DoStatsStuff();	

	return ret;
}

Bool CItemPartDefinitionComponent::PlugEntityTemplateToSlotInit( const THandle< CEntityTemplate >& eTamplate, CName slotName, Bool recursive /*= true */ )
{
	DetachSlotContent( slotName );
	Bool ret = PlugEntityTemplateToSlot( eTamplate, slotName, recursive );

	if( GGame->IsActive() )
		DoStatsStuff();	

	return ret;
}

Bool CItemPartDefinitionComponent::DetachSlotContentInit( CName slotName, Bool destroyEntity /*= true*/, Bool recursive /*= true*/ )
{
	if( DetachSlotContent( slotName, destroyEntity, recursive ) )
	{
		if( GGame->IsActive() )
			DoStatsStuff();

		return true;
	}
	return false;
}

Bool CItemPartDefinitionComponent::PlugEntityToSlot( CEntity* entity, CName slotName, Bool recursive /*= true*/ )
{
	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		CItemPartSlotDefinition* slot = m_slots[i];		
		if( slot->GetSlotName()==slotName )
		{						
			AttachEntityToSlot( slot, entity );
			return true;
		}
	}
	if( recursive )
	{
		for( Uint32 i=0; i<m_slots.Size(); ++i )
		{
			CItemPartSlotDefinition* slot = m_slots[i];
			if( slot->GetPluggenPart() )
			{
				if( slot->GetPluggenPart()->PlugEntityToSlot( entity, slotName, true ) )
				{
					HandleInParentSlots( slot->GetPluggenPart() );
					CAnimatedComponent* animCpm = GetEntity()->FindComponent< CAnimatedComponent >();
					if( animCpm )
					{
						slot->GetPluggenPart()->AttachToParentRootIfNeeded( animCpm );
					}
					return true;
				}
			}
		}		
	}
	return false;
}

Bool CItemPartDefinitionComponent::PlugEntityTemplateToSlot( const THandle< CEntityTemplate >& eTamplate, CName slotName, Bool recursive /*= true */ )
{
	//LOG_R6( TXT("slot1 name %s"), slotName.AsString().AsChar() );
	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		CItemPartSlotDefinition* slot = m_slots[i];
		//LOG_R6( TXT("slot1 name %s"), slot->GetSlotName().AsString().AsChar() );
		if( slot->GetSlotName()==slotName )
		{
			slot->SetEntityToPlug( eTamplate );
			SpawnSlotContent( slot );
			return true;
		}
	}
	if( recursive )
	{
		for( Uint32 i=0; i<m_slots.Size(); ++i )
		{
			CItemPartSlotDefinition* slot = m_slots[i];
			if( slot->GetPluggenPart() )
			{
				if( slot->GetPluggenPart()->PlugEntityTemplateToSlot( eTamplate, slotName, true ) )
				{
					HandleInParentSlots( slot->GetPluggenPart() );
					CAnimatedComponent* animCpm = GetEntity()->FindComponent< CAnimatedComponent >();
					if( animCpm )
					{
						slot->GetPluggenPart()->AttachToParentRootIfNeeded( animCpm );
					}
					return true;
				}
			}
		}		
	}
	return false;
}


void CItemPartDefinitionComponent::DetachSlots()
{
	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		CItemPartSlotDefinition* slot = m_slots[i];
		if( slot->GetPluggenPart() )
		{
			slot->GetPluggenPart()->DetachSlots();
			CEntity* pluggedEntity = slot->GetPluggenPart()->GetEntity();
			//pluggetEntity->SetParent( NULL );
			CHardAttachment* attachment = pluggedEntity->GetTransformParent();
			if( attachment != NULL )
			{
				attachment->Break();
			}
			slot->SetPluggedPart( NULL );
			pluggedEntity->Destroy();							
		}
	}
}

Bool CItemPartDefinitionComponent::DetachSlotContent( CName slotName,Bool destroyEntity /* = true*/, Bool recursive /*= true */ )
{
	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		CItemPartSlotDefinition* slot = m_slots[i];
		if( slot->GetSlotName() == slotName )
		{
			if( slot->GetPluggenPart() )
			{
				slot->GetPluggenPart()->DetachSlots();
				
				CEntity* pluggedEntity = slot->GetPluggenPart()->GetEntity();
				CHardAttachment* attachment = pluggedEntity->GetTransformParent();
				if( attachment != NULL )
				{
					attachment->Break();
				}
				if( destroyEntity )
				{
					pluggedEntity->Destroy();	
				}

				slot->SetPluggedPart( NULL );

			}
			return true;
		}
	}
	if( recursive )
	{
		for( Uint32 i=0; i<m_slots.Size(); ++i )
		{
			CItemPartSlotDefinition* slot = m_slots[i];
			if( slot->GetPluggenPart() )
			{
				if( slot->GetPluggenPart()->DetachSlotContent( slotName, destroyEntity, true ) )
				{								
					return true;
				}
			}
		}			
	}
	return false;
}

Bool CItemPartDefinitionComponent::FindEntityInSlot( CName slotName, CEntity*& retEntity )
{
	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		CItemPartSlotDefinition* slot = m_slots[i];
		if( slot->GetSlotName() == slotName )
		{
			CEntity* foundEnt = slot->GetPluggenPart()->GetEntity();
			retEntity = foundEnt;
			return true;
		}		
	}
	
	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		CItemPartSlotDefinition* slot = m_slots[i];
		if( slot->GetPluggenPart() )
		{
			if( slot->GetPluggenPart()->FindEntityInSlot( slotName, retEntity ) )
			{								
				return true;
			}
		}
	}			
	return false;
}

CEntity* CItemPartDefinitionComponent::GetRootEntity()
{
	if( m_parentPart.Get() != NULL )
	{
		return m_parentPart.Get()->GetRootEntity();
	}
	else
	{
		return GetEntity();
	}
}

void CItemPartDefinitionComponent::funProcessEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER_OPT( THandle< CUpgradeEventHandlerParam >, dynamicParams, NULL);

	FINISH_PARAMETERS;

	if( eventName )
	{
		ProcessEvent( eventName, dynamicParams.Get() );
	}
}

void CItemPartDefinitionComponent::funcPlugEntityToSlot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slotName, CName::NONE );
	GET_PARAMETER( THandle< CEntity >, entity, NULL );
	FINISH_PARAMETERS;

	if( slotName && entity.Get() )
	{
		PlugEntityToSlotInit( entity.Get(), slotName );
	}
}

void CItemPartDefinitionComponent::funcDetachSlotContent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slotName, CName::NONE );
	FINISH_PARAMETERS;

	DetachSlotContentInit( slotName, false, true );
}

void CItemPartDefinitionComponent::funcFindEntityInSlot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slotName, CName::NONE );
	FINISH_PARAMETERS;

	CEntity* returnetEntity = NULL;

	FindEntityInSlot( slotName, returnetEntity );

	THandle< CEntity > retHandle = returnetEntity;

	RETURN_HANDLE( CEntity, retHandle );
}
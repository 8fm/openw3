#include "build.h"

#include "upgradesSpawnerItemEntity.h"
#include "itemPartDefinitionComponent.h"

#include "../../common/core/depot.h"

IMPLEMENT_ENGINE_CLASS( CUpgradesSpawnerItemEntity );

CUpgradesSpawnerItemEntity::CUpgradesSpawnerItemEntity()
{
	m_customAttacht = true;
	m_customDetacht = true;
}

void CUpgradesSpawnerItemEntity::CustomDetach()
{	
	if( !m_owner.Get() )
	{
		ASSERT( m_owner.Get(), TXT( "No parent entity for item" ) );
		return;
	}
	CItemPartDefinitionComponent* itemCmp = m_owner.Get()->FindComponent< CItemPartDefinitionComponent >( );
	CItemPartDefinitionComponent* parentItem = m_owner.Get()->FindComponent< CItemPartDefinitionComponent >( );
	if( !parentItem )
	{
		LOG_R6( TXT("No CItemPartDefinitionComponent for entity %s"), m_owner.Get()->GetName().AsChar() );
		ASSERT( m_owner.Get(), TXT( "No CItemPartDefinitionComponent for item parent" ) );
		return; 
	}
	Bool ret =	parentItem->DetachSlotContentInit( m_slotName );
	return ;
}
Bool CUpgradesSpawnerItemEntity::CustomAttach()
{
	const CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	if ( !defMgr )
	{ 
		ASSERT( defMgr, TXT( "Definition manager not found") );
		return false;
	}

	const SItemDefinition* itemDef = defMgr->GetItemDefinition( m_proxy->m_itemName );
	if ( !itemDef )
	{
		LOG_R6( TXT("Item definition not found for item with name %s"), m_proxy->m_itemName.AsString().AsChar() );
		ASSERT( defMgr, TXT( "Item definition not found" ) );
		return false; 
	}
	
	const String& templatePath = itemDef->GetUpgradeBasedTemplateName( m_parentEntity.Get() && m_parentEntity.Get()->IsA< CPlayer >() );

	CResource* resource = GDepot->FindResource( templatePath );
	if( !resource )
	{
		LOG_R6( TXT("Resource not found for path %s"), templatePath.AsChar() );
		ASSERT( resource, TXT( "Resource not found" ) );
		return false; 
	}

	CEntityTemplate* itemTemplate = Cast< CEntityTemplate >( resource );
	if( !itemTemplate )
	{
		LOG_R6( TXT("Resource %s is not CEntityTemplate"), templatePath.AsChar() );
		ASSERT( resource, TXT( "Resource is not CEntityTemplate" ) );
		return false; 
	}

	CEntity* parentEntity = m_proxy->m_parentAttachmentEntity.Get();
	if( !parentEntity )
	{
		ASSERT( parentEntity, TXT( "No parent entity for item" ) );
		return false; 
	}
	m_owner = parentEntity;

	CItemPartDefinitionComponent* parentItem = parentEntity->FindComponent< CItemPartDefinitionComponent >( );
	if( !parentItem )
	{
		LOG_R6( TXT("No CItemPartDefinitionComponent for entity %s"), parentEntity->GetName().AsChar() );
		ASSERT( parentItem, TXT( "No CItemPartDefinitionComponent for item parent" ) );
		return false; 
	}	
	m_slotName = m_proxy->m_slotName;
	return parentItem->PlugEntityTemplateToSlotInit( itemTemplate, m_proxy->m_slotName, true );

}

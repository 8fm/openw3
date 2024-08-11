#include "build.h"
#include "questCharacterCustomizerBlock.h"
#include "questGraphSocket.h"
#include "../../common/engine/gameSaveManager.h"
#include "../../common/core/gatheredResource.h"
#include "../engine/idTagManager.h"
#include "../engine/tagManager.h"
#include "../engine/dynamicLayer.h"
#include "../engine/graphConnectionRebuilder.h"


///////////////////////////////////////////////////////////////////////////////

CGatheredResource resVirtualContainerTemplate( TXT("engine\\gameplay\\virtual_container.w2ent"), 0 );

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CVirtualContainerEntity );

void CVirtualContainerEntity::Initialize( const CGameplayEntity* gameplayEntity )
{
	if ( !m_characterState.Empty() )
	{
		WARN_GAME( TXT( "You're overwriting the state of a virtual container" ) );
	}

	m_characterState.Clear();
	CGameDataExternalStorage storage;
	storage.SetDataPtr( &m_characterState );
	CGameStorageSaver* saver = ( CGameStorageSaver* ) CGameSaveManager::CreateSaver( &storage, nullptr );

	CInventoryComponent* inventory = gameplayEntity->GetInventoryComponent();
	if( inventory )
	{
		inventory->OnSaveGameplayState( saver );
	}
}

void CVirtualContainerEntity::Restore( CGameplayEntity* gameplayEntity )
{
	CInventoryComponent* inventory = gameplayEntity->GetInventoryComponent();

	if( !inventory )
	{
		return;
	}

	if ( !m_characterState.Empty() )
	{
		CGameDataExternalStorage storage;
		storage.SetDataPtr( &m_characterState );
		CGameStorageLoader* loader = ( CGameStorageLoader* ) CGameSaveManager::CreateLoader( &storage, nullptr, nullptr );

		inventory->OnLoadGameplayState( loader );
	}

	inventory->SpawnMountedItems();
}

void CVirtualContainerEntity::RestoreIntoInventory( CInventoryComponent* inventory )
{
	ASSERT( inventory );
	if ( !m_characterState.Empty() )
	{
		// TODO: add version saving here!!!!!!!!!
		CGameDataExternalStorage storage;
		storage.SetDataPtr( &m_characterState );
		CGameStorageLoader* loader = ( CGameStorageLoader* )CGameSaveManager::CreateLoader( &storage, nullptr, nullptr );

		inventory->OnLoadGameplayState( loader );
	}
}

void CVirtualContainerEntity::OnSaveGameplayState( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME( VirtualContainerData ) );
	saver->WriteValue( CNAME( data ), m_characterState );
}

void CVirtualContainerEntity::OnLoadGameplayState( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME( VirtualContainerData ) );
	m_characterState = loader->ReadValue( CNAME( data ), TDynArray< Uint8 >() );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestCharacterCustomizerBlock );

CQuestCharacterCustomizerBlock::CQuestCharacterCustomizerBlock()
	: m_actorTag( CNAME( PLAYER ) )
{ 
	m_name = TXT("Character Customizer"); 
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestCharacterCustomizerBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestCharacterCustomizerBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	// find the actor
	CTagManager* tagManager = GGame->GetActiveWorld()->GetTagManager();
	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( tagManager->GetTaggedEntity( m_actorTag ) );
	if ( !gameplayEntity )
	{
		ThrowError( data, TXT( "Activation: No gameplay entity with the tag '%ls' was found" ), m_actorTag.AsString().AsChar() );
		return;
	}

	CCCOpVirtualContainerOp::s_lastVirtualContainer = NULL;

	for ( TDynArray< ICharacterCustomizationOperation* >::const_iterator it = m_operations.Begin(); it != m_operations.End(); ++it )
	{
		ICharacterCustomizationOperation* operation = *it;
		if ( operation != NULL )
		{
			operation->Execute( gameplayEntity );
		}
	}

	ActivateOutput( data, CNAME( Out ) );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( ICharacterCustomizationOperation );

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCCOpScript );

void CCCOpScript::Execute( CGameplayEntity* gameplayEntity ) const
{
	THandle< CGameplayEntity > hEntity( gameplayEntity );
	
	CallFunction< THandle< CGameplayEntity > >( const_cast< CCCOpScript* >( this ), CNAME( Execute ), hEntity );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCCOpClearInventory );

void CCCOpClearInventory::Execute( CGameplayEntity* gameplayEntity ) const
{
	CInventoryComponent* component = gameplayEntity->GetInventoryComponent();
	if( component )
	{
		component->RemoveAllItems();
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCCOpCustomizeInventory );

CCCOpCustomizeInventory::CCCOpCustomizeInventory()
	: m_template( NULL )
	, m_applyMounts( true )
{
}

void CCCOpCustomizeInventory::Execute( CGameplayEntity* gameplayEntity ) const
{
	if ( !m_template )
	{
		return;
	}

	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}

	CDynamicLayer* dynamicLayer = world->GetDynamicLayer();
	if ( !dynamicLayer )
	{
		WARN_GAME( TXT( "Can't instantiate an inventory entity template - there's no dynamic layer" ) );
		return;
	}

	// spawn the entity
	EntitySpawnInfo einfo;
	einfo.m_template = m_template;
	einfo.m_appearances = m_template->GetEnabledAppearancesNames();

	CEntity* entity = dynamicLayer->CreateEntitySync( einfo );
	if ( !entity )
	{
		WARN_GAME( TXT( "Can't instantiate an inventory entity template" ) );
		return;
	}

	TDynArray< CInventoryComponent* > sourceInventories;
	for ( ComponentIterator< CInventoryComponent > it( entity ); it; ++it )
	{
		sourceInventories.PushBack( *it );
	}

	if ( sourceInventories.Empty() )
	{
		WARN_GAME( TXT( "The specified inventory entity template doesn't contain any inventory component" ) );
		entity->Destroy();
		return;
	}

	CInventoryComponent* destinationInventory = gameplayEntity->GetInventoryComponent();
	if ( !destinationInventory )
	{
		WARN_GAME( TXT( "The specified gameplay entity doesn't have an inventory component" ) );
		entity->Destroy();
		return;
	}

	for ( TDynArray< CInventoryComponent* >::iterator it = sourceInventories.Begin(); it != sourceInventories.End(); ++it )
	{
		CInventoryComponent* sourceInventory = *it;
		if ( sourceInventory )
		{
			Uint32 itemsCount = sourceInventory->GetItemCount();
			for ( Uint32 i=0; i<itemsCount; ++i )
			{
				const SInventoryItem* sourceItem = sourceInventory->GetItem( (SItemUniqueId)i );
				ASSERT( sourceItem );

				if ( destinationInventory->GetCategoryDefaultItem( sourceItem->GetCategory() ) == sourceItem->GetName() )
				{
					// This is a default item, no duplicates please
					continue;
				}

				CInventoryComponent::SAddItemInfo addItemInfo;
				addItemInfo.m_informGui = false;
				TDynArray< SItemUniqueId > destItemId = destinationInventory->AddItem( *sourceItem, addItemInfo );
				ASSERT( destItemId[0] );
				const SInventoryItem* destItem = destinationInventory->GetItem( destItemId[0] );

				if ( m_applyMounts && sourceItem->IsMounted() )
				{
					CInventoryComponent::SMountItemInfo mountInfo;
					destinationInventory->MountItem( destItemId[0], mountInfo );
				}

				/*if ( sourceItem->HasFlag( SInventoryItem::FLAG_HELD ) )	<- held stuff should not be reflected at all
				{	
					CInventoryComponent::SMountItemInfo mountInfo;
					mountInfo.m_toHand = true;
					destinationInventory->MountItem( destItemId, mountInfo );
				}*/
			}
		}
	}

	// despawn the inventory entity
	entity->Destroy();
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCCOpVirtualContainerOp );

CCCOpVirtualContainerOp::CCCOpVirtualContainerOp()
{

}

void CCCOpVirtualContainerOp::Execute( CGameplayEntity* gameplayEntity ) const
{
	if ( !gameplayEntity || m_virtualContainerTag.Empty() )
	{
		return;
	}
	Execute( gameplayEntity, m_virtualContainerTag );
}

CVirtualContainerEntity* CCCOpVirtualContainerOp::GetVirtualContainer( CName tag ) const
{
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return NULL;
	}

	CTagManager* tagMgr = world->GetTagManager();
	if ( !tagMgr )
	{
		return NULL;
	}

	// try finding an existing entity
	CDynamicLayer* dynamicLayer = world->GetDynamicLayer();
	CVirtualContainerEntity* container = Cast< CVirtualContainerEntity >( tagMgr->GetTaggedEntity( tag ) );
	return container;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCCOpItemsToVirtualContainer );

CCCOpItemsToVirtualContainer::CCCOpItemsToVirtualContainer()
	: m_canOverride( false )
{}

void CCCOpItemsToVirtualContainer::Execute( CGameplayEntity* gameplayEntity, CName virtualContainerTag ) const
{
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}

	CTagManager* tagMgr = world->GetTagManager();
	if ( !tagMgr )
	{
		return;
	}

	// try finding an existing entity
	CVirtualContainerEntity* container = Cast< CVirtualContainerEntity >( tagMgr->GetTaggedEntity( virtualContainerTag ) );
	if ( container != NULL )
	{
		if ( !m_canOverride )
		{
			WARN_GAME( String::Printf( TXT( "Can't override the contents of a virtual container '%ls'" ), virtualContainerTag.AsString().AsChar() ).AsChar() );
			return;
		}
	}
	else
	{
		// the container doesn't exist - create one
		CDynamicLayer* dynamicLayer = world->GetDynamicLayer();
		CIdTagManager* idTagMgr = GGame->GetIdTagManager();
		if ( !dynamicLayer || !idTagMgr )
		{
			WARN_GAME( String::Printf( TXT( "Can't create a virtual container '%ls' - there's no dynamic layer or an ID tags manager" ), virtualContainerTag.AsString().AsChar() ).AsChar() );
			return;
		}

		CEntityTemplate* virtualContainerTemplate = resVirtualContainerTemplate.LoadAndGet< CEntityTemplate >();

		EntitySpawnInfo einfo;
		einfo.m_template = virtualContainerTemplate;
		einfo.m_idTag = idTagMgr->Allocate();
		einfo.m_entityFlags = EF_ManagedEntity;
		einfo.m_tags.AddTag( virtualContainerTag );

		container = Cast< CVirtualContainerEntity >( dynamicLayer->CreateEntitySync( einfo ) );
	}

	if ( container )
	{
		container->Initialize( gameplayEntity );
		CCCOpVirtualContainerOp::s_lastVirtualContainer = container;
	}
	else
	{
		WARN_GAME( String::Printf( TXT( "There's no virtual container '%ls' to store gameplay entity stuff in" ), virtualContainerTag.AsString().AsChar() ).AsChar() );
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCCOpItemsFromVirtualContainer );

void CCCOpItemsFromVirtualContainer::Execute( CGameplayEntity* gameplayEntity, CName virtualContainerTag ) const
{
	CVirtualContainerEntity* container = GetVirtualContainer( virtualContainerTag );
	if ( !container )
	{
		CVirtualContainerEntity* lastVC = s_lastVirtualContainer.Get();
		if ( lastVC && lastVC->GetTags().HasTag( virtualContainerTag ) )
		{
			container = lastVC;
		}
	}

	if ( container )
	{
		CInventoryComponent* containerInventory = container->GetInventoryComponent();
		if ( !containerInventory )
		{
			WARN_GAME( TXT("CCCOpItemsFromVirtualContainer: virtual container has no inventory component") );
			return;
		}

		container->RestoreIntoInventory( containerInventory );

		CInventoryComponent* destinationInventory = gameplayEntity->GetInventoryComponent();
		if ( !destinationInventory )
		{
			WARN_GAME( TXT( "CCCOpItemsFromVirtualContainer: The specified gameplay entity doesn't have an inventory component" ) );
			containerInventory->ClearInventory();
			return;
		}

		Uint32 itemsCount = containerInventory->GetItemCount();
		for ( Uint32 i=0; i<itemsCount; ++i )
		{
			const SInventoryItem* sourceItem = containerInventory->GetItem( (SItemUniqueId)i );
			ASSERT( sourceItem );

			if ( destinationInventory->GetCategoryDefaultItem( sourceItem->GetCategory() ) == sourceItem->GetName() )
			{
				// This is a default item, no duplicates please
				continue;
			}

			CInventoryComponent::SAddItemInfo addItemInfo;
			addItemInfo.m_informGui = false;
			TDynArray< SItemUniqueId > destItemId = destinationInventory->AddItem( *sourceItem, addItemInfo );
			ASSERT( destItemId[0] );
			const SInventoryItem* destItem = destinationInventory->GetItem( (SItemUniqueId)destItemId[0] );

			
			if ( m_applyMounts && sourceItem->IsMounted() )
			{
				CInventoryComponent::SMountItemInfo mountInfo;
				destinationInventory->MountItem( destItemId[0], mountInfo );
			}

			/*if ( sourceItem->HasFlag( SInventoryItem::FLAG_HELD ) )	<- held stuff should not be reflected at all
			{	
			CInventoryComponent::SMountItemInfo mountInfo;
			mountInfo.m_toHand = true;
			destinationInventory->MountItem( destItemId, mountInfo );
			}*/
			
		}
	}
	else
	{
		WARN_GAME( String::Printf( TXT( "There's no virtual container '%ls' to restore gameplay entity stuff from" ), virtualContainerTag.AsString().AsChar() ).AsChar() );
	}
}

CCCOpItemsFromVirtualContainer::CCCOpItemsFromVirtualContainer()
	: m_applyMounts( true )
{

}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCCOpItemsRemoveMatchingVirtualContainer );

void CCCOpItemsRemoveMatchingVirtualContainer::Execute( CGameplayEntity* gameplayEntity, CName virtualContainerTag ) const
{
	CVirtualContainerEntity* container = GetVirtualContainer( virtualContainerTag );

	if ( container )
	{
		CInventoryComponent* containerInventory = container->GetInventoryComponent();
		if ( !containerInventory )
		{
			WARN_GAME( TXT("CCCOpItemsRemoveMatchingVirtualContainer: virtual container has no inventory component") );
			return;
		}

		container->RestoreIntoInventory( containerInventory );

		CInventoryComponent* destinationInventory = gameplayEntity->GetInventoryComponent();
		if ( !destinationInventory )
		{
			WARN_GAME( TXT( "The specified gameplayl entity doesn't have an inventory component" ) );
			containerInventory->ClearInventory();
			return;
		}

		Uint32 itemsCount = containerInventory->GetItemCount();
		for ( Uint32 i=0; i<itemsCount; ++i )
		{
			const SInventoryItem* sourceItem = containerInventory->GetItem( (SItemUniqueId)i );
			ASSERT( sourceItem );

			SItemUniqueId itemToRemoveId = destinationInventory->GetItemId( sourceItem->GetName() );
			if ( itemToRemoveId )
			{
				// Has such item, remove
				destinationInventory->RemoveItem( itemToRemoveId, sourceItem->GetQuantity() );
			}
		}

		containerInventory->ClearInventory();
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCCOpItemsRemoveMatchingTemplate );

CCCOpItemsRemoveMatchingTemplate::CCCOpItemsRemoveMatchingTemplate()
{

}

void CCCOpItemsRemoveMatchingTemplate::Execute( CGameplayEntity* gameplayEntity ) const
{
	if ( !m_template )
	{
		return;
	}

	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}

	CDynamicLayer* dynamicLayer = world->GetDynamicLayer();
	if ( !dynamicLayer )
	{
		WARN_GAME( TXT( "Can't instantiate an inventory entity template - there's no dynamic layer" ) );
		return;
	}

	// spawn the entity
	EntitySpawnInfo einfo;
	einfo.m_template = m_template;
	einfo.m_appearances = m_template->GetEnabledAppearancesNames();

	CEntity* entity = dynamicLayer->CreateEntitySync( einfo );
	if ( !entity )
	{
		WARN_GAME( TXT( "Can't instantiate an inventory entity template" ) );
		return;
	}

	TDynArray< CInventoryComponent* > sourceInventories;
	for ( ComponentIterator< CInventoryComponent > it( entity ); it; ++it )
	{
		sourceInventories.PushBack( *it );
	}

	if ( sourceInventories.Empty() )
	{
		WARN_GAME( TXT( "The specified inventory entity template doesn't contain any inventory component" ) );
		entity->Destroy();
		return;
	}

	CInventoryComponent* destinationInventory = gameplayEntity->GetInventoryComponent();
	if ( !destinationInventory )
	{
		WARN_GAME( TXT( "The specified gameplay entity doesn't have an inventory component" ) );
		entity->Destroy();
		return;
	}

	for ( TDynArray< CInventoryComponent* >::iterator it = sourceInventories.Begin(); it != sourceInventories.End(); ++it )
	{
		CInventoryComponent* sourceInventory = *it;
		if ( sourceInventory )
		{
			Uint32 itemsCount = sourceInventory->GetItemCount();
			for ( Uint32 i=0; i<itemsCount; ++i )
			{
				const SInventoryItem* sourceItem = sourceInventory->GetItem( (SItemUniqueId)i );
				ASSERT( sourceItem );

				SItemUniqueId itemToRemoveId = destinationInventory->GetItemId( sourceItem->GetName() );
				if ( itemToRemoveId )
				{
					// Has such item, remove
					destinationInventory->RemoveItem( itemToRemoveId, sourceItem->GetQuantity() );
				}
			}
		}
	}

	// despawn the inventory entity
	entity->Destroy();
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCCOpPreserveVirtualContainerContents );

THandle< CVirtualContainerEntity >		CCCOpVirtualContainerOp::s_lastVirtualContainer = NULL;

CCCOpPreserveVirtualContainerContents::CCCOpPreserveVirtualContainerContents()
{

}


void CCCOpPreserveVirtualContainerContents::Execute( CGameplayEntity* gameplayEntity, CName virtualContainerTag ) const
{
	CVirtualContainerEntity* container = GetVirtualContainer( virtualContainerTag );
	if ( !container )
	{
		CVirtualContainerEntity* lastVC = s_lastVirtualContainer.Get();
		if ( lastVC && lastVC->GetTags().HasTag( virtualContainerTag ) )
		{
			container = lastVC;
		}
	}

	if ( container )
	{
		// Just copy the buffer
		GVirtualContainerStatePersistentBuffer = container->GetStateData();
	}
	else
	{
		WARN_GAME( String::Printf( TXT( "There's no virtual container '%ls'" ), virtualContainerTag.AsString().AsChar() ).AsChar() );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCCOpRestoreVirtualContainerContents );

CCCOpRestoreVirtualContainerContents::CCCOpRestoreVirtualContainerContents()
{

}

void CCCOpRestoreVirtualContainerContents::Execute( CGameplayEntity* gameplayEntity, CName virtualContainerTag ) const
{
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}

	CTagManager* tagMgr = world->GetTagManager();
	if ( !tagMgr )
	{
		return;
	}

	// try finding an existing entity
	CVirtualContainerEntity* container = Cast< CVirtualContainerEntity >( tagMgr->GetTaggedEntity( virtualContainerTag ) );
	if ( container != NULL )
	{
		WARN_GAME( String::Printf( TXT( "Can't override the contents of a virtual container '%ls'" ), virtualContainerTag.AsString().AsChar() ).AsChar() );
		return;
	}
	else
	{
		// the container doesn't exist - create one
		CDynamicLayer* dynamicLayer = world->GetDynamicLayer();
		CIdTagManager* idTagMgr = GGame->GetIdTagManager();
		if ( !dynamicLayer || !idTagMgr )
		{
			WARN_GAME( String::Printf( TXT( "Can't create a virtual container '%ls' - there's no dynamic layer or an ID tags manager" ), virtualContainerTag.AsString().AsChar() ).AsChar() );
			return;
		}

		CEntityTemplate* virtualContainerTemplate = resVirtualContainerTemplate.LoadAndGet< CEntityTemplate >();

		EntitySpawnInfo einfo;
		einfo.m_template = virtualContainerTemplate;
		einfo.m_idTag = idTagMgr->Allocate();
		einfo.m_entityFlags = EF_ManagedEntity;
		einfo.m_tags.AddTag( virtualContainerTag );

		container = Cast< CVirtualContainerEntity >( dynamicLayer->CreateEntitySync( einfo ) );
	}

	if ( container )
	{
		container->SetStateData( GVirtualContainerStatePersistentBuffer );
		CCCOpVirtualContainerOp::s_lastVirtualContainer = container;
	}
	else
	{
		WARN_GAME( String::Printf( TXT( "There's no virtual container '%ls'" ), virtualContainerTag.AsString().AsChar() ).AsChar() );
	}
}

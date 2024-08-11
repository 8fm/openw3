#include "build.h"

#include "../engine/appearanceComponent.h"
#include "../engine/performableAction.h"

#include "characterStats.h"
#include "commonGame.h"
#include "edEntitySetupListParam.h"
#include "entityParams.h"
#include "equipmentState.h"
#include "factsDB.h"
#include "gameplayEntity.h"
#include "gameWorld.h"
#include "interactionsManager.h"
#include "inventoryDefinition.h"
#include "../core/gameSave.h"
#include "../engine/drawableComponent.h"
#include "../engine/renderFrame.h"
#include "scriptedRenderFrame.h"
#include "../engine/tickManager.h"
#include "../engine/animGlobalParam.h"

RED_DEFINE_NAME( OnVisualDebug );

TDynArray< CGameplayEntity::ScriptAnimEventManager::SEventData > CGameplayEntity::ScriptAnimEventManager::s_queuedEvents;

CGameplayEntity::ScriptAnimEventManager::ChildListener::ChildListener( CNode* object, CName functionName )
	: m_object( object )
	, m_functionName( functionName )
{}

CGameplayEntity::ScriptAnimEventManager::Entry* CGameplayEntity::ScriptAnimEventManager::FindEntry( CName eventName )
{
	for ( Entry& entry : m_entries )
	{
		if ( entry.m_eventName != eventName )
		{
			continue;
		}
		return &entry;
	}
	return nullptr;
}

void CGameplayEntity::ScriptAnimEventManager::AddCallback( CName eventName, CName functionName )
{
	if ( Entry* entry = FindEntry( eventName ) )
	{
		entry->m_functionName = functionName;
	}
	else
	{
		Entry newEntry;
		newEntry.m_eventName = eventName;
		newEntry.m_functionName = functionName;
		m_entries.PushBack( newEntry );
	}
}

void CGameplayEntity::ScriptAnimEventManager::RemoveCallback( CName eventName )
{
	if ( Entry* entry = FindEntry( eventName ) )
	{
		entry->m_functionName = CName::NONE; // Remove later to support removal during iteration in OnEvent
	}
}

void CGameplayEntity::ScriptAnimEventManager::AddChildCallback( CNode* child, CName eventName, CName functionName )
{
	// Get entry

	Entry* entry = FindEntry( eventName );
	if ( !entry )
	{
		Entry newEntry;
		newEntry.m_eventName = eventName;
		m_entries.PushBack( newEntry );
		entry = &m_entries[ m_entries.Size() - 1 ];
	}

	// Exists?

	for ( ChildListener& childListener : entry->m_children )
	{
		if ( childListener.m_object == child )
		{
			childListener.m_functionName = functionName;
			return;
		}
	}

	// Add new callback

	entry->m_children.PushBack( ChildListener( child, functionName ) );
}

void CGameplayEntity::ScriptAnimEventManager::RemoveChildCallback( CNode* childNode, CName eventName )
{
	if ( Entry* entry = FindEntry( eventName ) )
	{
		for ( ChildListener& child : entry->m_children )
		{
			if ( child.m_object.Get() == childNode )
			{
				child.m_object = nullptr; // Remove later to support removal during iteration in OnEvent
				break;
			}
		}
	}
}

void CGameplayEntity::ScriptAnimEventManager::OnEvent( CGameplayEntity* entity, CName eventName, EAnimationEventType eventType, const SAnimationEventAnimInfo& animInfo )
{
	// Some animation events are fired during physics scene fetch.
	// Some of them trigger physics queries (on the scripts side) that will fail during fetch.
	// That is why we need to enqueue events and pass them to scripts "later".
	if ( entity != nullptr && GCommonGame && GCommonGame->IsActive() )
	{
		SEventData eventData;
		eventData.m_entity		= entity;
		eventData.m_eventName	= eventName;
		eventData.m_eventType	= eventType;
		eventData.m_animInfo	= animInfo;
		s_queuedEvents.PushBack( eventData );
	}
}

void CGameplayEntity::ScriptAnimEventManager::OnEvent_Internal( CGameplayEntity* entity, CName eventName, EAnimationEventType eventType, const SAnimationEventAnimInfo& animInfo )
{
	// Make sure the loop is safe wrt. adding and removing listeners

	for ( Uint32 i = 0; i < m_entries.Size(); ++i )
	{
		if ( m_entries[ i ].m_eventName != eventName )
		{
			continue;
		}

		// Notify entity

		if ( m_entries[ i ].m_functionName )
		{
			entity->CallEvent( m_entries[ i ].m_functionName, eventName, eventType, animInfo );
		}

		// Notify child listeners

		for ( Uint32 j = 0; j < m_entries[ i ].m_children.Size(); )
		{
			CGameplayEntity::ScriptAnimEventManager::ChildListener& listener = m_entries[ i ].m_children[ j ];
			if ( CNode* object = listener.m_object.Get() )
			{
				object->CallEvent( listener.m_functionName, eventName, eventType, animInfo );
				++j;
			}
			else // Clean up dead listeners
			{
				m_entries[ i ].m_children.RemoveAtFast( j );
			}
		}

		// To be removed?

		if ( !m_entries[ i ].m_functionName && m_entries[ i ].m_children.Empty() )
		{
			m_entries.RemoveAtFast( i );
		}

		return;
	}
}

void CGameplayEntity::ScriptAnimEventManager::FireQueuedEvents()
{
	PC_SCOPE_PIX( ScriptAnimEventManager_FireEvents )

	while ( s_queuedEvents.Size() > 0 )
	{
		SEventData eventData = s_queuedEvents[ 0 ];
		s_queuedEvents.RemoveAtFast( 0 );
		CGameplayEntity* entity = eventData.m_entity.Get();
		if ( entity != nullptr )
		{
			entity->m_scriptAnimEventManager.OnEvent_Internal( entity, eventData.m_eventName, eventData.m_eventType, eventData.m_animInfo );
		}
	}
}

void CGameplayEntity::ScriptAnimEventManager::ClearQueuedEvents()
{
	s_queuedEvents.ClearFast();
}

//////////////////////////////////////////////////////////////////////////
// CGameplayEntity

IMPLEMENT_RTTI_ENUM( EGameplayEntityFlags );
IMPLEMENT_RTTI_ENUM( EFocusModeVisibility );
IMPLEMENT_ENGINE_CLASS( CGameplayEntity );

CGameplayEntity::CGameplayEntity()
	: m_inventoryComponent( NULL )
	, m_gameplayFlags( 0 )
	, m_stats(NULL)
	, m_materialEffects( NULL )
#ifndef RED_FINAL_BUILD
	, m_scriptedRenderFrame( nullptr )
	, m_focusModeVisibility( FMV_None )
#endif
#ifdef RED_ASSERTS_ENABLED
	, m_sfxTagCachedFlag( false )
#endif
{
}

void CGameplayEntity::OnInitialized()
{
	TBaseClass::OnInitialized();

	CacheSfxTag();
}

void CGameplayEntity::OnPostComponentsInitializedAsync()
{
	TBaseClass::OnPostComponentsInitializedAsync();

	CacheSfxTag();
}

//////////////////////////////////////////////////////////////////////////
//
// CObject - called when entity property was changed by external source eg. animated properties
void CGameplayEntity::OnPropertyExternalChanged( const CName& propertyName )
{
	TBaseClass::OnPropertyExternalChanged( propertyName );

	if( propertyName == CNAME( transform ) )
	{
		CheckUpdateTransformMode();
		ScheduleUpdateTransformNode();
	}
}

String CGameplayEntity::GetDisplayName() const
{
	m_displayName.Load();

	const CAlternativeDisplayName* param = nullptr;
	
	if( m_template )
	{
		param = static_cast< const CAlternativeDisplayName* >( m_template->FindGameplayParam( CAlternativeDisplayName::GetStaticClass() ) );
	}
	String ret;
	CFactsDB *factsDB = GCommonGame ? GCommonGame->GetSystem< CFactsDB >() : NULL;	
	const LocalizedString& locString = param && factsDB && factsDB->DoesExist( param->GetFactID() ) ? param->GetAltName() : m_displayName;
	{
		locString.Load();
		ret = locString.GetString();	
	}		

	if ( ret.Empty() )
	{
#ifndef RED_FINAL_BUILD
		ret = TBaseClass::GetDisplayName();
#endif		
	}
	return ret;
}

void CGameplayEntity::OnCreated( CLayer* layer, const EntitySpawnInfo& info )
{
	// pass to base class
	TBaseClass::OnCreated( layer, info );

	m_displayName.Load();
	if ( info.m_entityNotSavable )
	{
		SetShouldSave( false );
	}
}

void CGameplayEntity::OnLoaded()
{
	// Pass to base class
	TBaseClass::OnLoaded();

	m_displayName.Load();
}

void CGameplayEntity::OnDestroyed( CLayer* layer )
{
	if ( m_materialEffects )
	{
		delete m_materialEffects;
		m_materialEffects = NULL;
	}

	// Pass to base class
	TBaseClass::OnDestroyed( layer );
}

void CGameplayEntity::OnLayerUnloading()
{
	// Pass to base class
	TBaseClass::OnLayerUnloading();
}

void CGameplayEntity::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	const CEntityTemplate* templ = GetEntityTemplate();
	const CAttackableArea* params = templ ? templ->FindGameplayParamT<CAttackableArea>() : NULL;
	if( params ) world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_AIRanges );

	// We're only interested in doing stuff when the game is active
	if ( world == GCommonGame->GetActiveWorld() )
	{
		// Reapply stored abilities
		if( m_stats )
		{
			m_stats->ReapplyAbilities();
		}

		// Register interaction activator
		if ( m_isInteractionActivator )
		{
			GCommonGame->GetSystem< CInteractionsManager >()->AddInteractionActivator( this );	
		}

		GCommonGame->OnAttachGameplayEntity( this );
	}

	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->OnAttached( world );
	}

#ifndef RED_FINAL_BUILD
	TSet< EShowFlags >::iterator itEnd = m_scriptedVisualDebugFlags.End();
	for ( TSet< EShowFlags >::iterator it = m_scriptedVisualDebugFlags.Begin(); it != itEnd; ++it )
	{
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, *it );
	}
#endif

	// Init inventory based on inventory definition and equipment definition
	InitInventory();
}

void CGameplayEntity::OnAttachFinished( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttachFinished( world );

	// Get the stats from template
	if ( CEntityTemplate* entTemplate = GetEntityTemplate() )
	{
		TDynArray< CGameplayEntityParam* > params;
		entTemplate->CollectGameplayParams( params, CCharacterStatsParam::GetStaticClass() );

		if ( !params.Empty() )
		{
			CCharacterStats* stats = GetCharacterStats();
			for( CGameplayEntityParam* param : params )
			{
				RED_ASSERT( param->IsA< CCharacterStatsParam >() );
				stats->AddAbilitiesFromParam( static_cast< CCharacterStatsParam* >( param ) );
			}
		}
	}

	// update the state
	if ( CGameWorld* gameWorld = Cast< CGameWorld >( world ) )
	{
		gameWorld->UpdateEntityState( this );
	}

	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->OnAttachFinished( world );
	}
}

void CGameplayEntity::OnAttachFinishedEditor( CWorld* world )
{
	TBaseClass::OnAttachFinishedEditor( world );

	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->OnAttachFinished( world );
	}
}

void CGameplayEntity::InitInventory( Bool initFromTemplate )
{
	CInventoryComponent* inventoryComponent = m_inventoryComponent.Get();

	// if inventory component hasn't been initialized yet..
	if ( inventoryComponent == nullptr )
	{
		// Find inventory component
		inventoryComponent = FindComponent<CInventoryComponent>();
		if ( inventoryComponent )
		{
			// Init inventory
			m_inventoryComponent = inventoryComponent;
		
			// We need to have valid entity template to initialize from
			if ( initFromTemplate && m_template )
			{
				inventoryComponent->InitFromTemplate( m_template.Get() );
			}
		}
	}
}

void CGameplayEntity::ApplyAppearanceEquipment( const CEntityAppearance* appearance )
{
	// Make sure we have inventory component
	CInventoryComponent* inventoryComponent = GetInventoryComponent();
	if ( !inventoryComponent )
	{
		return;
	}

	// Get the appearance inventory definition
	CEquipmentDefinition* eqDef = appearance->FindParameter< CEquipmentDefinition >();
	if ( !eqDef )
	{
		return;
	}

	// Apply equipment definition
	const TDynArray< CEquipmentDefinitionEntry* >& eqEntries = eqDef->GetEntries();
	TDynArray< CEquipmentDefinitionEntry* >::const_iterator entryIt = eqEntries.Begin();
	for ( ; entryIt != eqEntries.End(); ++entryIt )
	{
		// Get name of the item to mount
		CEquipmentDefinitionEntry* entry = *entryIt;
		const CName& itemName = entry->GetItem();

		// Get name of the default item for this category
		const CName& defaultItemName = entry->m_defaultItemName;

		// Check if there is already inventory item marked for mounting for given category
		// If not, allow mounting default item
		Bool allowMount = true;
		SItemUniqueId existingItemId = inventoryComponent->GetItemByCategory( entry->m_category );
		if ( existingItemId != SItemUniqueId::INVALID )
		{
			const SInventoryItem* existingItem = inventoryComponent->GetItem( existingItemId );
			if ( existingItem )
			{
			    allowMount = ( existingItem->IsMounted() == false );
			}
		}

		if ( defaultItemName != CName::NONE )
		{
			// Add default item if it is not present
			if ( !inventoryComponent->HasItem( defaultItemName ) )
			{
				CInventoryComponent::SAddItemInfo addItemInfo;
				addItemInfo.m_informGui = false;
				inventoryComponent->AddItem( defaultItemName, addItemInfo );
			}
		}

		// Get some item by the category type
		SItemUniqueId currentItemId = inventoryComponent->GetItemByCategory( entry->m_category );
		if ( currentItemId != SItemUniqueId::INVALID && inventoryComponent->GetItem( currentItemId )->GetName() == itemName )
		{
			continue;
		}

		// Make sure item exists
		SItemUniqueId itemToMountId;
		if ( itemName != CName::NONE )
		{
			if ( !inventoryComponent->HasItem( itemName ) )
			{
				CInventoryComponent::SAddItemInfo addItemInfo;
				addItemInfo.m_informGui = false;
				inventoryComponent->AddItem( itemName, addItemInfo );
			}
			itemToMountId = inventoryComponent->GetItemId( itemName );
		}
		else
		{
			itemToMountId = inventoryComponent->GetItemId( defaultItemName );
		}

		// Mount it
		SInventoryItem* item = inventoryComponent->GetItem( itemToMountId );
		if ( item )
		{
			if ( allowMount )
			{
				item->SetIsMounted( true );
			}
		}
		else
		{
			ITEM_ERR( TXT("CGameplayEntity::ApplyAppearanceEquipment(): Trying to add item that probably doesn't exits in xml definitions: %s"),
				itemName.AsString().AsChar() );
		}
	}
}

void CGameplayEntity::OnGrabItem( SItemUniqueId itemId, CName slot )
{
}

void CGameplayEntity::OnPutItem( SItemUniqueId itemId, Bool emptyHand )
{
}

void CGameplayEntity::OnMountItem( SItemUniqueId itemId, Bool wasHeld )
{
}

void CGameplayEntity::OnUnmountItem( SItemUniqueId itemId )
{
}

void CGameplayEntity::OnEnhanceItem( SItemUniqueId enhancedItemId, Int32 slotIndex )
{
}

void CGameplayEntity::OnRemoveEnhancementItem( SItemUniqueId enhancedItemId, Int32 slotIndex )
{
}

void CGameplayEntity::OnAddedItem( SItemUniqueId itemId )
{
}

void CGameplayEntity::OnRemovedItem( SItemUniqueId itemId )
{
}

void CGameplayEntity::OnItemAbilityAdded( SItemUniqueId itemId, CName ability )
{
}

void CGameplayEntity::OnItemAbilityRemoved( SItemUniqueId itemId, CName ability )
{
}

void CGameplayEntity::EquipItem( SItemUniqueId itemId, Bool ignoreMount )
{

}

void CGameplayEntity::UnequipItem( SItemUniqueId itemId )
{

}

void CGameplayEntity::OnDetached( CWorld* world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_AIRanges );

	// Pass to base class
	TBaseClass::OnDetached( world );

	// We're only interested in doing stuff when the game is active
	if ( world == GCommonGame->GetActiveWorld() )
	{
		// Register interaction activator
		if ( m_isInteractionActivator )
		{
			CInteractionsManager* manager = GCommonGame->GetSystem< CInteractionsManager >();
			
			// Make sure we have a manager (the manager can be NULL in the case the gameplay
			// entity is detached from the world because of a game shutdown - see 
			// CCommonGame::ShutDown)
			if ( manager )
			{
				manager->RemoveInteractionActivator( this );
			}
		}

		GCommonGame->OnDetachGameplayEntity( this );
	}

	// update animated properties
	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->OnDetached( world );
	}

#ifndef RED_FINAL_BUILD
	TSet< EShowFlags >::iterator itEnd = m_scriptedVisualDebugFlags.End();
	for ( TSet< EShowFlags >::iterator it = m_scriptedVisualDebugFlags.Begin(); it != itEnd; ++it )
	{
		world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, *it );
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
//
// save game state
void CGameplayEntity::OnSaveGameplayState( IGameSaver* saver )
{
	// PLEASE DO NOT CALL SCRIPT EVENTS during save
	// Saving game is performance-critical and calling script events is expensive.
	// Please do not do this.
	// CallEvent( CNAME( OnSaveGameplayState ) );

	// Pass to base class
	TBaseClass::OnSaveGameplayState( saver );

	// Save character stats
	{
		CGameSaverBlock block( saver, CNAME(stats) );

		CCharacterStats* stats = GetCharacterStats();
		const Bool hasStats = stats != NULL;
		saver->WriteValue( CNAME(hasStats), hasStats );

		// Save stats
		if ( hasStats )
		{
			stats->SaveState( saver );
		}
	}

	// Save timers
	
	GCommonGame->GetActiveWorld()->GetTickManager()->GetTimerManager().SaveTimers( this, saver );

	// store animated properties
	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->OnSaveGameplayState( saver );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// load game state
void CGameplayEntity::OnLoadGameplayState( IGameLoader* loader )
{
	// Pass to base class
	TBaseClass::OnLoadGameplayState( loader );

	// Restore stats
	{
		CGameSaverBlock block( loader, CNAME(stats) );

		const Bool hasStats = loader->ReadValue< Bool >( CNAME(hasStats) ); 
		if ( hasStats )
		{
			CCharacterStats* stats = GetCharacterStats();
			ASSERT( stats );

			stats->RestoreState( loader );
		}
	}

	// Load timers

	GCommonGame->GetActiveWorld()->GetTickManager()->GetTimerManager().LoadTimers( this, loader );

	// store animated properties
	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->OnLoadGameplayState( loader );
	}

	CallEvent( CNAME( OnLoadGameplayState ) );
}

Vector CGameplayEntity::GetAimPosition() const
{
	return GetWorldPosition() + GetLocalToWorld().TransformVector( m_aimVector );
}

Vector CGameplayEntity::GetBarPosition() const
{
	return GetWorldPosition() + m_aimVector;
}

void CGameplayEntity::GetStorageBounds( Box& box ) const
{
	if ( const CEntityTemplate* entityTemplate = GetEntityTemplate() )
	{
		if ( const CAttackableArea* params = entityTemplate->FindGameplayParamT< CAttackableArea >() )
		{
			const Vector cylinderWorldPos = m_localToWorld.TransformPoint( params->GetOffset() );
			box.Min = cylinderWorldPos - Vector( params->GetRadius(), params->GetRadius(), 0.0f );
			box.Max = cylinderWorldPos + Vector( params->GetRadius(), params->GetRadius(), params->GetHeight() );
			return;
		}
	}

	box.Min = box.Max = GetWorldPositionRef();
}

void CGameplayEntity::SetFocusModeVisibility( EFocusModeVisibility focusModeVisibility, Bool persistent /* = false */, Bool force /* = false */ )
{
	if ( m_focusModeVisibility != focusModeVisibility || force )
	{
		m_focusModeVisibility = focusModeVisibility;
		m_infoCache.Set( GICT_Custom4, false );
		if ( GCommonGame != nullptr && GCommonGame->IsActive() )
		{
			GCommonGame->OnFocusModeVisibilityChanged( this, persistent );
		}
	}
}

void CGameplayEntity::StartMaterialEffect( const CName& effectName )
{
	if ( !m_materialEffects )
	{
		m_materialEffects = new SEntityMaterialEffectInfo();

		// Save and disable any existing material replacement.
		if ( m_materialReplacementInfo )
		{
			m_materialEffects->SetMaterialReplacement( m_materialReplacementInfo );
			DisableMaterialReplacement();
		}
	}

	PlayEffect( effectName );
	m_materialEffects->m_activeEffects.PushBack( effectName );
}

void CGameplayEntity::StopMaterialEffect( const CName& effectName )
{
	if ( m_materialEffects && m_materialEffects->m_activeEffects.Remove( effectName ) )
	{
		StopEffect( effectName );

		// If this was the last effect, we need to clean up.
		if ( m_materialEffects->m_activeEffects.Empty() )
		{
			// If we have saved material replacement, re-apply it.
			if ( m_materialEffects->m_savedMaterialReplacementInfo )
			{
				SetMaterialReplacement(
					m_materialEffects->m_savedMaterialReplacementInfo->material,
					m_materialEffects->m_savedMaterialReplacementInfo->drawOriginal,
					m_materialEffects->m_savedMaterialReplacementInfo->tag,
					m_materialEffects->m_savedMaterialReplacementInfo->exclusionTag,
					&m_materialEffects->m_savedMaterialReplacementInfo->includeList,
					&m_materialEffects->m_savedMaterialReplacementInfo->excludeList,
					m_materialEffects->m_savedMaterialReplacementInfo->forceMeshAlternatives );
			}

			delete m_materialEffects;
			m_materialEffects = NULL;
		}
	}
}

void CGameplayEntity::SetComponentsVisible( const CName& componentTag, Bool visible )
{
	for ( ComponentIterator<CDrawableComponent> it( this ); it; ++it )
	{
		CDrawableComponent* comp = (*it);

		if ( comp->GetTags().HasTag( componentTag ) )
		{
			comp->SetVisible( visible );
		} 
	}
}

Bool CGameplayEntity::SetMaterialReplacement( IMaterial* material, Bool drawOriginal /* = false */, const CName& tag /* = CName::NONE */, const CName& exclusionTag /* = CName::NONE */, const TDynArray< CName >* includeList /* = nullptr */, const TDynArray< CName >* excludeList /* = nullptr */, Bool forceMeshAlternatives /* = false */ )
{
	// If we have an active material effect, just save the replacement info for later use.
	if ( m_materialEffects )
	{
		// As with default material replacement, we cannot set a new replacement if we already have one.
		ASSERT( material );
		if ( m_materialEffects->m_savedMaterialReplacementInfo )
		{
			WARN_GAME( TXT("Unable to set duplicated material replacement") );
			return false;
		}

		m_materialEffects->SetMaterialReplacement( material, drawOriginal, tag, exclusionTag, includeList, excludeList, forceMeshAlternatives );

		return true;
	}

	return TBaseClass::SetMaterialReplacement( material, drawOriginal, tag, exclusionTag, includeList, excludeList, forceMeshAlternatives );
}

void CGameplayEntity::DisableMaterialReplacement()
{
	// If we have an active material effect, we just clear out our saved info.
	if ( m_materialEffects )
	{
		ASSERT( m_materialEffects->m_savedMaterialReplacementInfo != NULL, TXT("Trying to disable material replacement, when there isn't one to start with") );
		m_materialEffects->ClearMaterialReplacement();
		return;
	}

	return TBaseClass::DisableMaterialReplacement();
}

void CGameplayEntity::CacheSfxTag()
{
	m_sfxTagCached = CName::NONE;

	if ( m_template.IsValid() )
	{
		CAnimGlobalParam* animParam = m_template->FindParameter< CAnimGlobalParam >( true );
		if( animParam != NULL )
		{
			m_sfxTagCached = animParam->GetSfxTag();
		}
	}

#ifdef RED_ASSERTS_ENABLED
	m_sfxTagCachedFlag = true;
#endif
}

//////////////////////////////////////////////////////////////////////////
void CGameplayEntity::funcGetInventory( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetInventoryComponent() );
}

void CGameplayEntity::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const*/
{
	new ( localizedStrings ) LocalizedStringEntry( &m_displayName, TXT( "Entity display name" ), m_template.Get() );
	const CAlternativeDisplayName* param = nullptr;
	if( m_template )
	{
		param = static_cast< const CAlternativeDisplayName* >( m_template->FindGameplayParam( CAlternativeDisplayName::GetStaticClass() ) );
	}
	if ( param )
	{
		new ( localizedStrings ) LocalizedStringEntry( const_cast<LocalizedString*> ( &param->GetAltName() ), TXT( "Alternative Entity display name ( from ent param )" ), m_template.Get() );
	}		
}

void CGameplayEntity::funcGetDisplayName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, fallBack, true );
	FINISH_PARAMETERS;

	if ( fallBack )
	{
		RETURN_STRING( GetDisplayName() );
	}
	else
	{
		RETURN_STRING( m_displayName.GetString() );
	}
}

void CGameplayEntity::funcGetCharacterStats( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetCharacterStats() );
}

void CGameplayEntity::PlayPropertyAnimation( CName animationName, Uint32 count, Float lengthScale, EPropertyCurveMode mode )
{
	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->Play( animationName, count, lengthScale, mode );
	}
}

void CGameplayEntity::funcPlayPropertyAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER_OPT( Uint32, count, 0 );
	GET_PARAMETER_OPT( Float, lengthScale, 1.0f );
	GET_PARAMETER_OPT( EPropertyCurveMode, mode, PCM_Forward );
	FINISH_PARAMETERS;

	PlayPropertyAnimation( animationName, count, lengthScale, mode );
}

void CGameplayEntity::StopPropertyAnimation( CName animationName, Bool restoreInitialValues )
{
	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->Stop( animationName, restoreInitialValues );
	}
}

void CGameplayEntity::StopAllPropertyAnimations( Bool restoreInitialValues )
{
	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->StopAll( restoreInitialValues );
	}
}

void CGameplayEntity::funcStopPropertyAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER( Bool, restoreInitialValues, true );
	FINISH_PARAMETERS;

	StopPropertyAnimation( animationName, restoreInitialValues );
}

void CGameplayEntity::RewindPropertyAnimation( CName animationName, Float time )
{
	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->Rewind( animationName, time );
	}
}

void CGameplayEntity::funcRewindPropertyAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER( Float, time, 0.0f );
	FINISH_PARAMETERS;

	RewindPropertyAnimation( animationName, time );
}

void CGameplayEntity::PausePropertyAnimation( CName animationName )
{
	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->Pause( animationName );
	}
}

void CGameplayEntity::UnpausePropertyAnimation( CName animationName )
{
	if ( m_propertyAnimationSet )
	{
		m_propertyAnimationSet->Unpause( animationName );
	}
}

Bool CGameplayEntity::GetPropertyAnimationInstanceTime( CName propertyName, CName animationName, Float& outTime )
{
	return m_propertyAnimationSet ? m_propertyAnimationSet->GetAnimationInstanceTime( propertyName, animationName, outTime ) : false;
}

void CGameplayEntity::funcGetPropertyAnimationInstanceTime( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, propertyName, CName::NONE );
	GET_PARAMETER( CName, animationName, CName::NONE );
	FINISH_PARAMETERS;

	Float time = 0.0f;
	GetPropertyAnimationInstanceTime( propertyName, animationName, time );

	RETURN_FLOAT( time );
}

Bool CGameplayEntity::GetPropertyAnimationLength( CName propertyName, CName animationName, Float& length )
{
	return m_propertyAnimationSet ? m_propertyAnimationSet->GetAnimationLength( propertyName, animationName, length ) : false;
}

void CGameplayEntity::funcGetPropertyAnimationLength( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, propertyName, CName::NONE );
	GET_PARAMETER( CName, animationName, CName::NONE );
	FINISH_PARAMETERS;

	Float length = 0.0f;
	GetPropertyAnimationLength( propertyName, animationName, length );

	RETURN_FLOAT( length );
}

Bool CGameplayEntity::GetPropertyAnimationTransformAt( CName propertyName, CName animationName, Float time, EngineTransform& outTransform )
{
	return m_propertyAnimationSet ? m_propertyAnimationSet->GetAnimationTransformAt( propertyName, animationName, time, outTransform ) : false;
}

void CGameplayEntity::funcGetPropertyAnimationTransformAt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, propertyName, CName::NONE );
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER( Float, time, 0.0f );
	FINISH_PARAMETERS;

	EngineTransform transform;
	if ( !GetPropertyAnimationTransformAt( propertyName, animationName, time, transform ) )
	{
		transform.Identity();
	}

	Matrix matrix;
	transform.CalcLocalToWorld( matrix );

	RETURN_STRUCT( Matrix, matrix );
}

void CGameplayEntity::funcGetGameplayEntityParam( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	CGameplayEntityParam* param = nullptr;
	if ( className != CName::NONE )
	{
		CEntityTemplate* templ = GetEntityTemplate();
		if ( templ != nullptr )
		{
			CClass* paramClass = SRTTI::GetInstance().FindClass( className );
			if ( paramClass != nullptr )
			{
				param = templ->FindGameplayParam( paramClass, true );
			}
		}
	}

	RETURN_OBJECT( param );
}

void CGameplayEntity::funcEnableVisualDebug( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EShowFlags, flag, SHOW_Meshes );
	GET_PARAMETER( Bool, enable, true );
	FINISH_PARAMETERS;

#ifndef RED_FINAL_BUILD
	if ( GetLayer()->GetWorld() != nullptr )
	{
		if ( enable )
		{
			if ( !m_scriptedVisualDebugFlags.Exist( flag ) )
			{
				m_scriptedVisualDebugFlags.Insert( flag );
				GetLayer()->GetWorld()->GetEditorFragmentsFilter().RegisterEditorFragment( this, flag );
			}
		}
		else
		{
			if ( m_scriptedVisualDebugFlags.Exist( flag ) )
			{
				m_scriptedVisualDebugFlags.Erase( flag );
				GetLayer()->GetWorld()->GetEditorFragmentsFilter().UnregisterEditorFragment( this, flag );
			}
		}
	}
#endif

	RETURN_VOID();
}

void CGameplayEntity::funcGetStorageBounds( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Box, box, Box() );
	FINISH_PARAMETERS;

	GetStorageBounds( box );

	RETURN_VOID();
}

void CGameplayEntity::funcGetGameplayInfoCache( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGameplayInfoCacheType, type, EGameplayInfoCacheType::GICT_IsInteractive );
	FINISH_PARAMETERS;

	RETURN_BOOL( m_infoCache.Get( this, type ) );
}

void CGameplayEntity::funcGetFocusModeVisibility( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_focusModeVisibility );
}

void CGameplayEntity::funcSetFocusModeVisibility( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EFocusModeVisibility, focusModeVisibility, FMV_None );
	GET_PARAMETER_OPT( Bool, persistent, false );
	GET_PARAMETER_OPT( Bool, force, false );
	FINISH_PARAMETERS;

	SetFocusModeVisibility( focusModeVisibility, persistent, force );
}

Bool CGameplayEntity::CheckShouldSave() const
{
	if ( IsManaged() )
	{
		return true;
	}

	if ( m_propertyAnimationSet && m_propertyAnimationSet->CheckShouldSave() )
	{
		return true;
	}

	return TBaseClass::CheckShouldSave();
}

Bool CGameplayEntity::CanExtractComponents( const Bool isOnStaticLayer ) const
{
	// it's never allowed to extract stuff from generic gameplay entity
	return false;
}

// Update transform data (called from multiple threads!!!)
void CGameplayEntity::OnUpdateTransformEntity()
{
	TBaseClass::OnUpdateTransformEntity();

	// Register in the update list
	GCommonGame->RegisterEntityForGameplayStorageUpdate( this );
}

void CGameplayEntity::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

#ifndef RED_FINAL_BUILD
	if ( file.IsGarbageCollector() )
	{
		file << m_scriptedRenderFrame;
	}
#endif
}

void CGameplayEntity::OnProxyAttached( CComponent* component, IRenderProxy* proxy )
{
	TBaseClass::OnProxyAttached( component, proxy );
	m_infoCache.Set( GICT_HasDrawableComponents, true );
}

void CGameplayEntity::OnStreamIn()
{
	// reset information about focus mode visibility being updated
	m_infoCache.Set( GICT_Custom4, false );
}

void CGameplayEntity::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if( flag & SHOW_AIRanges )
	{
		const CEntityTemplate* templ = GetEntityTemplate();
		const CAttackableArea* params = templ ? templ->FindGameplayParamT<CAttackableArea>() : NULL;

		if ( params != nullptr )
		{
			const Matrix& localToWorld = GetLocalToWorld();
			const Vector pos = localToWorld.TransformPoint( params->GetOffset() );
			frame->AddDebugWireframeTube( pos, Vector( 0.f, 0.f, params->GetHeight() ) + pos, params->GetRadius(), params->GetRadius(), Matrix::IDENTITY, Color::BLUE, Color::BLUE );
		}

	}

#ifndef RED_FINAL_BUILD

	if ( m_scriptedVisualDebugFlags.Exist( flag ) )
	{
		if ( m_scriptedRenderFrame == nullptr )
		{
			m_scriptedRenderFrame = CreateObject< CScriptedRenderFrame >( this );
		}
		m_scriptedRenderFrame->Set( frame );
		THandle< CScriptedRenderFrame > frameHandle( m_scriptedRenderFrame );
		CallEvent( CNAME( OnVisualDebug ), frameHandle, flag );
		m_scriptedRenderFrame->Set( nullptr );
	}

#endif
}

#ifndef NO_EDITOR
void CGameplayEntity::EditorOnTransformChangeStart()
{
	TBaseClass::EditorOnTransformChangeStart();
}

void CGameplayEntity::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();

	const Bool isInitialTransform = GIsEditor && !GIsEditorGame;
	if ( isInitialTransform && m_propertyAnimationSet )
	{
		m_propertyAnimationSet->UpdateInitialTransformFrom( this );
	}
}

void CGameplayEntity::EditorOnTransformChangeStop()
{
	TBaseClass::EditorOnTransformChangeStop();
}

void CGameplayEntity::EditorPostCreation()
{
	CEntityTemplate* t = m_template.Get();

	if ( t )
	{
		TDynArray< CGameplayEntityParam* > setupParams;
		t->CollectGameplayParams( setupParams, CEdEntitySetupListParam::GetStaticClass() );
		for ( auto it = setupParams.Begin(), end = setupParams.End(); it != end; ++it )
		{
			static_cast< CEdEntitySetupListParam* >( *it )->OnSpawn( this );
		}
	}
	TBaseClass::EditorPostCreation();
}

void CGameplayEntity::OnComponentTransformChanged( CComponent* component )
{
	const Bool isInitialTransform = GIsEditor && !GIsEditorGame;
	if ( isInitialTransform && m_propertyAnimationSet )
	{
		m_propertyAnimationSet->UpdateInitialTransformFrom( component );
	}
}

#endif

void CGameplayEntity::ProcessAnimationEvent( const CAnimationEventFired* event )
{
	// Dispatch the message to scripts

	if ( event->ReportToScript() )
	{
#ifndef RED_FINAL_BUILD
		CTimeCounter timer;
#endif
		m_scriptAnimEventManager.OnEvent( this, event->m_extEvent->GetEventName(), event->m_type, event->m_animInfo );

#ifndef RED_FINAL_BUILD
		CExtAnimEvent::StatsCollector::GetInstance().OnEvent( event->m_extEvent->GetEventName(), ( Float ) timer.GetTimePeriodMS() );
#endif
	}
}

void CGameplayEntity::funcAddAnimEventCallback( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER( CName, functionName, CName::NONE );
	FINISH_PARAMETERS;
	m_scriptAnimEventManager.AddCallback( eventName, functionName );
}

void CGameplayEntity::funcRemoveAnimEventCallback( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;
	m_scriptAnimEventManager.RemoveCallback( eventName );
}

void CGameplayEntity::funcAddAnimEventChildCallback( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, child, THandle< CNode >() );
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER( CName, functionName, CName::NONE );
	FINISH_PARAMETERS;
	m_scriptAnimEventManager.AddChildCallback( child, eventName, functionName );

}

void CGameplayEntity::funcRemoveAnimEventChildCallback( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, child, THandle< CNode >() );
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;
	m_scriptAnimEventManager.RemoveChildCallback( child, eventName );
}

void CGameplayEntity::funcGetSfxTag( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( GetSfxTag() );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( IEntityStateChangeRequest );

String IEntityStateChangeRequest::OnStateChangeRequestsDebugPage() const
{
	return GetFriendlyName();
}

void IEntityStateChangeRequest::SaveState( IGameSaver* saver )
{
	CGameSaverBlock block0( saver, CNAME(requestState) );
	saver->SaveObject( this );
}

IEntityStateChangeRequest* IEntityStateChangeRequest::RestoreState( IGameLoader* loader, CObject* parent )
{
	CGameSaverBlock block0( loader, CNAME(requestState) );
	IEntityStateChangeRequest* request = loader->RestoreObject< IEntityStateChangeRequest >();
	return request;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CScriptedEntityStateChangeRequest );

void CScriptedEntityStateChangeRequest::Execute( CGameplayEntity* entity )
{
	THandle< CGameplayEntity > hEntity( entity );
	CallFunction( this, CNAME( Execute ), hEntity );
}

///////////////////////////////////////////////////////////////////////////////

/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/engine/appearanceComponent.h"

#include "../../common/game/definitionsManager.h"
#include "../../common/core/jobGenericJobs.h"
#include "../../common/core/loadingJobManager.h"
#include "../../common/core/depot.h"
#include "../../common/engine/jobSpawnEntity.h"
#include "../core/dataError.h"
#include "../engine/actorInterface.h"
#include "../engine/mimicComponent.h"
#include "../engine/meshSkinningAttachment.h"
#include "../engine/animatedAttachment.h"
#include "../engine/attachmentUtils.h"
#include "../engine/dynamicLayer.h"
#include "../engine/rigidMeshComponent.h"
#include "../physics/physicsWrapper.h"
#include "../engine/utils.h"

#include "../../common/engine/slotComponent.h"


IMPLEMENT_ENGINE_CLASS( CItemEntity );
IMPLEMENT_RTTI_ENUM( EItemLatentAction );
IMPLEMENT_RTTI_ENUM( EItemHand );

RED_DEFINE_STATIC_NAME( OnMount );
RED_DEFINE_STATIC_NAME( OnDrop );
RED_DEFINE_STATIC_NAME( OnDetach );


const Float GPreloadedTemplatesExpireTime = 90.f; 
const Float GRefreshTemplatesTime = 5.f;

//Make item preloading jobs high prioriry
Bool GImmediateDeploymentEntitiesLoadingJobs = true;

//////////////////////////////////////////////////////////////////////////

namespace
{
	CName GetAppearanceName( const CItemEntityProxy* itemProxy )
	{
		CGameplayEntity* gpEntity = Cast<CGameplayEntity>( itemProxy->m_parentAttachmentEntity.Get() );

		CInventoryComponent* ic = gpEntity ? gpEntity->GetInventoryComponent() : nullptr;

		SInventoryItem* item = ic ? ic->GetItem( itemProxy->m_item ) : nullptr;

		if ( item && item->GetDyePreviewColor() != CName::NONE )
		{
			return item->GetDyePreviewColor();
		}
		else if ( item && item->GetDyeColorStats() != CName::NONE )
		{
			return item->GetDyeColorStats();
		}
		return CName::NONE;
	}
}


void CItemEntityProxy::SetAttachment( CEntity* parentEntity, CName slotName /*= String::EMPTY */ )
{
	// Save info
	m_parentAttachmentEntity = parentEntity;
	m_slotName = slotName;

	// Queue for apply
	SItemEntityManager::GetInstance().QueueItemEntityAttachmentUpdate( this );
}

void CItemEntityProxy::Reattach()
{
	// Queue for apply
	SItemEntityManager::GetInstance().QueueItemEntityAttachmentUpdate( this );
}

void CItemEntityProxy::EnableCollisionInfoReporting()
{
	// deprecated
}

void CItemEntityProxy::ChangeItem( CName itemName, CName slotOverride, const TDynArray< CName >& slotItems, const String& templateName, Bool collapse, SItemUniqueId item )
{
	m_itemName = itemName;
	m_slotName = slotOverride;
	m_slotItems = slotItems;
	m_template = templateName;
	m_collapse = collapse;
	m_dirty = true;
	m_item = item;

	SItemEntityManager::GetInstance().QueueItemEntitySpawnRequest( this );
}

void CItemEntityProxy::SkinOn( CEntity* parentEntity )
{
	// Save info
	m_parentAttachmentEntity = parentEntity;
	m_slotName = CName::NONE;

	// Queue for apply
	SItemEntityManager::GetInstance().QueueItemEntityAttachmentUpdate( this );
}

Bool CItemEntityProxy::GetItemSpawnInfo( SItemSpawnInfo& itemSpawnInfo ) const
{
	const CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	if ( defMgr == nullptr )
	{
		return false;
	}

	const SItemDefinition* itemDef = defMgr->GetItemDefinition( m_itemName );
	if ( itemDef == nullptr )
	{
		ITEM_ERR( TXT("Item entity spawn request - undefined item %s"), m_itemName.AsString().AsChar() );
		return false;
	}

	itemSpawnInfo.m_templatePath = defMgr->TranslateTemplateName( m_template );

	if ( itemSpawnInfo.m_appearance == CName::NONE )
	{
		if( CName appName = GetAppearanceName( this ) )
		{
			itemSpawnInfo.m_appearance = appName;
		}
		else
		{
			itemSpawnInfo.m_appearance = itemDef->GetItemAppearanceName( IsPlayerEntityItem() );
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

CItemEntity::CItemEntity() 
	: m_rootMesh( NULL )
	, m_timeToDespawn( 1.0f )
	, m_proxy( NULL )
	, m_reportToScript( false )
	, m_customAttacht( false )
	, m_customDetacht( false )
{
}

void CItemEntity::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	// Store ptr to the root rigid mesh component
	for ( ComponentIterator< CMeshTypeComponent > it( this ); it; ++it )
	{
		CMeshTypeComponent* current = *it;
		if ( current->GetParentAttachments().Empty() )
		{
			m_rootMesh = current;
		}
	}
}

void CItemEntity::OnDetached( CWorld* world )
{
	// We were detached
	Detach();

	// Pass to base class
	TBaseClass::OnDetached( world );
}


void CItemEntity::OnDestroyed( CLayer* layer )
{
	if ( m_parentEntity.Get() && m_parentEntity.Get()->IsA< CActor >())
	{
		CActor* actor = static_cast< CActor* >( m_parentEntity.Get() );

		for ( CComponent* component : m_components )
		{
			if ( component->IsA< CMimicComponent >() )
			{
				actor->SetMimicComponent( nullptr );
				break;
			}
		}
	}

	// Pass to base class
	TBaseClass::OnDestroyed( layer );
}

void CItemEntity::Drop()
{
	// We need to report this event to script
	if ( m_reportToScript )
	{
		CallEvent( CNAME( OnDrop ), m_parentEntity );
	}

	// Dropped item can be destroyed from scripts
	SetEntityFlag( EF_DestroyableFromScript );

	// Break attachments
	Detach();

}

void CItemEntity::Undrop()
{
	RemoveTimer( CNAME( Despawn ) );
}

Bool CItemEntity::ApplyHardAttachment( CComponent* component, const HardAttachmentSpawnInfo& info )
{
	// Attach all root components
	for ( CComponent* current : m_components )
	{
		CComponent* rootComponent = current;
		if ( rootComponent->GetParentAttachments().Empty() )
		{
			rootComponent->SetPosition( Vector::ZEROS );
			rootComponent->SetRotation( EulerAngles::ZEROS );
			component->Attach( rootComponent, info );

			rootComponent->ForceUpdateTransformNodeAndCommitChanges();
			rootComponent->ForceUpdateBoundsNode();
		}
	}	

	// Attach whole item to parent component
	IAttachment* att = component->Attach( this, info );
	return att != nullptr;
}

CComponent* CItemEntity::FindRootComponentWithAnimatedInterface() const
{
	CComponent* candidate = NULL;
	for ( CComponent* comp : m_components )
	{
		if ( !candidate && comp->QueryAnimatedObjectInterface() )
		{
			candidate = comp;

			// Check whether this is a root component in item entity
			const TList<IAttachment*>& attArray = comp->GetParentAttachments();
			for ( TList< IAttachment* >::const_iterator attIt = attArray.Begin(); attIt != attArray.End(); ++attIt )
			{
				IAttachment* att = *attIt;

				// Check only attachments to this entity
				CComponent* parentComponent = Cast< CComponent >( att->GetParent() );
				if ( !parentComponent || parentComponent->GetEntity() == this )
				{
					candidate = NULL;
					break;
				}
			}
		}
	}
	return candidate;
}

void CItemEntity::SetItemProxy( CItemEntityProxy* proxy )
{
	m_proxy = proxy;
}

Bool CItemEntity::ApplyAttachmentTo( CEntity* parentEntity, CName slot /* = CName::NONE */ )
{
	if ( !parentEntity )return false;

	// Detach from current entity
	Detach( !parentEntity->HasMaterialReplacement() );
	
	Bool entityAttachedToComponent = false;
	Bool entityAttachedToSlot = false;

	// Get animated component attached to entity
	CAnimatedComponent * animComponent = parentEntity->GetRootAnimatedComponent();
	const EntitySlot* entitySlot = nullptr;

	if ( animComponent )
	{
		// get each component from item entity and try to attach it
		for ( CComponent* cmpToAttach : m_components )
		{
			//we dont have slot
			if ( cmpToAttach->GetParentAttachments().Empty() )
			{
				//check if we need to attach to slot
				if ( slot == CName::NONE )
				{
					// attach mimic
					if ( CMimicComponent* mCmpToAttach = Cast< CMimicComponent >( cmpToAttach ) )
					{
						AnimatedAttachmentSpawnInfo sinfo;
						sinfo.m_parentSlotName = mCmpToAttach->GetAttachmentSlotName();
						CAnimatedAttachment* att = SafeCast< CAnimatedAttachment >( animComponent->Attach( mCmpToAttach, sinfo ) );
					
						if( !att ) return false;

						mCmpToAttach->ForceUpdateTransformNodeAndCommitChanges();
						mCmpToAttach->ForceUpdateBoundsNode();

						if ( parentEntity->IsA< CActor >() )
						{
							CActor* actor = SafeCast< CActor >( parentEntity );
							actor->SetMimicComponent( mCmpToAttach );
						}
						entityAttachedToComponent = true;
					}
					// attach dangling component
					else if ( cmpToAttach->QueryAnimatedObjectInterface() )
					{
						CAnimatedAttachment* att = SafeCast< CAnimatedAttachment >( animComponent->Attach( cmpToAttach, ClassID< CAnimatedAttachment >() ) );

						if( !att ) return false;

						// Do we need this?
						cmpToAttach->ForceUpdateTransformNodeAndCommitChanges();
						cmpToAttach->ForceUpdateBoundsNode();
						entityAttachedToComponent = true;
					}
					// Create appropriate attachment depending on component type - see appearanceComponent.cpp
					else if ( cmpToAttach->IsA< CMeshTypeComponent >() )
					{
						CMeshTypeComponent* meshComponent = Cast< CMeshTypeComponent >( cmpToAttach );
						CAttachmentUtils::CreateSkinningAttachment( parentEntity, animComponent, meshComponent );

						cmpToAttach->ForceUpdateTransformNodeAndCommitChanges();
						cmpToAttach->ForceUpdateBoundsNode();
						entityAttachedToComponent = true;
					}
				}
				else 
				{
					// Find slot
					entitySlot = parentEntity->GetEntityTemplate()->FindSlotByName( slot, true );
					if ( entitySlot )
					{
						// Connect the item
						if ( CComponent* parentComponent = parentEntity->FindComponent( entitySlot->GetComponentName() ) )
						{
							// Prepare attachment info
							HardAttachmentSpawnInfo hardAttachmentInfo;

							// Slot info found, apply it to attachment info
							hardAttachmentInfo.m_parentSlotName = entitySlot->GetBoneName();
							hardAttachmentInfo.m_relativePosition = entitySlot->GetTransform().GetPosition();
							hardAttachmentInfo.m_relativeRotation = entitySlot->GetTransform().GetRotation();

							// Connect the item
							CHardAttachment* hAtt = SafeCast< CHardAttachment >( parentComponent->Attach( cmpToAttach, hardAttachmentInfo ) );
							entityAttachedToComponent = hAtt != nullptr;
						}
					}
					else
					{
						RED_ASSERT( false, TXT( "Could not find the slot specifed %s in entity '%s', required by item '%s'" ), slot.AsChar(), parentEntity->GetEntityTemplate()->GetDepotPath().AsChar(), m_proxy->m_itemName.AsChar() );
					}
				}
			}
		}
	}


	// If we still didnt hit anything in canimatedcomponent try slotcomponent
	// its used in community/ap's for spawning items in poses set by skeleton
	if ( !entityAttachedToComponent )
	{
		for ( ComponentIterator< CSlotComponent > it( parentEntity ); it; ++it )
		{
			CSlotComponent* slotComp = *it;
			const SSlotInfo* slotInfo = slotComp->GetSlotByName( slot );

			if ( slotInfo )
			{
				// Prepare attachment info	
				HardAttachmentSpawnInfo hardAttachmentInfo;

				// Slot info found, apply it to attachment info
				hardAttachmentInfo.m_parentSlotName = slotInfo->m_parentSlotName;
				hardAttachmentInfo.m_relativePosition = slotInfo->m_relativePosition;
				hardAttachmentInfo.m_relativeRotation = slotInfo->m_relativeRotation;

				// Connect the item
				// TODO - returns always 'true' because of items hacks
				/*entityAttachedToSlot =*/ApplyHardAttachment( animComponent ? (CComponent*) animComponent : (CComponent*) slotComp, hardAttachmentInfo );

				// We were successfully connected
				entityAttachedToSlot = true;
				break;
			}
		}
	}

	// Attach item entity to parent component
	if ( !entityAttachedToSlot && entitySlot != nullptr )
	{
		// Connect the item
		if ( CComponent* parentComponent = parentEntity->FindComponent( entitySlot->GetComponentName() ) )
		{
			HardAttachmentSpawnInfo attachmentInfo;

			// Slot info found, apply it to attachment info
			attachmentInfo.m_parentSlotName = entitySlot->GetBoneName();
			attachmentInfo.m_relativePosition = entitySlot->GetTransform().GetPosition();
			attachmentInfo.m_relativeRotation = entitySlot->GetTransform().GetRotation();

			// Connect the item
			entityAttachedToSlot = ( parentComponent->Attach( this, attachmentInfo ) != nullptr );
		}
	}
	if ( !entityAttachedToSlot )
	{
		HardAttachmentSpawnInfo attachmentInfo;
		IAttachment* att = parentEntity->Attach( this, attachmentInfo );
		entityAttachedToSlot = att != nullptr;;
	}

	// Apply any material replacement
	if ( parentEntity->HasMaterialReplacement() )
	{
		const SMaterialReplacementInfo* info = parentEntity->GetMaterialReplacementInfo();
		SetMaterialReplacement( info->material, info->drawOriginal, info->tag, info->exclusionTag, &info->includeList, &info->excludeList, info->forceMeshAlternatives );
	}	

	// Remember attached entity
	m_parentEntity = parentEntity;
	SetForceNoLOD( m_parentEntity && !m_parentEntity->IsGameplayLODable() );

	return entityAttachedToComponent || entityAttachedToSlot;
}

void CItemEntity::Detach( Bool keepMaterialReplacement /* =false */ )
{
	// Drop material replacement
	if ( !keepMaterialReplacement && HasMaterialReplacement() )
	{
		DisableMaterialReplacement();
	}
		
	// Report to script
	if ( m_reportToScript )
	{
		CallEvent( CNAME( OnDetach ), m_parentEntity );
	}

	// Process components
	for ( CComponent* component : m_components )
	{
		TDynArray< IAttachment* > attArray;

		// Break attachments
		for ( IAttachment* att : component->GetParentAttachments() )
		{
			// Break only attachments to parent entity
			CComponent* parentComponent = Cast< CComponent >( att->GetParent() );
			if ( parentComponent && parentComponent->GetEntity() != this )
			{
				attArray.PushBack( att );
			}
		}

		// Deffered break to avoid iterator corruption
		for( Uint32 j=0; j<attArray.Size(); j++ )
		{
			attArray[ j ]->Break();
		}
	}

	// Break the parent transform attachment on the entity itself
	CHardAttachment* itemParentAttachment = GetTransformParent();
	if ( itemParentAttachment )
	{
		itemParentAttachment->Break();
	}

	// We need to call that explicitly, because some of the components may have been attached to another entity
	// (which means item entity may be "invisible" at the moment) and breaking attachment does not automatically
	// refresh visibility flag.
	RefreshChildrenVisibility();

	// We are not attached to an entity any more
	m_parentEntity = nullptr;
	SetForceNoLOD( false );
}


void CItemEntity::Collapse( Bool collapse )
{
	ComponentIterator< CDrawableComponent > it( this );
	for ( ; it; ++it )
	{
		CDrawableComponent* comp = *it;
		comp->SetUsesVertexCollapsed( collapse );
	}
}

Bool CItemEntity::IsVisible() const
{
	if ( !WasVisibleLastFrame() )
	{
		return false;
	}
	if ( GGame == nullptr || GGame->GetActiveWorld() == nullptr )
	{
		return false;
	}
	CCameraDirector* cameraDirector = GGame->GetActiveWorld()->GetCameraDirector();
	if ( cameraDirector == nullptr )
	{
		return false;
	}
	// Too far
	const Vector diff = GetWorldPositionRef() - cameraDirector->GetCameraPosition();
	if ( MAbs( diff.X ) > 20.0f || MAbs( diff.Y ) > 20.0f )
	{
		return false;
	}
	// Behind camera (no need to renormalize, since we check only sign)
	const Vector cameraForward = cameraDirector->GetCameraForward();
	if ( Vector::Dot2( diff, cameraForward ) < 0.0f )
	{
		return false;
	}
	return true;
}

void CItemEntity::funcGetMeshComponent( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	if ( result )
	{
		*(THandle<CComponent>*)result = m_rootMesh;
	}
}

void CItemEntity::funcGetParentEntity( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	if ( result )
	{
		*(THandle<CEntity>*)result = m_parentEntity;
	}
}

void CItemEntity::funcGetItemCategory( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	if( !m_proxy )
	{
		RETURN_NAME( CNAME( default ) );
		return;
	}

	const SItemDefinition* definition = GCommonGame->GetDefinitionsManager()->GetItemDefinition( m_proxy->m_itemName );
	if( !definition )
	{
		RETURN_NAME( CNAME( default ) );
		return;
	}
	
	RETURN_NAME( definition->m_category );
}

void CItemEntity::funcGetItemTags( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray<CName>, output, TDynArray<CName>() );
	FINISH_PARAMETERS;
	if( !m_proxy )
	{
		return;
	}
	const SItemDefinition* definition = GCommonGame->GetDefinitionsManager()->GetItemDefinition( m_proxy->m_itemName );
	if( !definition )
	{
		return;
	}

	output = definition->GetItemTags();
}

void CItemEntity::HackCopyRootMeshComponentLocalToWorld()
{
	if ( m_rootMesh.Get() )
	{
		// HACK: When fx spawner positions particle component, it relies on entity localToWorld
		m_localToWorld = m_rootMesh.Get()->GetLocalToWorld();
	}
}

Bool CItemEntity::IsForegroundEntity() const
{
	CEntity* parent = m_parentEntity.Get();
	if ( parent )
	{
		return parent->IsForegroundEntity();
	}
	return false;
}

void CItemEntity::OnAppearanceChanged( const CEntityAppearance& appearance )
{
	if ( m_proxy != nullptr )
	{
		m_proxy->Reattach();
	}
}

void CItemEntity::EnableCollisions( Bool enable )
{
	ComponentIterator< CRigidMeshComponent > it( this );
	for ( ; it; ++it )
	{
		CPhysicsWrapperInterface* physicsInterface = ( *it )->GetPhysicsRigidBodyWrapper();
		if ( physicsInterface != nullptr )
		{
			physicsInterface->SetFlag( PRBW_CollisionDisabled, !enable ); 
		}
	}
}

void CItemEntity::SwitchToKinematic( Bool isKinematic )
{
	ComponentIterator< CRigidMeshComponent > it( this );
	for ( ; it; ++it )
	{
		CPhysicsWrapperInterface* physicsInterface = ( *it )->GetPhysicsRigidBodyWrapper();
		if ( physicsInterface != nullptr )
		{
			physicsInterface->SwitchToKinematic( isKinematic );
		}
	}
}

const IActorInterface* CItemEntity::QueryActorInterface() const 
{
	if( GetTransformParent() && GetTransformParent()->GetParent() && GetTransformParent()->GetParent()->IsA<CEntity>() )
	{
		return static_cast< CEntity* >( GetTransformParent()->GetParent() )->QueryActorInterface();
	}
	return nullptr;
}

void CItemEntity::OnAttachmentUpdate()
{
	for( CComponent* comp : m_components )
	{
		comp->OnItemEntityAttached( m_parentEntity );
	}
	CallEvent( CNAME( OnAttachmentUpdate ), m_parentEntity, m_proxy->m_itemName);
}

#ifndef NO_DATA_VALIDATION
void CItemEntity::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );

	// Should never land on static layer
	if ( !isInTemplate )
	{
		if ( GetLayer() && GetLayer()->GetWorld() )
		{
			if ( GetLayer() != GetLayer()->GetWorld()->GetDynamicLayer() )
			{
				DATA_HALT( DES_Uber, CResourceObtainer::GetResource( this ), TXT("World"), TXT( "Entity is placed on static layer. That's not allowed." ) );
			}
		}
	}
}

#endif // NO_DATA_VALIDATION

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

SItemSpawnRequestInfo::SItemSpawnRequestInfo()
	: m_proxy( nullptr )
	, m_resourceJob( nullptr )
	, m_resource( nullptr )
	, m_spawnItemJob( nullptr )
	, m_itemEntity( nullptr )
{
}

SItemSpawnRequestInfo::~SItemSpawnRequestInfo()
{
	if ( m_resourceJob != nullptr )
	{
		m_resourceJob->Release();
		m_resourceJob = nullptr;
	}
	m_resource = nullptr;
	if ( m_spawnItemJob != nullptr )
	{
		m_spawnItemJob->Release();
		m_spawnItemJob = nullptr;
	}
	m_itemEntity = nullptr;
}

SItemSpawnRequestInfo::EItemSpawnRequestResult SItemSpawnRequestInfo::Process( CItemEntityManager* manager )
{
	if ( m_itemEntity != nullptr )
	{
		if ( InitItemEntity( manager ) )
		{
			return ISRR_Succeed;
		}
		return ISRR_Waiting;
	}
	else if ( m_resource != nullptr )
	{
		if ( !SpawnEntity( manager ) )
		{
			return ISRR_FailedToSpawnEntity;
		}
		return ISRR_Waiting;
	}
	else // m_resource == nullptr
	{
		if ( !LoadResource( manager ) )
		{
			return ISRR_FailedToLoadResource;
		}
		return ISRR_Waiting;
	}
	return ISRR_FailedUnknown;
}

SItemSpawnRequestInfo::EItemSpawnRequestResult SItemSpawnRequestInfo::ProcessCancel( CItemEntityManager* manager )
{
	if ( m_itemEntity != nullptr )
	{
		m_itemEntity->Destroy();
		m_itemEntity = nullptr;
	}
	if ( m_spawnItemJob != nullptr )
	{
		if ( !m_spawnItemJob->IsCanceled() )
		{
			m_spawnItemJob->Cancel();
		}
		if ( !m_spawnItemJob->HasEnded() )
		{
			return ISRR_Waiting;
		}
	}
	return ISRR_Succeed;
}

Bool SItemSpawnRequestInfo::LoadResource( CItemEntityManager* manager )
{
	if ( THandle< CEntityTemplate > handle = manager->CheckForLoadedEntityTemplate( this ) )
	{
		m_resource = handle;
		return true;
	}

	RED_ASSERT( m_proxy != nullptr );
	
	if ( m_resourceJob != nullptr )
	{
		if ( !m_resourceJob->HasEnded() )
		{
			return true;
		}
		else
		{
			if ( !m_resourceJob->HasFinishedWithoutErrors() )
			{
				return false;
			}
			else
			{
				CResource* res = m_resourceJob->GetResource();
				if ( res != nullptr )
				{
					CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( res );
					RED_ASSERT( entityTemplate != nullptr, TXT( "Loaded resource was expected to be entity template of inventory item, but it's not, debug!" ) );

					m_resource = entityTemplate;
					m_resourceJob->Release();
					m_resourceJob = nullptr;				

					return true;
				}
				else
				{
					return false;
				}
			}
		}
	}
	else
	{
		return false;
	}
}

Bool SItemSpawnRequestInfo::SpawnEntity( CItemEntityManager* manager )
{
	RED_ASSERT( m_proxy != nullptr );

	if ( m_spawnItemJob == nullptr )
	{
		// Set owner position, item will be streamed if owner needs it.
		// We don't know exact position because this information is in components that need to be streamed.
		CEntity* parentEntity = m_proxy->m_parentAttachmentEntity.Get();
		// Item may have been unmounted between mount and spawn so the parent entity is null.
		// We don't want to spawn it anymore.
		if ( parentEntity != nullptr )
		{
			// Template ready, spawn entity now!
			EntitySpawnInfo spawnInfo;
			spawnInfo.m_template = m_resource;
			spawnInfo.m_spawnPosition = parentEntity->GetWorldPositionRef();
			spawnInfo.m_forceNonStreamed = true;

			if ( m_appearance == CName::NONE )
			{
				m_appearance = GetAppearanceName( m_proxy );
			}

			if ( m_appearance != CName::NONE )
			{
				spawnInfo.m_appearances.PushBack( m_appearance );
			}

			if ( !GGame || !GGame->IsActive() || !GGame->GetActiveWorld() )
			{
				CEntity* entity = m_proxy->m_subjectLayer->CreateEntitySync( Move( spawnInfo ) );
				return SetSpawnedEntity( entity );
			}

			m_spawnItemJob = m_proxy->m_subjectLayer->CreateEntityAsync( Move( spawnInfo ) );

			return ( m_spawnItemJob != nullptr );
		}
		else
		{
			return false;
		}
	}
	else
	{
		if ( !m_spawnItemJob->HasEnded() )
		{
			return true;
		}
		else
		{
			if ( !m_spawnItemJob->HasFinishedWithoutErrors() )
			{
				return false;
			}
			else
			{
				CEntity* spawnedEntity = m_spawnItemJob->GetSpawnedEntity( true );
				RED_ASSERT( spawnedEntity );
				return SetSpawnedEntity( spawnedEntity );
			}
		}
	}
}

Bool SItemSpawnRequestInfo::SetSpawnedEntity( CEntity* spawnedEntity )
{
	m_itemEntity = Cast< CItemEntity >( spawnedEntity );
	if ( m_itemEntity == nullptr )
	{
		ITEM_ERR( TXT( "Template %s defines unsupported entity type, should be CItemEntity based" ), m_resource->GetDepotPath().AsChar() );
		spawnedEntity->Destroy(); // No DestroyEntityInternal here
		return false;
	}
	
	m_itemEntity->SetHideInGame( true, false );
	return true;
}

Bool SItemSpawnRequestInfo::InitItemEntity( CItemEntityManager* manager )
{
	RED_ASSERT( m_itemEntity != nullptr );

	Bool finishedStreaming = m_itemEntity->IsFullyLoaded();
	if ( finishedStreaming )
	{
		CItemEntity *pItemEntityRegistry = manager->m_itemEntitiesRegistry[ m_proxy ].Get();
		if ( pItemEntityRegistry )
		{
			// This proxy has spawned entity already, that means it is a proxy reuse and we want to destroy the previous entity right now
			manager->DestroyEntityInternal( pItemEntityRegistry );
		}

		// Put the result in registry
		manager->m_itemEntitiesRegistry[ m_proxy ] = m_itemEntity;

		m_proxy->m_dirty = false;

		// Let item entity know what proxy it is bound to
		m_itemEntity->SetItemProxy( m_proxy );

		if ( m_proxy->m_collapse )
		{
			m_itemEntity->Collapse( true );
		}

		// Queue for attachment update
		manager->QueueItemEntityAttachmentUpdate( m_proxy );

		return true;
	}

	return false;
}

Bool SItemSpawnRequestInfo::ShouldRestart( const SItemSpawnInfo& itemSpawnInfo ) const
{
	// If appearance name differs
	if ( m_appearance != itemSpawnInfo.m_appearance )
	{
		return true;
	}

	// ... or source template is different
	if( !m_resource.IsValid() && !m_resourceJob )
	{
		return true;
	}
	if ( m_resource.IsValid() && m_resource->GetDepotPath() != itemSpawnInfo.m_templatePath )
	{
		return true;
	}
	if ( m_resourceJob != nullptr && m_resourceJob->GetResourceToLoad() != itemSpawnInfo.m_templatePath )
	{
		return true;
	}	

	return false;
}

//////////////////////////////////////////////////////////////////////////

CItemEntityManager::CItemEntityManager()
	: m_refreshTemplatesTimer( 0.f )
{
}

Bool CItemEntityManager::IsDoingSomething() const
{
	return !m_templatesPreloading.Empty() || !m_effectQueue.Empty() || !m_spawnEntitiesQueue.Empty() || !m_canceledSpawnRequests.Empty() || !m_attachProxiesQueue.Empty() || !m_droppedItems.Empty() || !m_latentItemActionsList.Empty();
}

void CItemEntityManager::OnTick( Float timeDelta )
{
	//m_templatesToRelease.Clear();
	if ( !m_itemEntitiesRegistry.Empty() )
	{
		ProcessEffectRequests();
		ProcessSpawnRequests();
		ProcessAttachmentRequests();
		ProcessDroppedItems( timeDelta );
		ProcessLatentActions( timeDelta );
	}

	ProcessPreloadingTemplates( timeDelta );
}

CItemEntityProxy* CItemEntityManager::CreateNewProxy( CName itemName, const SItemDefinition& itemDef, const TDynArray< CName >& enhancements, CLayer* subjectLayer, const String& templateName, Bool collapse, Bool invisible, SItemUniqueId item )
{
	RED_ASSERT( SIsMainThread() );

	CItemEntityProxy* proxy = nullptr;
	if ( subjectLayer != nullptr )
	{
		proxy = new CItemEntityProxy( itemName, subjectLayer, templateName, collapse, invisible, item );
		proxy->m_slotItems = enhancements;
		m_itemEntitiesRegistry.Insert( proxy, nullptr );
	}
	else
	{
		RED_ASSERT( subjectLayer != nullptr, TXT( "Unable to create item proxy on null layer" ) );
	}
	// Return brand new proxy
	return proxy;
}

void CItemEntityManager::QueueItemEntityAttachmentUpdate( CItemEntityProxy* itemProxy )
{
	RED_ASSERT( SIsMainThread() );

	if ( !m_itemEntitiesRegistry.KeyExist( itemProxy ) )
	{
		return;
	}

	AquireDeploymentEntityTemplate( itemProxy->m_itemName, itemProxy, true );

	if ( m_itemEntitiesRegistry[ itemProxy ].Get() == NULL )
	{
		// Proxy has no real entity spawned, queue it for spawn
		// It will be queued for attachment automatically after spawning succeeds
		itemProxy->m_dirty = true;
		QueueItemEntitySpawnRequest( itemProxy );
		return;
	}

	// If entry for this proxy already exists, the current update should overwrite the previous one
	// It is the same proxy anyway, so change is already updated
	m_attachProxiesQueue.PushBackUnique( itemProxy );
}

void CItemEntityManager::QueueItemEntitySpawnRequest( CItemEntityProxy* itemProxy )
{
	RED_ASSERT( SIsMainThread() );

	if ( !m_itemEntitiesRegistry.KeyExist( itemProxy ) )
	{
		return;
	}

	SItemSpawnInfo itemSpawnInfo;
	if ( !itemProxy->GetItemSpawnInfo( itemSpawnInfo ) )
	{
		return;
	}

	// First, let's check if there is a spawn request for the same proxy
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_spawnEntitiesQueueLock );

		TDynArray< SItemSpawnRequestInfo* >::iterator itEnd = m_spawnEntitiesQueue.End();
		for ( TDynArray< SItemSpawnRequestInfo* >::iterator it = m_spawnEntitiesQueue.Begin(); it != itEnd; ++it )
		{
			if ( ( *it )->m_proxy == itemProxy )
			{
				// If we need to restart spawn request, we move the "old one" to separate queue, so it can be properly canceled
				if ( ( *it )->ShouldRestart( itemSpawnInfo ) )
				{
					m_canceledSpawnRequests.PushBack( *it );
					m_spawnEntitiesQueue.EraseFast( it );
					ITEM_LOG( TXT("Item entity spawn request has been replaced, new item name is: %s"), itemProxy->m_itemName.AsString().AsChar() );
					break;
				}
				else
				{
					// If no need to restart, let's leave it that way
					return;
				}
			}
		}
	}

	// Push spawn request
	SItemSpawnRequestInfo* spawnInfo = new SItemSpawnRequestInfo();
	spawnInfo->m_proxy = itemProxy;
	spawnInfo->m_resource = nullptr;

	if( CName appName = GetAppearanceName( itemProxy ) )
	{
		spawnInfo->m_appearance = appName;
	}
	else if ( itemSpawnInfo.m_appearance != CName::NONE )
	{
		spawnInfo->m_appearance = itemSpawnInfo.m_appearance;
	}

	if ( !CheckForLoadedEntityTemplate( spawnInfo ) )
	{
		if ( spawnInfo->m_resourceJob == nullptr )
		{
			spawnInfo->m_resourceJob = new CJobLoadResource( itemSpawnInfo.m_templatePath );

			// Issue this job to the job manager
			SJobManager::GetInstance().Issue( spawnInfo->m_resourceJob );
		}
	}

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_spawnEntitiesQueueLock );
		m_spawnEntitiesQueue.PushBack( spawnInfo );
	}
}

THandle< CEntityTemplate > CItemEntityManager::CheckForLoadedEntityTemplate( SItemSpawnRequestInfo* spawnInfo ) const
{
	if ( !spawnInfo || !spawnInfo->m_proxy )
	{
		return nullptr;
	}

	SItemSpawnInfo itemSpawnInfo;
	if ( !spawnInfo->m_proxy->GetItemSpawnInfo( itemSpawnInfo ) )
	{
		return nullptr;
	}

	THandle< CResource > resource = GDepot->FindResource( itemSpawnInfo.m_templatePath );
	if ( !resource )
	{
		return nullptr;
	}

	THandle< CEntityTemplate > resultTemplate = Cast< CEntityTemplate >( resource );

	return resultTemplate;
}

void CItemEntityManager::ProcessSpawnRequests()
{
	PC_SCOPE_PIX( ItemsManager_ProcessSpawnRequests );

	m_unfinishedRequestsTemp.ClearFast();

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_spawnEntitiesQueueLock );

	// Iterate requests
	TSpawnRequestIterator it = m_spawnEntitiesQueue.Begin();
	while( it != m_spawnEntitiesQueue.End() )
	{
		SItemSpawnRequestInfo::EItemSpawnRequestResult res = ( *it )->Process( this );
		CItemEntityProxy* proxy = ( *it )->m_proxy;
		if ( res == SItemSpawnRequestInfo::ISRR_FailedToLoadResource )
		{
			ITEM_LOG( TXT( "Cannot load resource for item %s " ), proxy->m_itemName.AsChar() );
			m_failedToLoadProxies.PushBack( proxy );
			delete *it;
		}
		else if ( res == SItemSpawnRequestInfo::ISRR_FailedToSpawnEntity )
		{
			ITEM_LOG( TXT( "Cannot spawn item %s " ), proxy->m_itemName.AsChar() );
			m_failedToLoadProxies.PushBack( proxy );
			delete *it;
		}
		else if ( res == SItemSpawnRequestInfo::ISRR_FailedUnknown )
		{
			ITEM_LOG( TXT( "Unknown error while spawning item %s " ), proxy->m_itemName.AsChar() );
			m_failedToLoadProxies.PushBack( proxy );
			delete *it;
		}
		else if ( res == SItemSpawnRequestInfo::ISRR_Waiting )
		{
			m_unfinishedRequestsTemp.PushBack( *it );
		}
		else if ( res == SItemSpawnRequestInfo::ISRR_Succeed )
		{
			delete *it;
		}
		m_spawnEntitiesQueue.EraseFast( it );
		it = m_spawnEntitiesQueue.Begin();
	}

	// Continue processing unfinished requests in next frame
	m_spawnEntitiesQueue = m_unfinishedRequestsTemp;

	auto predicate = [this]( SItemSpawnRequestInfo* requestInfo ) 
	{
		SItemSpawnRequestInfo::EItemSpawnRequestResult res = requestInfo->ProcessCancel( this );
		if( res != SItemSpawnRequestInfo::ISRR_Waiting  )
		{
			delete requestInfo;
			return true;
		}

		return false;
	};

	m_canceledSpawnRequests.Erase( RemoveIf( m_canceledSpawnRequests.Begin(), m_canceledSpawnRequests.End(), predicate  ), m_canceledSpawnRequests.End() );
}

void CItemEntityManager::ProcessAttachmentRequests()
{
	RED_ASSERT( SIsMainThread() );

	// Perform attachment modifications
	while( m_attachProxiesQueue.Empty() == false )
	{
		Bool success = false;
		CItemEntityProxy* proxy = m_attachProxiesQueue.PopBack();
		CItemEntity* itemEntity = nullptr;

		if ( m_itemEntitiesRegistry.KeyExist( proxy ) )
		{
			itemEntity = m_itemEntitiesRegistry[ proxy ].Get();
			if ( itemEntity )
			{
				if( itemEntity->m_customAttacht )
				{
					// Reset item position (it got owner position for steaming, as we couldn't have attached it correctly while waiting for components)
					// now it will have right position
					itemEntity->SetPosition( Vector::ZERO_3D_POINT );
					itemEntity->UpdateLocalToWorld();

					if( !itemEntity->CustomAttach() )
					{
						DestroyEntityInternal( itemEntity );
					}
				}
				else
				{
					CEntity* newParentEntity = proxy->m_parentAttachmentEntity.Get();
					if ( newParentEntity && newParentEntity->IsAttached() )
					{
						// Disable item physics
						itemEntity->SwitchToKinematic( true );
						itemEntity->EnableCollisions( false );

						// Reset item position (it got owner position for steaming, as we couldn't have attached it correctly while waiting for components)
						// now it will have right position
						itemEntity->SetPosition( Vector::ZERO_3D_POINT );
						itemEntity->UpdateLocalToWorld();

						success = itemEntity->ApplyAttachmentTo( newParentEntity, proxy->m_slotName );

						if ( success )
						{
							itemEntity->OnAttachmentUpdate();
							// item has proper position now, so it can be shown
							itemEntity->SetHideInGame( proxy->m_invisible, false );

						}
						else
						{
							// Quick fix
							ITEM_LOG( TXT("Item %s entity destroyed due to failed attachment attempt"), proxy->m_itemName.AsString().AsChar() );
							DestroyEntityInternal( itemEntity );
						}
					}
					else
					{
						// It's either a detach request, or entity was destroyed, either way all parent attachments should be cut
						itemEntity->Detach();
					}
				}
			}
		}
	}
}

void CItemEntityManager::DestroyProxy( CItemEntityProxy* proxy )
{
	if ( m_itemEntitiesRegistry.KeyExist( proxy ) )
	{
		CItemEntity *pItemEntity = m_itemEntitiesRegistry[ proxy ].Get();
		if ( pItemEntity )
		{			
			if ( pItemEntity->IsAttached() )
			{
				DestroyEntityInternal( pItemEntity );
			}
		}
		m_itemEntitiesRegistry.Erase( proxy );
		m_failedToLoadProxies.Remove( proxy );

		// Remove attach request for this proxy (if found)
		m_attachProxiesQueue.Remove( proxy );
		
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_spawnEntitiesQueueLock );

			// Remove spawn request if found
			TDynArray< SItemSpawnRequestInfo* >::iterator sriIter = m_spawnEntitiesQueue.Begin();
			while ( sriIter != m_spawnEntitiesQueue.End() )
			{
				if ( ( *sriIter )->m_proxy == proxy )
				{
					m_canceledSpawnRequests.PushBack( *sriIter );
					m_spawnEntitiesQueue.EraseFast( sriIter );
					sriIter = m_spawnEntitiesQueue.Begin();
				}
				else
				{
					++sriIter;
				}
			}
		}

		// Remove effect request if found
		TDynArray< SItemEffectStartRequestInfo >::iterator effectIter = m_effectQueue.Begin();
		while ( effectIter != m_effectQueue.End() )
		{
			if ( effectIter->m_proxy == proxy )
			{
				m_effectQueue.Erase( effectIter );
				effectIter = m_effectQueue.Begin();
			}
			else
			{
				++effectIter;
			}
		}

		// Remove dropped item info
		auto predicate = [proxy]( const SDroppedItemInfo & itemInfo ) { return itemInfo.m_proxy == proxy; };
		m_droppedItems.Erase( RemoveIf( m_droppedItems.Begin(), m_droppedItems.End(), predicate ), m_droppedItems.End() );

		delete proxy;
	}
}

Bool CItemEntityManager::IsProxyRegistred( CItemEntityProxy* proxy ) const
{
	return m_itemEntitiesRegistry.KeyExist( proxy );
}

Bool CItemEntityManager::IsProxyInSpawnQue( const CItemEntityProxy* proxy ) const
{
	RED_FATAL_ASSERT( proxy, "Proxy can not be nullptr" );
	for ( const SItemSpawnRequestInfo* info : m_spawnEntitiesQueue )
	{
		if ( info->m_proxy == proxy )
		{
			return true;
		}
	}

	return false;
}

Bool CItemEntityManager::IsProxyInAttachQue( const CItemEntityProxy* proxy ) const
{
	RED_FATAL_ASSERT( proxy, "Proxy can not be nullptr" );
	return m_attachProxiesQueue.Exist( const_cast< CItemEntityProxy* >( proxy ) );
}

void CItemEntityManager::DestroyItem( CItemEntityProxy* proxy )
{
	if ( m_itemEntitiesRegistry.KeyExist( proxy ) )
	{
		CItemEntity *pItemEntity = m_itemEntitiesRegistry[ proxy ].Get();
		if ( pItemEntity )
		{
			DestroyEntityInternal( pItemEntity );
		}
	}
}

void CItemEntityManager::DestroyAll()
{
	ITEM_LOG( TXT("--------------Cleaning Item Entity Manager state--------------") );
	ITEM_LOG( TXT("Number of spawn entity requests: %i"), m_spawnEntitiesQueue.Size() );
	ITEM_LOG( TXT("Number of attach requests:\t %i"), m_attachProxiesQueue.Size() );
	ITEM_LOG( TXT("Number of dropped items:\t %i"), m_droppedItems.Size() );
	ITEM_LOG( TXT("Number of latent item actions:\t %i"), m_latentItemActionsList.Size() );
	ITEM_LOG( TXT("Number of latent queued item actions:\t %i"), m_latentQueuedItemActionsList.Size() );
	
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_spawnEntitiesQueueLock );
		m_spawnEntitiesQueue.ClearPtr();
		m_canceledSpawnRequests.ClearPtr();
	}
	
	m_attachProxiesQueue.Clear();
	m_droppedItems.Clear();
	m_effectQueue.Clear();
	m_latentItemActionsList.ClearPtr();
	m_latentQueuedItemActionsList.ClearPtr();

	// Print and destroy leftovers from items registry, if none, that's perfect
	{
		ITEM_LOG( TXT("Destroying items in registry. If none listed, it means stuff works fine.") );
		TItemEntitiesMap::iterator itemsIter = m_itemEntitiesRegistry.Begin();
		for ( ; itemsIter != m_itemEntitiesRegistry.End(); ++itemsIter )
		{
			Bool entitySpawned = false;
			CName itemName = itemsIter->m_first->m_itemName;
			String friendlyParentEntityDescription = TXT("no parent");
			if ( itemsIter->m_first->GetParentEntity() )
			{
				friendlyParentEntityDescription = itemsIter->m_first->GetParentEntity()->GetFriendlyName().AsChar();
			}

			CItemEntity *pItemEntity = itemsIter->m_second.Get();
			if ( pItemEntity )
			{		
				entitySpawned = true;
				delete itemsIter->m_first;
				DestroyEntityInternal( pItemEntity );
				itemsIter->m_second = NULL;
			}

			ITEM_LOG( TXT("-- Item %s attached to %s with item entity %s"), itemName.AsString().AsChar(), friendlyParentEntityDescription.AsChar(), entitySpawned ? TXT("spawned") : TXT("not spawned") );
		}
		m_failedToLoadProxies.Clear();
		m_itemEntitiesRegistry.Clear();
	}

	// Remove preloading jobs
	{
		// Remove old templates
		TList< STemplatePreloadInfo >::iterator it = m_templatesPreloading.Begin();
		for ( ; it != m_templatesPreloading.End(); ++it )
		{
			it->m_job->Release();
			it->m_job = NULL;
		}
		m_templatesPreloading.Clear();
	}

	// Remove preloaded templates
	{
		auto it = m_preloadedTemplates.Begin();
		for ( ; it != m_preloadedTemplates.End(); ++it )
		{
			//it->m_second.m_template->RemoveFromRootSet();
			it->m_second.m_template = NULL;
		}
		m_preloadedTemplates.Clear();
	}
}

void CItemEntityManager::PlayEffectOnEntity( CItemEntityProxy* itemProxy, CName effectName )
{
	if ( m_itemEntitiesRegistry.KeyExist( itemProxy ) )
	{
		CItemEntity* itemEntity = m_itemEntitiesRegistry[ itemProxy ].Get();
		if ( itemEntity && !itemProxy->m_dirty )
		{
			//itemEntity->HackCopyRootMeshComponentLocalToWorld();

			itemEntity->PlayEffect( effectName );
			return;
		}

		QueueItemEffectStart( itemProxy, effectName );
	}
}

void CItemEntityManager::StopEffectOnEntity( CItemEntityProxy* itemProxy, CName effectName )
{
	if ( m_itemEntitiesRegistry.KeyExist( itemProxy ) )
	{
		CItemEntity* itemEntity = m_itemEntitiesRegistry[ itemProxy ].Get();
		if ( itemEntity )
		{
			itemEntity->StopEffect( effectName );
		}
	}
}

void CItemEntityManager::PlayAnimationOnEntity( CItemEntityProxy* itemProxy, CName animationName )
{
	if ( m_itemEntitiesRegistry.KeyExist( itemProxy ) )
	{
		CItemEntity* itemEntity = m_itemEntitiesRegistry[ itemProxy ].Get();
		if ( itemEntity )
		{
			CAnimatedComponent* animComponent = itemEntity->GetRootAnimatedComponent();
			if ( animComponent )
			{
				Bool ret = animComponent->PlayAnimationOnSkeleton( animationName );
				if ( ret == false )
				{
					ITEM_LOG( TXT("Item's '%ls' play animation function returns false. Please check resource. Animation: '%ls'"), 
						itemProxy->m_itemName.AsString().AsChar(), animationName.AsString().AsChar() );
				}
			}
		}
	}
}

void CItemEntityManager::StopAnimationOnEntity( CItemEntityProxy* itemProxy )
{
	if ( m_itemEntitiesRegistry.KeyExist( itemProxy ) )
	{
		CItemEntity* itemEntity = m_itemEntitiesRegistry[ itemProxy ].Get();
		if ( itemEntity )
		{
			CAnimatedComponent* animComponent = itemEntity->GetRootAnimatedComponent();
			if ( animComponent )
			{
				animComponent->StopAllAnimationsOnSkeleton();
			}
		}
	}
}

void CItemEntityManager::RaiseBehaviorEventOnEntity( CItemEntityProxy* itemProxy, CName eventName )
{
	if ( m_itemEntitiesRegistry.KeyExist( itemProxy ) )
	{
		CItemEntity* itemEntity = m_itemEntitiesRegistry[ itemProxy ].Get();
		if ( itemEntity )
		{
			CAnimatedComponent* animComponent = itemEntity->GetRootAnimatedComponent();
			if ( animComponent )
			{
				CBehaviorGraphStack* behStack = animComponent->GetBehaviorStack();
				if ( behStack )
				{
					Bool ret = behStack->GenerateBehaviorForceEvent( eventName );
					if ( ret == false )
					{
						ITEM_LOG( TXT("Item's '%ls' raise force event function returns false. Please check resource. Active behavior: '%ls', event: '%ls'"), 
							itemProxy->m_itemName.AsString().AsChar(), behStack->GetActiveTopInstance().AsString().AsChar(), eventName.AsString().AsChar() );
					}
				}
			}
		}
	}
}

void CItemEntityManager::EnableCollisionInfoReporting( CItemEntityProxy* itemProxy )
{
	if ( !m_itemEntitiesRegistry.KeyExist( itemProxy ) )
	{
		return;
	}

	itemProxy->EnableCollisionInfoReporting();
}

CItemEntity* CItemEntityManager::GetItemEntity( CItemEntityProxy* proxy )
{
	if ( m_itemEntitiesRegistry.KeyExist( proxy ) )
	{
		return m_itemEntitiesRegistry[ proxy ].Get();
	}
	return NULL;
}

void CItemEntityManager::QueueItemEffectStart( CItemEntityProxy* itemProxy, CName effectName )
{
	RED_ASSERT( SIsMainThread() );

	SItemEffectStartRequestInfo info;
	info.m_proxy = itemProxy;
	info.m_effectName = effectName;
	m_effectQueue.PushBack( info );
}

void CItemEntityManager::ProcessEffectRequests()
{
	TDynArray< SItemEffectStartRequestInfo >::iterator requestIt = m_effectQueue.Begin();
	while ( requestIt != m_effectQueue.End() )
	{
		CItemEntity* itemEntity = GetItemEntityIfSpawned( requestIt->m_proxy );
		
		if ( itemEntity && itemEntity->IsAttached() && !requestIt->m_proxy->m_dirty )
		{
			itemEntity->PlayEffect( requestIt->m_effectName );
			m_effectQueue.EraseFast( requestIt );
			requestIt = m_effectQueue.Begin();
		}
		else
		{
			++requestIt;
		}
	}
}

Bool CItemEntityManager::EntityItemFaliedToLoad( const CItemEntityProxy* proxy ) const
{
	RED_ASSERT( m_itemEntitiesRegistry.KeyExist( const_cast< CItemEntityProxy* >( proxy ) ) );

	return m_failedToLoadProxies.Exist( const_cast< CItemEntityProxy* >( proxy ) );
}

const CItemEntity* CItemEntityManager::GetItemEntityIfSpawned( const CItemEntityProxy* proxy ) const
{
	RED_ASSERT( m_itemEntitiesRegistry.KeyExist( const_cast< CItemEntityProxy* >( proxy ) ) );

	THandle< CItemEntity > handle;
	m_itemEntitiesRegistry.Find( const_cast< CItemEntityProxy* >( proxy ), handle );
	return handle.Get();
}

CItemEntity* CItemEntityManager::GetItemEntityIfSpawned( CItemEntityProxy* proxy )
{
	return const_cast< CItemEntity* >( static_cast< const CItemEntityManager* >( this )->GetItemEntityIfSpawned( proxy ) );
}

void CItemEntityManager::RegisterItemDropped( CItemEntityProxy* proxy, Float duration /* = -1.0f */, IDroppedItemClaimer* claimer /* = nullptr */ )
{
	RED_ASSERT( SIsMainThread() );

	if ( proxy == nullptr || !m_itemEntitiesRegistry.KeyExist( proxy ) )
	{
		return;
	}
	
	SDroppedItemInfo info;
	info.m_proxy = proxy;
	info.m_claimer = claimer;
	CItemEntity* itemEntity = m_itemEntitiesRegistry[ proxy ].Get();
	if ( duration != -1.0f )
	{
		info.m_timeLeft = duration;
	}
	else if ( itemEntity != nullptr )
	{
		info.m_timeLeft = itemEntity->GetTimeToDespawn();
	}
	else
	{
		info.m_timeLeft = 10.0f;
	}
	
	// Make sure such proxy isn't dropped already
	if ( m_droppedItems.Exist( info ) )
	{
		ITEM_WARN( TXT("Registering item %s proxy as dropped while already registered"), proxy->m_itemName.AsString().AsChar() );
		return;
	}

	m_droppedItems.PushBack( info );
}

void CItemEntityManager::UnregisterItemDropped( CItemEntityProxy* proxy, Bool finalize )
{
	RED_ASSERT( SIsMainThread() );

	if ( proxy == nullptr || !m_itemEntitiesRegistry.KeyExist( proxy ) )
	{
		return;
	}

	TDynArray< SDroppedItemInfo >::iterator droppedIter = m_droppedItems.Begin();
	for ( ; droppedIter != m_droppedItems.End(); ++droppedIter )
	{
		if ( droppedIter->m_proxy == proxy )
		{
			break;
		}
	}

	if ( droppedIter == m_droppedItems.End() )
	{
		ITEM_WARN( TXT("Unregistering of item %s proxy as dropped while not registered anyway"), proxy->m_itemName.AsString().AsChar() );
		return;
	}

	Bool destroy = false;

	// Call claimer if defined, destroy item completely if not
	if ( finalize )
	{
		destroy = true;
		if ( droppedIter->m_claimer != nullptr )
		{
			destroy = !droppedIter->m_claimer->OnDroppedItemTimeOut( droppedIter->m_proxy );
		}
		if ( destroy )
		{
			// DestroyProxy also removes proxy from m_droppedItems list
			DestroyProxy( droppedIter->m_proxy );
		}
	}

	if ( !destroy )
	{
		m_droppedItems.Erase( droppedIter );
	}
}

Bool CItemEntityManager::ClaimDroppedItem( CItemEntityProxy* proxy, IDroppedItemClaimer* claimer, Bool force /* = false */ )
{
	if ( proxy == nullptr || !m_itemEntitiesRegistry.KeyExist( proxy ) )
	{
		return false;
	}

	for ( SDroppedItemInfo& droppedItem : m_droppedItems )
	{
		if ( droppedItem.m_proxy == proxy )
		{
			if ( force || ( droppedItem.m_claimer == nullptr ) )
			{
				droppedItem.m_claimer = claimer;
				return true;
			}
			return false;
		}
	}
	return false;
}

void CItemEntityManager::ProcessDroppedItems( Float timeDelta )
{
	TDynArray< SDroppedItemInfo > elementsToRemove;
	TDynArray< CItemEntityProxy* > proxiesToDestroy;
	for ( SDroppedItemInfo& droppedItem : m_droppedItems )
	{
		if ( droppedItem.m_timeLeft > 0.0f )
		{
			droppedItem.m_timeLeft -= timeDelta;
		}

		// time to remove proxy/item
		if ( droppedItem.m_timeLeft <= 0.0f )
		{
			// if there's no item or item is not visible -> we can remove proxy and inform claimers
			CItemEntityProxy* proxy = droppedItem.m_proxy;
			CItemEntity* itemEntity = m_itemEntitiesRegistry[ proxy ];
			// removing dropped items if it's not visible, or we have too many dropped items
			if ( itemEntity == nullptr || !itemEntity->IsVisible() || ( m_droppedItems.Size() > MAX_DROPPED_ITEMS + elementsToRemove.Size() ) )
			{
				elementsToRemove.PushBack( droppedItem );

				// Call claimer if defined, destroy item completely if not
				Bool destroy = true;
				if ( droppedItem.m_claimer != nullptr )
				{
					destroy = !droppedItem.m_claimer->OnDroppedItemTimeOut( proxy );
				}
				if ( destroy )
				{
					proxiesToDestroy.PushBack( proxy );
				}
			}
		}
	}

	for ( SDroppedItemInfo& elem : elementsToRemove )
	{
		m_droppedItems.Remove( elem );
	}

	// destroy proxies after elements were removed from list to avoid removing element twice
	for ( CItemEntityProxy* proxy : proxiesToDestroy )
	{
		DestroyProxy( proxy );
	}
}

void CItemEntityManager::SetProxyAttachment( CItemEntityProxy* proxy, CEntity* parentEntity, CName slotName /*= CName::NONE */, Bool force /* = false */ )
{
	if ( !m_itemEntitiesRegistry.KeyExist( proxy ) )
	{
		return;
	}

	if ( !force && proxy->m_parentAttachmentEntity == parentEntity && proxy->m_slotName == slotName )
	{
		// don't attach again to the same entity/slot unless forced to
		return;
	}

	// Save info
	proxy->m_parentAttachmentEntity = parentEntity;
	proxy->m_slotName = slotName;

	// Queue for apply
	QueueItemEntityAttachmentUpdate( proxy );
}

void CItemEntityManager::SkinProxyItem( CItemEntityProxy* proxy, CEntity* parentEntity )
{
	if ( !m_itemEntitiesRegistry.KeyExist( proxy ) )
	{
		return;
	}

	proxy->SkinOn( parentEntity );
}

void CItemEntityManager::ChangeProxyItem( CItemEntityProxy* proxy, CName itemName, CName slotOverride, const TDynArray< CName >& slotItems, const String& templateName, Bool collapse, SItemUniqueId item )
{
	if ( !m_itemEntitiesRegistry.KeyExist( proxy ) )
	{
		return;
	}

	proxy->ChangeItem( itemName, slotOverride, slotItems, templateName, collapse, item );
}

void CItemEntityManager::DropItemByProxy( CItemEntityProxy* proxy, Float duration /* = -1.0f */, IDroppedItemClaimer* claimer /*= NULL */ )
{
	if ( !m_itemEntitiesRegistry.KeyExist( proxy ) )
	{
		return;
	}

	RegisterItemDropped( proxy, duration, claimer );

	proxy->m_parentAttachmentEntity = nullptr;
	CItemEntity* itemEntity = SItemEntityManager::GetInstance().GetItemEntity( proxy );
	
	if ( itemEntity )
	{
		itemEntity->SwitchToKinematic( false );
		itemEntity->EnableCollisions( true );
		itemEntity->Drop();
	}
}

void CItemEntityManager::DestroyEntityInternal( CItemEntity* itemEntity )
{	
	RED_ASSERT( SIsMainThread() );

	CEntityTemplate* entityTemplate = itemEntity->GetEntityTemplate();
	RED_ASSERT( entityTemplate != nullptr, TXT( "ItemEntity has null template" ) );

	if( itemEntity->m_customDetacht )
	{
		itemEntity->CustomDetach();
	}
	itemEntity->Destroy();
}

void CItemEntityManager::ProcessPreloadingTemplates( Float timeDelta )
{
	m_refreshTemplatesTimer += timeDelta;

	if( m_refreshTemplatesTimer > GRefreshTemplatesTime )
	{
		m_refreshTemplatesTimer = 0.f;

		TDynArray<CName> toRemove;

		
		for( Int32 i = m_preloadedTemplates.Size() - 1; i >= 0 ; i-- )
		{
			TPair< CName, SDeploymentItemEntityTemplate >& iter = m_preloadedTemplates[i];
			if( iter.m_second.IsExpired() )
			{
				m_preloadedTemplates.RemoveAtFast( i );
			}
		}
	}

	if ( m_templatesPreloading.Empty() )
	{
		return;
	}

	TList< STemplatePreloadInfo >::iterator it = m_templatesPreloading.Begin();
	while( it != m_templatesPreloading.End() )
	{
		if ( !it->m_job->HasEnded() )
		{
			it->m_timeTaken += timeDelta;
			++it;
			continue;
		}

		CResource* loadedResource = it->m_job->GetResource();
		if ( loadedResource )
		{
			CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( loadedResource );
			if ( entityTemplate )
			{
				SDeploymentItemEntityTemplate templateInfo( entityTemplate );
				m_preloadedTemplates.PushBack( MakePair( it->m_itemName, templateInfo ) );
				if ( it->m_queueAttachment && it->m_proxy )
				{
					m_attachProxiesQueue.PushBack( it->m_proxy );
				}
			}
		}

		it->m_job->Release();
		it->m_job = NULL;
		it = m_templatesPreloading.Erase( it );
	}
}


/************************************************************************/
/* Items latent actions                                                 */
/************************************************************************/

//#define DEBUG_ITEM_LATENT_ACTIONS

Bool CItemEntityManager::RegisterItemLatentAction( CLatentItemAction* action )
{
	RED_ASSERT( SIsMainThread() );

	if ( !action->IsValid() )
	{
		ITEM_WARN( TXT("Input data for latent draw item action is invalid") );
		action->LogWarn();
		RED_ASSERT( action->IsValid(), TXT( "See log file. Input data for latent draw item action is invalid. Please check definition." ) );

		// Finish invalid action
		action->Finish();

		ProcessQueuedLatentActions();

		return false;
	}

	m_latentItemActionsList.PushBack( action );

	return true;
}

void CItemEntityManager::QueueItemLatentAction( CLatentItemQueuedAction* action )
{
	RED_ASSERT( SIsMainThread() );

	const CActor* actor = action->GetActor();

	Int32 num = 0;

	for ( Int32 i = (Int32)m_latentQueuedItemActionsList.Size()-1; i>=0; --i )
	{
		CLatentItemQueuedAction* action = m_latentQueuedItemActionsList[ i ];
		if ( action->GetActor() == actor )
		{
			delete action;
			m_latentQueuedItemActionsList.Erase( m_latentQueuedItemActionsList.Begin() + i );
			num++;
		}
	}

	m_latentQueuedItemActionsList.PushBack( action );
}

Bool CItemEntityManager::HasActorAnyLatentAction( const CActor* actor ) const
{
	for ( TLatentItemActionsList::const_iterator it = m_latentItemActionsList.Begin(); it != m_latentItemActionsList.End(); ++it )
	{
		const CLatentItemAction* action = (*it);
		if ( action->GetActor() == actor )
		{
			return true;
		}
	}

	return false;
}

Bool CItemEntityManager::HasActorLatentActionForItem( const CActor* actor, SItemUniqueId itemId ) const
{
	for ( TLatentItemActionsList::const_iterator it = m_latentItemActionsList.Begin(); it != m_latentItemActionsList.End(); ++it )
	{
		const CLatentItemAction* action = (*it);
		if ( action->GetItem() == itemId && action->GetActor() == actor )
		{
			return true;
		}
	}

	return false;
}

Bool CItemEntityManager::HasActorQueuedLatentActionForItem(const CActor* actor, SItemUniqueId itemId) const
{
	for ( auto action : m_latentQueuedItemActionsList )
	{
		if ( action->GetItem() == itemId && action->GetActor() == actor )
		{
			return true;
		}
	}

	return false;
}

Bool CItemEntityManager::HasActorCollidableLatentActionsInProgress( const CActor* actor, const SItemDefinition* itemDef ) const
{
	for ( Uint32 i=0; i<m_latentItemActionsList.Size(); ++i )
	{
		CLatentItemAction* action = m_latentItemActionsList[ i ];

		if ( action->IsCollidableWith( actor, itemDef ) )
		{
			return true;
		}
	}

	return false;
}

void CItemEntityManager::CancelLatentActionsForActor( CActor* actor, CName holdSlot )
{
	TLatentItemActionsList::iterator it = m_latentItemActionsList.Begin();
	for ( ; it != m_latentItemActionsList.End(); )
	{
		CLatentItemAction* action = *it;

		if ( action->GetActor() == actor && action->GetHoldSlot() == holdSlot )
		{
			m_latentItemActionsList.Erase( it );
			it = m_latentItemActionsList.Begin();
			action->Cancel();
		}
		else
		{
			++it;
		}
	}
}

Bool CItemEntityManager::GetActorLatentDrawActionsItems( const CActor* actor, TDynArray< SItemUniqueId >& items ) const
{
	items.Clear();
	for ( Uint32 i=0; i<m_latentItemActionsList.Size(); ++i )
	{
		CLatentItemAction* action = m_latentItemActionsList[ i ];
		if ( action->GetType() == LIAT_Draw ) // Drey TODO: Should add processing checks
		{
			items.PushBack( action->GetItem() );
		}
	}

	return !items.Empty();
}

void CItemEntityManager::ProcessLatentActions( Float timeDelta )
{
	Bool isAnyActionFinished = false;

	for ( Int32 i = m_latentItemActionsList.Size() - 1; i>=0; --i )
	{
		CLatentItemAction* action = m_latentItemActionsList[ i ];

		if ( !action->Update( timeDelta ) )
		{
			// Job is done
			action->Finish();

			m_latentItemActionsList.Erase( m_latentItemActionsList.Begin() + i );

			isAnyActionFinished = true;
		}
	}

	if ( isAnyActionFinished )
	{
		ProcessQueuedLatentActions();
	}
}

void CItemEntityManager::ProcessQueuedLatentActions()
{
	static TLatentItemQueuedActionsList toRemove;
	toRemove.ClearFast();

	for ( TLatentItemQueuedActionsList::iterator it = m_latentQueuedItemActionsList.Begin(); it != m_latentQueuedItemActionsList.End(); ++it )
	{
		CLatentItemQueuedAction* action = (*it);

		if ( action->CanBeProcessed() )
		{
			action->Process();

			toRemove.PushBack( action );
		}
	}

	for ( TLatentItemQueuedActionsList::iterator it = toRemove.Begin(); it != toRemove.End(); ++it )
	{
		CLatentItemQueuedAction* action = (*it);

		m_latentQueuedItemActionsList.Remove( action );

		delete action;
	}
}

void CItemEntityManager::OnItemAnimSyncEvent( const SAnimItemSyncEvent& syncEvent )
{
	Int32 num = 0;

	for ( TLatentItemActionsList::iterator it = m_latentItemActionsList.Begin(); it != m_latentItemActionsList.End(); ++it )
	{
		CLatentItemAction* action = (*it);

		if ( action->ShouldProcessSyncEvent( syncEvent ) )
		{
			action->ProcessSyncEvent( syncEvent );
			num++;
		}
	}
}

void CItemEntityManager::AquireDeploymentEntityTemplate( CName itemName, CItemEntityProxy* proxy, bool queueAttachment )
{
	if ( !proxy )
	{
		return;
	}
	//Red::Threads::CScopedLock< Red::Threads::CMutex > preloadLock( st_startPreloadingMutex );

	// Check if there is no existing preloading task for this item
	TList< STemplatePreloadInfo >::iterator it = m_templatesPreloading.Begin();
	for ( ; it != m_templatesPreloading.End(); ++it )
	{
		if ( it->m_itemName == itemName )
		{
			//LOG_GAME( TXT("============= %s +%i"), itemName.AsChar(), it->m_initialRefCount );
			return;
		}
	}

	{
		// Find deployment entity path
		const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( itemName );
		if ( !itemDef )
		{
			return;
		}
		const String& templatePath = GCommonGame->GetDefinitionsManager()->TranslateTemplateName( proxy->m_template );
		if ( templatePath == String::EMPTY )
		{
			return;
		}

		CEntityTemplate* itp = GetPreloadedEntityTemplate( itemName, templatePath );
		if ( itp && itp->GetFile()->GetDepotPath() == templatePath )
		{
			return;
		}

		// Try already loaded resource
		{
			CResource* res = GDepot->FindResource( templatePath );
			if ( res )
			{
				CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( res );
				if ( entityTemplate )
				{
					// Already loaded, add entry
					//entityTemplate->AddToRootSet();
					SDeploymentItemEntityTemplate templateInfo( entityTemplate );		
					m_preloadedTemplates.PushBack( MakePair( itemName, templateInfo ) );
					return;
				}
			}
		}

		// Have to create job
		{
			STemplatePreloadInfo newInfo;
			newInfo.m_itemName = itemName;
			newInfo.m_proxy = proxy;
			newInfo.m_job = new CJobLoadResource( templatePath, GImmediateDeploymentEntitiesLoadingJobs ? JP_Immediate : JP_Resources );
			newInfo.m_queueAttachment = queueAttachment;

			SJobManager::GetInstance().Issue( newInfo.m_job );
			m_templatesPreloading.PushBack( newInfo );	
		}
	}
}

CEntityTemplate* CItemEntityManager::GetPreloadedEntityTemplate( CName itemName, const String& templatePath )
{
	for( TPair< CName, SDeploymentItemEntityTemplate >& it : m_preloadedTemplates )
	{
		CEntityTemplate* templ = it.m_second.m_template.Get();
		if( itemName == it.m_first && templatePath == templ->GetFile()->GetDepotPath() )
		{
			it.m_second.RefreshLifetime();
			return templ;
		}
	}
	return NULL;
}

void CItemEntityManager::OnSerialize( IFile &file )
{
	if ( file.IsGarbageCollector() )
	{
		{
			auto it = m_preloadedTemplates.Begin();
			for ( ; it != m_preloadedTemplates.End(); ++it ) 
			{
				file << it->m_second.m_template;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

const Float CLatentItemAction::MAX_TIME_FOR_LATENT_ACTION = 10.f;

CLatentItemAction::CLatentItemAction( CActor* actor, 
									  SItemUniqueId itemId, 
									  const CName& act, 
									  const CName& deact, 
									  const CName& equipSlot, 
									  const CName& holdSlot )
	: m_timer( 0.f )
	, m_started( false )
	, m_ended( false )
	, m_actor( actor )
	, m_itemId( itemId )
	, m_activationNotification( act )
	, m_deactivationNotification( deact )
	, m_equipSlot( equipSlot )
	, m_holdSlot( holdSlot )
	, m_finished( false )
	, m_type( LIAT_Draw )
{}

Bool CLatentItemAction::Process()
{
	if ( !IsProcessed() )
	{
		return OnProcessed();
	}

	return true;
}

Bool CLatentItemAction::DoesActionStart( const CActor* actor ) const
{
	const CAnimatedComponent* root = actor->GetRootAnimatedComponent();
	if ( !root )
	{
		return false;
	}

	const CBehaviorGraphStack* stack = root->GetBehaviorStack();
	if ( !stack )
	{
		return false;
	}

	return stack->ActivationNotificationReceived( m_activationNotification );
}

Bool CLatentItemAction::DoesActionEnd( const CActor* actor ) const
{
	const CAnimatedComponent* root = actor->GetRootAnimatedComponent();
	if ( !root )
	{
		return false;
	}

	const CBehaviorGraphStack* stack = root->GetBehaviorStack();
	if ( !stack )
	{
		return false;
	}

	return stack->DeactivationNotificationReceived( m_deactivationNotification );
}

void CLatentItemAction::Cancel()
{
	RED_ASSERT( !m_finished, TXT( "Trying to Cancel already finished action" ) );
	if ( !m_finished )
	{
		delete this;
	}
}

void CLatentItemAction::Finish()
{
	m_finished = true;
	OnFinished();
	delete this;
}

Bool CLatentItemAction::Update( Float timeDelta )
{
	const CActor* actor = m_actor.Get();
	if ( !actor )
	{
		return false;
	}

	m_timer += timeDelta;

	if ( !m_started )
	{
		if ( DoesActionStart( actor ) )
		{
			m_started = true;
		}
	}

	if ( m_started && !m_ended )
	{
		if ( DoesActionEnd( actor ) )
		{
			// Instant for proper item logic
			if ( !Process() )
			{
#ifdef DEBUG_ITEM_LATENT_ACTIONS
				RED_ASSERT( false, TXT( "Result from DoLatentAction is false" ) );
#endif
			}

			m_ended = true;
			return false;
		}
	}

	if( actor->GetRootAnimatedComponent()->IsFrozen() )
	{
		if ( !Process() )
		{
#ifdef DEBUG_ITEM_LATENT_ACTIONS
			RED_ASSERT( false, TXT( "Result from DoLatentAction is false" ) );
#endif
		}

		return false;
	}

	if ( m_timer > MAX_TIME_FOR_LATENT_ACTION )
	{
		ITEM_WARN( TXT("Latent action is canceled. Timeout is occurred.") );
		LogWarn();

#ifdef DEBUG_ITEM_LATENT_ACTIONS
		RED_ASSERT( m_timer > MAX_TIME_FOR_LATENT_ACTION, TXT( "See log file. Timeout for item latent action." ) );
#endif

		// Instant for proper item logic
		if ( !Process() )
		{
#ifdef DEBUG_ITEM_LATENT_ACTIONS
			RED_ASSERT( false, TXT( "Result from DoLatentAction is false" ) );
#endif
		}

		return false;
	}

	return true;
}

Bool CLatentItemAction::IsCollidableWith( const CActor* actionOwner, const SItemDefinition* itemDef ) const
{
	return	actionOwner == m_actor.Get() && !m_finished && (
			itemDef->GetEquipSlot( actionOwner->IsA< CPlayer >() ) == m_equipSlot ||
			itemDef->GetHoldSlot( actionOwner->IsA< CPlayer >() ) == m_holdSlot );
}

const CActor* CLatentItemAction::GetActor() const
{
	return m_actor.Get();
}

SItemUniqueId CLatentItemAction::GetItem() const
{
	return m_itemId;
}

Bool CLatentItemAction::IsValid() const
{
	return	m_actor.Get() != NULL &&
			m_equipSlot != CName::NONE &&
			m_holdSlot != CName::NONE &&
			m_itemId != SItemUniqueId::INVALID &&
			m_activationNotification != CName::NONE &&
			m_deactivationNotification != CName::NONE;
}

void CLatentItemAction::LogWarn() const
{
	const CActor* actor = m_actor.Get();
	if ( !actor )
	{
		return;
	}

	CInventoryComponent* inv = actor->GetInventoryComponent();
	if ( !inv )
	{
		return;
	}

	const SInventoryItem* item = inv->GetItem( m_itemId );
	if ( !item )
	{
		return;
	}

	ITEM_WARN( TXT("   >Item '%ls' with category '%ls'"), item->GetName().AsString().AsChar(), item->GetCategory().AsString().AsChar() );
	ITEM_WARN( TXT("   >Animation for this item should have item sync event with params:") );
	ITEM_WARN( TXT("      >m_equipSlot - %s"), m_equipSlot.AsString().AsChar() );
	ITEM_WARN( TXT("      >m_holdSlot - %s"), m_holdSlot.AsString().AsChar() );
	ITEM_WARN( TXT("   >Behavior should have params:") );
	ITEM_WARN( TXT("      >activation notification - %s"), m_activationNotification.AsString().AsChar() );
	ITEM_WARN( TXT("      >deactivation notification - %s"), m_deactivationNotification.AsString().AsChar() );
}

//////////////////////////////////////////////////////////////////////////

CLatentItemActionSingleSync::CLatentItemActionSingleSync( CActor* actor, 
														 SItemUniqueId itemId, 
														 const CName& act, 
														 const CName& deact, 
														 const CName& equipSlot, 
														 const CName& holdSlot )
	 : CLatentItemAction( actor, itemId, act, deact, equipSlot, holdSlot )
	 , m_sync( false )
	 , m_processed( false )
{}

Bool CLatentItemActionSingleSync::OnProcessed()
{
	m_processed = true;
	return true;
}

Bool CLatentItemActionSingleSync::IsProcessed() const
{
	return m_processed;
}

void CLatentItemActionSingleSync::ProcessSyncEvent( const SAnimItemSyncEvent& syncEvent )
{
	// Check
	RED_ASSERT( !m_ended, TXT( "Trying to sync event that has already finished" ) );

	if ( !m_started )
	{
		ITEM_WARN( TXT("Item sync event is processed but activation notification for latent item action was not occurred.") );
		LogWarn();

#ifdef DEBUG_ITEM_LATENT_ACTIONS
		RED_ASSERT( m_started, TXT( "See log file. Item sync event is processed but activation notification for latent item action was not occurred." ) );
#endif
	}

	// If for preventing 0.0 time delta from animation
	if ( !m_sync )
	{
		m_sync = true;

		if ( !Process() )
		{
#ifdef DEBUG_ITEM_LATENT_ACTIONS
			RED_ASSERT( false, TXT( "Result from DoLatentAction is false" ) );
#endif
		}
	}	
}

//////////////////////////////////////////////////////////////////////////

CLatentItemActionDraw::CLatentItemActionDraw( CActor* actor, 
											  SItemUniqueId itemId, 
											  const CName& act, 
											  const CName& deact, 
											  const CName& equipSlot, 
											  const CName& holdSlot )
	: CLatentItemActionSingleSync( actor, itemId, act, deact, equipSlot, holdSlot )
{
	m_type = LIAT_Draw;
}

Bool CLatentItemActionDraw::OnProcessed()
{
	Bool ret = CLatentItemActionSingleSync::OnProcessed();

	if ( !m_sync )
	{
		ITEM_WARN( TXT("Item sync event is not occurred") );
		LogWarn();

#ifdef DEBUG_ITEM_LATENT_ACTIONS
		RED_ASSERT( m_sync, TXT( "Item sync event is not occurred. See log file and check resources." ) );
#endif
	}

	CActor* actor = m_actor.Get();
	if ( actor )
	{
		CInventoryComponent* inventory = actor->GetInventoryComponent();
		if ( inventory )
		{
			ret &= inventory->DrawItem( m_itemId, true, true );

#ifdef DEBUG_ITEM_LATENT_ACTIONS
			if ( !ret )
			{
				RED_ASSERT( false, TXT( "Result from DoLatentAction is false" ) );
			}
#endif
		}
	}

	return ret;
}

Bool CLatentItemActionDraw::ShouldProcessSyncEvent( const SAnimItemSyncEvent& syncEvent ) const
{
	return	syncEvent.m_actionType == ILA_Draw &&
			syncEvent.m_type == AET_Tick &&
			syncEvent.m_equipSlot == m_equipSlot &&
			syncEvent.m_holdSlot == m_holdSlot &&
			syncEvent.m_itemOwner == m_actor.Get();
}

//////////////////////////////////////////////////////////////////////////

CLatentItemActionHolster::CLatentItemActionHolster(	CActor* actor, 
													SItemUniqueId itemId, 
													const CName& act, 
													const CName& deact, 
													const CName& equipSlot, 
													const CName& holdSlot )
	: CLatentItemActionSingleSync( actor, itemId, act, deact, equipSlot, holdSlot )
{
	m_type = LIAT_Holster;
}

Bool CLatentItemActionHolster::OnProcessed()
{
	Bool ret = CLatentItemActionSingleSync::OnProcessed();

	if ( !m_sync )
	{
		ITEM_WARN( TXT("Item sync event is not occurred") );
		LogWarn();

#ifdef DEBUG_ITEM_LATENT_ACTIONS
		RED_ASSERT( m_sync, TXT( "Item sync event is not occurred. See log file and check resources." ) );
#endif
	}

	CActor* actor = m_actor.Get();
	if ( actor )
	{
		CInventoryComponent* inventory = actor->GetInventoryComponent();
		if ( inventory )
		{
			ret &= inventory->HolsterItem( m_itemId, true, false );
		}

#ifdef DEBUG_ITEM_LATENT_ACTIONS
		if ( !ret )
		{
			RED_ASSERT( false, TXT( "Result from DoLatentAction is false" ) );
		}
#endif
	}

	return ret;
}

Bool CLatentItemActionHolster::ShouldProcessSyncEvent( const SAnimItemSyncEvent& syncEvent ) const
{
	return	syncEvent.m_actionType == ILA_Holster &&
			syncEvent.m_type == AET_Tick &&
			syncEvent.m_equipSlot == m_equipSlot &&
			syncEvent.m_holdSlot == m_holdSlot &&
			syncEvent.m_itemOwner == m_actor.Get();
}

//////////////////////////////////////////////////////////////////////////

CLatentItemActionSequentialSwitch::CLatentItemActionSequentialSwitch(	CActor* actor, 
																		SItemUniqueId itemIdToHolster, 
																		SItemUniqueId itemIdToDraw, 
																		const CName& holsterAct, 
																		const CName& holsterDeact, 
																		const CName& holsterEquipSlot, 
																		const CName& holsterHoldSlot )
	: CLatentItemActionHolster( actor, itemIdToHolster, holsterAct, holsterDeact, holsterEquipSlot, holsterHoldSlot )
	, m_itemIdToDraw( itemIdToDraw )
{
	m_type = LIAT_Switch;
}

void CLatentItemActionSequentialSwitch::OnFinished()
{
	CActor* actor = m_actor.Get();
	if ( actor )
	{
		CInventoryComponent* inventory = actor->GetInventoryComponent();
		if ( inventory )
		{
			Bool ret = inventory->DrawItem( m_itemIdToDraw, false, false );

#ifdef DEBUG_ITEM_LATENT_ACTIONS
			if ( !ret )
			{
				RED_ASSERT( false, TXT( "Result from DoLatentAction is false" ) );
			}
#endif
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CLatentItemActionDoubleSync::CLatentItemActionDoubleSync( CActor* actor, 
														 SItemUniqueId itemIdToChange,
														 SItemUniqueId itemIdNew, 
														 const CName& act, 
														 const CName& deact, 
														 const CName& equipSlot, 
														 const CName& holdSlot )
	 : CLatentItemAction( actor, itemIdNew, act, deact, equipSlot, holdSlot )
	 , m_itemIdToChange( itemIdToChange )
	 , m_syncStart( false )
	 , m_syncEnd( false )
	 , m_processedFirst( false )
	 , m_processedSecond( false )
{}

Bool CLatentItemActionDoubleSync::IsProcessed() const
{
	return m_processedFirst && m_processedSecond;
}

void CLatentItemActionDoubleSync::ProcessSyncEvent( const SAnimItemSyncEvent& syncEvent )
{
	// Check
	RED_ASSERT( !m_ended, TXT( "Trying to sync event that has already finished" ) );

	if ( !m_started )
	{
		ITEM_WARN( TXT("Item sync event is processed but activation notification for latent item action was not occurred.") );
		LogWarn();

#ifdef DEBUG_ITEM_LATENT_ACTIONS
		RED_ASSERT( m_started, TXT( "See log file. Item sync event is processed but activation notification for latent item action was not occurred." ) );
#endif
	}

	if ( !m_syncStart )
	{
		if ( syncEvent.m_type != AET_DurationStart )
		{
			RED_ASSERT( syncEvent.m_type != AET_DurationStart, TXT( "Event doesn't have duration type" ) );
			return;
		}

		m_syncStart = true;

		Bool ret = ProcessActionForFirstItem();

#ifdef DEBUG_ITEM_LATENT_ACTIONS
		if ( !ret )
		{
			RED_ASSERT( False, TXT( "Result from DoLatentAction is false" ) );
		}
#endif
	}
	else if ( !m_syncEnd )
	{
		if ( syncEvent.m_type != AET_DurationEnd )
		{
			RED_ASSERT( syncEvent.m_type != AET_DurationEnd, TXT( "Event doesn't have duration type" ) );
			return;
		}

		m_syncEnd = true;

		Bool ret = ProcessActionForSecItem();

#ifdef DEBUG_ITEM_LATENT_ACTIONS
		if ( !ret )
		{
			RED_ASSERT( false, TXT( "Result from DoLatentAction is false" ) );
		}
#endif
	}
}

Bool CLatentItemActionDoubleSync::OnProcessed()
{
	Bool ret = ProcessActionForFirstItem();
	ret &= ProcessActionForSecItem();
	return ret;
}

Bool CLatentItemActionDoubleSync::ProcessActionForFirstItem()
{
	if ( !m_processedFirst )
	{
		m_processedFirst = true;
		return OnProcessActionForFirstItem();
	}
	return true;
}

Bool CLatentItemActionDoubleSync::ProcessActionForSecItem()
{
	if ( !m_processedSecond )
	{
		m_processedSecond = true;
		return OnProcessActionForSecItem();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

CLatentItemActionSmoothSwitch::CLatentItemActionSmoothSwitch(	CActor* actor, 
													SItemUniqueId itemIdToChange,
													SItemUniqueId itemIdNew, 
													const CName& act, 
													const CName& deact, 
													const CName& equipSlot, 
													const CName& holdSlot )
	: CLatentItemActionDoubleSync( actor, itemIdToChange, itemIdNew, act, deact, equipSlot, holdSlot )
{
	m_type = LIAT_Switch;
}

Bool CLatentItemActionSmoothSwitch::OnProcessActionForFirstItem()
{
	if ( !m_syncStart )
	{
		ITEM_WARN( TXT("Item sync event start is not occurred") );
		LogWarn();

#ifdef DEBUG_ITEM_LATENT_ACTIONS
		RED_ASSERT( m_syncStart, TXT( "Item sync event start is not occurred. See log file and check resources." ) );
#endif
	}

	CActor* actor = m_actor.Get();
	if ( actor )
	{
		CInventoryComponent* inventory = actor->GetInventoryComponent();
		if ( inventory )
		{
			return inventory->HolsterItem( m_itemIdToChange, true, false );
		}
	}

	return false;
}

Bool CLatentItemActionSmoothSwitch::OnProcessActionForSecItem()
{
	if ( !m_syncEnd )
	{
		ITEM_WARN( TXT("Item sync event end is not occurred") );
		LogWarn();

#ifdef DEBUG_ITEM_LATENT_ACTIONS
		ASSERT( m_syncEnd, TXT( "Item sync event end is not occurred. See log file and check resources." ) );
#endif
	}

	if ( !m_syncStart )
	{
		ITEM_WARN( TXT("Item sync event end is not occurred") );
		LogWarn();

#ifdef DEBUG_ITEM_LATENT_ACTIONS
		RED_ASSERT( m_syncEnd, TXT( "Item sync event end is not occurred. See log file and check resources." ) );
#endif
	}

	CActor* actor = m_actor.Get();
	if ( actor )
	{
		CInventoryComponent* inventory = actor->GetInventoryComponent();
		if ( inventory )
		{
			return inventory->DrawItem( m_itemId, true, true );
		}
	}

	return false;
}

Bool CLatentItemActionSmoothSwitch::ShouldProcessSyncEvent( const SAnimItemSyncEvent& syncEvent ) const
{
	return	syncEvent.m_actionType == ILA_Switch &&
			( syncEvent.m_type == AET_DurationStart || syncEvent.m_type == AET_DurationEnd ) &&
			syncEvent.m_equipSlot == m_equipSlot &&
			syncEvent.m_holdSlot == m_holdSlot &&
			syncEvent.m_itemOwner == m_actor.Get();
}

//////////////////////////////////////////////////////////////////////////

CLatentItemQueuedAction::CLatentItemQueuedAction( CActor* actor, SItemUniqueId itemId, const SItemDefinition* itemDef )
	: m_actor( actor )
	, m_itemId( itemId )
	, m_itemDef( itemDef )
{

}

const CActor* CLatentItemQueuedAction::GetActor() const
{
	return m_actor.Get();
}

Bool CLatentItemQueuedAction::CanBeProcessed() const
{
	CActor* actor = m_actor.Get();
	return actor ? !SItemEntityManager::GetInstance().HasActorCollidableLatentActionsInProgress( actor, m_itemDef ) : true;
}

CLatentItemQueuedActionDraw::CLatentItemQueuedActionDraw( CActor* actor, SItemUniqueId itemId, const SItemDefinition* itemDef )
	: CLatentItemQueuedAction( actor, itemId, itemDef )
{

}

Bool CLatentItemQueuedActionDraw::Process()
{
	CActor* actor = m_actor.Get();
	if ( actor )
	{
		CInventoryComponent* inv = actor->GetInventoryComponent();
		if ( inv )
		{
			return inv->DrawItem( m_itemId, false );
		}
	}
	return false;
}

CLatentItemQueuedActionHolster::CLatentItemQueuedActionHolster( CActor* actor, SItemUniqueId itemId, const SItemDefinition* itemDef )
	: CLatentItemQueuedAction( actor, itemId, itemDef )
{

}

Bool CLatentItemQueuedActionHolster::Process()
{
	CActor* actor = m_actor.Get();
	if ( actor )
	{
		CInventoryComponent* inv = actor->GetInventoryComponent();
		if ( inv )
		{
			return inv->HolsterItem( m_itemId, false );
		}
	}
	return false;
}

Bool CItemEntityManager::SDeploymentItemEntityTemplate::IsExpired()
{
	return Float( GEngine->GetRawEngineTime().operator Double() - m_lastUseTime ) > GPreloadedTemplatesExpireTime;
}

void CItemEntityManager::SDeploymentItemEntityTemplate::RefreshLifetime()
{
	m_lastUseTime = GEngine->GetRawEngineTime();
}

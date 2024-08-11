#include "build.h"

#include "r6InventoryComponent.h"
#include "r6InventoryItemComponent.h"
#include "../../common/engine/physicsBodyWrapper.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/tickManager.h"
#include "../../common/engine/rigidMeshComponent.h"

IMPLEMENT_ENGINE_CLASS( CR6InventoryComponent );

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CR6InventoryComponent::CR6InventoryComponent()
{
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::AddItem( CR6InventoryItemComponent* _component )
{
	if( !_component )
		return;

	// remove item from world
	if( !RemoveItemFromWorld( _component ) )
		return;

	// add to inventory
	m_inventoryItems.PushBackUnique( THandle<CR6InventoryItemComponent>( _component ) );
	_component->m_inventory = this;

	//debug log
	RED_LOG( RED_LOG_CHANNEL( InventoryDebug ), TXT( "%s: Added %s to inventory." ), GetEntity()->GetName().AsChar(), _component->GetEntity()->GetName().AsChar() );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CR6InventoryComponent::RemoveItem( CR6InventoryItemComponent* _component )
{
	if( !_component )
		return false;

	// find item to drop
	for( Uint32 i=0; i<m_inventoryItems.Size(); ++i )
	{
		if( m_inventoryItems[ i ].Get() != _component )
			continue;

		m_inventoryItems.RemoveAt( i );
		_component->m_inventory = NULL;

		RED_LOG( RED_LOG_CHANNEL( InventoryDebug ), TXT( "Removed item %s from inventory." ), _component->GetEntity()->GetName().AsChar() );
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CR6InventoryComponent::DropItem( CR6InventoryItemComponent* _component )
{
	// get item entity & inventory owner
	CEntity* entity = GetEntity();
	if( !entity )
		return false;

	// set new item position
	Vector newPos = entity->GetWorldPosition() + entity->GetWorldForward() * 0.5f + entity->GetWorldUp();
	EulerAngles newRot = entity->GetWorldRotation();
	Vector newVelocity = entity->GetWorldForward() * 2.0f;

	return DropItem( _component, newPos, newRot, newVelocity );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CR6InventoryComponent::DropItem( CR6InventoryItemComponent* _component, Vector3 _position, EulerAngles _rotation, Vector3 _velocity )
{
	// get item entity & inventory owner
	CEntity* entity = _component->GetEntity();
	CEntity* owner = GetEntity();
	if( !entity || !owner )
		return false;

	// remove item from inventory
	if( !RemoveItem( _component ) )
		return false;

	// set new item position
	entity->SetPosition( _position );
	entity->SetRotation( _rotation );
	entity->ForceUpdateTransform();

	// add item to world
	if( !AddItemToWorld( _component ) )
		return false;

	// activate rigid body?
	ComponentIterator< CRigidMeshComponent > it( entity );
	CRigidMeshComponent* rigidBody = ( it ? *it : NULL ); 
	if( rigidBody )
	{
		// drop and let physics take over
		CPhysicsWrapperInterface* wrapper = rigidBody->GetPhysicsRigidBodyWrapper();	
		if( wrapper )
		{			
			wrapper->SwitchToKinematic( false );
			wrapper->SetVelocityLinear( _velocity );
		}		
	}
	else
	{
		// ...otherwise place by feet
		entity->SetPosition( owner->GetWorldPosition() + owner->GetWorldForward() * 0.4f );
	}

	// debug log
	RED_LOG( RED_LOG_CHANNEL( InventoryDebug ), TXT( "Dropped item %s to world." ), entity->GetName().AsChar() );
	return true;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CR6InventoryComponent::DestroyItem( CR6InventoryItemComponent* _component )
{
	// get item entity
	CEntity* entity = _component->GetEntity();
	if( !entity )
		return false;

	if( !RemoveItem( _component ) )
		return false;

	// kill off entity
	//entity->Destroy();	// This wants to check out the level for some reason...
	return true;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::RemoveAllItems()
{
	while( m_inventoryItems.Size() > 0 )
	{
		CR6InventoryItemComponent* item = m_inventoryItems[ 0 ].Get();
		if( item )
		{
			RemoveItem( item );
		}
		else
		{
			// This should never happen...
			m_inventoryItems.RemoveAt( 0 );
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CR6InventoryComponent::HasItem( CName _itemName, Bool _recursive )
{
	return GetItemByName( _itemName, _recursive ) != NULL;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CR6InventoryItemComponent* CR6InventoryComponent::GetItemByName( CName _itemName, Bool _recursive )
{
	for( Uint32 i=0; i<m_inventoryItems.Size(); ++i )
	{
		CR6InventoryItemComponent* item = m_inventoryItems[ i ].Get();
		if( item && item->GetItemName() == _itemName )
		{
			return item;
		}

		// check inventories of items as well?
		if( _recursive )
		{
			ComponentIterator<CR6InventoryComponent> it( item->GetEntity() );
			for( ; it; ++it )
			{
				CR6InventoryItemComponent* foundItem = (*it)->GetItemByName( _itemName, _recursive );
				if( foundItem )
					return foundItem;
			}
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::GetItemsByCategory( CName _categoryName, TDynArray< CR6InventoryItemComponent* >& _result, Bool _recursive )
{
	for( Uint32 i=0; i<m_inventoryItems.Size(); ++i )
	{
		CR6InventoryItemComponent* item = m_inventoryItems[ i ].Get();
		if( item && item->GetItemCategory() == _categoryName )
		{
			_result.PushBack( item );
		}

		// check inventories of items as well?
		if( _recursive )
		{
			ComponentIterator<CR6InventoryComponent> it( item->GetEntity() );
			for( ; it; ++it )
			{
				(*it)->GetItemsByCategory( _categoryName, _result, _recursive );
			}
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Uint32 CR6InventoryComponent::GetItemQuantity( CName _itemName, Bool _recursive )
{
	Uint32 itemCount = 0;

	for( Uint32 i=0; i<m_inventoryItems.Size(); ++i )
	{
		CR6InventoryItemComponent* item = m_inventoryItems[ i ].Get();
		if( item && item->GetItemName() == _itemName )
		{
			itemCount += item->GetQuantity();
		}

		// check inventories of items as well?
		if( _recursive )
		{
			ComponentIterator<CR6InventoryComponent> it( item->GetEntity() );
			for( ; it; ++it )
			{
				itemCount += (*it)->GetItemQuantity( _itemName, _recursive );
			}
		}
	}

	return itemCount;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CR6InventoryComponent::RemoveItemFromWorld( CR6InventoryItemComponent* _component )
{
	if( !_component )
		return false;

	CEntity* entity = _component->GetEntity();
	if( !entity )
		return false;

	// remove from world (if it's attached)
	if( entity->IsAttached() )
	{
		entity->DetachFromWorld( entity->GetLayer()->GetWorld() );		
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CR6InventoryComponent::AddItemToWorld( CR6InventoryItemComponent* _component, Bool _disableRigidbody )
{
	CEntity* entity = _component->GetEntity();
	if( !entity )
		return false;

	// add item to dynamic layer
	CLayer* dynamicLayer = GGame->GetActiveWorld()->GetDynamicLayer();

	if( !dynamicLayer->FindEntity( entity->GetGUID() ) )
	{
		// create a new unique name
		entity->SetName( dynamicLayer->GenerateUniqueEntityName( entity->GetName() ) );

		// add entity to world
		dynamicLayer->AddEntity( entity );
	}
	else
	{
		// re-attach entity
		entity->AttachToWorld( GGame->GetActiveWorld() );
	}

	// deactivate rigid body?
	if( _disableRigidbody )
	{
		ComponentIterator< CRigidMeshComponent > it( entity );
		CRigidMeshComponent* rigidBody = ( it ? *it : NULL ); 
		if( rigidBody )
		{
			CPhysicsWrapperInterface* wrapper = rigidBody->GetPhysicsRigidBodyWrapper();	
			if( wrapper )
			{			
				wrapper->SwitchToKinematic( true );
			}		
		}
	}
	
	return true;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetTickManager()->AddToGroup( this, TICK_Main );

	if( !GGame->IsActive() )
		return;

	// spawn starting items			(TODO: might be best not to spawn these at this time for containers and such)
	for( Uint32 i=0; i<m_startingItems.Size(); ++i )
	{
		// get template
		CEntityTemplate* entityTemplate = m_startingItems[ i ].Get();
		if( !entityTemplate )
		{					
			RED_LOG( RED_LOG_CHANNEL( InventoryDebug ), TXT( "Inventory starting item list contained a NULL item at position: %i" ), i );
			continue;
		}

		// spawn info
		EntitySpawnInfo einfo;
		einfo.m_template = entityTemplate;

		// spawn entity
		CEntity* newEntity = GGame->GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( einfo );

		// grab inventory item component
		CR6InventoryItemComponent* inventoryItem = newEntity->FindComponent<CR6InventoryItemComponent>();
		if( inventoryItem )
		{
			// add item to inventory
			AddItem( inventoryItem );
		}
		else
		{
			// this template must be borken (since an item must contain an inventoryItem component)
			RED_LOG( RED_LOG_CHANNEL( InventoryDebug ), TXT( "Failed to create starting item: %s (since it does not contain a CR6InventoryItemComponent)" ), newEntity->GetName().AsChar() );

			// delete entity
			newEntity->Destroy();
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	world->GetTickManager()->RemoveFromGroup( this, TICK_Main );	
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::OnTick( Float timeDelta )
{
	// TODO: maybe later we will tick certain items while in the inventory?
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcAddItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( THandle<CR6InventoryItemComponent>, _item, THandle<CR6InventoryItemComponent>() );
	FINISH_PARAMETERS;
	AddItem( _item.Get() );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcRemoveItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( THandle<CR6InventoryItemComponent>, _item, THandle<CR6InventoryItemComponent>() );
	FINISH_PARAMETERS;
	RETURN_BOOL( RemoveItem( _item.Get() ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcDropItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( THandle<CR6InventoryItemComponent>, _item, THandle<CR6InventoryItemComponent>() );
	FINISH_PARAMETERS;
	RETURN_BOOL( DropItem( _item.Get() ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcDropItemToLocation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( THandle<CR6InventoryItemComponent>, _item, THandle<CR6InventoryItemComponent>() );
	GET_PARAMETER( Vector, _position, Vector::ZERO_3D_POINT );
	GET_PARAMETER( EulerAngles, _rotation, EulerAngles::ZEROS );
	FINISH_PARAMETERS;
	RETURN_BOOL( DropItem( _item.Get(), _position, _rotation, Vector::ZERO_3D_POINT ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcDestroyItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( THandle<CR6InventoryItemComponent>, _item, THandle<CR6InventoryItemComponent>() );
	FINISH_PARAMETERS;
	RETURN_BOOL( DestroyItem( _item.Get() ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcRemoveAllItems( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RemoveAllItems();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcHasItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, _itemName, CName::NONE );
	GET_PARAMETER_OPT( Bool, _recursive, false );
	FINISH_PARAMETERS;
	RETURN_BOOL( HasItem( _itemName, _recursive ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcGetItemByName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, _itemName, CName::NONE );
	GET_PARAMETER_OPT( Bool, _recursive, false );
	FINISH_PARAMETERS;
	CR6InventoryItemComponent* item = GetItemByName( _itemName, _recursive );
	RETURN_HANDLE( CR6InventoryItemComponent, item );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcGetItemsByCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, _categoryName, CName::NONE );
	GET_PARAMETER_OPT( Bool, _recursive, false );
	FINISH_PARAMETERS;

	if( result )
	{
		TDynArray< THandle<CR6InventoryItemComponent> > & resultArr = *(TDynArray< THandle<CR6InventoryItemComponent> >*) result;

		// get items by category
		TDynArray<CR6InventoryItemComponent*> items;
		GetItemsByCategory( _categoryName, items, _recursive );

		// return as list of handles
		for( Uint32 i=0; i<items.Size(); ++i )
		{
			resultArr.PushBack( THandle< CR6InventoryItemComponent >( items[ i ] ) );
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcGetItemQuantity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, _itemName, CName::NONE );
	GET_PARAMETER_OPT( Bool, _recursive, false );
	FINISH_PARAMETERS;
	RETURN_INT( GetItemQuantity( _itemName, _recursive ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcRemoveItemFromWorld( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( THandle<CR6InventoryItemComponent>, _item, THandle<CR6InventoryItemComponent>() );
	FINISH_PARAMETERS;
	RETURN_BOOL( RemoveItemFromWorld( _item.Get() ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryComponent::funcAddItemToWorld( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( THandle<CR6InventoryItemComponent>, _item, THandle<CR6InventoryItemComponent>() );
	GET_PARAMETER( Bool, _disableRigidbody, false );
	FINISH_PARAMETERS;
	RETURN_BOOL( AddItemToWorld( _item.Get(), _disableRigidbody ) );
}
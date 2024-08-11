#include "build.h"
#include "bgNpcItem.h"
#include "../../common/core/depot.h"
#include "../engine/meshSkinningAttachment.h"

IMPLEMENT_RTTI_ENUM( EItemState );
IMPLEMENT_ENGINE_CLASS( CBgNpcItemComponent );

CBgNpcItemComponent::CBgNpcItemComponent()
	: m_defaultState( IS_MOUNT )
	, m_state( IS_INVALID )
{

}

void CBgNpcItemComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CBgNpcItemComponent_OnAttached );

	SetItemState( m_defaultState );
}

void CBgNpcItemComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
}

const CName& CBgNpcItemComponent::GetItemCategory() const
{
	return m_itemCategory;
}

const CName& CBgNpcItemComponent::GetItemName() const
{
	return m_itemName;
}

void CBgNpcItemComponent::SetItemState( EItemState state )
{
	if ( TryGetMesh() )
	{
		switch ( state )
		{
		case IS_MOUNT:
			if ( m_state != IS_MOUNT ) HoldItem();
			break;
		case IS_HOLD:
			if ( m_state != IS_HOLD ) EquipItem();
			break;
		case IS_HIDDEN:
			if ( m_state != IS_HIDDEN ) HideItem();
			break;
		default:
			ASSERT( 0 );
		}

		m_state = state;
	}
}

void CBgNpcItemComponent::EquipItem()
{
	CheckAttachment( m_equipSlot );

	if ( !IsVisible() )
	{
		SetVisible( true );
	}
}

void CBgNpcItemComponent::HoldItem()
{
	CheckAttachment( m_holdSlot );

	if ( !IsVisible() )
	{
		SetVisible( true );
	}
}

void CBgNpcItemComponent::HideItem()
{
	if ( IsVisible() )
	{
		SetVisible( false );
	}
}

void CBgNpcItemComponent::CheckAttachment( const CName& slotName )
{
	CHardAttachment* att = GetTransformParent();
	if ( att && att->IsExactlyA< CHardAttachment >() )
	{
		AttachComponentToSlot( slotName, this );
	}
}

void CBgNpcItemComponent::AttachComponentToSlot( const CName& slotName, CComponent* component )
{
	HardAttachmentSpawnInfo si;
	si.m_parentSlotName = slotName;

	CEntity* parent = GetEntity();
	CAnimatedComponent* root = parent->GetRootAnimatedComponent();

	if ( root )
	{
		component->BreakAllAttachments();

		if ( !root->Attach( component, si ) && parent->GetEntityTemplate() )
		{
			const EntitySlot* entitySlot = parent->GetEntityTemplate()->FindSlotByName( si.m_parentSlotName, true );
			if ( entitySlot )
			{	
				HardAttachmentSpawnInfo si2;

				si2.m_parentSlotName = entitySlot->GetBoneName();
				si2.m_relativePosition = entitySlot->GetTransform().GetPosition();
				si2.m_relativeRotation = entitySlot->GetTransform().GetRotation();

				root->Attach( component, si2 );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR

void CBgNpcItemComponent::SetItemDefaultState( EItemState state )
{
	m_defaultState = state;

	SetItemState( m_defaultState );
}

void CBgNpcItemComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("itemName") )
	{
		SetItemName( m_itemName );
	}
	else if ( property->GetName() == TXT("defaultState") )
	{
		SetItemDefaultState( m_defaultState );
	}
}

void CBgNpcItemComponent::SetItemName( const CName& itemName )
{
	m_itemName = itemName;
}

CMeshComponent* CBgNpcItemComponent::GetMeshFromItem( const String& templatePath ) const
{
	ResourceLoadingContext context;
	CEntityTemplate* entityWithItems = LoadResource< CEntityTemplate >( templatePath, context );
	if ( entityWithItems )
	{
		const CEntity* ent = entityWithItems->GetEntityObject();
		if ( ent )
		{
			return ent->FindComponent< CMeshComponent >();
		}
	}

	return NULL;
}

#endif

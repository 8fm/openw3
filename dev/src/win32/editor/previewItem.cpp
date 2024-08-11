/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "previewItem.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/bitmapTexture.h"


CGatheredResource resPreviewHelperIcon( TXT("engine\\textures\\icons\\vertexicon.xbm"), RGF_Startup );
CGatheredResource resPreviewHelperIconSmall( TXT("engine\\textures\\icons\\sloticon.xbm"), RGF_Startup );

//////////////////////////////////////////////////////////////////////////

void IEntityPreviewItem::Init( const String& name )
{
	ASSERT( GetItemContainer() && GetItemContainer() );

	// Create entity
	CWorld* world = GetItemContainer()->GetPreviewItemWorld();
	if ( world )
	{
		EntitySpawnInfo sinfo;
		sinfo.m_name = String::Printf( TXT("%s_%d"), name.AsChar(), (Int32)this );
		m_entity = world->GetDynamicLayer()->CreateEntitySync( sinfo );
		if ( m_entity )
		{
			m_entity->AddToRootSet();
			// Create new component
			m_component = Cast< CPreviewHelperComponent >( m_entity->CreateComponent( ClassID< CPreviewHelperComponent >(), SComponentSpawnInfo() ) );
			ASSERT( m_component );

			m_component->SetContainer( GetItemContainer() );
			m_component->SetName( name );
			m_component->SetItem( this );
		}
	}
}

void IEntityPreviewItem::Destroy()
{
	// Destroy component
	if ( m_entity )
	{
		m_entity->DestroyAllComponents();
		m_entity->RemoveFromRootSet();
		m_entity->Destroy();
	}
}

CEntity* IEntityPreviewItem::GetEntity() const
{
	return m_entity;
}


//////////////////////////////////////////////////////////////////////////


void IPreviewItem::Init( const String& name )
{
	ASSERT( GetItemContainer() && GetItemContainer()->GetItemEntityContainer() );

	// Create new component
	m_component = Cast< CPreviewHelperComponent >( GetItemContainer()->GetItemEntityContainer()->CreateComponent( ClassID< CPreviewHelperComponent >(), SComponentSpawnInfo() ) );
	ASSERT( m_component );
	m_component->SetContainer( GetItemContainer() );
	m_component->SetName( name );
	m_component->SetItem( this );
}

void IPreviewItem::Destroy()
{
	// Destroy component
	if ( GetItemContainer() && GetItemContainer()->GetItemEntityContainer() )
	{
		GetItemContainer()->GetItemEntityContainer()->DestroyComponent( m_component );
	}
}

void IPreviewItem::SetPosition( const Vector& newPos )
{
	m_component->SetPosition( newPos );
	m_component->ForceUpdateTransformNodeAndCommitChanges();
}

void IPreviewItem::SetRotation( const EulerAngles& newRot )
{
	m_component->SetRotation( newRot );
	m_component->ForceUpdateTransformNodeAndCommitChanges();
}

void IPreviewItem::InternalEditor_SetPosition( const Vector& newPos )
{
	m_component->CComponent::SetPosition( newPos );
	m_component->ForceUpdateTransformNodeAndCommitChanges();
}

void IPreviewItem::InternalEditor_SetRotation( const EulerAngles& newRot )
{
	m_component->CComponent::SetRotation( newRot );
	m_component->ForceUpdateTransformNodeAndCommitChanges();
}

void IPreviewItem::RefreshTransform( const Vector& newPos, const EulerAngles& newRot )
{
	m_component->RefreshTransform( newPos, newRot );
	m_component->ForceUpdateTransformNodeAndCommitChanges();
}

void IPreviewItem::RefreshTransform( const EngineTransform& newTransform )
{
	m_component->RefreshTransform( newTransform );
	m_component->ForceUpdateTransformNodeAndCommitChanges();
}


void IPreviewItem::SetSize( PreviewSize size )
{
	m_component->SetSize( size );
}

void IPreviewItem::SetColor( const Color& color )
{
	m_component->SetColor( color );
}

const String& IPreviewItem::GetName() const
{
	return m_component ? m_component->GetName() : String::EMPTY;
}

void IPreviewItem::SetVisible( Bool flag )
{
	m_component->SetVisible( flag );
}

const CPreviewHelperComponent* IPreviewItem::GetComponent() const
{
	return m_component;
}

void IPreviewItem::Select()
{
	if ( m_component->GetWorld() && m_component->GetWorld()->GetSelectionManager() )
	{
		m_component->GetWorld()->GetSelectionManager()->Select( m_component );
	}
}


//////////////////////////////////////////////////////////////////////////


void IPreviewItemContainer::InitItemContainer()
{
	ASSERT( !m_itemContainer );

	// Create entity
	CWorld* world = GetPreviewItemWorld();
	if ( world )
	{
		EntitySpawnInfo sinfo;
		sinfo.m_name = String::Printf( TXT("IPreviewItemContainer_%d"), (Int32)this );
		m_itemContainer = world->GetDynamicLayer()->CreateEntitySync( sinfo );

		if ( m_itemContainer )
		{
			m_itemContainer->AddToRootSet();
		}
	}
}

void IPreviewItemContainer::DestroyItemContainer()
{
	// Destroy items
	ClearItems();

	// Destroy entity
	if ( m_itemContainer )
	{
		m_itemContainer->RemoveFromRootSet();
		m_itemContainer->Destroy();
		m_itemContainer = NULL;
	}
}

void IPreviewItemContainer::RefreshItems()
{
	for ( Int32 i=(Int32)m_items.Size()-1; i>=0; --i )
	{
		IPreviewItem* item = m_items[i];
		if ( !m_items[i]->IsValid() )
		{
			m_items[i]->Destroy();
			m_items.Erase( m_items.Begin() + i );
		}
		else
		{
			m_items[i]->Refresh();
		}
	}
}

void IPreviewItemContainer::SetItemPosition( const String& itemName, const Vector& newPos, const EulerAngles& newRot )
{
	for ( Int32 i=(Int32)m_items.Size()-1; i>=0; --i )
	{
		if ( m_items[i]->GetName() == itemName )
		{
			m_items[i]->SetPosition( newPos );
			m_items[i]->SetRotation( newRot );
		}
	}
}

Bool IPreviewItemContainer::HasItem( const String& itemName ) const
{
	for ( Uint32 i=0; i<m_items.Size(); ++i )
	{
		if ( m_items[i]->GetName() == itemName )
		{
			return true;
		}
	}

	return false;
}

Bool IPreviewItemContainer::IsEmpty() const
{
	return m_items.Empty();
}

void IPreviewItemContainer::AddItem( IPreviewItem* item )
{
	m_items.PushBack( item );
}

void IPreviewItemContainer::ClearItem( IPreviewItem* item )
{
	m_items.RemoveFast( item );
	item->Destroy();
}

void IPreviewItemContainer::ClearItems() 
{ 
	for ( Uint32 i=0; i<m_items.Size(); ++i )
	{
		m_items[i]->Destroy();
	}
	m_items.Clear();
}

CEntity* IPreviewItemContainer::GetItemEntityContainer() const 
{ 
	return m_itemContainer; 
}

Bool IPreviewItemContainer::HandleItemSelection( const TDynArray< CHitProxyObject* >& objects )
{
	Bool ret = false;

	CWorld* world = GetPreviewItemWorld();
	if ( world )
	{
		// Toggle selection
		CSelectionManager* selectionMgr = world->GetSelectionManager();
		CSelectionManager::CSelectionTransaction transaction( *selectionMgr );

		if ( !selectionMgr )
		{
			return false;
		}

		if ( objects.Size() )
		{
			const CPreviewHelperComponent* component = Cast< CPreviewHelperComponent >( objects[0]->GetHitObject() );
			if ( component && component->GetContainer() == this )
			{
				// Deselect all selected object
				if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
					selectionMgr->DeselectAll();


				// Toggle selection
				for ( Uint32 i=0; i<objects.Size(); i++ )
				{
					CPreviewHelperComponent* tc = Cast< CPreviewHelperComponent >( objects[i]->GetHitObject() );
					if ( !tc || !tc->HasManualControl() )
					{
						continue;
					}

					ret = true;

					if ( tc->IsSelected() && !IsSelectionBoxDragging() )
					{
						selectionMgr->Deselect( tc );
					}
					else if ( !tc->IsSelected() )
					{
						selectionMgr->Select( tc );
						OnSelectItem( tc->GetItem() );
					}
				}
			}

			if ( objects.Size() == 0 )
			{
				OnDeselectAllItem();
			}
		}
	}

	return ret;
}


//////////////////////////////////////////////////////////////////////////


IMPLEMENT_ENGINE_CLASS( CPreviewHelperComponent );
 
CPreviewHelperComponent::CPreviewHelperComponent()
	: m_manualControl( true )
	, m_size( IPreviewItem::PS_Normal )
	, m_overlay( true )
	, m_drawArrows( false )
{	
}


void CPreviewHelperComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Gizmo );
}

void CPreviewHelperComponent::OnDetached( CWorld* world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Gizmo );

	TBaseClass::OnDetached( world );
}


Color CPreviewHelperComponent::RandColor() const
{
	static Uint32 color = 1;
	color = (color + 1) % 7;	
	if ( !color ) color = 1;

	return Color( color & 1 ? 255 : 0, 
		color & 2 ? 255 : 0,
		color & 4 ? 255 : 0 );
}

void CPreviewHelperComponent::SetItem( IPreviewItem* item )
{
	m_item = item;
	m_color = RandColor();
}

IPreviewItem* CPreviewHelperComponent::GetItem() const
{
	return m_item;
}

void CPreviewHelperComponent::SetColor( const Color& color )
{
	m_color = color;
}

void CPreviewHelperComponent::SetSize( IPreviewItem::PreviewSize size )
{
	m_size = size;
}

Float CPreviewHelperComponent::CalcSpriteSize() const
{
	switch ( m_size )
	{
	case IPreviewItem::PS_Normal:
		return 0.25f;
	case IPreviewItem::PS_Small:
		return 0.1f;
	case IPreviewItem::PS_Tiny:
		return 0.04f;
	default:
		return 0.25;
	}
}

void CPreviewHelperComponent::SetManualControl( Bool flag )
{
	m_manualControl = flag;
}

Bool CPreviewHelperComponent::HasManualControl() const
{
	return m_manualControl;
}

void CPreviewHelperComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if ( IsVisible() )
	{
		if ( m_drawArrows )
		{
			// Shows the actual rotation of the particle helper
			frame->AddDebug3DArrow( GetPosition(), GetLocalToWorld().GetAxisZ(), 0.3f, 0.003f, 0.009f, 0.025f, Color::DARK_BLUE, Color::DARK_BLUE );
			frame->AddDebug3DArrow( GetPosition(), GetLocalToWorld().GetAxisY(), 0.3f, 0.003f, 0.009f, 0.025f, Color::DARK_GREEN, Color::DARK_GREEN );
			frame->AddDebug3DArrow( GetPosition(), GetLocalToWorld().GetAxisX(), 0.3f, 0.003f, 0.009f, 0.025f, Color::DARK_RED, Color::DARK_RED );
		}
	
		if ( flags == SHOW_Sprites )
		{
			if ( m_item && m_item->HasCaption() )
			{
				// Text
				frame->AddDebugText( GetWorldPosition() + Vector(0.f, 0.f, 0.1f) , m_name, false, CalcSpriteColor() );
			}
		}
		else if ( flags == SHOW_Gizmo )
		{
			if ( m_item && IsSelected() )
			{
				m_item->DrawGizmo( frame );
			}
		}
	}
}

Color CPreviewHelperComponent::CalcSpriteColor() const
{
	if ( IsSelected() )
	{
		return Color::WHITE;
	}
	else
	{
		return m_color;
	}
}

CBitmapTexture* CPreviewHelperComponent::GetSpriteIcon() const
{
	return m_size == IPreviewItem::PS_Small ? resPreviewHelperIconSmall.LoadAndGet< CBitmapTexture >() : resPreviewHelperIcon.LoadAndGet< CBitmapTexture >();
}

Bool CPreviewHelperComponent::IsOverlay() const
{
	return m_overlay;
}

void CPreviewHelperComponent::SetPosition( const Vector& position )
{
	Vector prevPos = GetPosition();
	Vector pos = position;

	if ( m_item ) m_item->SetPositionFromPreview( prevPos, pos );

	TBaseClass::SetPosition( pos );
}

void CPreviewHelperComponent::SetRotation( const EulerAngles& rotation )
{
	EulerAngles prevRot = GetRotation();
	EulerAngles rot = rotation;

	if ( m_item ) m_item->SetRotationFromPreview( prevRot, rot );

	TBaseClass::SetRotation( rot );
}
void CPreviewHelperComponent::SetScale( const Vector& scale )
{
	Vector prevScale = GetScale();
	Vector sc = scale;

	if ( m_item ) m_item->SetScaleFromPreview( prevScale, sc );

	TBaseClass::SetScale( sc );
}

void CPreviewHelperComponent::RefreshTransform( const Vector& position, const EulerAngles& rotation )
{
	TBaseClass::SetPosition( position );
	TBaseClass::SetRotation( rotation );
}

void CPreviewHelperComponent::RefreshTransform( const EngineTransform& newTransform )
{
	/*
	// Simplified from this, except fewer checks and stuff. Plus Set* and Get* can't be inlined.
	TBaseClass::SetPosition( newTransform.GetPosition() );
	TBaseClass::SetRotation( newTransform.GetRotation() );
	TBaseClass::SetScale( newTransform.GetScale() );
	*/
	m_transform = newTransform;
	ScheduleUpdateTransformNode();
}

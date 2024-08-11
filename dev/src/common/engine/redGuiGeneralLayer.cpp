/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiGeneralLayer.h"
#include "redGuiManager.h"

namespace RedGui
{
	CRedGuiGeneralLayer::CRedGuiGeneralLayer( const String& name, Bool pick/* = true*/ )
		: m_name( name )
		, m_isPick( pick )
		, m_viewSize( 0.0f, 0.0f )
	{
		// GetRenderManager()->GetViewSize() gets (0, 0) at this point anyway
	}

	void CRedGuiGeneralLayer::AddChildItem( IRedGuiLayerItem* item )
	{
		m_childItems.PushBack(item);
	}

	void CRedGuiGeneralLayer::RemoveChildItem( IRedGuiLayerItem* item )
	{
		IRedGuiLayerItem* parent = item->GetLayerItemParent();
		if(parent != nullptr)
		{
			parent->RemoveChildItem(item);
		}

		m_childItems.Remove(item);
	}

	Uint32 CRedGuiGeneralLayer::GetChildItemCount() const
	{
		return m_childItems.Size();
	}

	IRedGuiLayerItem* CRedGuiGeneralLayer::GetChildItemAt( Uint32 index )
	{
		return m_childItems[index];
	}

	void CRedGuiGeneralLayer::BringToFront(IRedGuiLayerItem* item)
	{
		if( (m_childItems.Size() < 2) || (m_childItems.Back() == item))
		{
			return;
		}

		m_childItems.Remove(item);
		m_childItems.PushBack(item);
	}

	IRedGuiLayerItem* CRedGuiGeneralLayer::GetLayerItemByPoint( const Vector2& position ) const
	{
		if(m_isPick == false)
		{
			return nullptr;
		}

		for(Int32 i=m_childItems.Size()-1; i>=0; --i)
		{
			IRedGuiLayerItem* item = m_childItems[i]->GetLayerItemByPoint( position );
			if(item != nullptr)
			{
				return item;
			}
		}

		return nullptr;
	}

	Vector2 CRedGuiGeneralLayer::GetSize() const
	{
		return GRedGui::GetInstance().GetRenderManager()->GetViewSize();
	}

	void CRedGuiGeneralLayer::DrawLayer()
	{
		for(Uint32 i=0; i<m_childItems.Size(); ++i)
		{
			m_childItems[i]->DrawLayerItem();
		}
	}

	void CRedGuiGeneralLayer::ResizeView(const Vector2& viewSize )
	{
		for(Uint32 i=0; i<m_childItems.Size(); ++i)
		{
			m_childItems[i]->ResizeLayerItemView(m_viewSize, viewSize);
		}
		m_viewSize = viewSize;
	}

	CRedGuiGeneralLayer::~CRedGuiGeneralLayer()
	{
		for(Uint32 i=0; i<m_childItems.Size(); ++i)
		{
			RemoveChildItem(m_childItems[i]);
		}
		m_childItems.Clear();
	}

	const String& CRedGuiGeneralLayer::GetName() const
	{
		return m_name;
	}

	void CRedGuiGeneralLayer::Dispose()
	{
		GRedGui::GetInstance().GetLayerManager()->DisposeLayer( this );
	}

}	// namespace RedGui

#endif	// NO_RED_GUI

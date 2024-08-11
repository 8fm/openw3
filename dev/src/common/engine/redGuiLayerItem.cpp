/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiLayer.h"
#include "redGuiLayerItem.h"
#include "redGuiManager.h"

namespace RedGui
{
	CRedGuiLayerItem::CRedGuiLayerItem()
		: m_layer(nullptr)
		, m_layerItemParent(nullptr)
	{
		/* intentionally empty */
	}

	CRedGuiLayerItem::~CRedGuiLayerItem()
	{
		/* intentionally empty */
	}

	IRedGuiLayer* CRedGuiLayerItem::GetLayer() const
	{
		return m_layer;
	}

	IRedGuiLayerItem* CRedGuiLayerItem::GetLayerItemParent() const
	{
		return m_layerItemParent;
	}

	void CRedGuiLayerItem::AttachToLayer(const String& layerName)
	{
		IRedGuiLayer* layer = GRedGui::GetInstance().GetLayerManager()->GetByName(layerName);
		if(layer != nullptr)
		{
			m_layer = layer;
			layer->AddChildItem(this);
		}
	}

	void CRedGuiLayerItem::DetachFromLayer()
	{
		if(m_layer != nullptr)
		{
			m_layer->RemoveChildItem(this);
		}
	}

	void CRedGuiLayerItem::UpLayerItem(Bool deep/* = true*/)
	{
		if(deep == true)
		{
			IRedGuiLayerItem* parentItem = this;
			for(IRedGuiLayerItem* item = this; item!= nullptr; item = item->GetLayerItemParent())
			{
				parentItem = item;
			}
			parentItem->UpLayerItem(false);
		}

		if(m_layer != nullptr)
		{
			m_layer->BringToFront(this);
		}

		for(Uint32 i=0; i<m_layerItems.Size(); ++i)
		{
			m_layerItems[i]->UpLayerItem(false);
		}
	}

	void CRedGuiLayerItem::AddChildItem(IRedGuiLayerItem* item)
	{
		m_layerItems.PushBack(item);
		static_cast<CRedGuiLayerItem*>(item)->m_layerItemParent = this;
	}

	void CRedGuiLayerItem::RemoveChildItem(IRedGuiLayerItem* item)
	{
		static_cast<CRedGuiLayerItem*>(item)->m_layerItemParent = nullptr;
		m_layerItems.Remove(item);
	}

}	// namespace RedGui

#endif	// NO_RED_GUI

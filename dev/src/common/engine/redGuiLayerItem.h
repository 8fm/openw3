/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"

namespace RedGui
{
	class IRedGuiLayerItem
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		virtual ~IRedGuiLayerItem() { /* intentionally empty */ }

		virtual IRedGuiLayer* GetLayer() const = 0;
		virtual IRedGuiLayerItem* GetLayerItemParent() const = 0;

		virtual IRedGuiLayerItem* GetLayerItemByPoint( const Vector2& position ) const = 0;

		virtual void ResizeLayerItemView( const Vector2& oldView, const Vector2& newView ) = 0;
		virtual void AttachToLayer( const String& layerName ) = 0;
		virtual void DetachFromLayer() = 0;
		virtual void UpLayerItem( Bool deep = true ) = 0;

		virtual void DrawLayerItem() = 0;
		virtual void Draw() = 0;

		virtual void AddChildItem( IRedGuiLayerItem* item ) = 0;
		virtual void RemoveChildItem( IRedGuiLayerItem* item ) = 0;
	};


	class CRedGuiLayerItem : public IRedGuiLayerItem
	{
	public:
		CRedGuiLayerItem();
		virtual ~CRedGuiLayerItem();

		virtual IRedGuiLayer* GetLayer() const;
		virtual IRedGuiLayerItem* GetLayerItemParent() const;

		virtual void AttachToLayer( const String& layerName );
		virtual void DetachFromLayer();
		virtual void UpLayerItem(Bool deep = true);

		virtual void AddChildItem( IRedGuiLayerItem* item );
		virtual void RemoveChildItem( IRedGuiLayerItem* item );

	private:
		IRedGuiLayer* m_layer;
		IRedGuiLayerItem* m_layerItemParent;
		ArrayLayerItemPtr m_layerItems;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI

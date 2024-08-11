/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"

namespace RedGui
{
	class IRedGuiLayer
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		virtual ~IRedGuiLayer() { /* intentionally empty */ }
		
		virtual void AddChildItem( IRedGuiLayerItem* item ) = 0;
		virtual void RemoveChildItem( IRedGuiLayerItem* item ) = 0;
		virtual Uint32 GetChildItemCount() const = 0;
		virtual IRedGuiLayerItem* GetChildItemAt( Uint32 index ) = 0;

		virtual void BringToFront( IRedGuiLayerItem* item ) = 0;

		virtual IRedGuiLayerItem* GetLayerItemByPoint( const Vector2& position ) const = 0;

		virtual Vector2 GetSize() const = 0;

		virtual void DrawLayer() = 0;

		virtual void ResizeView( const Vector2& viewSize ) = 0;		

		virtual const String& GetName() const = 0;

		virtual void Dispose() = 0;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI

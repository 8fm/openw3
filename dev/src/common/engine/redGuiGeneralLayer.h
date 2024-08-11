/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"
#include "redGuiLayer.h"

namespace RedGui
{
	class CRedGuiGeneralLayer : public IRedGuiLayer
	{
	public:
		CRedGuiGeneralLayer( const String& name, Bool pick = true );
		virtual ~CRedGuiGeneralLayer();

		virtual void AddChildItem( IRedGuiLayerItem* item );
		virtual void RemoveChildItem( IRedGuiLayerItem* item );
		virtual Uint32 GetChildItemCount() const;
		virtual IRedGuiLayerItem* GetChildItemAt( Uint32 index );

		virtual void BringToFront( IRedGuiLayerItem* item );

		virtual IRedGuiLayerItem* GetLayerItemByPoint( const Vector2& position ) const;

		virtual Vector2 GetSize() const;

		virtual void DrawLayer();

		virtual void ResizeView(const Vector2& viewSize );

		virtual const String& GetName() const;

		virtual void Dispose();

	protected:
		void RestoreAlwaysOnTopControls();

		String				m_name;			//!< layer name
		Bool				m_isPick;		//!<
		Vector2				m_viewSize;		//!<
		ArrayLayerItemPtr	m_childItems;	//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI

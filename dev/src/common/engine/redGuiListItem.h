/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiListItem : public CRedGuiUserData
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiListItem
		(
			const String& text				= String::EMPTY,
			RedGuiAny userData				= nullptr,
			const Color& textColor			= Color::WHITE,
			const Color& backgroundColor	= Color( 45,45,45,255 )
		);
		~CRedGuiListItem();
		
		//Events
		Event2_PackageUint32 EventTextChanged;

		const String& GetText( Uint32 column = 0 ) const;
		void SetText( const String& text, Uint32 column = 0 );
		Color GetBackgroundColor() const;
		void SetBackgroundColor( const Color& color );
		const Color& GetTextColor() const;
		void SetTextColor( const Color& color );
		Bool GetSelected() const;
		void SetSelected( Bool value );

	private:
		TDynArray< String, MC_RedGuiControls, MemoryPool_RedGui >		m_texts;			//!< 
		Color															m_backgroundColor;	//!< 
		Color															m_textColor;		//!< 
		Bool															m_selected;			//!< 
	};

}	// namespace RedGui

#endif	// NO_RED_GUI

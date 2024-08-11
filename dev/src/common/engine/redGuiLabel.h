/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiLabel : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiLabel(Uint32 x, Uint32 y, Uint32 width, Uint32 height);
		virtual ~CRedGuiLabel();

		virtual const String& GetText() const;
		virtual void SetText(const String& text, const Color& textColor = Color::WHITE);		

		Bool GetAutoSize() const;
		void SetAutoSize( Bool value );

		void ClearText();

		void Draw();

	private:
		void AdjustToText();
		void CopyText();

		virtual void OnKeyButtonPressed( enum EInputKey key, Char text );
		virtual void OnMouseButtonPressed( const Vector2& mousePosition, enum EMouseButton button );

	private:
		String	m_text;		//!<
		Bool	m_autoSize;	//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI

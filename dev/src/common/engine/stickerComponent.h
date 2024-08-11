/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "component.h"

/// Visual label
class CStickerComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CStickerComponent, CComponent, 0 );

public:
	CStickerComponent();

	//! Get sticker caption ?
	RED_INLINE String GetText() const { return m_text; }
	
	//! Is the sticker visible
	RED_INLINE Bool IsVisible() const { return m_isVisible; }

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	//! Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	//! Set sticker text
	void SetText( const String& text );

	//! Set sticker visibility
	void SetVisible( Bool visible );

private:
	String				m_text;				//!< Text to display
	Color				m_textColor;		//!< Text color
	Bool				m_isVisible;		//!< Is the sticker visible
};

BEGIN_CLASS_RTTI( CStickerComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_CUSTOM_EDIT( m_text, TXT("Sticker text"), TXT("StickerPropertyEditor") );
	PROPERTY_EDIT( m_textColor,   TXT("Sticker text color") );
END_CLASS_RTTI();

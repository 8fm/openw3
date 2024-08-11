/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "component.h"

/// Visual label
class CHelpTextComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CHelpTextComponent, CComponent, 0 );

public:
	CHelpTextComponent();

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	//! Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	//! Set text
	RED_INLINE void SetText( const String& text ) { m_text = text; }

	//! Set text color
	RED_INLINE void SetTextColor( const Color& color ) { m_textColor = color; }

	//! Set backgorund color
	RED_INLINE void SetBackgroundColor( const Color& color ) { m_backgroundColor = color; }

	//! Set draw background
	RED_INLINE void SetDrawBackground( Bool value ) { m_drawBackground = value; }

	//! Get text
	RED_INLINE String GetText() const { return m_text; }

	//! Get text color
	RED_INLINE Color GetTextColor() const { return m_textColor; }

	//! Get background color
	RED_INLINE Color GetBackgroundColor() const { return m_backgroundColor; }

	//! Get draw background
	RED_INLINE Bool GetDrawBackground() const { return m_drawBackground; }

private:
	String				m_text;				//!< Text to display
	Color				m_textColor;		//!< Text color
	Color				m_backgroundColor;	//!< Background color
	Bool				m_drawBackground;	//!< Is background is visible
};

BEGIN_CLASS_RTTI( CHelpTextComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_CUSTOM_EDIT( m_text,		TXT("Text"), TXT("StickerPropertyEditor") );
	PROPERTY_EDIT( m_textColor,			TXT("Text color") );
	PROPERTY_EDIT( m_backgroundColor,   TXT("Background color") );
	PROPERTY_EDIT( m_drawBackground,	TXT("Draw background") );
END_CLASS_RTTI();

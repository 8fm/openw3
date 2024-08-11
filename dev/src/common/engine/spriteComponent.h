/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "component.h"

/// Sprite based drawable component
class CSpriteComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CSpriteComponent, CComponent, 0 );

protected:
	Bool						m_isVisible;		//!< Visibility flag
	THandle< CBitmapTexture >	m_icon;				//!< Sprite icon to diplay

public:
	// Get visibility status
	RED_INLINE Bool IsVisible() const { return m_isVisible; }

public:
	CSpriteComponent();

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	// Get sprite icon
	virtual CBitmapTexture* GetSpriteIcon() const;

	// Get sprite rendering color
	virtual Color CalcSpriteColor() const;

	// Get sprite rendering size
	virtual Float CalcSpriteSize() const;

public:
	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

public:
	// Set visibility status
	void SetVisible( Bool visible );

	// Change sprite icon
	void SetSpriteIcon( CBitmapTexture* icon );

protected:
	// Draw sprite with overlay flag
	virtual Bool IsOverlay() const;
};

BEGIN_CLASS_RTTI( CSpriteComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_isVisible, TXT("Is component visible ?") );
	PROPERTY_EDIT( m_icon, TXT("Sprite icon") );
END_CLASS_RTTI();

/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "spriteComponent.h"

/// Dummy transformable component
class CEffectDummyComponent : public CSpriteComponent
{
	DECLARE_ENGINE_CLASS( CEffectDummyComponent, CSpriteComponent, 0 );

public:
	CEffectDummyComponent();

	// Get sprite icon
	virtual CBitmapTexture* GetSpriteIcon() const;

	virtual Uint32 GetMinimumStreamingDistance() const override;
};

DEFINE_SIMPLE_RTTI_CLASS( CEffectDummyComponent, CSpriteComponent );

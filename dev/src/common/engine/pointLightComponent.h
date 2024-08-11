/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "lightComponent.h"

// Face filter for dynamic shadows
enum ELightCubeSides
{
	LCS_NegativeX	= FLAG( 0 ),		//!< Side facing negative X
	LCS_PositiveX	= FLAG( 1 ),		//!< Side facing positive X
	LCS_NegativeY	= FLAG( 2 ),		//!< Side facing negative Y
	LCS_PositiveY	= FLAG( 3 ),		//!< Side facing positive Y
	LCS_NegativeZ	= FLAG( 4 ),		//!< Side facing negative Z
	LCS_PositiveZ	= FLAG( 5 ),		//!< Side facing positive Z
};

BEGIN_BITFIELD_RTTI( ELightCubeSides, 1 );
	BITFIELD_OPTION( LCS_NegativeX );
	BITFIELD_OPTION( LCS_PositiveX );
	BITFIELD_OPTION( LCS_NegativeY );
	BITFIELD_OPTION( LCS_PositiveY );
	BITFIELD_OPTION( LCS_NegativeZ );
	BITFIELD_OPTION( LCS_PositiveZ );
END_BITFIELD_RTTI();

/// Omni directional light
class CPointLightComponent : public CLightComponent
{
	DECLARE_ENGINE_CLASS( CPointLightComponent, CLightComponent, 0 );

protected:
	// point lights can cache the content
	Bool		m_cacheStaticShadows;

	// face filtering
	Uint8		m_dynamicShadowsFaceMask;

public:
	// Return true if this light should cache the static shadows
	RED_INLINE Bool IsCachingStaticShadows() const { return m_cacheStaticShadows; }

	// Get the mask of which sides of the cube are allowed to cast dynamic shadows
	RED_INLINE Uint8 GetDynamicShadowsFaceMask() const { return m_dynamicShadowsFaceMask; }

public:
	CPointLightComponent();

	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

#ifdef USE_UMBRA
	virtual Bool OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds );
#endif

	// Get sprite icon
	virtual CBitmapTexture* GetSpriteIcon() const;
};

BEGIN_CLASS_RTTI( CPointLightComponent );
	PARENT_CLASS( CLightComponent );
	PROPERTY_EDIT( m_cacheStaticShadows, TXT("Light is caching shadows from static geometry") );
	PROPERTY_BITFIELD_EDIT( m_dynamicShadowsFaceMask, ELightCubeSides, TXT("Which sides of the cube are allowed to cast dynamic shadows") );
END_CLASS_RTTI();


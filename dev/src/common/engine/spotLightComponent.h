/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "lightComponent.h"


/// Spot light 
class CSpotLightComponent : public CLightComponent
{
	DECLARE_ENGINE_CLASS( CSpotLightComponent, CLightComponent, 0 );

protected:
	Float						m_innerAngle;								// Inner cone angle
	Float						m_outerAngle;								// Outer cone angle
	Float						m_softness;									// Cone softness
	Float						m_projectionTextureAngle;					// Projection texture angle
	Float						m_projectionTexureUBias;					// Projection texture U bias
	Float						m_projectionTexureVBias;					// Projection texture V bias
	THandle< CBitmapTexture >	m_projectionTexture;						// Projection texture

public:
	// Get inner angle
	RED_INLINE Float GetInnerAngle() const { return m_innerAngle; }

	// Get projection angle
	RED_INLINE Float GetProjectionTextureAngle() const { return m_projectionTextureAngle; }

	// Get projection U bias
	RED_INLINE Float GetProjectionTextureUBias() const { return m_projectionTexureUBias; }

	// Get projection V bias
	RED_INLINE Float GetProjectionTextureVBias() const { return m_projectionTexureVBias; }

	// Get outer angle
	RED_INLINE Float GetOuterAngle() const { return m_outerAngle; }

	// Get cone softness
	RED_INLINE Float GetSoftness() const { return m_softness; }
	
	// Get projection texture
	RED_INLINE CBitmapTexture* GetProjectionTexture() const { return m_projectionTexture.Get(); }

public:
	CSpotLightComponent();

	// Set inner angle
	void SetInnerAngle( Float innerAngle );

	// Set outer angle
	void SetOuterAngle( Float outerAngle );

	// Set softness
	void SetSoftness( Float softness );

public:
	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	// Get sprite icon
	virtual CBitmapTexture* GetSpriteIcon() const;

#ifdef USE_UMBRA
	virtual Bool OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds );
#endif

public:
	// Effect interface
	virtual Bool GetEffectParameterValue( CName paramName, EffectParameterValue &value /* out */ ) const;
	virtual Bool SetEffectParameterValue( CName paramName, const EffectParameterValue &value );
	virtual void EnumEffectParameters( CFXParameters &effectParams /* out */ );
};

BEGIN_CLASS_RTTI( CSpotLightComponent );
	PARENT_CLASS( CLightComponent );
	PROPERTY_EDIT_RANGE( m_innerAngle, TXT("Inner cone angle"), 0.01f, 359.8f );
	PROPERTY_EDIT_RANGE( m_outerAngle, TXT("Outer cone angle"), 0.01f, 359.9f );
	PROPERTY_EDIT_RANGE( m_softness, TXT("Cone softness"), 0.001f, 1000.0f );
	PROPERTY_EDIT( m_projectionTexture, TXT("Projection texture") );
	PROPERTY_EDIT( m_projectionTextureAngle, TXT("Projection texture angle") );
	PROPERTY_EDIT( m_projectionTexureUBias, TXT("U bias for projection texture") );
	PROPERTY_EDIT( m_projectionTexureVBias, TXT("V bias for projection texture") );
END_CLASS_RTTI();

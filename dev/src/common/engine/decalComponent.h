/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "drawableComponent.h"
#include "renderSettings.h"

/// Component rendering mesh
class CDecalComponent : public CDrawableComponent
{
	DECLARE_ENGINE_CLASS( CDecalComponent, CDrawableComponent, 0 )

public:
	// Allow a decal to be rendered past its auto-hide distance by this much. This allows for fading the decal out when it
	// reached the auto-hide distance.
	static const Float AUTOHIDE_MARGIN;

private:
	static Box									m_unitBox;

protected:
	THandle< CBitmapTexture >					m_diffuseTexture;						//!< Decal diffuse texture
	Float										m_normalThreshold;
	Float										m_autoHideDistance;						//!< Decal autohide distance
	Bool										m_verticalFlip;
	Bool										m_horizontalFlip;
	Color										m_specularColor;
	Float										m_specularity;

	Float										m_fadeTime;

public:
	// Decal textures
	RED_INLINE CBitmapTexture*	GetDiffuseTexture() const { return m_diffuseTexture.Get(); }
	RED_INLINE void				SetDiffuseTexture( CBitmapTexture* diffuseTexture ) { m_diffuseTexture = diffuseTexture; }

	RED_INLINE Float			GetNormalThreshold() const { return m_normalThreshold; }

	RED_INLINE	 Float			GetSpecularity() const { return m_specularity; }
	RED_INLINE	 Color			GetSpecularColor() const { return m_specularColor; }

	RED_INLINE	 Bool			GetVerticalFlip() const { return m_verticalFlip; }
	RED_INLINE	 Bool			GetHorizontalFlip() const { return m_horizontalFlip; }

	RED_INLINE	Float			GetFadeTime() const { return m_fadeTime; }

public:
	CDecalComponent();
	virtual ~CDecalComponent();

public:
	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	// Update world space bounding box
	virtual void OnUpdateBounds();

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world
	virtual void OnDetached( CWorld* world );

#ifdef USE_UMBRA
	virtual Bool ShouldBeCookedAsOcclusionData() const { return true; }
	virtual Bool OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds );
#endif

#ifndef NO_DATA_VALIDATION
	// Check data
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif

	static Float CalculateAutoHideDistance( const Float autoHideDistance, const Box& boundingBox )
	{
		// default and max mirrored below.
		return CComponent::CalculateAutoHideDistance( autoHideDistance, boundingBox, Config::cvDecalsHideDistance.Get(), 300.0f );
	}

	virtual Float GetAutoHideDistance() const { return CalculateAutoHideDistance( m_autoHideDistance, m_boundingBox ); }
	virtual Float GetDefaultAutohideDistance() const { return Config::cvDecalsHideDistance.Get(); }
	virtual Float GetMaxAutohideDistance() const { return 300.0f; }

	virtual Uint32 GetOcclusionId() const override;

public:
	// Effect interface
	virtual Bool GetEffectParameterValue( CName paramName, EffectParameterValue &value /* out */ ) const;
	virtual Bool SetEffectParameterValue( CName paramName, const EffectParameterValue &value );

	virtual void EnumEffectParameters( CFXParameters &effectParams /* out */ );

	virtual Uint32 GetMinimumStreamingDistance() const override;
};

BEGIN_CLASS_RTTI( CDecalComponent );
	PARENT_CLASS( CDrawableComponent );
	PROPERTY_EDIT( m_diffuseTexture, TXT("Diffuse texture") );
	PROPERTY_EDIT( m_specularity, TXT("Specularity of the decal") );
	PROPERTY_EDIT( m_specularColor, TXT("Color of the specular. Works only if specularity is > 0. Alpha controls blending betwen diffuse color in texture and this fixed color.") );
	PROPERTY_EDIT_RANGE( m_normalThreshold, TXT("Normal threshold (in degrees)"), 0.0f, 90.0f );
	PROPERTY_EDIT( m_autoHideDistance, TXT( "Auto hide distance. -1 will set default value from ini file" ) );
	PROPERTY_EDIT( m_verticalFlip , TXT("Flip texture verticaly") );
	PROPERTY_EDIT( m_horizontalFlip , TXT("Flip texture horizontaly") );
	PROPERTY_EDIT( m_fadeTime, TXT("How long it takes to fade in/out when at auto hide distance.") );
END_CLASS_RTTI();

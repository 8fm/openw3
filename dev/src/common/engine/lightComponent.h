/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxDefinition.h"
#include "spriteComponent.h"
#include "environmentAreaParams.h"

class IRenderProxy;

RED_DEFINE_STATIC_NAME( lightUsageMask );

enum ELightUsageMask
{
	LUM_Custom0					= FLAG( 0 ),		//!< Custom flag
	LUM_Custom1					= FLAG( 1 ),		//!< Custom flag
	LUM_Custom2					= FLAG( 2 ),		//!< Custom flag
	LUM_Custom3					= FLAG( 3 ),		//!< Custom flag
	LUM_Custom4					= FLAG( 4 ),		//!< Custom flag
	LUM_Custom5					= FLAG( 5 ),		//!< Custom flag
	LUM_Custom6					= FLAG( 6 ),		//!< Custom flag
	LUM_Custom7					= FLAG( 7 ),		//!< Custom flag - up to 31 available in total
	LUM_RenderToEnvProbe		= FLAG( 8 ),		//!< Should be rendered into envprobe
	LUM_ExcludeFromSceneRender	= FLAG( 9 ),		//!< Should be excluded from scene rendering
	LUM_IsInteriorOnly			= FLAG( 10 ),		//!< Interior only
	LUM_IsExteriorOnly			= FLAG( 11 ),		//!< Exterior only
};

BEGIN_BITFIELD_RTTI( ELightUsageMask, 4 );
	BITFIELD_OPTION( LUM_Custom0 );
	BITFIELD_OPTION( LUM_Custom1 );
	BITFIELD_OPTION( LUM_Custom2 );
	BITFIELD_OPTION( LUM_Custom3 );
	BITFIELD_OPTION( LUM_Custom4 );
	BITFIELD_OPTION( LUM_Custom5 );
	BITFIELD_OPTION( LUM_Custom6 );
	BITFIELD_OPTION( LUM_Custom7 );
	BITFIELD_OPTION( LUM_RenderToEnvProbe );
	BITFIELD_OPTION( LUM_ExcludeFromSceneRender );
	BITFIELD_OPTION( LUM_IsInteriorOnly );
	BITFIELD_OPTION( LUM_IsExteriorOnly );
END_BITFIELD_RTTI();

/// Light clipping
struct SLightFlickering
{
	DECLARE_RTTI_STRUCT( SLightFlickering );

	Float		m_positionOffset;
	Float		m_flickerStrength;
	Float		m_flickerPeriod;	

	SLightFlickering()
		: m_positionOffset( 0.0f )
		, m_flickerStrength( 0.0f )
		, m_flickerPeriod( 0.2f )		
	{};
};

BEGIN_CLASS_RTTI( SLightFlickering );
	PROPERTY_EDIT( m_positionOffset, TXT("Position offset") );
	PROPERTY_EDIT( m_flickerStrength, TXT("Flicker strength") );
	PROPERTY_EDIT( m_flickerPeriod, TXT("Flicker period") );	
END_CLASS_RTTI();

enum ELightShadowCastingMode
{
	LSCM_None,
	LSCM_Normal,
	LSCM_OnlyDynamic,
	LSCM_OnlyStatic,
};

BEGIN_ENUM_RTTI( ELightShadowCastingMode );
	ENUM_OPTION( LSCM_None );
	ENUM_OPTION( LSCM_Normal );
	ENUM_OPTION( LSCM_OnlyDynamic );
	ENUM_OPTION( LSCM_OnlyStatic );
END_ENUM_RTTI();

/// Base light component
class CLightComponent : public CSpriteComponent
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CLightComponent, CSpriteComponent );

	friend class CRenderScene;
	friend class CFXTrackItemDynamicLight;

protected:
	Color						m_color;					//!< Light color
	Float						m_radius;					//!< Light radius
	Float						m_brightness;				//!< Light brightness
	Float						m_attenuation;				//!< Light attenuation
	Float						m_autoHideDistance;			//!< Auto hide distance
	Float						m_autoHideRange;			//!< Auto hide range
	IRenderProxy*				m_renderProxy;				//!< Created proxy for light
	SLightFlickering			m_lightFlickering;			//!< Light flickering
	Bool						m_allowDistantFade;			//!< Allow light to become distant light
	Bool						m_isFadeOnToggle;			//!< Should fades be performed when light is turned on/off
	EEnvColorGroup				m_envColorGroup;			//!< Environment color group
	Bool						m_isEnabled;				//!< Is the light enabled
	Bool						m_diffuseOnly;				//!< Diffuse only
	Uint32						m_lightUsageMask;			//!< Light flags determining in which situations light is used
	Float						m_shadowBlendFactor;		//!< Shadow blend factor

	// new shadow system
	ELightShadowCastingMode		m_shadowCastingMode;		//!< How the shadows are casted
	Float						m_shadowFadeDistance;		//!< Distance from light at which the shadow should start to fade away (if 0 then disabled)
	Float						m_shadowFadeRange;			//!< Shadow fading range

public:
	// Get light color
	RED_INLINE const Color& GetColor() const { return m_color; }

	// Is light enabled ?
	RED_INLINE Bool IsEnabled() const { return m_isEnabled; }

	// Get light radius
	RED_INLINE Float GetRadius() const { return m_radius; }

	// Get light brightness
	RED_INLINE Float GetBrightness() const { return m_brightness; }

	// Get light attenuation
	RED_INLINE Float GetAttenuation() const { return m_attenuation; }

	// Light flickering
	RED_INLINE const SLightFlickering* GetLightFlickering() const { return &m_lightFlickering; }

	// Get environment color group
	RED_INLINE EEnvColorGroup GetEnvColorGroup() const { return m_envColorGroup; }

	// Get auto hide distance
	virtual Float GetAutoHideDistance() const { return m_autoHideDistance; }
	virtual Float GetDefaultAutohideDistance() const { return 50.0f; }
	virtual Float GetMaxAutohideDistance() const { return 300.0f; }

	// Get fade on toggle
	Bool GetFadeOnToggle() const { return m_isFadeOnToggle; }

#ifdef USE_UMBRA
	virtual Uint32 GetOcclusionId() const override;
#endif // USE_UMBRA

	// Get allow to become distant light
	RED_INLINE Bool GetAllowDistantLight() const { return m_allowDistantFade; }

	// Get auto hide range
	RED_INLINE Float GetAutoHideRange() const { return m_autoHideRange; }

	// Get Light Usage Mask
	RED_INLINE Uint32 GetLightUsageMask() const { return m_lightUsageMask; }

	// Get Shadow amount
	RED_INLINE Float GetShadowBlendFactor() const { return m_shadowBlendFactor; }

	//dex++
	// Get light shadow casting mode
	RED_INLINE ELightShadowCastingMode GetShadowCastingMode() const { return m_shadowCastingMode; }

	// Returns true if light is casting shadows
	RED_INLINE Bool IsCastingShadows() const { return m_shadowCastingMode != LSCM_None; }

	// Get the light shadow fade distance
	RED_INLINE Float GetShadowFadeDistance() const { return m_shadowFadeDistance; }

	// Get the light shadow fade range
	RED_INLINE Float GetShadowFadeRange() const { return m_shadowFadeRange; }

	//dex--

public:
	CLightComponent();

	// Set light color
	void SetColor( const Color& color );

	// Toggle light shadows
	void SetCastingShadows( Bool enabled );

	//dex++: Change shadow casting mode
	void SetShadowCastingMode( ELightShadowCastingMode mode );

	// Set the light shadow fade distance
	void SetShadowFadeDistance( Float shadowFadeDistance );

	// Set the light shadow fade range
	void SetShadowFadeRange( Float shadowFadeRange );
	//dex--

	// Set light radius
	void SetRadius( Float radius );

	// Set auto hide distance and range
	void SetAutoHideDistance( Float autoHideDistance, Float autoHideRange );

	// Set allowing of distant light
	void SetAllowDistantFade( bool allowDistantFade );

	// Set light flickering
	void SetLightFlickering( const SLightFlickering& lightFlickering );

	// Set light brightness
	void SetBrightness( Float brightness );

	// Set light attenuation
	void SetAttenuation( Float attenuation );

	// Set shadow blend factor
	void SetShadowBlendFactor( Float shadowBlendFactor );

	// Set fade on toggle
	void SetFadeOnToggle( Bool enableFade );

	// Enable component
	virtual void SetEnabled( Bool flag );

	// Refresh render proxy
	virtual void RefreshRenderProxies();

public:
	// Get sprite rendering color
	virtual Color CalcSpriteColor() const;

	// Get sprite icon
	virtual CBitmapTexture* GetSpriteIcon() const;

public:
	// Update component transform
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

#ifdef USE_UMBRA
	virtual Bool ShouldBeCookedAsOcclusionData() const;
#endif

	// Property changed
	virtual void OnPropertyPostChange( IProperty* property );

	virtual void OnPropertyExternalChanged( const CName& propertyName ) override; 

#ifndef NO_DATA_VALIDATION
	// Check data
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif
public:

	// Effect interface
	virtual Bool GetEffectParameterValue( CName paramName, EffectParameterValue &value /* out */ ) const;
	virtual Bool SetEffectParameterValue( CName paramName, const EffectParameterValue &value );
	virtual void EnumEffectParameters( CFXParameters &effectParams /* out */ );

	void RefreshInRenderSceneIfAttached();
	

protected:
	//dex++: Old properties handling
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );
	//dex--

	// Attach to render scene
	void ConditionalAttachToRenderScene( CWorld* world );
};

BEGIN_ABSTRACT_CLASS_RTTI( CLightComponent );
	PARENT_CLASS( CSpriteComponent );
	PROPERTY_EDIT( m_isEnabled, TXT("Toggles light on and off") );
	PROPERTY_EDIT( m_shadowCastingMode, TXT("Shadow casting mode") );
	PROPERTY_EDIT( m_shadowFadeDistance, TXT("Distance from light at which the shadow should start to fade away (if 0 then disabled)") );
	PROPERTY_EDIT( m_shadowFadeRange, TXT("Shadow fading range") );
	PROPERTY_EDIT_RANGE( m_shadowBlendFactor, TXT("[0.1, 1.0]"), 0.1f, 1.f );
	PROPERTY_EDIT_RANGE( m_radius, TXT("Light radius"), 0.001f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_brightness, TXT("Brightness level"), 0.0f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_attenuation, TXT("Attenuation"), 0.0f, 1.f );
	PROPERTY_EDIT( m_color, TXT("Light color") );
	PROPERTY_EDIT( m_envColorGroup, TXT("Environment color group") );
	PROPERTY_EDIT( m_autoHideDistance, TXT("Auto hide distance") );
	PROPERTY_EDIT( m_autoHideRange, TXT("Auto hide range") );
	PROPERTY_EDIT( m_lightFlickering, TXT("Light flickering") );
	PROPERTY_EDIT( m_allowDistantFade, TXT("Allow to light become distant light after some distance") );
	PROPERTY_BITFIELD_EDIT( m_lightUsageMask, ELightUsageMask, TXT("Flags to use light in special situations") );
END_CLASS_RTTI();

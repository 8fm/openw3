/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


// --------------------------------------------------------------------------

class CCameraComponent;

class CEnvRadialBlurParameters
{
	DECLARE_RTTI_STRUCT_WITH_ALLOCATOR( CEnvRadialBlurParameters, MC_Engine );

public:	
	Vector  m_radialBlurSource;
	Float	m_radialBlurAmount;
	Float	m_sineWaveAmount;
	Float   m_sineWaveSpeed;
	Float   m_sineWaveFreq;
	Float	m_centerMultiplier;
	Float	m_distance;
	
public:
	CEnvRadialBlurParameters();
};

BEGIN_CLASS_RTTI( CEnvRadialBlurParameters )
	PROPERTY_EDIT(m_radialBlurSource,		TXT("Source of the radial blur") );
	PROPERTY_EDIT(m_radialBlurAmount,		TXT("Amount of the radial blur") );
	PROPERTY_EDIT(m_sineWaveAmount,			TXT("Amount of the sonic wave") );
	PROPERTY_EDIT(m_sineWaveSpeed,			TXT("Speed of the sonic wave") );
	PROPERTY_EDIT(m_sineWaveFreq,			TXT("Number of waves visible") );
	PROPERTY_EDIT(m_centerMultiplier,		TXT("How much is this effect center-aligned") );
	PROPERTY_EDIT(m_distance,				TXT("Distance") );
END_CLASS_RTTI();

// --------------------------------------------------------------------------

struct SEnvLightShaftParameters
{
public:	
	Vector  m_source;
	Vector  m_rotation;
	Vector	m_bloomTint;
	Float	m_radius;
	Float   m_distanceFade;
	Float	m_outerRadius;
	Float	m_innerRadius;
	Float	m_bloomScale;
	Float	m_bloomTreshold;
	Float	m_blurScale;
	Float	m_screenFactor;
	Float	m_autoHideDistance;
	Bool	m_spotLight;
	Bool	m_additiveBlend;
	Bool	m_ignoreWorldSpace;

public:
	SEnvLightShaftParameters();	
};

// --------------------------------------------------------------------------

class CEnvDayCycleOverrideParameters
{
	DECLARE_RTTI_STRUCT( CEnvDayCycleOverrideParameters );

public:
	Bool		m_fakeDayCycleEnable;
	Float		m_fakeDayCycleHour;
	Bool		m_enableCustomSunRotation;
	EulerAngles	m_customSunRotation;

public:
	CEnvDayCycleOverrideParameters ();
	/// Get custom sun direction
	Vector GetCustomSunDirection() const;
};

BEGIN_CLASS_RTTI( CEnvDayCycleOverrideParameters )
	PROPERTY_EDIT(m_fakeDayCycleEnable,			TXT("Is fake day cycle enabled") );
	PROPERTY_EDIT(m_fakeDayCycleHour,			TXT("Fake day cycle hour") );
	PROPERTY_EDIT(m_enableCustomSunRotation,	TXT("Is custom sun rotation enabled") );
	PROPERTY_EDIT(m_customSunRotation,			TXT("Custom sun rotation") );
END_CLASS_RTTI();

// --------------------------------------------------------------------------

class CEnvBrightnessTintParameters
{
	DECLARE_RTTI_STRUCT( CEnvBrightnessTintParameters );

public:
	Vector		m_tint;
	Float		m_brightness;

public:
	CEnvBrightnessTintParameters (){ m_tint = Vector::ONES; m_brightness = 1.0f; }

};

BEGIN_CLASS_RTTI( CEnvBrightnessTintParameters )
PROPERTY_EDIT(m_tint,				TXT("Color tint") );
PROPERTY_EDIT(m_brightness,			TXT("Brightness") );
END_CLASS_RTTI();

// --------------------------------------------------------------------------

class CEnvDisplaySettingsParams
{
	DECLARE_RTTI_STRUCT( CEnvDisplaySettingsParams );

public:
	Float	m_gamma;
	Bool	m_enableInstantAdaptation;
	Bool	m_enableGlobalLightingTrajectory;
	Bool	m_enableEnvProbeInstantUpdate;
	Bool	m_allowEnvProbeUpdate;
	Bool	m_allowBloom;
	Bool	m_allowColorMod;
	Bool	m_allowAntialiasing;
	Bool	m_allowGlobalFog;
	Bool	m_allowDOF;
	Bool	m_allowSSAO;
	Bool	m_allowCloudsShadow;
	Bool	m_allowVignette;
	Bool	m_disableTonemapping;
	Bool	m_forceCutsceneDofMode;
	Bool	m_allowWaterShader;
	Uint8	m_displayMode;

public:
	CEnvDisplaySettingsParams ();
};

BEGIN_CLASS_RTTI( CEnvDisplaySettingsParams );
	PROPERTY_EDIT( m_enableInstantAdaptation,			TXT("Is instant adaptation enabled") );	
	PROPERTY_EDIT( m_enableGlobalLightingTrajectory,	TXT("Is global lighting trajectory display enabled") );
	PROPERTY_EDIT( m_enableEnvProbeInstantUpdate,		TXT("Is fast envProbe instant update enabled") );	
	PROPERTY_EDIT( m_allowEnvProbeUpdate,				TXT("Allow envProbe update") );	
	PROPERTY_EDIT( m_allowBloom,						TXT("Allow bloom") );
	PROPERTY_EDIT( m_allowColorMod,						TXT("Allow color mod") );
	PROPERTY_EDIT( m_allowAntialiasing,					TXT("Allow antialiasing") );
	PROPERTY_EDIT( m_allowGlobalFog,					TXT("Allow global fog") );
	PROPERTY_EDIT( m_allowDOF,							TXT("Allow depth of field") );
	PROPERTY_EDIT( m_allowSSAO,							TXT("Allow SSAO") );
	PROPERTY_EDIT( m_allowCloudsShadow,					TXT("Allow clouds shadow") );
	PROPERTY_EDIT( m_allowVignette,						TXT("Allow vignette") );
	PROPERTY_EDIT( m_allowWaterShader,					TXT("Allow Water shader to be displayed") );
	PROPERTY_EDIT( m_gamma,								TXT("Gamma correction exponent") );	
END_CLASS_RTTI();

// --------------------------------------------------------------------------

class CGameEnvironmentParams
{
	DECLARE_RTTI_STRUCT( CGameEnvironmentParams );

public:
	CEnvRadialBlurParameters		m_radialBlur;
	CEnvDayCycleOverrideParameters	m_dayCycleOverride;
	CEnvDisplaySettingsParams		m_displaySettings;
	CEnvBrightnessTintParameters	m_brightnessTint;
	TStaticArray<SEnvLightShaftParameters,4> m_lightShaft;
	Float							m_fullscreenBlurIntensity;
	Float							m_gameUnderwaterBrightness;
	bool							m_cutsceneDofMode;
	bool							m_cutsceneOrDialog;

public:
	CGameEnvironmentParams();
};

BEGIN_CLASS_RTTI( CGameEnvironmentParams )
	PROPERTY_EDIT(m_radialBlur,				TXT("Radial blur") );
	PROPERTY_EDIT(m_fullscreenBlurIntensity,TXT("Fullscreen blur intensity") );
	PROPERTY_EDIT(m_gameUnderwaterBrightness,TXT("Brightness of underwater scenes exposed to scripts") );	
	PROPERTY_EDIT(m_dayCycleOverride,		TXT("Day cycle override") );
	PROPERTY_EDIT(m_brightnessTint,			TXT("Brightness tint override") );
	PROPERTY_EDIT(m_displaySettings,		TXT("Display settings") );
	PROPERTY_EDIT(m_cutsceneDofMode,		TXT("Cutscene quality dof") );
	PROPERTY_EDIT(m_cutsceneOrDialog,		TXT("Cutscene or dialog") );
END_CLASS_RTTI();

class CRadialBlurManager
{
private:
	TDynArray< CEnvRadialBlurParameters* >		m_radialBlurParams;

public:
	CRadialBlurManager(){}

	CEnvRadialBlurParameters*					GenerateNewRadialBlurParams();
	void										RemoveRadialBlurParams( CEnvRadialBlurParameters* params );

	void										GenerateFinalParams( CEnvRadialBlurParameters& outRadialBlurParams, CCameraComponent* comp = NULL );

};

class CBrightnessTintManager
{
private:
	TDynArray< CEnvBrightnessTintParameters* >	m_params;

public:
	CBrightnessTintManager(){}

	CEnvBrightnessTintParameters*				GenerateNewParams();
	void										RemoveParams( CEnvBrightnessTintParameters* params );

	void										GenerateFinalParams( CEnvBrightnessTintParameters& outParams );

};

class CLightShaftManager
{
private:
	TDynArray< SEnvLightShaftParameters* >		m_params;

public:
	CLightShaftManager(){}

	SEnvLightShaftParameters*					GenerateNewParams();
	void										RemoveParams( SEnvLightShaftParameters* params );

	void										GenerateFinalParams( TStaticArray<SEnvLightShaftParameters,4>& outParams, CCameraComponent* comp = NULL );

};


// --------------------------------------------------------------------------

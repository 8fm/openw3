/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/curveSimple.h"

class CBitmapTexture;
struct SCameraLightModifiers;
struct SDayPointEnvironmentParams;


enum EEnvParamsResetMode
{
	EnvResetMode_CurvesEmpty,
	EnvResetMode_CurvesDefault,

	// Default construction mode
	EnvResetMode_Default = EnvResetMode_CurvesEmpty	// Empty, because doesn't need any alloc's
};


/// Ambient probes generation parameters
class CEnvAmbientProbesGenParameters
{
	DECLARE_RTTI_STRUCT( CEnvAmbientProbesGenParameters );

public:
	bool		 m_activated;
	SSimpleCurve m_colorAmbient;
	SSimpleCurve m_colorSceneAdd;
	SSimpleCurve m_colorSkyTop;
	SSimpleCurve m_colorSkyHorizon;
	SSimpleCurve m_skyShape;

public:
	CEnvAmbientProbesGenParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvAmbientProbesGenParameters &src, const CEnvAmbientProbesGenParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvAmbientProbesGenParameters );
PROPERTY_EDIT( m_activated,				TXT("m_activated") );
PROPERTY_EDIT( m_colorAmbient,			TXT("m_colorAmbient") );
PROPERTY_EDIT( m_colorSceneAdd,			TXT("m_colorSceneAdd") );
PROPERTY_EDIT( m_colorSkyTop,			TXT("m_colorSkyTop") );
PROPERTY_EDIT( m_colorSkyHorizon,		TXT("m_colorSkyHorizon") );
PROPERTY_EDIT( m_skyShape,				TXT("m_skyShape") );
END_CLASS_RTTI();

/// Ambient probes generation parameters at given point in time ( usualy now :P )
class CEnvAmbientProbesGenParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_colorAmbient;
	SSimpleCurvePoint	m_colorSceneAdd;
	SSimpleCurvePoint	m_colorSkyTop;
	SSimpleCurvePoint	m_colorSkyHorizon;
	SSimpleCurvePoint	m_skyShape;

public:
	CEnvAmbientProbesGenParametersAtPoint() {};
	CEnvAmbientProbesGenParametersAtPoint( const CEnvAmbientProbesGenParameters& source );
	void SetPreviewPanelValues();
};


/// Camera light parameters
class CEnvCameraLightParameters
{
	DECLARE_RTTI_STRUCT( CEnvCameraLightParameters );

public:
	bool		 m_activated;
	SSimpleCurve m_color;
	SSimpleCurve m_attenuation;
	SSimpleCurve m_radius;
	SSimpleCurve m_offsetFront;
	SSimpleCurve m_offsetRight;
	SSimpleCurve m_offsetUp;

public:
	CEnvCameraLightParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvCameraLightParameters &src, const CEnvCameraLightParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvCameraLightParameters );
PROPERTY_EDIT( m_activated,				TXT("m_activated") );
PROPERTY_EDIT( m_color,					TXT("m_color") );
PROPERTY_EDIT( m_attenuation,			TXT("m_attenuation") );
PROPERTY_EDIT( m_radius,				TXT("m_radius") );
PROPERTY_EDIT( m_offsetFront,			TXT("m_offsetFront") );
PROPERTY_EDIT( m_offsetRight,			TXT("m_offsetRight") );
PROPERTY_EDIT( m_offsetUp,				TXT("m_offsetUp") );
END_CLASS_RTTI();

/// Canera light parameters at given point in time
class CEnvCameraLightParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_color;
	SSimpleCurvePoint	m_attenuation;
	SSimpleCurvePoint	m_radius;
	SSimpleCurvePoint	m_offsetFront;
	SSimpleCurvePoint	m_offsetRight;
	SSimpleCurvePoint	m_offsetUp;

public:
	CEnvCameraLightParametersAtPoint() {};
	CEnvCameraLightParametersAtPoint( const CEnvCameraLightParameters& source );
	void SetDisabledValues();

public:
	Float  GetRadius( const SCameraLightModifiers &lightModifier ) const;
	Vector GetColor( const SCameraLightModifiers &lightModifier, const Vector &interiorLightColor, Float interiorAmount ) const;
	Vector GetOffsetForwardRightUp( const SCameraLightModifiers &lightModifier ) const;
	Float  GetAttenuation( const SCameraLightModifiers &lightModifier ) const;
};


/// Camera lights setup parameters
class CEnvCameraLightsSetupParameters
{
	DECLARE_RTTI_STRUCT( CEnvCameraLightsSetupParameters );

public:
	bool		 						m_activated;
	CEnvCameraLightParameters			m_gameplayLight0;
	CEnvCameraLightParameters			m_gameplayLight1;
	CEnvCameraLightParameters			m_sceneLight0;
	CEnvCameraLightParameters			m_sceneLight1;
	CEnvCameraLightParameters			m_dialogLight0;
	CEnvCameraLightParameters			m_dialogLight1;
	CEnvCameraLightParameters			m_interiorLight0;
	CEnvCameraLightParameters			m_interiorLight1;
	SSimpleCurve						m_playerInInteriorLightsScale;
	SSimpleCurve						m_sceneLightColorInterior0;
	SSimpleCurve						m_sceneLightColorInterior1;
	SSimpleCurve						m_cameraLightsNonCharacterScale;

public:
	CEnvCameraLightsSetupParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvCameraLightsSetupParameters &src, const CEnvCameraLightsSetupParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvCameraLightsSetupParameters );
PROPERTY_EDIT( m_activated,				TXT("m_activated") );
PROPERTY_EDIT( m_gameplayLight0,		TXT("m_gameplayLight0") );
PROPERTY_EDIT( m_gameplayLight1,		TXT("m_gameplayLight1") );
PROPERTY_EDIT( m_sceneLight0,			TXT("m_sceneLight0") );
PROPERTY_EDIT( m_sceneLight1,			TXT("m_sceneLight1") );
PROPERTY_EDIT( m_dialogLight0,			TXT("m_dialogLight0") );
PROPERTY_EDIT( m_dialogLight1,			TXT("m_dialogLight1") );
PROPERTY_EDIT( m_interiorLight0,		TXT("m_interiorLight0") );
PROPERTY_EDIT( m_interiorLight1,		TXT("m_interiorLight1") );
PROPERTY_EDIT( m_playerInInteriorLightsScale,	TXT("m_playerInInteriorLightsScale") );
PROPERTY_EDIT( m_sceneLightColorInterior0,	TXT("m_sceneLightColorInterior0") );
PROPERTY_EDIT( m_sceneLightColorInterior1,	TXT("m_sceneLightColorInterior1") );
PROPERTY_EDIT( m_cameraLightsNonCharacterScale,	TXT("m_cameraLightsNonCharacterScale") );
END_CLASS_RTTI();

/// Canera lights setup at given point in time
class CEnvCameraLightsSetupParametersAtPoint
{
public:
	bool								m_activated;
	CEnvCameraLightParametersAtPoint	m_gameplayLight0;
	CEnvCameraLightParametersAtPoint	m_gameplayLight1;
	CEnvCameraLightParametersAtPoint	m_sceneLight0;
	CEnvCameraLightParametersAtPoint	m_sceneLight1;
	CEnvCameraLightParametersAtPoint	m_dialogLight0;
	CEnvCameraLightParametersAtPoint	m_dialogLight1;
	CEnvCameraLightParametersAtPoint	m_interiorLight0;
	CEnvCameraLightParametersAtPoint	m_interiorLight1;
	SSimpleCurvePoint					m_playerInInteriorLightsScale;
	SSimpleCurvePoint					m_sceneLightColorInterior0;
	SSimpleCurvePoint					m_sceneLightColorInterior1;
	SSimpleCurvePoint					m_cameraLightsNonCharacterScale;

public:
	CEnvCameraLightsSetupParametersAtPoint() {};
	CEnvCameraLightsSetupParametersAtPoint( const CEnvCameraLightsSetupParameters& source );
	void SetDisabledValues();
};


/// Reflection probes generation parameters
class CEnvReflectionProbesGenParameters
{
	DECLARE_RTTI_STRUCT( CEnvReflectionProbesGenParameters );

public:
	bool		 m_activated;
	SSimpleCurve m_colorAmbient;
	SSimpleCurve m_colorSceneMul;
	SSimpleCurve m_colorSceneAdd;
	SSimpleCurve m_colorSkyMul;
	SSimpleCurve m_colorSkyAdd;
	SSimpleCurve m_remapOffset;
	SSimpleCurve m_remapStrength;
	SSimpleCurve m_remapClamp;

public:
	CEnvReflectionProbesGenParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvReflectionProbesGenParameters &src, const CEnvReflectionProbesGenParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvReflectionProbesGenParameters );
PROPERTY_EDIT( m_activated,				TXT("m_activated") );
PROPERTY_EDIT( m_colorAmbient,			TXT("m_colorAmbient") );
PROPERTY_EDIT( m_colorSceneMul,			TXT("m_colorSceneMul") );
PROPERTY_EDIT( m_colorSceneAdd,			TXT("m_colorSceneAdd") );
PROPERTY_EDIT( m_colorSkyMul,			TXT("m_colorSkyMul") );
PROPERTY_EDIT( m_colorSkyAdd,			TXT("m_colorSkyAdd") );
PROPERTY_EDIT( m_remapOffset,			TXT("m_remapOffset") );
PROPERTY_EDIT( m_remapStrength,			TXT("m_remapStrength") );
PROPERTY_EDIT( m_remapClamp,			TXT("m_remapClamp") );
END_CLASS_RTTI();

///Reflection probes generation parameters at given point in time ( usualy now :P )
class CEnvReflectionProbesGenParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_colorAmbient;
	SSimpleCurvePoint	m_colorSceneMul;
	SSimpleCurvePoint	m_colorSceneAdd;
	SSimpleCurvePoint	m_colorSkyMul;
	SSimpleCurvePoint	m_colorSkyAdd;
	SSimpleCurvePoint	m_remapOffset;
	SSimpleCurvePoint	m_remapStrength;
	SSimpleCurvePoint	m_remapClamp;

public:
	CEnvReflectionProbesGenParametersAtPoint() {};
	CEnvReflectionProbesGenParametersAtPoint( const CEnvReflectionProbesGenParameters& source );
	void SetPreviewPanelValues();
};


/// Interior Fallback Parameters
class CEnvInteriorFallbackParameters
{
	DECLARE_RTTI_STRUCT( CEnvInteriorFallbackParameters );

public:
	bool		 m_activated;
	SSimpleCurve m_colorAmbientMul;
	SSimpleCurve m_colorReflectionLow;
	SSimpleCurve m_colorReflectionMiddle;
	SSimpleCurve m_colorReflectionHigh;

public:
	CEnvInteriorFallbackParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvInteriorFallbackParameters &src, const CEnvInteriorFallbackParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvInteriorFallbackParameters );
PROPERTY_EDIT( m_activated,				TXT("m_activated") );
PROPERTY_EDIT( m_colorAmbientMul,		TXT("m_colorAmbientMul") );
PROPERTY_EDIT( m_colorReflectionLow,	TXT("m_colorReflectionLow") );
PROPERTY_EDIT( m_colorReflectionMiddle,	TXT("m_colorReflectionMiddle") );
PROPERTY_EDIT( m_colorReflectionHigh,	TXT("m_colorReflectionHigh") );
END_CLASS_RTTI();

/// Interior Fallback parameters at given point in time
class CEnvInteriorFallbackParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_colorAmbientMul;
	SSimpleCurvePoint	m_colorReflectionLow;
	SSimpleCurvePoint	m_colorReflectionMiddle;
	SSimpleCurvePoint	m_colorReflectionHigh;

public:
	CEnvInteriorFallbackParametersAtPoint() {};
	CEnvInteriorFallbackParametersAtPoint( const CEnvInteriorFallbackParameters& source );
};


/// Global light related environment parameters
class CEnvGlobalLightParameters
{
	DECLARE_RTTI_STRUCT( CEnvGlobalLightParameters );

public:
	bool		 m_activated;
	bool		 m_activatedGlobalLightActivated;
	Float		 m_globalLightActivated;
	bool		 m_activatedActivatedFactorLightDir;
	Float		 m_activatedFactorLightDir;
	SSimpleCurve m_sunColor;
	SSimpleCurve m_sunColorLightSide;
	SSimpleCurve m_sunColorLightOppositeSide;
	SSimpleCurve m_sunColorCenterArea;
	SSimpleCurve m_sunColorSidesMargin;
	SSimpleCurve m_sunColorBottomHeight;
	SSimpleCurve m_sunColorTopHeight;
	
	SSimpleCurve m_forcedLightDirAnglesYaw;
	SSimpleCurve m_forcedLightDirAnglesPitch;
	SSimpleCurve m_forcedLightDirAnglesRoll;
	SSimpleCurve m_forcedSunDirAnglesYaw;
	SSimpleCurve m_forcedSunDirAnglesPitch;
	SSimpleCurve m_forcedSunDirAnglesRoll;
	SSimpleCurve m_forcedMoonDirAnglesYaw;
	SSimpleCurve m_forcedMoonDirAnglesPitch;
	SSimpleCurve m_forcedMoonDirAnglesRoll;

	SSimpleCurve m_translucencyViewDependency;
	SSimpleCurve m_translucencyBaseFlatness;
	SSimpleCurve m_translucencyFlatBrightness;
	SSimpleCurve m_translucencyGainBrightness;
	SSimpleCurve m_translucencyFresnelScaleLight;
	SSimpleCurve m_translucencyFresnelScaleReflection;
	
	CEnvAmbientProbesGenParameters		m_envProbeBaseLightingAmbient;
	CEnvReflectionProbesGenParameters	m_envProbeBaseLightingReflection;

	SSimpleCurve m_charactersLightingBoostAmbientLight;
	SSimpleCurve m_charactersLightingBoostAmbientShadow;
	SSimpleCurve m_charactersLightingBoostReflectionLight;
	SSimpleCurve m_charactersLightingBoostReflectionShadow;

	SSimpleCurve m_charactersEyeBlicksColor;
	SSimpleCurve m_charactersEyeBlicksShadowedScale;

	SSimpleCurve m_envProbeAmbientScaleLight;
	SSimpleCurve m_envProbeAmbientScaleShadow;
	SSimpleCurve m_envProbeReflectionScaleLight;
	SSimpleCurve m_envProbeReflectionScaleShadow;
	SSimpleCurve m_envProbeDistantScaleFactor;
	
public:
	CEnvGlobalLightParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvGlobalLightParameters &src, const CEnvGlobalLightParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvGlobalLightParameters );
	PROPERTY_EDIT( m_activated,					TXT("m_activated") );
	PROPERTY_EDIT( m_activatedGlobalLightActivated, TXT("m_activatedGlobalLightActivated") );
	PROPERTY_EDIT( m_globalLightActivated,		TXT("m_globalLightActivated") );
	PROPERTY_EDIT( m_activatedActivatedFactorLightDir, TXT("m_activatedActivatedFactorLightDir") );
	PROPERTY_EDIT( m_activatedFactorLightDir,	TXT("m_activatedFactorLightDir") );
	PROPERTY_EDIT( m_sunColor,					TXT("m_sunColor") );
	PROPERTY_EDIT( m_sunColorLightSide,			TXT("m_sunColorLightSide") );
	PROPERTY_EDIT( m_sunColorLightOppositeSide,	TXT("m_sunColorLightOppositeSide") );
	PROPERTY_EDIT( m_sunColorCenterArea,	TXT("m_sunColorCenterArea") );
	PROPERTY_EDIT( m_sunColorSidesMargin,	TXT("m_sunColorSidesMargin") );
	PROPERTY_EDIT( m_sunColorBottomHeight,	TXT("m_sunColorBottomHeight") );
	PROPERTY_EDIT( m_sunColorTopHeight,	TXT("m_sunColorTopHeight") );
	
	PROPERTY_EDIT( m_forcedLightDirAnglesYaw,	TXT("m_forcedLightDirAnglesYaw") );
	PROPERTY_EDIT( m_forcedLightDirAnglesPitch,	TXT("m_forcedLightDirAnglesPitch") );
	PROPERTY_EDIT( m_forcedLightDirAnglesRoll,	TXT("m_forcedLightDirAnglesRoll") );

	PROPERTY_EDIT( m_forcedSunDirAnglesYaw,		TXT("m_forcedSunDirAnglesYaw") );
	PROPERTY_EDIT( m_forcedSunDirAnglesPitch,	TXT("m_forcedSunDirAnglesPitch") );
	PROPERTY_EDIT( m_forcedSunDirAnglesRoll,	TXT("m_forcedSunDirAnglesRoll") );

	PROPERTY_EDIT( m_forcedMoonDirAnglesYaw,	TXT("m_forcedMoonDirAnglesYaw") );
	PROPERTY_EDIT( m_forcedMoonDirAnglesPitch,	TXT("m_forcedMoonDirAnglesPitch") );
	PROPERTY_EDIT( m_forcedMoonDirAnglesRoll,	TXT("m_forcedMoonDirAnglesRoll") );

	PROPERTY_EDIT( m_translucencyViewDependency,	TXT("Translucency View Dependency" ) );
	PROPERTY_EDIT( m_translucencyBaseFlatness,		TXT("Translucency Base Flatness" ) );
	PROPERTY_EDIT( m_translucencyFlatBrightness,	TXT("Translucency Flat Brightness" ) );
	PROPERTY_EDIT( m_translucencyGainBrightness,	TXT("Translucency Gain Brightness" ) );
	PROPERTY_EDIT( m_translucencyFresnelScaleLight,			TXT("Translucency Fresnel Scale Light" ) );
	PROPERTY_EDIT( m_translucencyFresnelScaleReflection,	TXT("Translucency Fresnel Scale Reflection" ) );

	PROPERTY_EDIT( m_envProbeBaseLightingAmbient,				TXT("m_envProbeBaseLightingAmbient" ) );
	PROPERTY_EDIT( m_envProbeBaseLightingReflection,			TXT("m_envProbeBaseLightingReflection" ) );

	PROPERTY_EDIT( m_charactersLightingBoostAmbientLight,		TXT("Characters lighting boost ambient light") );
	PROPERTY_EDIT( m_charactersLightingBoostAmbientShadow,		TXT("Characters lighting boost ambient shadow") );
	PROPERTY_EDIT( m_charactersLightingBoostReflectionLight,	TXT("Characters lighting boost reflection light") );
	PROPERTY_EDIT( m_charactersLightingBoostReflectionShadow,	TXT("Characters lighting boost reflection shadow") );

	PROPERTY_EDIT( m_charactersEyeBlicksColor,			TXT("Characters eye blicks color") );
	PROPERTY_EDIT( m_charactersEyeBlicksShadowedScale,	TXT("Characters eye blicks scale in shadow") );

	PROPERTY_EDIT( m_envProbeAmbientScaleLight,		TXT("EnvProbe final ambient scale light" ) );
	PROPERTY_EDIT( m_envProbeAmbientScaleShadow,	TXT("EnvProbe final ambient scale shadow" ) );
	PROPERTY_EDIT( m_envProbeReflectionScaleLight,	TXT("EnvProbe final reflection scale light" ) );
	PROPERTY_EDIT( m_envProbeReflectionScaleShadow,	TXT("EnvProbe final reflection scale shadow" ) );
	PROPERTY_EDIT( m_envProbeDistantScaleFactor,	TXT("EnvProbe distant scale factor" ) );

END_CLASS_RTTI();

/// Global light related environment parameters at given point in time ( usualy now :P )
class CEnvGlobalLightParametersAtPoint
{
public:
	bool				m_activated;
	bool				m_activatedGlobalLightActivated;
	Float				m_globalLightActivated;
	bool				m_activatedActivatedFactorLightDir;
	Float				m_activatedFactorLightDir;
	SSimpleCurvePoint	m_sunColor;
	SSimpleCurvePoint	m_sunColorLightSide;
	SSimpleCurvePoint	m_sunColorLightOppositeSide;
	SSimpleCurvePoint	m_sunColorCenterArea;
	SSimpleCurvePoint	m_sunColorSidesMargin;
	SSimpleCurvePoint	m_sunColorBottomHeight;
	SSimpleCurvePoint	m_sunColorTopHeight;
	
	EulerAngles			m_forcedLightDirAngles;
	EulerAngles			m_forcedSunDirAngles;
	EulerAngles			m_forcedMoonDirAngles;

	SSimpleCurvePoint	m_translucencyViewDependency;
	SSimpleCurvePoint	m_translucencyBaseFlatness;
	SSimpleCurvePoint	m_translucencyFlatBrightness;
	SSimpleCurvePoint	m_translucencyGainBrightness;
	SSimpleCurvePoint	m_translucencyFresnelScaleLight;
	SSimpleCurvePoint	m_translucencyFresnelScaleReflection;
	
	CEnvAmbientProbesGenParametersAtPoint		m_envProbeBaseLightingAmbient;
	CEnvReflectionProbesGenParametersAtPoint	m_envProbeBaseLightingReflection;

	SSimpleCurvePoint	m_charactersLightingBoostAmbientLight;
	SSimpleCurvePoint	m_charactersLightingBoostAmbientShadow;
	SSimpleCurvePoint	m_charactersLightingBoostReflectionLight;
	SSimpleCurvePoint	m_charactersLightingBoostReflectionShadow;

	SSimpleCurvePoint	m_charactersEyeBlicksColor;
	SSimpleCurvePoint	m_charactersEyeBlicksShadowedScale;

	SSimpleCurvePoint	m_envProbeAmbientScaleLight;
	SSimpleCurvePoint	m_envProbeAmbientScaleShadow;
	SSimpleCurvePoint	m_envProbeReflectionScaleLight;
	SSimpleCurvePoint	m_envProbeReflectionScaleShadow;
	SSimpleCurvePoint	m_envProbeDistantScaleFactor;
	
public:
	CEnvGlobalLightParametersAtPoint() {};
	CEnvGlobalLightParametersAtPoint( const CEnvGlobalLightParameters& source );
};

/// Speed Tree random colors related parameters
class CEnvSpeedTreeRandomColorParameters
{
	DECLARE_RTTI_STRUCT( CEnvSpeedTreeRandomColorParameters );

public:
	SSimpleCurve m_luminanceWeights;
	SSimpleCurve m_randomColor0;
	SSimpleCurve m_saturation0;
	SSimpleCurve m_randomColor1;
	SSimpleCurve m_saturation1;
	SSimpleCurve m_randomColor2;
	SSimpleCurve m_saturation2;

public:
	CEnvSpeedTreeRandomColorParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvSpeedTreeRandomColorParameters &src, const CEnvSpeedTreeRandomColorParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvSpeedTreeRandomColorParameters );
PROPERTY_EDIT( m_luminanceWeights,		TXT("m_luminanceWeights") );
PROPERTY_EDIT( m_randomColor0,			TXT("m_randomColor0") );
PROPERTY_EDIT( m_saturation0,			TXT("m_saturation0") );
PROPERTY_EDIT( m_randomColor1,			TXT("m_randomColor1") );
PROPERTY_EDIT( m_saturation1,			TXT("m_saturation1") );
PROPERTY_EDIT( m_randomColor2,			TXT("m_randomColor2") );
PROPERTY_EDIT( m_saturation2,			TXT("m_saturation2") );
END_CLASS_RTTI();

/// Speed Tree random colors related parameters at given point in time ( usualy now :P )
class CEnvSpeedTreeRandomColorParametersAtPoint
{
public:
	SSimpleCurvePoint m_luminanceWeights;
	SSimpleCurvePoint m_randomColor0;
	SSimpleCurvePoint m_saturation0;
	SSimpleCurvePoint m_randomColor1;
	SSimpleCurvePoint m_saturation1;
	SSimpleCurvePoint m_randomColor2;
	SSimpleCurvePoint m_saturation2;

public:
	CEnvSpeedTreeRandomColorParametersAtPoint() {};
	CEnvSpeedTreeRandomColorParametersAtPoint( const CEnvSpeedTreeRandomColorParameters& source );

public:
	void BuildShaderParams( Vector &outLumWeights, Vector &outColorParams0, Vector &outColorParams1, Vector &outColorParams2 ) const;
};

/// Speed Tree related environment parameters
class CEnvSpeedTreeParameters
{
	DECLARE_RTTI_STRUCT( CEnvSpeedTreeParameters );

public:
	bool		 m_activated;
	SSimpleCurve m_diffuse;
	SSimpleCurve m_specularScale;
	SSimpleCurve m_translucencyScale;
	SSimpleCurve m_ambientOcclusionScale;
	SSimpleCurve m_billboardsColor;
	SSimpleCurve m_billboardsTranslucency;
	CEnvSpeedTreeRandomColorParameters m_randomColorsTrees;
	CEnvSpeedTreeRandomColorParameters m_randomColorsBranches;
	CEnvSpeedTreeRandomColorParameters m_randomColorsGrass;
	SSimpleCurve m_randomColorsFallback;
	SSimpleCurve m_pigmentBrightness;
	SSimpleCurve m_pigmentFloodStartDist;
	SSimpleCurve m_pigmentFloodRange;
	SSimpleCurve m_billboardsLightBleed;

public:
	CEnvSpeedTreeParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvSpeedTreeParameters &src, const CEnvSpeedTreeParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvSpeedTreeParameters );
PROPERTY_EDIT( m_activated,					TXT("m_activated") );
PROPERTY_EDIT( m_diffuse,					TXT("SpeedTree Diffuse color") );
PROPERTY_EDIT( m_specularScale,				TXT("SpeedTree Specular") );
PROPERTY_EDIT( m_translucencyScale,			TXT("SpeedTree Translucency") );
PROPERTY_EDIT( m_ambientOcclusionScale,		TXT("SpeedTree Ambient Occlusion scale") );
PROPERTY_EDIT( m_billboardsColor,			TXT("SpeedTree Billboards color") );
PROPERTY_EDIT( m_billboardsTranslucency,	TXT("SpeedTree Billboards translucency") );
PROPERTY_EDIT( m_randomColorsTrees,			TXT("m_randomColorsTrees") );
PROPERTY_EDIT( m_randomColorsBranches,		TXT("m_randomColorsBranches") );
PROPERTY_EDIT( m_randomColorsGrass,			TXT("m_randomColorsGrass") );
PROPERTY_EDIT( m_randomColorsFallback,		TXT("m_randomColorsFallback") );
PROPERTY_EDIT( m_pigmentBrightness,			TXT("m_pigmentBrightness") );
PROPERTY_EDIT( m_pigmentFloodStartDist,		TXT("m_pigmentFloodStartDist") );
PROPERTY_EDIT( m_pigmentFloodRange,			TXT("m_pigmentFloodRange") );
PROPERTY_EDIT( m_billboardsLightBleed,		TXT("m_billboardsLightBleed") );
END_CLASS_RTTI();

/// Speed Tree related environment parameters at given point in time ( usualy now :P )
class CEnvSpeedTreeParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_diffuse;
	SSimpleCurvePoint	m_specularScale;
	SSimpleCurvePoint	m_translucencyScale;
	SSimpleCurvePoint	m_ambientOcclusionScale;
	SSimpleCurvePoint	m_billboardsColor;
	SSimpleCurvePoint	m_billboardsTranslucency;
	CEnvSpeedTreeRandomColorParametersAtPoint	m_randomColorsTrees;
	CEnvSpeedTreeRandomColorParametersAtPoint	m_randomColorsBranches;
	CEnvSpeedTreeRandomColorParametersAtPoint	m_randomColorsGrass;
	SSimpleCurvePoint m_randomColorsFallback;
	SSimpleCurvePoint m_pigmentBrightness;
	SSimpleCurvePoint m_pigmentFloodStartDist;
	SSimpleCurvePoint m_pigmentFloodRange;
	SSimpleCurvePoint m_billboardsLightBleed;

public:
	CEnvSpeedTreeParametersAtPoint() {};
	CEnvSpeedTreeParametersAtPoint( const CEnvSpeedTreeParameters& source );
};

// Tone mapping curve parameters
class CEnvToneMappingCurveParameters
{
	DECLARE_RTTI_STRUCT( CEnvToneMappingCurveParameters );

public:
	SSimpleCurve m_shoulderStrength;
	SSimpleCurve m_linearStrength;
	SSimpleCurve m_linearAngle;
	SSimpleCurve m_toeStrength;
	SSimpleCurve m_toeNumerator;
	SSimpleCurve m_toeDenominator;

public:
	CEnvToneMappingCurveParameters ( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvToneMappingCurveParameters &src, const CEnvToneMappingCurveParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvToneMappingCurveParameters );
PROPERTY_EDIT( m_shoulderStrength,		TXT("m_shoulderStrength") );	
PROPERTY_EDIT( m_linearStrength,		TXT("m_linearStrength") );
PROPERTY_EDIT( m_linearAngle,			TXT("m_linearAngle") );
PROPERTY_EDIT( m_toeStrength,			TXT("m_toeStrength") );
PROPERTY_EDIT( m_toeNumerator,			TXT("m_toeNumerator") );
PROPERTY_EDIT( m_toeDenominator,		TXT("m_toeDenominator") );
END_CLASS_RTTI();

class CEnvToneMappingCurveParametersAtPoint
{
public:
	SSimpleCurvePoint m_shoulderStrength;
	SSimpleCurvePoint m_linearStrength;
	SSimpleCurvePoint m_linearAngle;
	SSimpleCurvePoint m_toeStrength;
	SSimpleCurvePoint m_toeNumerator;
	SSimpleCurvePoint m_toeDenominator;

public:
	CEnvToneMappingCurveParametersAtPoint() {};
	CEnvToneMappingCurveParametersAtPoint( const CEnvToneMappingCurveParameters& source );

public:
	/// Sets values so that curve is as near as linear as possible (it's not guaranteed to be EXACTLY linear though)
	CEnvToneMappingCurveParametersAtPoint& SetLinearApproximate();
};

/// Tone mapping parameters
class CEnvToneMappingParameters
{
	DECLARE_RTTI_STRUCT( CEnvToneMappingParameters );

public:
	bool			m_activated;
	SSimpleCurve	m_skyLuminanceCustomValue;
	SSimpleCurve	m_skyLuminanceCustomAmount;
	SSimpleCurve	m_luminanceLimitShape;
	SSimpleCurve	m_luminanceLimitMin;
	SSimpleCurve	m_luminanceLimitMax;
	SSimpleCurve	m_rejectThreshold;
	SSimpleCurve	m_rejectSmoothExtent;
	CEnvToneMappingCurveParameters	m_newToneMapCurveParameters;
	SSimpleCurve	m_newToneMapWhitepoint;
	SSimpleCurve	m_newToneMapPostScale;
	SSimpleCurve	m_exposureScale;
	SSimpleCurve	m_postScale;

public:
	CEnvToneMappingParameters ( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvToneMappingParameters &src, const CEnvToneMappingParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvToneMappingParameters );
	PROPERTY_EDIT( m_activated,				TXT("m_activated") );	
	PROPERTY_EDIT( m_skyLuminanceCustomValue,	TXT("m_skyLuminanceCustomValue") );
	PROPERTY_EDIT( m_skyLuminanceCustomAmount,	TXT("m_skyLuminanceCustomAmount") );
	PROPERTY_EDIT( m_luminanceLimitShape,	TXT("m_luminanceLimitShape") );
	PROPERTY_EDIT( m_luminanceLimitMin,		TXT("m_luminanceLimitMin") );
	PROPERTY_EDIT( m_luminanceLimitMax,		TXT("m_luminanceLimitMax") );
	PROPERTY_EDIT( m_rejectThreshold,		TXT("m_rejectThreshold") );
	PROPERTY_EDIT( m_rejectSmoothExtent,	TXT("m_rejectSmoothExtent") );
	PROPERTY_EDIT( m_newToneMapCurveParameters,	TXT("m_newToneMapCurveParameters") );
	PROPERTY_EDIT( m_newToneMapWhitepoint,	TXT("m_newToneMapWhitepoint") );
	PROPERTY_EDIT( m_newToneMapPostScale,	TXT("m_newToneMapPostScale") );
	PROPERTY_EDIT( m_exposureScale,			TXT("m_exposureScale") );
	PROPERTY_EDIT( m_postScale,				TXT("m_postScale") );
END_CLASS_RTTI();

class CEnvToneMappingParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_skyLuminanceCustomValue;
	SSimpleCurvePoint	m_skyLuminanceCustomAmount;
	SSimpleCurvePoint	m_luminanceLimitShape;
	SSimpleCurvePoint	m_luminanceLimitMin;
	SSimpleCurvePoint	m_luminanceLimitMax;
	SSimpleCurvePoint	m_rejectThreshold;
	SSimpleCurvePoint	m_rejectSmoothExtent;
	CEnvToneMappingCurveParametersAtPoint m_newToneMapCurveParameters;
	SSimpleCurvePoint	m_newToneMapWhitepoint;
	SSimpleCurvePoint	m_newToneMapPostScale;
	SSimpleCurvePoint	m_exposureScale;
	SSimpleCurvePoint	m_postScale;

public:
	CEnvToneMappingParametersAtPoint() {};
	CEnvToneMappingParametersAtPoint( const CEnvToneMappingParameters& source );
};

/// New bloom parameters
class CEnvBloomNewParameters
{
	DECLARE_RTTI_STRUCT( CEnvBloomNewParameters );

public:
	bool			m_activated;
	SSimpleCurve	m_brightPassWeights;
	SSimpleCurve	m_color;
	SSimpleCurve	m_dirtColor;
	SSimpleCurve	m_threshold;
	SSimpleCurve	m_thresholdRange;
	SSimpleCurve	m_brightnessMax;
	SSimpleCurve	m_shaftsColor;
	SSimpleCurve	m_shaftsRadius;
	SSimpleCurve	m_shaftsShapeExp;
	SSimpleCurve	m_shaftsShapeInvSquare;
	SSimpleCurve	m_shaftsThreshold;
	SSimpleCurve	m_shaftsThresholdRange;

public:
	CEnvBloomNewParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	///Import daypoint value
	void ImportDayPointValue( const CEnvBloomNewParameters &src, const CEnvBloomNewParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvBloomNewParameters );
PROPERTY_EDIT( m_activated,				TXT("m_activated") );	
PROPERTY_EDIT( m_brightPassWeights,		TXT("m_brightPassWeights") );
PROPERTY_EDIT( m_color,					TXT("m_color") );	
PROPERTY_EDIT( m_dirtColor,				TXT("m_dirtColor") );	
PROPERTY_EDIT( m_threshold,				TXT("m_threshold") );
PROPERTY_EDIT( m_thresholdRange,		TXT("m_thresholdRange") );
PROPERTY_EDIT( m_brightnessMax,			TXT("m_brightnessMax") );
PROPERTY_EDIT( m_shaftsColor,			TXT("m_shaftsColor") );
PROPERTY_EDIT( m_shaftsRadius,			TXT("m_shaftsRadius") );
PROPERTY_EDIT( m_shaftsShapeExp,		TXT("m_shaftsShapeExp") );
PROPERTY_EDIT( m_shaftsShapeInvSquare,	TXT("m_shaftsShapeInvSquare") );
PROPERTY_EDIT( m_shaftsThreshold,		TXT("m_shaftsThreshold") );
PROPERTY_EDIT( m_shaftsThresholdRange,	TXT("m_shaftsThresholdRange") );
END_CLASS_RTTI();

class CEnvBloomNewParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_brightPassWeights;
	SSimpleCurvePoint	m_color;
	SSimpleCurvePoint	m_dirtColor;
	SSimpleCurvePoint	m_threshold;
	SSimpleCurvePoint	m_thresholdRange;
	SSimpleCurvePoint	m_brightnessMax;
	SSimpleCurvePoint	m_shaftsColor;
	SSimpleCurvePoint	m_shaftsRadius;
	SSimpleCurvePoint	m_shaftsShapeExp;
	SSimpleCurvePoint	m_shaftsShapeInvSquare;
	SSimpleCurvePoint	m_shaftsThreshold;
	SSimpleCurvePoint	m_shaftsThresholdRange;

public:
	CEnvBloomNewParametersAtPoint() {};
	CEnvBloomNewParametersAtPoint( const CEnvBloomNewParameters& source );

	// Is camera dirt exposed
	Bool IsCameraDirtExposed( const SDayPointEnvironmentParams &dayPointParams ) const;

	// Is bloom exposed (parameters related to it don't disable bloom)
	Bool IsBloomExposed( const SDayPointEnvironmentParams &dayPointParams ) const;

	// Is shafts exposed
	Bool IsShaftsExposed( const SDayPointEnvironmentParams &dayPointParams ) const;
};

/// Distance range parameters
class CEnvDistanceRangeParameters
{
	DECLARE_RTTI_STRUCT( CEnvDistanceRangeParameters );

public:
	bool		 m_activated;
	SSimpleCurve m_distance;
	SSimpleCurve m_range;

public:
	CEnvDistanceRangeParameters ( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvDistanceRangeParameters &src, const CEnvDistanceRangeParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvDistanceRangeParameters );
	PROPERTY_EDIT( m_activated,			TXT("m_activated") );
	PROPERTY_EDIT( m_distance,			TXT("m_distance") );
	PROPERTY_EDIT( m_range,				TXT("m_range") );
END_CLASS_RTTI();

class CEnvDistanceRangeParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_distance;
	SSimpleCurvePoint	m_range;

public:
	CEnvDistanceRangeParametersAtPoint() {};
	CEnvDistanceRangeParametersAtPoint( const CEnvDistanceRangeParameters& source );
};

/// Environment auto hide group
enum EEnvAutoHideGroup : CEnum::TValueType
{
	EAHG_None,
	EAHG_Custom0,
	EAHG_Custom1,
	EAHG_Custom2,
	EAHG_Custom3,

	EAHG_Max
};

BEGIN_ENUM_RTTI( EEnvAutoHideGroup );
	ENUM_OPTION( EAHG_None );
	ENUM_OPTION( EAHG_Custom0 );
	ENUM_OPTION( EAHG_Custom1 );
	ENUM_OPTION( EAHG_Custom2 );
	ENUM_OPTION( EAHG_Custom3 );
END_ENUM_RTTI();


/// Transparency color filtering related environment parameters
class CEnvColorModTransparencyParameters
{
	DECLARE_RTTI_STRUCT( CEnvColorModTransparencyParameters );

public:
	bool		 m_activated;
	SSimpleCurve m_commonFarDist;
	SSimpleCurve m_filterNearColor;
	SSimpleCurve m_filterFarColor;
	SSimpleCurve m_contrastNearStrength;
	SSimpleCurve m_contrastFarStrength;
	CEnvDistanceRangeParameters	m_autoHideCustom0;
	CEnvDistanceRangeParameters	m_autoHideCustom1;
	CEnvDistanceRangeParameters	m_autoHideCustom2;
	CEnvDistanceRangeParameters	m_autoHideCustom3;

public:
	CEnvColorModTransparencyParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvColorModTransparencyParameters &src, const CEnvColorModTransparencyParameters &dest, Float lerpFactor, Float time );

	// Get curve for given auto hide group
	const CEnvDistanceRangeParameters& GetParamsForAutoHideGroup( EEnvAutoHideGroup group ) const;
};

BEGIN_CLASS_RTTI( CEnvColorModTransparencyParameters );
	PROPERTY_EDIT( m_activated,						TXT("m_activated") );
	PROPERTY_EDIT( m_commonFarDist,					TXT("m_commonFarDist") );
	PROPERTY_EDIT( m_filterNearColor,				TXT("m_filterNearColor") );
	PROPERTY_EDIT( m_filterFarColor,				TXT("m_filterFarColor") );
	PROPERTY_EDIT( m_contrastNearStrength,			TXT("m_contrastNearStrength") );
	PROPERTY_EDIT( m_contrastFarStrength,			TXT("m_contrastFarStrength") );
	PROPERTY_EDIT( m_autoHideCustom0,				TXT("m_autoHideCustom0") );
	PROPERTY_EDIT( m_autoHideCustom1,				TXT("m_autoHideCustom1") );
	PROPERTY_EDIT( m_autoHideCustom2,				TXT("m_autoHideCustom2") );
	PROPERTY_EDIT( m_autoHideCustom3,				TXT("m_autoHideCustom3") );
END_CLASS_RTTI();

class CEnvColorModTransparencyParametersAtPoint
{
public:
	bool								m_activated;
	SSimpleCurvePoint					m_commonFarDist;
	SSimpleCurvePoint					m_filterNearColor;
	SSimpleCurvePoint					m_filterFarColor;
	SSimpleCurvePoint					m_contrastNearStrength;
	SSimpleCurvePoint					m_contrastFarStrength;
	CEnvDistanceRangeParametersAtPoint	m_autoHideCustom0;
	CEnvDistanceRangeParametersAtPoint	m_autoHideCustom1;
	CEnvDistanceRangeParametersAtPoint	m_autoHideCustom2;
	CEnvDistanceRangeParametersAtPoint	m_autoHideCustom3;

public:
	CEnvColorModTransparencyParametersAtPoint() {};
	CEnvColorModTransparencyParametersAtPoint( const CEnvColorModTransparencyParameters& source );

	// Get curve for given auto hide group
	const CEnvDistanceRangeParametersAtPoint& GetParamsForAutoHideGroup( EEnvAutoHideGroup group ) const;
};

/// Shadows related environment parameters
class CEnvShadowsParameters
{
	DECLARE_RTTI_STRUCT( CEnvShadowsParameters );

public:
	Bool			m_activatedAutoHide;
	SSimpleCurve	m_autoHideBoxSizeMin;
	SSimpleCurve	m_autoHideBoxSizeMax;
	SSimpleCurve	m_autoHideBoxCompMaxX;
	SSimpleCurve	m_autoHideBoxCompMaxY;
	SSimpleCurve	m_autoHideBoxCompMaxZ;
	SSimpleCurve	m_autoHideDistScale;

public:
	CEnvShadowsParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvShadowsParameters &src, const CEnvShadowsParameters &dest, Float lerpFactor, Float time );		
};

BEGIN_CLASS_RTTI( CEnvShadowsParameters );		
	PROPERTY_EDIT( m_activatedAutoHide,				TXT("m_activatedAutoHide") );
	PROPERTY_EDIT( m_autoHideBoxSizeMin,			TXT("m_autoHideBoxSizeMin") );
	PROPERTY_EDIT( m_autoHideBoxSizeMax,			TXT("m_autoHideBoxSizeMax") );
	PROPERTY_EDIT( m_autoHideBoxCompMaxX,			TXT("m_autoHideBoxCompMinX") );
	PROPERTY_EDIT( m_autoHideBoxCompMaxY,			TXT("m_autoHideBoxCompMaxY") );
	PROPERTY_EDIT( m_autoHideBoxCompMaxZ,			TXT("m_autoHideBoxCompMaxZ") );
	PROPERTY_EDIT( m_autoHideDistScale,				TXT("m_autoHideDistScale") );
END_CLASS_RTTI();

class CEnvShadowsParametersAtPoint
{
public:	
	Bool					m_activatedAutoHide;
	SSimpleCurvePoint		m_autoHideBoxSizeMin;
	SSimpleCurvePoint		m_autoHideBoxSizeMax;
	SSimpleCurvePoint		m_autoHideBoxCompMaxX;
	SSimpleCurvePoint		m_autoHideBoxCompMaxY;
	SSimpleCurvePoint		m_autoHideBoxCompMaxZ;
	SSimpleCurvePoint		m_autoHideDistScale;

public:
	CEnvShadowsParametersAtPoint() {};
	CEnvShadowsParametersAtPoint( const CEnvShadowsParameters& source );

};

/// Global fog environment parameters
class CEnvGlobalFogParameters
{
	DECLARE_RTTI_STRUCT( CEnvGlobalFogParameters );

public:

	static Float			m_fogStartOffset;
	static Float			m_fogDensityMultiplier;

public:
	bool			m_fogActivated;
	SSimpleCurve	m_fogAppearDistance;
	SSimpleCurve	m_fogAppearRange;
	SSimpleCurve	m_fogColorFront;
	SSimpleCurve	m_fogColorMiddle;
	SSimpleCurve	m_fogColorBack;
	SSimpleCurve	m_fogDensity;
	SSimpleCurve	m_fogVertOffset;
	SSimpleCurve	m_fogVertDensity;
	SSimpleCurve	m_fogVertDensityLightFront;
	SSimpleCurve	m_fogVertDensityLightBack;
	SSimpleCurve	m_fogSkyDensityScale;
	SSimpleCurve	m_fogSkyVertDensityLightFrontScale;
	SSimpleCurve	m_fogSkyVertDensityLightBackScale;
	SSimpleCurve	m_fogCloudsDensityScale;
	SSimpleCurve	m_fogVertDensityRimRange;
	SSimpleCurve	m_fogDistClamp;
	SSimpleCurve	m_fogFinalExp;	
	SSimpleCurve	m_fogCustomColor;
	SSimpleCurve	m_fogCustomColorStart;
	SSimpleCurve	m_fogCustomColorRange;
	SSimpleCurve	m_fogCustomAmountScale;
	SSimpleCurve	m_fogCustomAmountScaleStart;
	SSimpleCurve	m_fogCustomAmountScaleRange;
	SSimpleCurve	m_aerialColorFront;
	SSimpleCurve	m_aerialColorMiddle;
	SSimpleCurve	m_aerialColorBack;
	SSimpleCurve	m_aerialFinalExp;
	SSimpleCurve	m_ssaoImpactClamp;
	SSimpleCurve	m_ssaoImpactNearValue;
	SSimpleCurve	m_ssaoImpactFarValue;
	SSimpleCurve	m_ssaoImpactNearDistance;
	SSimpleCurve	m_ssaoImpactFarDistance;
	SSimpleCurve	m_distantLightsIntensityScale;

public:
	CEnvGlobalFogParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvGlobalFogParameters &src, const CEnvGlobalFogParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvGlobalFogParameters );
	PROPERTY_EDIT( m_fogActivated,					TXT("m_fogActivated") );
	PROPERTY_EDIT( m_fogAppearDistance,				TXT("m_fogAppearDistance") );
	PROPERTY_EDIT( m_fogAppearRange,				TXT("m_fogAppearRange") );
	PROPERTY_EDIT( m_fogColorFront,					TXT("m_fogColorFront") );
	PROPERTY_EDIT( m_fogColorMiddle,				TXT("m_fogColorMiddle") );
	PROPERTY_EDIT( m_fogColorBack,					TXT("m_fogColorBack") );
	PROPERTY_EDIT( m_fogDensity,					TXT("m_fogDensity") );
	PROPERTY_EDIT( m_fogFinalExp,					TXT("m_fogFinalExp") );
	PROPERTY_EDIT( m_fogDistClamp,					TXT("m_fogDistClamp") );
	PROPERTY_EDIT( m_fogVertOffset,					TXT("m_fogVertOffset") );
	PROPERTY_EDIT( m_fogVertDensity,				TXT("m_fogVertDensity") );
	PROPERTY_EDIT( m_fogVertDensityLightFront,		TXT("m_fogVertDensityLightFront") );
	PROPERTY_EDIT( m_fogVertDensityLightBack,		TXT("m_fogVertDensityLightBack") );
	PROPERTY_EDIT( m_fogSkyDensityScale,			TXT("m_fogSkyDensityScale") );
	PROPERTY_EDIT( m_fogCloudsDensityScale,			TXT("m_fogCloudsDensityScale") );
	PROPERTY_EDIT( m_fogSkyVertDensityLightFrontScale,	TXT("m_fogSkyVertDensityLightFrontScale") );
	PROPERTY_EDIT( m_fogSkyVertDensityLightBackScale,	TXT("m_fogSkyVertDensityLightBackScale") );
	PROPERTY_EDIT( m_fogVertDensityRimRange,		TXT("m_fogVertDensityRimRange") );
	PROPERTY_EDIT( m_fogCustomColor,				TXT("m_fogCustomColor") );
	PROPERTY_EDIT( m_fogCustomColorStart,			TXT("m_fogCustomColorStart") );
	PROPERTY_EDIT( m_fogCustomColorRange,			TXT("m_fogCustomColorRange") );
	PROPERTY_EDIT( m_fogCustomAmountScale,			TXT("m_fogCustomAmountScale") );
	PROPERTY_EDIT( m_fogCustomAmountScaleStart,		TXT("m_fogCustomAmountScaleStart") );
	PROPERTY_EDIT( m_fogCustomAmountScaleRange,		TXT("m_fogCustomAmountScaleRange") );
	PROPERTY_EDIT( m_aerialColorFront,				TXT("m_aerialColorFront") );
	PROPERTY_EDIT( m_aerialColorMiddle,				TXT("m_aerialColorMiddle") );
	PROPERTY_EDIT( m_aerialColorBack,				TXT("m_aerialColorBack") );
	PROPERTY_EDIT( m_aerialFinalExp,				TXT("m_aerialFinalExp") );
	PROPERTY_EDIT( m_ssaoImpactClamp,				TXT("m_ssaoImpactClamp") );
	PROPERTY_EDIT( m_ssaoImpactNearValue,			TXT("m_ssaoImpactNearValue") );
	PROPERTY_EDIT( m_ssaoImpactFarValue,			TXT("m_ssaoImpactFarValue") );
	PROPERTY_EDIT( m_ssaoImpactNearDistance,		TXT("m_ssaoImpactNearDistance") );
	PROPERTY_EDIT( m_ssaoImpactFarDistance,			TXT("m_ssaoImpactFarDistance") );
	PROPERTY_EDIT( m_distantLightsIntensityScale,	TXT("m_distantLightsIntensityScale") );
END_CLASS_RTTI();

class CEnvGlobalFogParametersAtPoint
{
public:
	bool				m_fogActivated;
	SSimpleCurvePoint	m_fogAppearDistance;
	SSimpleCurvePoint	m_fogAppearRange;
	SSimpleCurvePoint	m_fogColorFront;
	SSimpleCurvePoint	m_fogColorMiddle;
	SSimpleCurvePoint	m_fogColorBack;
	SSimpleCurvePoint	m_fogDensity;
	SSimpleCurvePoint	m_fogVertOffset;
	SSimpleCurvePoint	m_fogVertDensity;
	SSimpleCurvePoint	m_fogVertDensityLightFront;
	SSimpleCurvePoint	m_fogVertDensityLightBack;
	SSimpleCurvePoint	m_fogSkyDensityScale;
	SSimpleCurvePoint	m_fogCloudsDensityScale;
	SSimpleCurvePoint	m_fogSkyVertDensityLightFrontScale;
	SSimpleCurvePoint	m_fogSkyVertDensityLightBackScale;
	SSimpleCurvePoint	m_fogVertDensityRimRange;
	SSimpleCurvePoint	m_fogDistClamp;
	SSimpleCurvePoint	m_fogFinalExp;	
	SSimpleCurvePoint	m_fogCustomColor;
	SSimpleCurvePoint	m_fogCustomColorStart;
	SSimpleCurvePoint	m_fogCustomColorRange;
	SSimpleCurvePoint	m_fogCustomAmountScale;
	SSimpleCurvePoint	m_fogCustomAmountScaleStart;
	SSimpleCurvePoint	m_fogCustomAmountScaleRange;
	SSimpleCurvePoint	m_aerialColorFront;
	SSimpleCurvePoint	m_aerialColorMiddle;
	SSimpleCurvePoint	m_aerialColorBack;
	SSimpleCurvePoint	m_aerialFinalExp;
	SSimpleCurvePoint	m_ssaoImpactClamp;
	SSimpleCurvePoint	m_ssaoImpactNearValue;
	SSimpleCurvePoint	m_ssaoImpactFarValue;
	SSimpleCurvePoint	m_ssaoImpactNearDistance;
	SSimpleCurvePoint	m_ssaoImpactFarDistance;
	SSimpleCurvePoint	m_distantLightsIntensityScale;

public:
	CEnvGlobalFogParametersAtPoint() {};
	CEnvGlobalFogParametersAtPoint( const CEnvGlobalFogParameters& source );

	void SetDisabledParams();
};

/// Global sky environment parameters
class CEnvGlobalSkyParameters
{
	DECLARE_RTTI_STRUCT( CEnvGlobalSkyParameters );

public:
	bool			m_activated;
	bool			m_activatedActivateFactor;
	Float			m_activateFactor;
	SSimpleCurve	m_skyColor;
	SSimpleCurve	m_skyColorHorizon;
	SSimpleCurve	m_horizonVerticalAttenuation;
	SSimpleCurve	m_sunColorSky;
	SSimpleCurve	m_sunColorSkyBrightness;
	SSimpleCurve	m_sunAreaSkySize;
	SSimpleCurve	m_sunColorHorizon;
	SSimpleCurve	m_sunColorHorizonHorizontalScale;		// TODO: remove
	SSimpleCurve	m_sunBackHorizonColor;
	SSimpleCurve	m_sunInfluence;
	SSimpleCurve	m_moonColorSky;
	SSimpleCurve	m_moonColorSkyBrightness;
	SSimpleCurve	m_moonAreaSkySize;
	SSimpleCurve	m_moonColorHorizon;
	SSimpleCurve	m_moonColorHorizonHorizontalScale;		// TODO: remove
	SSimpleCurve	m_moonBackHorizonColor;
	SSimpleCurve	m_moonInfluence;
	SSimpleCurve	m_globalSkyBrightness;	

public:
	CEnvGlobalSkyParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvGlobalSkyParameters &src, const CEnvGlobalSkyParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvGlobalSkyParameters );
PROPERTY_EDIT( m_activated,						TXT("m_activated") );
PROPERTY_EDIT( m_activatedActivateFactor,		TXT("m_activatedActivateFactor") );
PROPERTY_EDIT( m_activateFactor,				TXT("m_activateFactor") );
PROPERTY_EDIT( m_skyColor,						TXT("m_skyColor") );	
PROPERTY_EDIT( m_skyColorHorizon,				TXT("m_skyColorHorizon") );	
PROPERTY_EDIT( m_horizonVerticalAttenuation,	TXT("m_horizonVerticalAttenuation") );	
PROPERTY_EDIT( m_sunColorSky,					TXT("m_sunColorSky") );	
PROPERTY_EDIT( m_sunColorSkyBrightness,			TXT("m_sunColorSkyBrightness") );	
PROPERTY_EDIT( m_sunAreaSkySize,				TXT("m_sunAreaSkySize") );	
PROPERTY_EDIT( m_sunColorHorizon,				TXT("m_sunColorHorizon") );	
PROPERTY_EDIT( m_sunColorHorizonHorizontalScale,TXT("m_sunColorHorizonHorizontalScale") );	
PROPERTY_EDIT( m_sunBackHorizonColor,			TXT("m_sunBackHorizonColor") );	
PROPERTY_EDIT( m_sunInfluence,					TXT("m_sunInfluence") );	
PROPERTY_EDIT( m_moonColorSky,						TXT("m_moonColorSky") );	
PROPERTY_EDIT( m_moonColorSkyBrightness,			TXT("m_moonColorSkyBrightness") );	
PROPERTY_EDIT( m_moonAreaSkySize,					TXT("m_moonAreaSkySize") );	
PROPERTY_EDIT( m_moonColorHorizon,					TXT("m_moonColorHorizon") );	
PROPERTY_EDIT( m_moonColorHorizonHorizontalScale,	TXT("m_moonColorHorizonHorizontalScale") );	
PROPERTY_EDIT( m_moonBackHorizonColor,				TXT("m_moonBackHorizonColor") );	
PROPERTY_EDIT( m_moonInfluence,						TXT("m_moonInfluence") );	
PROPERTY_EDIT( m_globalSkyBrightness,			TXT("m_globalSkyBrightness") );

END_CLASS_RTTI();

class CEnvGlobalSkyParametersAtPoint
{
public:
	bool										m_activated;
	bool										m_activatedActivateFactor;
	Float										m_activateFactor;
	SSimpleCurvePoint							m_skyColor;
	SSimpleCurvePoint							m_skyColorHorizon;
	SSimpleCurvePoint							m_horizonVerticalAttenuation;
	SSimpleCurvePoint							m_sunColorSky;
	SSimpleCurvePoint							m_sunColorSkyBrightness;
	SSimpleCurvePoint							m_sunAreaSkySize;
	SSimpleCurvePoint							m_sunColorHorizon;
	SSimpleCurvePoint							m_sunColorHorizonHorizontalScale;
	SSimpleCurvePoint							m_sunBackHorizonColor;
	SSimpleCurvePoint							m_sunInfluence;
	SSimpleCurvePoint							m_moonColorSky;
	SSimpleCurvePoint							m_moonColorSkyBrightness;
	SSimpleCurvePoint							m_moonAreaSkySize;
	SSimpleCurvePoint							m_moonColorHorizon;
	SSimpleCurvePoint							m_moonColorHorizonHorizontalScale;
	SSimpleCurvePoint							m_moonBackHorizonColor;
	SSimpleCurvePoint							m_moonInfluence;
	SSimpleCurvePoint							m_globalSkyBrightness;	

public:
	CEnvGlobalSkyParametersAtPoint() {};
	CEnvGlobalSkyParametersAtPoint( const CEnvGlobalSkyParameters& source );
};


class CEnvDepthOfFieldParameters
{
	DECLARE_RTTI_STRUCT( CEnvDepthOfFieldParameters );

public:
	bool			m_activated;
	SSimpleCurve	m_nearBlurDist;
	SSimpleCurve	m_nearFocusDist;
	SSimpleCurve	m_farFocusDist;
	SSimpleCurve	m_farBlurDist;
	SSimpleCurve	m_intensity;
	bool			m_activatedSkyThreshold;
	Float			m_skyThreshold;
	bool			m_activatedSkyRange;
	Float			m_skyRange;

public:
	CEnvDepthOfFieldParameters ( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Is depth of field visible (has visual effect)
	bool IsDofExposed( Float dayCycleProgress ) const;

	// Import daypoint value
	void ImportDayPointValue( const CEnvDepthOfFieldParameters &src, const CEnvDepthOfFieldParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvDepthOfFieldParameters )
	PROPERTY_EDIT( m_activated,			TXT("m_activated") );
	PROPERTY_EDIT( m_nearBlurDist,		TXT("m_nearBlurDist") );
	PROPERTY_EDIT( m_nearFocusDist,		TXT("m_nearFocusDist") );
	PROPERTY_EDIT( m_farFocusDist,		TXT("m_farFocusDist") );
	PROPERTY_EDIT( m_farBlurDist,		TXT("m_farBlurDist") );
	PROPERTY_EDIT( m_intensity,			TXT("m_intensity") );
	PROPERTY_EDIT( m_activatedSkyThreshold,	TXT("m_activatedSkyThreshold") );
	PROPERTY_EDIT( m_skyThreshold,			TXT("m_skyThreshold") );
	PROPERTY_EDIT( m_activatedSkyRange,		TXT("m_activatedSkyRange") );
	PROPERTY_EDIT( m_skyRange,				TXT("m_skyRange") );
END_CLASS_RTTI();

class CEnvDepthOfFieldParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_nearBlurDist;
	SSimpleCurvePoint	m_nearFocusDist;
	SSimpleCurvePoint	m_farFocusDist;
	SSimpleCurvePoint	m_farBlurDist;
	SSimpleCurvePoint	m_intensity;
	bool				m_activatedSkyThreshold;
	Float				m_skyThreshold;
	bool				m_activatedSkyRange;
	Float				m_skyRange;

public:
	CEnvDepthOfFieldParametersAtPoint() {};
	CEnvDepthOfFieldParametersAtPoint( const CEnvDepthOfFieldParameters& source );

	// Is depth of field visible (has visual effect)
	bool IsDofExposed() const;
};

class CEnvNVSSAOParameters
{
	DECLARE_RTTI_STRUCT( CEnvNVSSAOParameters );

public:
	Bool			m_activated;

	// Parameters derived from HBAO+
	SSimpleCurve	m_radius;				// The AO radius in meters
	SSimpleCurve	m_bias;					// To hide low-tessellation artifacts // 0.0~0.5
	SSimpleCurve	m_detailStrength;		// Scale factor for the detail AO, the greater the darker // 0.0~2.0
	SSimpleCurve	m_coarseStrength;		// Scale factor for the coarse AO, the greater the darker // 0.0~2.0
	SSimpleCurve	m_powerExponent;		// The final AO output is pow(AO, powerExponent)
	SSimpleCurve	m_blurSharpness;		// The higher, the more the blur preserves edges // 0.0~16.0

	// Parameters added by us
	SSimpleCurve	m_valueClamp;				// 
	SSimpleCurve	m_ssaoColor;				// 
	SSimpleCurve	m_nonAmbientInfluence;		// 
	SSimpleCurve	m_translucencyInfluence;	//
	
public:
	CEnvNVSSAOParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvNVSSAOParameters &src, const CEnvNVSSAOParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvNVSSAOParameters )

	PROPERTY_EDIT( 			m_activated,			TXT("m_activated") );

	PROPERTY_EDIT(			m_radius,				TXT("The AO radius in meters") );
	PROPERTY_EDIT_RANGE(	m_bias,					TXT("To hide low-tessellation artifacts"), 0.f, 1.0f );
	PROPERTY_EDIT_RANGE(	m_detailStrength,		TXT("Scale factor for the detail AO, the greater the darker"), 0.f, 20.f );
	PROPERTY_EDIT_RANGE(	m_coarseStrength,		TXT("Scale factor for the coarse AO, the greater the darker"), 0.f, 20.f );
	PROPERTY_EDIT(			m_powerExponent,		TXT("The final AO output is pow(AO, powerExponent)") );
	PROPERTY_EDIT_RANGE(	m_blurSharpness,		TXT("The higher, the more the blur preserves edges"), 0.f, 64.f );
	PROPERTY_EDIT( 			m_valueClamp,			TXT("m_valueClamp") );
	PROPERTY_EDIT( 			m_ssaoColor,			TXT("m_ssaoColor") );
	PROPERTY_EDIT_RANGE( 	m_nonAmbientInfluence,	TXT("m_nonAmbientInfluence"), 0.f, 1.f );
	PROPERTY_EDIT_RANGE( 	m_translucencyInfluence,TXT("m_translucencyInfluence"), 0.f, 1.f );

END_CLASS_RTTI();

class CEnvMSSSAOParameters
{
	DECLARE_RTTI_STRUCT( CEnvMSSSAOParameters );

public:
	Bool m_activated;

	// Parameters derived from MSSSAO+
	// This is necessary to filter out pixel shimmer due to bilateral upsampling with too much lost resolution.  High
	// frequency detail can sometimes not be reconstructed, and the noise filter fills in the missing pixels with the
	// result of the higher resolution SSAO.
	SSimpleCurve m_noiseFilterTolerance;	// suggested values -8 to 0
	SSimpleCurve m_blurTolerance;			// suggested values -8.0f to -1.0f
	SSimpleCurve m_upsampleTolerance;		// suggested values -12.0f to -1.0f

	// Controls how aggressive to fade off samples that occlude spheres but by so much as to be unreliable.
	// This is what gives objects a dark halo around them when placed in front of a wall.  If you want to
	// fade off the halo, boost your rejection falloff.  The tradeoff is that it reduces overall AO.
	SSimpleCurve m_rejectionFalloff;		// suggested values 1.0f to 10.0f

	// The higher quality modes blend wide and narrow sampling patterns.  The wide
	// pattern is due to deinterleaving and requires blurring.  The narrow pattern is
	// not on a deinterleaved buffer, but it only samples every other pixel.  The blur
	// on it is optional.  If you combine the two before blurring, the narrow will get
	// blurred as well.  This creates a softer effect but can remove any visible noise
	// from having 50% sample coverage.
	Bool m_combineResolutionsBeforeBlur;

	// When combining the wide and narrow patterns, a mul() operation can be used or
	// a min() operation.  Multiplication exaggerates the result creating even darker
	// creases.  This is an artistic choice.  I think it looks less natural, but often
	// art teams prefer more exaggerated contrast.  For me, it's more about having the
	// right AO falloff so that it's a smooth gradient rather than falling off precipitously
	// and forming overly dark recesses.
	Bool m_combineResolutionsWithMul;

	SSimpleCurve m_hierarchyDepth;		// valid values 1 to 4
	SSimpleCurve m_normalAOMultiply;	// more to darken
	SSimpleCurve m_normalToDepthBrightnessEqualiser; // includes compensation for fade by depth, possibly ought to be a far plane multiply?

	SSimpleCurve m_normalBackProjectionTolerance; //angle in degrees

public:
	CEnvMSSSAOParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvMSSSAOParameters &src, const CEnvMSSSAOParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvMSSSAOParameters )

	PROPERTY_EDIT( 		 m_activated,						TXT("m_activated") );

	PROPERTY_EDIT_RANGE( m_noiseFilterTolerance,			TXT("Filter out pixel shimmer due to bilateral upsampling."), -8.f, 0.0f );
	PROPERTY_EDIT_RANGE( m_blurTolerance,					TXT("Filter out pixel shimmer through blurring."), -8.f, -1.f );
	PROPERTY_EDIT_RANGE( m_upsampleTolerance,				TXT("Filter out pixel shimmer through upsample tolerance."), -12.f, -1.f );
	PROPERTY_EDIT_RANGE( m_rejectionFalloff,				TXT("Controls how aggressive to fade off samples that occlude spheres."), 1.f, 10.f );
	PROPERTY_EDIT(		 m_combineResolutionsBeforeBlur,	TXT("Combines resolution before blurring.") );
	PROPERTY_EDIT( 		 m_combineResolutionsWithMul,		TXT("Combines resolution with multiplication weight.") );
	PROPERTY_EDIT_RANGE( m_hierarchyDepth,					TXT("Hierarchy depth for sampling."), 1.f, 4.f );
	PROPERTY_EDIT_RANGE( m_normalAOMultiply,				TXT("Normal AO weight. More to darken."), 0.f, 12.f );
	PROPERTY_EDIT_RANGE( m_normalToDepthBrightnessEqualiser,TXT("Normal to depth brightness equaliser."), 100.f, 1000.f );
	PROPERTY_EDIT_RANGE( m_normalBackProjectionTolerance,	TXT("Normal back projection tolerance - more to soften SSAO."), 0.f, 90.f );

END_CLASS_RTTI();

class CEnvNVSSAOParametersAtPoint
{
public:
	Bool			m_activated;

	// Parameters derived from HBAO+
	SSimpleCurvePoint	m_radius;				// The AO radius in meters
	SSimpleCurvePoint	m_bias;					// To hide low-tessellation artifacts
	SSimpleCurvePoint	m_detailStrength;		// Scale factor for the detail AO, the greater the darker
	SSimpleCurvePoint	m_coarseStrength;		// Scale factor for the coarse AO, the greater the darker
	SSimpleCurvePoint	m_powerExponent;		// The final AO output is pow(AO, powerExponent)
	SSimpleCurvePoint	m_blurSharpness;		// The higher, the more the blur preserves edges

	SSimpleCurvePoint	m_valueClamp;
	SSimpleCurvePoint	m_ssaoColor;
	SSimpleCurvePoint	m_nonAmbientInfluence;
	SSimpleCurvePoint	m_translucencyInfluence;

public:
	CEnvNVSSAOParametersAtPoint() {};
	CEnvNVSSAOParametersAtPoint( const CEnvNVSSAOParameters& source );
};

class CEnvMSSSAOParametersAtPoint
{
public:
	Bool			m_activated;

	// Parameters derived from MSSSAO+
	SSimpleCurvePoint m_noiseFilterTolerance;
	SSimpleCurvePoint m_blurTolerance;
	SSimpleCurvePoint m_upsampleTolerance;
	SSimpleCurvePoint m_rejectionFalloff;
	Bool m_combineResolutionsBeforeBlur;
	Bool m_combineResolutionsWithMul;
	SSimpleCurvePoint m_hierarchyDepth;
	SSimpleCurvePoint m_normalAOMultiply;
	SSimpleCurvePoint m_normalToDepthBrightnessEqualiser;
	SSimpleCurvePoint m_normalBackProjectionTolerance;
public:
	CEnvMSSSAOParametersAtPoint() {};
	CEnvMSSSAOParametersAtPoint( const CEnvMSSSAOParameters& source );
};

/// Environment parametric balance parameters
class CEnvParametricBalanceParameters
{
	DECLARE_RTTI_STRUCT( CEnvParametricBalanceParameters );

public:	
	SSimpleCurve			m_saturation;
	SSimpleCurve			m_color;

public:
	CEnvParametricBalanceParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvParametricBalanceParameters &src, const CEnvParametricBalanceParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvParametricBalanceParameters )

	PROPERTY_EDIT( m_saturation,			TXT("m_saturation") );
	PROPERTY_EDIT( m_color,					TXT("m_color") );

END_CLASS_RTTI();

class CEnvParametricBalanceParametersAtPoint
{
public:
	SSimpleCurvePoint				m_saturation;
	SSimpleCurvePoint				m_color;

public:
	CEnvParametricBalanceParametersAtPoint() {};
	CEnvParametricBalanceParametersAtPoint( const CEnvParametricBalanceParameters& source );
};

/// Environment final color balance parameters
class CEnvFinalColorBalanceParameters
{
	DECLARE_RTTI_STRUCT( CEnvFinalColorBalanceParameters );

public:
	bool			m_activated;
	bool			m_activatedBalanceMap;
	bool			m_activatedParametricBalance;

	SSimpleCurve	m_vignetteWeights;
	SSimpleCurve	m_vignetteColor;
	SSimpleCurve	m_vignetteOpacity;
	SSimpleCurve	m_chromaticAberrationSize;

	SSimpleCurve					m_balanceMapLerp;
	SSimpleCurve					m_balanceMapAmount;
	TSoftHandle< CBitmapTexture >	m_balanceMap0;
	TSoftHandle< CBitmapTexture >	m_balanceMap1;
	SSimpleCurve					m_balancePostBrightness;

	SSimpleCurve			m_levelsShadows;
	SSimpleCurve			m_levelsMidtones;
	SSimpleCurve			m_levelsHighlights;
	SSimpleCurve			m_midtoneRangeMin;
	SSimpleCurve			m_midtoneRangeMax;
	SSimpleCurve			m_midtoneMarginMin;
	SSimpleCurve			m_midtoneMarginMax;
	CEnvParametricBalanceParameters	m_parametricBalanceLow;
	CEnvParametricBalanceParameters	m_parametricBalanceMid;
	CEnvParametricBalanceParameters	m_parametricBalanceHigh;

public:
	CEnvFinalColorBalanceParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvFinalColorBalanceParameters &src, const CEnvFinalColorBalanceParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvFinalColorBalanceParameters )

	PROPERTY_EDIT( m_activated,						TXT("m_activated") );
	PROPERTY_EDIT( m_activatedBalanceMap,			TXT("m_activatedBalanceMap") );
	PROPERTY_EDIT( m_activatedParametricBalance,	TXT("m_activatedParametricBalance") );
	PROPERTY_EDIT( m_vignetteWeights,			TXT("m_vignetteWeights") );	
	PROPERTY_EDIT( m_vignetteColor,				TXT("m_vignetteColor") );
	PROPERTY_EDIT( m_vignetteOpacity,			TXT("m_vignetteOpacity") );
	PROPERTY_EDIT( m_chromaticAberrationSize,	TXT("m_chromaticAberrationSize") );
	PROPERTY_EDIT( m_balanceMapLerp,		TXT("m_balanceMapLerp") );
	PROPERTY_EDIT( m_balanceMapAmount,		TXT("m_balanceMapAmount") );
	PROPERTY_EDIT( m_balanceMap0,			TXT("m_balanceMap0") );
	PROPERTY_EDIT( m_balanceMap1,			TXT("m_balanceMap1") );
	PROPERTY_EDIT( m_balancePostBrightness,	TXT("m_balancePostBrightness") );
	PROPERTY_EDIT( m_levelsShadows,			TXT("m_levelsShadows") );
	PROPERTY_EDIT( m_levelsMidtones,		TXT("m_levelsMidtones") );
	PROPERTY_EDIT( m_levelsHighlights,		TXT("m_levelsHighlights") );
	PROPERTY_EDIT( m_midtoneRangeMin,		TXT("m_midtoneRangeMin") );
	PROPERTY_EDIT( m_midtoneRangeMax,		TXT("m_midtoneRangeMax") );
	PROPERTY_EDIT( m_midtoneMarginMin,		TXT("m_midtoneMarginMin") );
	PROPERTY_EDIT( m_midtoneMarginMax,		TXT("m_midtoneMarginMax") );
	PROPERTY_EDIT( m_parametricBalanceLow,	TXT("m_parametricBalanceLow") );
	PROPERTY_EDIT( m_parametricBalanceMid,	TXT("m_parametricBalanceMid") );
	PROPERTY_EDIT( m_parametricBalanceHigh,	TXT("m_parametricBalanceHigh") );

END_CLASS_RTTI();

class CEnvFinalColorBalanceParametersAtPoint
{
public:
	bool				m_activated;
	bool				m_activatedBalanceMap;
	bool				m_activatedParametricBalance;
	SSimpleCurvePoint	m_vignetteWeights;
	SSimpleCurvePoint	m_vignetteColor;
	SSimpleCurvePoint	m_vignetteOpacity;
	SSimpleCurvePoint	m_chromaticAberrationSize;
	SSimpleCurvePoint				m_balanceMapLerp;
	SSimpleCurvePoint				m_balanceMapAmount;
	TSoftHandle< CBitmapTexture >	m_balanceMap0;
	TSoftHandle< CBitmapTexture >	m_balanceMap1;
	SSimpleCurvePoint				m_balancePostBrightness;
	SSimpleCurvePoint				m_levelsShadows;
	SSimpleCurvePoint				m_levelsMidtones;
	SSimpleCurvePoint				m_levelsHighlights;
	SSimpleCurvePoint				m_midtoneRangeMin;
	SSimpleCurvePoint				m_midtoneRangeMax;
	SSimpleCurvePoint				m_midtoneMarginMin;
	SSimpleCurvePoint				m_midtoneMarginMax;
	CEnvParametricBalanceParametersAtPoint m_parametricBalanceLow;
	CEnvParametricBalanceParametersAtPoint m_parametricBalanceMid;
	CEnvParametricBalanceParametersAtPoint m_parametricBalanceHigh;

public:
	CEnvFinalColorBalanceParametersAtPoint() {};
	CEnvFinalColorBalanceParametersAtPoint( const CEnvFinalColorBalanceParameters& source );
};

/// Environment water control parameters
class CEnvWaterParameters
{
	DECLARE_RTTI_STRUCT( CEnvWaterParameters );

public:
	bool			m_activated;
	SSimpleCurve	m_waterFlowIntensity;
	SSimpleCurve	m_underwaterBrightness;
	SSimpleCurve	m_underWaterFogIntensity;
	SSimpleCurve	m_waterColor;
	SSimpleCurve	m_underWaterColor;
	SSimpleCurve	m_waterFresnel;
	SSimpleCurve	m_waterCaustics;
	SSimpleCurve	m_waterFoamIntensity;
	SSimpleCurve	m_waterAmbientScale;
	SSimpleCurve	m_waterDiffuseScale;

public:
	CEnvWaterParameters( EEnvParamsResetMode mode = EnvResetMode_CurvesDefault ); //EnvResetMode_Default 

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvWaterParameters &src, const CEnvWaterParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvWaterParameters )
	PROPERTY_EDIT( m_activated,				TXT("m_activated") );
	PROPERTY_EDIT( m_waterFlowIntensity,	TXT("Water rain-flow intensity") );	
	PROPERTY_EDIT( m_underwaterBrightness,	TXT("Underwater Brightness") );
	PROPERTY_EDIT( m_underWaterFogIntensity,TXT("Underwater fog intensity, 0 - 1") );	
	PROPERTY_EDIT( m_waterColor,			TXT("m_waterColor") );	
	PROPERTY_EDIT( m_underWaterColor,		TXT("Underwater (Fog) Color") );	
	PROPERTY_EDIT( m_waterFresnel,			TXT("m_waterFresnel") );
	PROPERTY_EDIT( m_waterCaustics,			TXT("m_waterCaustics") );
	PROPERTY_EDIT( m_waterFoamIntensity,	TXT("Foam Intensity") );
	PROPERTY_EDIT( m_waterAmbientScale,		TXT("m_waterAmbientScale") );
	PROPERTY_EDIT( m_waterDiffuseScale,		TXT("m_waterDiffuseScale") );
END_CLASS_RTTI();

class CEnvWaterParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_waterTransparency;
	SSimpleCurvePoint	m_underwaterBrightness;
	SSimpleCurvePoint	m_underWaterFogIntensity;
	SSimpleCurvePoint	m_waterColor;
	SSimpleCurvePoint	m_underWaterColor;	
	SSimpleCurvePoint	m_waterFresnel;
	SSimpleCurvePoint	m_waterCaustics;
	SSimpleCurvePoint	m_waterFoamIntensity;
	SSimpleCurvePoint	m_waterAmbientScale;
	SSimpleCurvePoint	m_waterDiffuseScale;

public:
	CEnvWaterParametersAtPoint() {};
	CEnvWaterParametersAtPoint( const CEnvWaterParameters& source );
};

/// Environment sharpen parameters
class CEnvSharpenParameters
{
	DECLARE_RTTI_STRUCT( CEnvSharpenParameters );

public:
	bool			m_activated;
	SSimpleCurve	m_sharpenNear;
	SSimpleCurve	m_sharpenFar;
	SSimpleCurve	m_distanceNear;
	SSimpleCurve	m_distanceFar;
	SSimpleCurve	m_lumFilterOffset;
	SSimpleCurve	m_lumFilterRange;

public:
	CEnvSharpenParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvSharpenParameters &src, const CEnvSharpenParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvSharpenParameters )
	PROPERTY_EDIT( m_activated,			TXT("m_activated") );
	PROPERTY_EDIT( m_sharpenNear,		TXT("m_sharpenNear") );	
	PROPERTY_EDIT( m_sharpenFar,		TXT("m_sharpenFar") );	
	PROPERTY_EDIT( m_distanceNear,		TXT("m_distanceNear") );	
	PROPERTY_EDIT( m_distanceFar,		TXT("m_distanceFar") );
	PROPERTY_EDIT( m_lumFilterOffset,	TXT("m_lumFilterOffset") );
	PROPERTY_EDIT( m_lumFilterRange,	TXT("m_lumFilterRange") );
END_CLASS_RTTI();

class CEnvSharpenParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_sharpenNear;
	SSimpleCurvePoint	m_sharpenFar;
	SSimpleCurvePoint	m_distanceNear;
	SSimpleCurvePoint	m_distanceFar;
	SSimpleCurvePoint	m_lumFilterOffset;
	SSimpleCurvePoint	m_lumFilterRange;

public:
	CEnvSharpenParametersAtPoint() {};
	CEnvSharpenParametersAtPoint( const CEnvSharpenParameters& source );
};

/// Environment paintEffect parameters
class CEnvPaintEffectParameters
{
	DECLARE_RTTI_STRUCT( CEnvPaintEffectParameters );

public:
	bool			m_activated;
	SSimpleCurve	m_amount;

public:
	CEnvPaintEffectParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvPaintEffectParameters &src, const CEnvPaintEffectParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvPaintEffectParameters )
	PROPERTY_EDIT( m_activated,			TXT("m_activated") );
	PROPERTY_EDIT( m_amount,			TXT("m_amount") );
END_CLASS_RTTI();

class CEnvPaintEffectParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_amount;

public:
	CEnvPaintEffectParametersAtPoint() {};
	CEnvPaintEffectParametersAtPoint( const CEnvPaintEffectParameters& source );
};

/// Env Flare color parameters
class CEnvFlareColorParameters
{
	DECLARE_RTTI_STRUCT( CEnvFlareColorParameters );

public:
	Bool			m_activated;
	SSimpleCurve	m_color0;
	SSimpleCurve	m_opacity0;
	SSimpleCurve	m_color1;
	SSimpleCurve	m_opacity1;
	SSimpleCurve	m_color2;
	SSimpleCurve	m_opacity2;
	SSimpleCurve	m_color3;
	SSimpleCurve	m_opacity3;

public:
	CEnvFlareColorParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	/// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	/// Import daypoint value
	void ImportDayPointValue( const CEnvFlareColorParameters &src, const CEnvFlareColorParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvFlareColorParameters )
PROPERTY_EDIT( m_activated,				TXT("m_activated") );
PROPERTY_EDIT( m_color0,				TXT("Color 0") );	
PROPERTY_EDIT( m_opacity0,				TXT("Opacity 0") );	
PROPERTY_EDIT( m_color1,				TXT("Color 1") );	
PROPERTY_EDIT( m_opacity1,				TXT("Opacity 1") );	
PROPERTY_EDIT( m_color2,				TXT("Color 2") );	
PROPERTY_EDIT( m_opacity2,				TXT("Opacity 2") );	
PROPERTY_EDIT( m_color3,				TXT("Color 3") );	
PROPERTY_EDIT( m_opacity3,				TXT("Opacity 3") );	
END_CLASS_RTTI();

class CEnvFlareColorParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_color0;
	SSimpleCurvePoint	m_opacity0;
	SSimpleCurvePoint	m_color1;
	SSimpleCurvePoint	m_opacity1;
	SSimpleCurvePoint	m_color2;
	SSimpleCurvePoint	m_opacity2;
	SSimpleCurvePoint	m_color3;
	SSimpleCurvePoint	m_opacity3;

public:
	CEnvFlareColorParametersAtPoint() {};
	CEnvFlareColorParametersAtPoint( const CEnvFlareColorParameters& source );

	/// Get color curve by index
	Vector GetColorByIndex( Uint32 curveIndex ) const;

	/// Get opacity curve by index
	Float GetOpacityByIndex( Uint32 curveIndex ) const;
};


/// Environment flare color group
enum EEnvFlareColorGroup : CEnum::TValueType
{
	EFCG_Default,
	EFCG_Sun,
	EFCG_Moon,
	EFCG_Custom0,
	EFCG_Custom1,
	EFCG_Custom2,
};

BEGIN_ENUM_RTTI( EEnvFlareColorGroup );
ENUM_OPTION( EFCG_Default );
ENUM_OPTION( EFCG_Sun );
ENUM_OPTION( EFCG_Moon );
ENUM_OPTION( EFCG_Custom0 );
ENUM_OPTION( EFCG_Custom1 );
ENUM_OPTION( EFCG_Custom2 );
END_ENUM_RTTI();


/// Env Flare color groups parameters
class CEnvFlareColorGroupsParameters
{
	DECLARE_RTTI_STRUCT( CEnvFlareColorGroupsParameters );

public:
	Bool						m_activated;
	CEnvFlareColorParameters	m_default;
	// sun/moon parameters in other structure
	CEnvFlareColorParameters	m_custom0;
	CEnvFlareColorParameters	m_custom1;
	CEnvFlareColorParameters	m_custom2;

public:
	CEnvFlareColorGroupsParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	/// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	/// Import daypoint value
	void ImportDayPointValue( const CEnvFlareColorGroupsParameters &src, const CEnvFlareColorGroupsParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvFlareColorGroupsParameters )
PROPERTY_EDIT( m_activated,				TXT("m_activated") );
PROPERTY_EDIT( m_default,				TXT("Default") );	
PROPERTY_EDIT( m_custom0,				TXT("Custom 0") );	
PROPERTY_EDIT( m_custom1,				TXT("Custom 1") );	
PROPERTY_EDIT( m_custom2,				TXT("Custom 2") );
END_CLASS_RTTI();


class CEnvFlareColorGroupsParametersAtPoint
{
public:
	bool							m_activated;
	CEnvFlareColorParametersAtPoint	m_default;
	CEnvFlareColorParametersAtPoint	m_custom0;
	CEnvFlareColorParametersAtPoint m_custom1;
	CEnvFlareColorParametersAtPoint m_custom2;

public:
	CEnvFlareColorGroupsParametersAtPoint() {};
	CEnvFlareColorGroupsParametersAtPoint( const CEnvFlareColorGroupsParameters& source );
};


/// Environment sun control parameters
class CEnvSunAndMoonParameters
{
	DECLARE_RTTI_STRUCT( CEnvSunAndMoonParameters );

public:
	Bool			m_activated;

	SSimpleCurve	m_sunSize;
	SSimpleCurve	m_sunColor;
	SSimpleCurve	m_sunFlareSize;
	CEnvFlareColorParameters m_sunFlareColor;

	SSimpleCurve	m_moonSize;
	SSimpleCurve	m_moonColor;
	SSimpleCurve	m_moonFlareSize;
	CEnvFlareColorParameters m_moonFlareColor;

public:
	CEnvSunAndMoonParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvSunAndMoonParameters &src, const CEnvSunAndMoonParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvSunAndMoonParameters )
PROPERTY_EDIT( m_activated,				TXT("m_activated") );
PROPERTY_EDIT( m_sunSize,				TXT("m_sunSize") );
PROPERTY_EDIT( m_sunColor,				TXT("m_sunColor") );
PROPERTY_EDIT( m_sunFlareSize,			TXT("m_sunFlareSize") );
PROPERTY_EDIT( m_sunFlareColor,			TXT("m_sunFlareColor") );
PROPERTY_EDIT( m_moonSize,				TXT("m_moonSize") );	
PROPERTY_EDIT( m_moonColor,				TXT("m_moonColor") );	
PROPERTY_EDIT( m_moonFlareSize,			TXT("m_moonFlareSize") );
PROPERTY_EDIT( m_moonFlareColor,		TXT("m_moonFlareColor") );	
END_CLASS_RTTI();

class CEnvSunAndMoonParametersAtPoint
{
public:
	Bool				m_activated;

	SSimpleCurvePoint	m_sunSize;
	SSimpleCurvePoint	m_sunColor;
	SSimpleCurvePoint	m_sunFlareSize;
	CEnvFlareColorParametersAtPoint m_sunFlareColor;

	SSimpleCurvePoint	m_moonSize;
	SSimpleCurvePoint	m_moonColor;
	SSimpleCurvePoint	m_moonFlareSize;
	CEnvFlareColorParametersAtPoint m_moonFlareColor;

public:
	CEnvSunAndMoonParametersAtPoint() {};
	CEnvSunAndMoonParametersAtPoint( const CEnvSunAndMoonParameters& source );
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/// Environment gameplay effects parameters
class CEnvGameplayEffectsParameters
{
	DECLARE_RTTI_STRUCT( CEnvGameplayEffectsParameters );

public:
	Bool				m_activated;
	SSimpleCurve		m_catEffectBrightnessMultiply;
	SSimpleCurve		m_behaviorAnimationMultiplier;
	SSimpleCurve		m_specularityMultiplier;
	SSimpleCurve		m_glossinessMultiplier;

public:
	CEnvGameplayEffectsParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvGameplayEffectsParameters &src, const CEnvGameplayEffectsParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvGameplayEffectsParameters )
PROPERTY_EDIT( m_activated,						TXT("m_activated") );
PROPERTY_EDIT( m_catEffectBrightnessMultiply,	TXT("m_catEffectBrightnessMultiply") );	
PROPERTY_EDIT( m_behaviorAnimationMultiplier,	TXT("m_behaviorAnimationMultiplier") );	
PROPERTY_EDIT( m_specularityMultiplier,			TXT("m_specularityMultiplier") );	
PROPERTY_EDIT( m_glossinessMultiplier,			TXT("m_glossinessMultiplier") );	
END_CLASS_RTTI();


class CEnvGameplayEffectsParametersAtPoint
{
public:
	bool					m_activated;

	SSimpleCurvePoint		m_catEffectBrightnessMultiply;
	SSimpleCurvePoint		m_behaviorAnimationMultiplier;
	SSimpleCurvePoint		m_specularityMultiplier;
	SSimpleCurvePoint		m_glossinessMultiplier;

public:
	CEnvGameplayEffectsParametersAtPoint() {};
	CEnvGameplayEffectsParametersAtPoint( const CEnvGameplayEffectsParameters& source );
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/// Environment gameplay effects parameters
class CEnvMotionBlurParameters
{
	DECLARE_RTTI_STRUCT( CEnvMotionBlurParameters );

public:
	Bool				m_activated;
	SSimpleCurve		m_strength;

public:
	CEnvMotionBlurParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvMotionBlurParameters &src, const CEnvMotionBlurParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvMotionBlurParameters )
PROPERTY_EDIT( m_activated,			TXT("m_activated") );
PROPERTY_EDIT( m_strength,			TXT("m_strength") );	
END_CLASS_RTTI();

class CEnvMotionBlurParametersAtPoint
{
public:
	bool					m_activated;

	SSimpleCurvePoint		m_strength;

public:
	CEnvMotionBlurParametersAtPoint() {};
	CEnvMotionBlurParametersAtPoint( const CEnvMotionBlurParameters& source );
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/// Environment wind control parameters
class CEnvWindParameters
{
	DECLARE_RTTI_STRUCT( CEnvWindParameters );

public:
	Bool				m_activated;
	SSimpleCurve		m_windStrengthOverride;
	SSimpleCurve		m_cloudsVelocityOverride;

public:
	CEnvWindParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvWindParameters &src, const CEnvWindParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvWindParameters )
PROPERTY_EDIT( m_activated,					TXT("m_activated") );
PROPERTY_EDIT( m_windStrengthOverride,			TXT("m_strengthOverride") );
PROPERTY_EDIT( m_cloudsVelocityOverride,	TXT("m_cloudsVelocityOverride") );
END_CLASS_RTTI();

class CEnvWindParametersAtPoint
{
public:
	bool					m_activated;

	SSimpleCurvePoint		m_windStrengthOverride;
	SSimpleCurvePoint		m_cloudsVelocityOverride;
	
public:
	CEnvWindParametersAtPoint() {};
	CEnvWindParametersAtPoint( const CEnvWindParameters& source );
};


class CEnvDialogLightParameters
{
	DECLARE_RTTI_STRUCT( CEnvDialogLightParameters );
public:
	CEnvDialogLightParameters()
	{
		Reset( EnvResetMode_CurvesDefault );
	}

	Bool			m_activated;
	SSimpleCurve	m_lightColor;
	SSimpleCurve	m_lightColor2;
	SSimpleCurve	m_lightColor3;


	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvDialogLightParameters &src, const CEnvDialogLightParameters &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CEnvDialogLightParameters )
	PROPERTY_EDIT( m_activated, TXT("") )
	PROPERTY_EDIT( m_lightColor, TXT("Light color") );
	PROPERTY_EDIT( m_lightColor2, TXT("Light color2") );
	PROPERTY_EDIT( m_lightColor3, TXT("Light color3") );
END_CLASS_RTTI();

/// Environment color group
enum EEnvColorGroup : CEnum::TValueType
{
	ECG_Default,
	ECG_LightsDefault,
	ECG_LightsDawn,
	ECG_LightsNoon,
	ECG_LightsEvening,
	ECG_LightsNight,
	ECG_FX_Default,
	ECG_FX_Fire,
	ECG_FX_FireFlares,
	ECG_FX_FireLight,
	ECG_FX_Smoke,
	ECG_FX_SmokeExplosion,

	ECG_FX_Sky,
	ECG_FX_SkyNight,
	ECG_FX_SkyDawn,
	ECG_FX_SkyNoon,
	ECG_FX_SkySunset,
	ECG_FX_SkyRain,

	ECG_FX_MainCloudsMiddle,
	ECG_FX_MainCloudsFront,
	ECG_FX_MainCloudsBack,
	ECG_FX_MainCloudsRim,
	ECG_FX_BackgroundCloudsFront,
	ECG_FX_BackgroundCloudsBack,
	ECG_FX_BackgroundHazeFront,
	ECG_FX_BackgroundHazeBack,

	ECG_FX_Blood,
	ECG_FX_Water,
	ECG_FX_Fog,
	ECG_FX_LightShaft,
	ECG_FX_LightShaftSun,
	ECG_FX_LightShaftInteriorDawn,
	ECG_FX_LightShaftSpotlightDawn,
	ECG_FX_LightShaftReflectionLightDawn,
	ECG_FX_LightShaftInteriorNoon,
	ECG_FX_LightShaftSpotlightNoon,
	ECG_FX_LightShaftReflectionLightNoon,
	ECG_FX_LightShaftInteriorEvening,
	ECG_FX_LightShaftSpotlightEvening,
	ECG_FX_LightShaftReflectionLightEvening,
	ECG_FX_LightShaftInteriorNight,
	ECG_FX_LightShaftSpotlightNight,
	ECG_FX_LightShaftReflectionLightNight,
	ECG_FX_Trails,
	ECG_FX_ScreenParticles,
	ECG_Custom0,
	ECG_Custom1,
	ECG_Custom2,

	ECG_MAX
};

BEGIN_ENUM_RTTI( EEnvColorGroup );
	ENUM_OPTION( ECG_Default );
	ENUM_OPTION( ECG_LightsDefault );
	ENUM_OPTION( ECG_LightsDawn );
	ENUM_OPTION( ECG_LightsNoon );
	ENUM_OPTION( ECG_LightsEvening );
	ENUM_OPTION( ECG_LightsNight );
	ENUM_OPTION( ECG_FX_Default );
	ENUM_OPTION( ECG_FX_Fire );
	ENUM_OPTION( ECG_FX_FireFlares );
	ENUM_OPTION( ECG_FX_FireLight );
	ENUM_OPTION( ECG_FX_Smoke );
	ENUM_OPTION( ECG_FX_SmokeExplosion );

	ENUM_OPTION( ECG_FX_Sky );
	ENUM_OPTION( ECG_FX_SkyNight );
	ENUM_OPTION( ECG_FX_SkyDawn );
	ENUM_OPTION( ECG_FX_SkyNoon );
	ENUM_OPTION( ECG_FX_SkySunset );
	ENUM_OPTION( ECG_FX_SkyRain );

	ENUM_OPTION( ECG_FX_MainCloudsMiddle );
	ENUM_OPTION( ECG_FX_MainCloudsFront );
	ENUM_OPTION( ECG_FX_MainCloudsBack );
	ENUM_OPTION( ECG_FX_MainCloudsRim );
	ENUM_OPTION( ECG_FX_BackgroundCloudsFront );
	ENUM_OPTION( ECG_FX_BackgroundCloudsBack );
	ENUM_OPTION( ECG_FX_BackgroundHazeFront );
	ENUM_OPTION( ECG_FX_BackgroundHazeBack );

	ENUM_OPTION( ECG_FX_Blood );
	ENUM_OPTION( ECG_FX_Water );
	ENUM_OPTION( ECG_FX_Fog );
	ENUM_OPTION( ECG_FX_LightShaft );
	ENUM_OPTION( ECG_FX_LightShaftSun );
	ENUM_OPTION( ECG_FX_LightShaftInteriorDawn );
	ENUM_OPTION( ECG_FX_LightShaftSpotlightDawn );
	ENUM_OPTION( ECG_FX_LightShaftReflectionLightDawn );
	ENUM_OPTION( ECG_FX_LightShaftInteriorNoon );
	ENUM_OPTION( ECG_FX_LightShaftSpotlightNoon );
	ENUM_OPTION( ECG_FX_LightShaftReflectionLightNoon );
	ENUM_OPTION( ECG_FX_LightShaftInteriorEvening );
	ENUM_OPTION( ECG_FX_LightShaftSpotlightEvening );
	ENUM_OPTION( ECG_FX_LightShaftReflectionLightEvening );
	ENUM_OPTION( ECG_FX_LightShaftInteriorNight );
	ENUM_OPTION( ECG_FX_LightShaftSpotlightNight );
	ENUM_OPTION( ECG_FX_LightShaftReflectionLightNight );
	ENUM_OPTION( ECG_FX_Trails );
	ENUM_OPTION( ECG_FX_ScreenParticles );
	ENUM_OPTION( ECG_Custom0 );
	ENUM_OPTION( ECG_Custom1 );
	ENUM_OPTION( ECG_Custom2 );
END_ENUM_RTTI();


/// Environment color groups parameters
class CEnvColorGroupsParameters
{
	DECLARE_RTTI_STRUCT( CEnvColorGroupsParameters );

public:
	bool			m_activated;
	SSimpleCurve	m_defaultGroup;
	SSimpleCurve	m_lightsDefault;
	SSimpleCurve	m_lightsDawn;
	SSimpleCurve	m_lightsNoon;
	SSimpleCurve	m_lightsEvening;
	SSimpleCurve	m_lightsNight;
	SSimpleCurve	m_fxDefault;
	SSimpleCurve	m_fxFire;
	SSimpleCurve	m_fxFireFlares;
	SSimpleCurve	m_fxFireLight;
	SSimpleCurve	m_fxSmoke;
	SSimpleCurve	m_fxSmokeExplosion;

	SSimpleCurve	m_fxSky;
	SSimpleCurve	m_fxSkyAlpha;
	SSimpleCurve	m_fxSkyNight;
	SSimpleCurve	m_fxSkyNightAlpha;
	SSimpleCurve	m_fxSkyDawn;
	SSimpleCurve	m_fxSkyDawnAlpha;
	SSimpleCurve	m_fxSkyNoon;
	SSimpleCurve	m_fxSkyNoonAlpha;
	SSimpleCurve	m_fxSkySunset;
	SSimpleCurve	m_fxSkySunsetAlpha;
	SSimpleCurve	m_fxSkyRain;
	SSimpleCurve	m_fxSkyRainAlpha;

	// these groups replace the ones above (fxSky...) and add two more 
	// (renaming would cause SSimpleCurve deserialization impossible or time costly to figure out because OnPropertyMissing/OnReadUnknownProperty don't work)
	SSimpleCurve	m_mainCloudsMiddle;
	SSimpleCurve	m_mainCloudsMiddleAlpha;
	SSimpleCurve	m_mainCloudsFront;
	SSimpleCurve	m_mainCloudsFrontAlpha;
	SSimpleCurve	m_mainCloudsBack;
	SSimpleCurve	m_mainCloudsBackAlpha;
	SSimpleCurve	m_mainCloudsRim;
	SSimpleCurve	m_mainCloudsRimAlpha;
	SSimpleCurve	m_backgroundCloudsFront;
	SSimpleCurve	m_backgroundCloudsFrontAlpha;
	SSimpleCurve	m_backgroundCloudsBack;
	SSimpleCurve	m_backgroundCloudsBackAlpha;
	SSimpleCurve	m_backgroundHazeFront;
	SSimpleCurve	m_backgroundHazeFrontAlpha;
	SSimpleCurve	m_backgroundHazeBack;
	SSimpleCurve	m_backgroundHazeBackAlpha;

	SSimpleCurve	m_fxBlood;
	SSimpleCurve	m_fxWater;
	SSimpleCurve	m_fxFog;
	SSimpleCurve	m_fxTrails;
	SSimpleCurve	m_fxScreenParticles;
	SSimpleCurve	m_fxLightShaft;
	SSimpleCurve	m_fxLightShaftSun;
	SSimpleCurve	m_fxLightShaftInteriorDawn;
	SSimpleCurve	m_fxLightShaftSpotlightDawn;
	SSimpleCurve	m_fxLightShaftReflectionLightDawn;
	SSimpleCurve	m_fxLightShaftInteriorNoon;
	SSimpleCurve	m_fxLightShaftSpotlightNoon;
	SSimpleCurve	m_fxLightShaftReflectionLightNoon;
	SSimpleCurve	m_fxLightShaftInteriorEvening;
	SSimpleCurve	m_fxLightShaftSpotlightEvening;
	SSimpleCurve	m_fxLightShaftReflectionLightEvening;
	SSimpleCurve	m_fxLightShaftInteriorNight;
	SSimpleCurve	m_fxLightShaftSpotlightNight;
	SSimpleCurve	m_fxLightShaftReflectionLightNight;
	bool			m_activatedCustom0;
	SSimpleCurve	m_customGroup0;
	bool			m_activatedCustom1;
	SSimpleCurve	m_customGroup1;
	bool			m_activatedCustom2;
	SSimpleCurve	m_customGroup2;

public:
	CEnvColorGroupsParameters( EEnvParamsResetMode mode = EnvResetMode_Default );

	// Reset to defaults
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CEnvColorGroupsParameters &src, const CEnvColorGroupsParameters &dest, Float lerpFactor, Float time );

	// Get curve for given color group
	const SSimpleCurve& GetCurveForColorGroup( EEnvColorGroup group ) const;
};

BEGIN_CLASS_RTTI( CEnvColorGroupsParameters )
	PROPERTY_EDIT( m_activated,				TXT("m_activated") );
	PROPERTY_EDIT( m_defaultGroup,			TXT("Default group") );	
	PROPERTY_EDIT( m_lightsDefault,			TXT("Default group for lights") );	
	PROPERTY_EDIT( m_lightsDawn,			TXT("Dawn lights") );	
	PROPERTY_EDIT( m_lightsNoon,			TXT("Noon lights") );
	PROPERTY_EDIT( m_lightsEvening,			TXT("Evening lights") );
	PROPERTY_EDIT( m_lightsNight,			TXT("Night lights") );
	PROPERTY_EDIT( m_fxDefault,				TXT("Default group for FX") );
	PROPERTY_EDIT( m_fxFire,				TXT("Fire fx") );
	PROPERTY_EDIT( m_fxFireFlares,			TXT("Fire flares fx") );
	PROPERTY_EDIT( m_fxFireLight,			TXT("Fire light fx") );
	PROPERTY_EDIT( m_fxSmoke,				TXT("Smoke fx") );
	PROPERTY_EDIT( m_fxSmokeExplosion,		TXT("Smoke fx explosion") );

	PROPERTY_EDIT( m_fxSky,					TXT("Sky fx") );
	PROPERTY_EDIT( m_fxSkyAlpha,			TXT("Sky fx") );
	PROPERTY_EDIT( m_fxSkyNight,			TXT("Sky fx") );
	PROPERTY_EDIT( m_fxSkyNightAlpha,		TXT("Sky fx") );
	PROPERTY_EDIT( m_fxSkyDawn,				TXT("Sky fx") );
	PROPERTY_EDIT( m_fxSkyDawnAlpha,		TXT("Sky fx") );
	PROPERTY_EDIT( m_fxSkyNoon,				TXT("Sky fx") );
	PROPERTY_EDIT( m_fxSkyNoonAlpha,		TXT("Sky fx") );
	PROPERTY_EDIT( m_fxSkySunset,			TXT("Sky fx") );
	PROPERTY_EDIT( m_fxSkySunsetAlpha,		TXT("Sky fx") );
	PROPERTY_EDIT( m_fxSkyRain,				TXT("Sky fx") );
	PROPERTY_EDIT( m_fxSkyRainAlpha,		TXT("Sky fx") );

	PROPERTY_EDIT( m_mainCloudsMiddle,			TXT("Sky fx") );
	PROPERTY_EDIT( m_mainCloudsMiddleAlpha,		TXT("Sky fx") );
	PROPERTY_EDIT( m_mainCloudsFront,			TXT("Sky fx") );
	PROPERTY_EDIT( m_mainCloudsFrontAlpha,		TXT("Sky fx") );
	PROPERTY_EDIT( m_mainCloudsBack,			TXT("Sky fx") );
	PROPERTY_EDIT( m_mainCloudsBackAlpha,		TXT("Sky fx") );
	PROPERTY_EDIT( m_mainCloudsRim,				TXT("Sky fx") );
	PROPERTY_EDIT( m_mainCloudsRimAlpha,		TXT("Sky fx") );
	PROPERTY_EDIT( m_backgroundCloudsFront,		TXT("Sky fx") );
	PROPERTY_EDIT( m_backgroundCloudsFrontAlpha,TXT("Sky fx") );
	PROPERTY_EDIT( m_backgroundCloudsBack,		TXT("Sky fx") );
	PROPERTY_EDIT( m_backgroundCloudsBackAlpha,	TXT("Sky fx") );
	PROPERTY_EDIT( m_backgroundHazeFront,		TXT("Sky fx") );
	PROPERTY_EDIT( m_backgroundHazeFrontAlpha,	TXT("Sky fx") );
	PROPERTY_EDIT( m_backgroundHazeBack,		TXT("Sky fx") );
	PROPERTY_EDIT( m_backgroundHazeBackAlpha,	TXT("Sky fx") );

	PROPERTY_EDIT( m_fxBlood,				TXT("Blood fx") );
	PROPERTY_EDIT( m_fxWater,				TXT("Water fx") );
	PROPERTY_EDIT( m_fxFog,					TXT("Fog fx") );
	PROPERTY_EDIT( m_fxTrails,				TXT("Trails fx") );
	PROPERTY_EDIT( m_fxScreenParticles,		TXT("ScreenParticles fx") );
	PROPERTY_EDIT( m_fxLightShaft,			TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftSun,		TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftInteriorDawn,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftSpotlightDawn,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftReflectionLightDawn,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftInteriorNoon,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftSpotlightNoon,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftReflectionLightNoon,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftInteriorEvening,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftSpotlightEvening,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftReflectionLightEvening,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftInteriorNight,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftSpotlightNight,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_fxLightShaftReflectionLightNight,	TXT("Light shaft fx") );
	PROPERTY_EDIT( m_activatedCustom0,		TXT("m_activatedCustom0") );
	PROPERTY_EDIT( m_customGroup0,			TXT("m_customGroup0") );
	PROPERTY_EDIT( m_activatedCustom1,		TXT("m_activatedCustom1") );
	PROPERTY_EDIT( m_customGroup1,			TXT("m_customGroup1") );
	PROPERTY_EDIT( m_activatedCustom2,		TXT("m_activatedCustom2") );
	PROPERTY_EDIT( m_customGroup2,			TXT("m_customGroup2") );
END_CLASS_RTTI();

class CEnvColorGroupsParametersAtPoint
{
public:
	bool				m_activated;
	SSimpleCurvePoint	m_defaultGroup;
	SSimpleCurvePoint	m_lightsDefault;
	SSimpleCurvePoint	m_lightsDawn;
	SSimpleCurvePoint	m_lightsNoon;
	SSimpleCurvePoint	m_lightsEvening;
	SSimpleCurvePoint	m_lightsNight;
	SSimpleCurvePoint	m_fxDefault;
	SSimpleCurvePoint	m_fxFire;
	SSimpleCurvePoint	m_fxFireFlares;
	SSimpleCurvePoint	m_fxFireLight;
	SSimpleCurvePoint	m_fxSmoke;
	SSimpleCurvePoint	m_fxSmokeExplosion;

	SSimpleCurvePoint	m_fxSky;
	SSimpleCurvePoint	m_fxSkyAlpha;
	SSimpleCurvePoint	m_fxSkyNight;
	SSimpleCurvePoint	m_fxSkyNightAlpha;
	SSimpleCurvePoint	m_fxSkyDawn;
	SSimpleCurvePoint	m_fxSkyDawnAlpha;
	SSimpleCurvePoint	m_fxSkyNoon;
	SSimpleCurvePoint	m_fxSkyNoonAlpha;
	SSimpleCurvePoint	m_fxSkySunset;
	SSimpleCurvePoint	m_fxSkySunsetAlpha;
	SSimpleCurvePoint	m_fxSkyRain;
	SSimpleCurvePoint	m_fxSkyRainAlpha;

	SSimpleCurvePoint	m_mainCloudsMiddle;
	SSimpleCurvePoint	m_mainCloudsMiddleAlpha;
	SSimpleCurvePoint	m_mainCloudsFront;
	SSimpleCurvePoint	m_mainCloudsFrontAlpha;
	SSimpleCurvePoint	m_mainCloudsBack;
	SSimpleCurvePoint	m_mainCloudsBackAlpha;
	SSimpleCurvePoint	m_mainCloudsRim;
	SSimpleCurvePoint	m_mainCloudsRimAlpha;
	SSimpleCurvePoint	m_backgroundCloudsFront;
	SSimpleCurvePoint	m_backgroundCloudsFrontAlpha;
	SSimpleCurvePoint	m_backgroundCloudsBack;
	SSimpleCurvePoint	m_backgroundCloudsBackAlpha;
	SSimpleCurvePoint	m_backgroundHazeFront;
	SSimpleCurvePoint	m_backgroundHazeFrontAlpha;
	SSimpleCurvePoint	m_backgroundHazeBack;
	SSimpleCurvePoint	m_backgroundHazeBackAlpha;

	SSimpleCurvePoint	m_fxBlood;
	SSimpleCurvePoint	m_fxWater;
	SSimpleCurvePoint	m_fxFog;
	SSimpleCurvePoint	m_fxTrails;
	SSimpleCurvePoint	m_fxScreenParticles;
	SSimpleCurvePoint	m_fxLightShaft;
	SSimpleCurvePoint	m_fxLightShaftSun;
	SSimpleCurvePoint	m_fxLightShaftInteriorDawn;
	SSimpleCurvePoint	m_fxLightShaftSpotlightDawn;
	SSimpleCurvePoint	m_fxLightShaftReflectionLightDawn;
	SSimpleCurvePoint	m_fxLightShaftInteriorNoon;
	SSimpleCurvePoint	m_fxLightShaftSpotlightNoon;
	SSimpleCurvePoint	m_fxLightShaftReflectionLightNoon;
	SSimpleCurvePoint	m_fxLightShaftInteriorEvening;
	SSimpleCurvePoint	m_fxLightShaftSpotlightEvening;
	SSimpleCurvePoint	m_fxLightShaftReflectionLightEvening;
	SSimpleCurvePoint	m_fxLightShaftInteriorNight;
	SSimpleCurvePoint	m_fxLightShaftSpotlightNight;
	SSimpleCurvePoint	m_fxLightShaftReflectionLightNight;
	bool				m_activatedCustom0;
	SSimpleCurvePoint	m_customGroup0;
	bool				m_activatedCustom1;
	SSimpleCurvePoint	m_customGroup1;
	bool				m_activatedCustom2;
	SSimpleCurvePoint	m_customGroup2;

public:
	CEnvColorGroupsParametersAtPoint() {};
	CEnvColorGroupsParametersAtPoint( const CEnvColorGroupsParameters& source );

	// Get curve for given color group
	const SSimpleCurvePoint& GetCurveForColorGroup( EEnvColorGroup group ) const;
	// Get alpha for given color group
	const Float GetAlphaForColorGroup( EEnvColorGroup group ) const;
};


/// Environment parameters
class CAreaEnvironmentParams
{
	DECLARE_RTTI_STRUCT( CAreaEnvironmentParams );

public:
	CEnvFinalColorBalanceParameters		m_finalColorBalance;
	CEnvSharpenParameters				m_sharpen;
	CEnvPaintEffectParameters			m_paintEffect;
	CEnvNVSSAOParameters				m_nvSsao;
	CEnvMSSSAOParameters				m_msSsao;
	CEnvGlobalLightParameters			m_globalLight;
	CEnvInteriorFallbackParameters		m_interiorFallback;
	CEnvSpeedTreeParameters				m_speedTree;
	CEnvToneMappingParameters			m_toneMapping;
	CEnvBloomNewParameters				m_bloomNew;
	CEnvGlobalFogParameters				m_globalFog;
	CEnvGlobalSkyParameters				m_sky;
	CEnvDepthOfFieldParameters			m_depthOfField;
	CEnvColorModTransparencyParameters	m_colorModTransparency;
	CEnvShadowsParameters				m_shadows;
	CEnvWaterParameters					m_water;
	CEnvColorGroupsParameters			m_colorGroups;	
	CEnvFlareColorGroupsParameters		m_flareColorGroups;	
	CEnvSunAndMoonParameters			m_sunAndMoonParams;
	CEnvWindParameters					m_windParams;
	CEnvGameplayEffectsParameters		m_gameplayEffects;
	CEnvMotionBlurParameters			m_motionBlur;
	CEnvCameraLightsSetupParameters		m_cameraLightsSetup;
	CEnvDialogLightParameters			m_dialogLightParams;

public:
	CAreaEnvironmentParams( EEnvParamsResetMode mode = EnvResetMode_Default );	

	// Reset
	void Reset( EEnvParamsResetMode mode );

	// Import daypoint value
	void ImportDayPointValue( const CAreaEnvironmentParams &src, const CAreaEnvironmentParams &dest, Float lerpFactor, Float time );
};

BEGIN_CLASS_RTTI( CAreaEnvironmentParams );	
	PROPERTY_EDIT_NAME( m_finalColorBalance,	TXT("m_finalColorBalance"),		TXT("m_finalColorBalance") );
	PROPERTY_EDIT_NAME( m_sharpen,				TXT("m_sharpen"),				TXT("m_sharpen") );	
	PROPERTY_EDIT_NAME( m_paintEffect,			TXT("m_paintEffect"),			TXT("m_paintEffect") );
	PROPERTY_EDIT_NAME( m_nvSsao,				TXT("m_ssaoNV"),				TXT("m_ssaoNV") );
	PROPERTY_EDIT_NAME( m_msSsao,				TXT("m_ssaoMS"),				TXT("m_ssaoMS") );
	PROPERTY_EDIT_NAME( m_globalLight,			TXT("m_globalLight"),			TXT("m_globalLight") );
	PROPERTY_EDIT_NAME( m_interiorFallback,		TXT("m_interiorFallback"),		TXT("m_interiorFallback") );
	PROPERTY_EDIT_NAME( m_speedTree,			TXT("m_speedTree"),				TXT("m_speedTree") );
	PROPERTY_EDIT_NAME( m_toneMapping,			TXT("m_toneMapping"),			TXT("m_toneMapping") );
	PROPERTY_EDIT_NAME( m_bloomNew,				TXT("m_bloomNew"),				TXT("m_bloomNew") );
	PROPERTY_EDIT_NAME( m_globalFog,			TXT("m_globalFog"),				TXT("m_globalFog") );
	PROPERTY_EDIT_NAME( m_sky,					TXT("m_sky"),					TXT("m_sky") );
	PROPERTY_EDIT_NAME( m_depthOfField,			TXT("m_depthOfField"),			TXT("m_depthOfField") );	
	PROPERTY_EDIT_NAME( m_colorModTransparency,	TXT("m_colorModTransparency"),	TXT("m_colorModTransparency") );
	PROPERTY_EDIT_NAME( m_shadows,				TXT("m_shadows"),				TXT("m_shadows") );	
	PROPERTY_EDIT_NAME( m_water,				TXT("m_water"),					TXT("m_water") );	
	PROPERTY_EDIT_NAME( m_colorGroups,			TXT("m_colorGroups"),			TXT("m_colorGroups") );		
	PROPERTY_EDIT_NAME( m_flareColorGroups,		TXT("m_flareColorGroups"),		TXT("m_flareColorGroups") );		
	PROPERTY_EDIT_NAME( m_sunAndMoonParams,		TXT("m_sunAndMoonParams"),		TXT("m_sunAndMoonParams") );	
	PROPERTY_EDIT_NAME( m_windParams,			TXT("m_windParams"),			TXT("m_windParams") );
	PROPERTY_EDIT_NAME( m_gameplayEffects,		TXT("m_gameplayEffects"),		TXT("m_gameplayEffects") );
	PROPERTY_EDIT_NAME( m_motionBlur,			TXT("m_motionBlur"),			TXT("m_motionBlur") );
	PROPERTY_EDIT_NAME( m_cameraLightsSetup,	TXT("m_cameraLightsSetup"),		TXT("m_cameraLightsSetup") );
	PROPERTY_EDIT_NAME( m_dialogLightParams,	TXT("m_dialogLightParams"),		TXT("m_dialogLightParams") );	
END_CLASS_RTTI();

template<> RED_INLINE Bool TTypedClass<CAreaEnvironmentParams, (EMemoryClass)CAreaEnvironmentParams::MemoryClass, CAreaEnvironmentParams::MemoryPool>::OnReadUnknownProperty( void* object, const CName& propName, const CVariant& propValue ) const
{
	CAreaEnvironmentParams* data = (CAreaEnvironmentParams*)object;
	if ( propName == CNAME( m_ssao ) )
	{
		CEnvNVSSAOParameters params;
		if ( propValue.AsType(params) )
		{
			data->m_nvSsao = params;
			return true;
		}
	}
	return false;
}

class CAreaEnvironmentParamsAtPoint
{
public:
	CEnvFinalColorBalanceParametersAtPoint		m_finalColorBalance;
	CEnvSharpenParametersAtPoint				m_sharpen;
	CEnvPaintEffectParametersAtPoint			m_paintEffect;
	CEnvNVSSAOParametersAtPoint					m_nvSsao;
	CEnvMSSSAOParametersAtPoint					m_msSsao;
	CEnvGlobalLightParametersAtPoint			m_globalLight;
	CEnvInteriorFallbackParametersAtPoint		m_interiorFallback;
	CEnvSpeedTreeParametersAtPoint				m_speedTree;
	CEnvToneMappingParametersAtPoint			m_toneMapping;
	CEnvBloomNewParametersAtPoint				m_bloomNew;
	CEnvGlobalFogParametersAtPoint				m_globalFog;
	CEnvGlobalSkyParametersAtPoint				m_sky;
	CEnvDepthOfFieldParametersAtPoint			m_depthOfField;
	CEnvColorModTransparencyParametersAtPoint	m_colorModTransparency;
	CEnvShadowsParametersAtPoint				m_shadows;
	CEnvWaterParametersAtPoint					m_water;
	CEnvColorGroupsParametersAtPoint			m_colorGroups;	
	CEnvFlareColorGroupsParametersAtPoint		m_flareColorGroups;	
	CEnvSunAndMoonParametersAtPoint				m_sunParams;
	CEnvWindParametersAtPoint					m_windParams;
	CEnvGameplayEffectsParametersAtPoint		m_gameplayEffects;
	CEnvMotionBlurParametersAtPoint				m_motionBlurParameters;
	CEnvCameraLightsSetupParametersAtPoint		m_cameraLightsSetup;

public:
	CAreaEnvironmentParamsAtPoint() {};
	CAreaEnvironmentParamsAtPoint( const CAreaEnvironmentParams& source );

	const CEnvFlareColorParametersAtPoint& GetFlareColorParameters( EEnvFlareColorGroup group ) const;
	Vector GetFlareColor( EEnvFlareColorGroup group, Uint32 paramIndex ) const;
	Float GetFlareOpacity( EEnvFlareColorGroup group, Uint32 paramIndex ) const;
};

/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../../../bin/shaders/include/globalConstants.fx"
#include "envParameters.h"
#include "environmentAreaParams.h"
#include "bitmapTexture.h"

IMPLEMENT_ENGINE_CLASS( CEnvSpeedTreeRandomColorParameters );
IMPLEMENT_ENGINE_CLASS( CEnvSpeedTreeParameters );
IMPLEMENT_ENGINE_CLASS( CEnvGlobalLightParameters );
IMPLEMENT_ENGINE_CLASS( CEnvAmbientProbesGenParameters );
IMPLEMENT_ENGINE_CLASS( CEnvReflectionProbesGenParameters );
IMPLEMENT_ENGINE_CLASS( CEnvInteriorFallbackParameters );
IMPLEMENT_ENGINE_CLASS( CEnvCameraLightParameters );
IMPLEMENT_ENGINE_CLASS( CEnvCameraLightsSetupParameters );
IMPLEMENT_ENGINE_CLASS( CEnvToneMappingParameters );
IMPLEMENT_ENGINE_CLASS( CEnvToneMappingCurveParameters );
IMPLEMENT_ENGINE_CLASS( CEnvBloomNewParameters );
IMPLEMENT_ENGINE_CLASS( CEnvDistanceRangeParameters );
IMPLEMENT_ENGINE_CLASS( CEnvColorModTransparencyParameters );
IMPLEMENT_ENGINE_CLASS( CEnvShadowsParameters );
IMPLEMENT_ENGINE_CLASS( CEnvGlobalFogParameters );
IMPLEMENT_ENGINE_CLASS( CEnvDepthOfFieldParameters );
IMPLEMENT_ENGINE_CLASS( CEnvNVSSAOParameters );
IMPLEMENT_ENGINE_CLASS( CEnvMSSSAOParameters );
IMPLEMENT_ENGINE_CLASS( CEnvParametricBalanceParameters );
IMPLEMENT_ENGINE_CLASS( CEnvFinalColorBalanceParameters );
IMPLEMENT_ENGINE_CLASS( CEnvSharpenParameters );
IMPLEMENT_ENGINE_CLASS( CEnvPaintEffectParameters );
IMPLEMENT_ENGINE_CLASS( CEnvWaterParameters );
IMPLEMENT_ENGINE_CLASS( CEnvGameplayEffectsParameters );
IMPLEMENT_ENGINE_CLASS( CEnvMotionBlurParameters );
IMPLEMENT_ENGINE_CLASS( CEnvGlobalSkyParameters );
IMPLEMENT_ENGINE_CLASS( CEnvSunAndMoonParameters );
IMPLEMENT_ENGINE_CLASS( CEnvWindParameters );
IMPLEMENT_ENGINE_CLASS( CEnvDialogLightParameters );
IMPLEMENT_RTTI_ENUM(    EEnvColorGroup ); 
IMPLEMENT_RTTI_ENUM(    EEnvFlareColorGroup );
IMPLEMENT_RTTI_ENUM(    EEnvAutoHideGroup );
IMPLEMENT_ENGINE_CLASS( CEnvColorGroupsParameters );
IMPLEMENT_ENGINE_CLASS( CAreaEnvironmentParams );
IMPLEMENT_ENGINE_CLASS( CEnvFlareColorParameters );
IMPLEMENT_ENGINE_CLASS( CEnvFlareColorGroupsParameters );

Float			CEnvGlobalFogParameters::m_fogStartOffset = 0.0f;
Float			CEnvGlobalFogParameters::m_fogDensityMultiplier = 1.0f;


#define INIT_FLOAT_ACTIVATE_ATTRIB( attrib )							do { attrib = (EnvResetMode_CurvesDefault == mode);	} while (false)
#define INIT_GRAPH_COLOR(			graph, color )						do { graph.Reset( SCT_Color, 1.f, 0.f );				if ( EnvResetMode_CurvesDefault == mode ) graph.AddPoint( 0.1f, SSimpleCurvePoint::BuildValue( color, 1.f ) );				} while (false)
#define INIT_GRAPH_COLORSCALED(		graph, color, scale, editScale )	do { graph.Reset( SCT_ColorScaled, editScale, 0.f );	if ( EnvResetMode_CurvesDefault == mode ) graph.AddPoint( 0.1f, SSimpleCurvePoint::BuildValue( color, scale ) );			} while (false)
#define INIT_GRAPH_FLOAT(			graph, scalar, editScale )			do { graph.Reset( SCT_Float, editScale, 0.f );			if ( EnvResetMode_CurvesDefault == mode ) graph.AddPoint( 0.1f, SSimpleCurvePoint::BuildValue( Color::WHITE, scalar ) );	} while (false)

// HACK for underwater colors addition (post release feature)
#define INIT_GRAPH_UNDERWATER_COLOR(graph)	do{ graph.Reset( SCT_Color, 1.f, 0.f ); if ( EnvResetMode_CurvesDefault == mode ) { \
		const Color defaultUnderwaterColorDay = Color( (Uint8)153, (Uint8)204, (Uint8)178 );										\
		const Color defaultUnderwaterColorSunset = Color( (Uint8)30, (Uint8)40, (Uint8)34 );										\
		const Color defaultUnderwaterColorNight = Color( (Uint8)10, (Uint8)15, (Uint8)12 );										\
		graph.AddPoint( 0.15f, SSimpleCurvePoint::BuildValue( defaultUnderwaterColorNight, 1.f ) );								\
		graph.AddPoint( 0.2f, SSimpleCurvePoint::BuildValue( defaultUnderwaterColorSunset, 1.f ) );								\
		graph.AddPoint( 0.5f, SSimpleCurvePoint::BuildValue( defaultUnderwaterColorDay, 1.f ) );									\
		graph.AddPoint( 0.8f, SSimpleCurvePoint::BuildValue( defaultUnderwaterColorSunset, 1.f ) );								\
		graph.AddPoint( 0.85f, SSimpleCurvePoint::BuildValue( defaultUnderwaterColorNight, 1.f ) );		} } while (false)


#define LERP_GROUP_BEGIN( attrib )						do {										\
	Bool *activ_curr = &this->attrib;			\
	const Bool  activ_src  = src.attrib;		\
	const Bool  activ_dest = dest.attrib;

template< class _T >
Float ModifyCurveLerpAttribute( const _T &srcAttrib, const _T &destAttrib, Float lerpFactor )
{
	return lerpFactor;
}

RED_INLINE Float ModifyCurveLerpAttributeByCondition( Bool srcCondition, Bool destCondition, Float lerpFactor )
{
	// condition is 

	if ( srcCondition && !destCondition )
	{
		return 0;
	}

	if ( !srcCondition && destCondition )
	{
		ASSERT( !"It doesn't happen in practice because we start envs blending with defaultCurves environment, which has all conditions enabled (filled in curves with default values and float params activated with bools)" );
		return 1;
	}

	return lerpFactor;
}

template<>
Float ModifyCurveLerpAttribute<SSimpleCurve>( const SSimpleCurve &srcAttrib, const SSimpleCurve &destAttrib, Float lerpFactor )
{
	return ModifyCurveLerpAttributeByCondition( !srcAttrib.IsEmpty(), !destAttrib.IsEmpty(), lerpFactor );
}

Float ModifyCurveLerpAttributeAndConditionByCondition( Bool srcCondition, Bool destCondition, Float lerpFactor, Bool &refDestCondition )
{
	if ( destCondition )
	{
		Float val = ModifyCurveLerpAttributeByCondition( srcCondition, destCondition, lerpFactor );
		refDestCondition = true;
		return val;
	}

	refDestCondition = srcCondition;
	return 0;
}

#define LERP_ATTRIBUTE( attrib )										if ( activ_dest )		{ attrib.ImportDayPointValue( src.attrib, dest.attrib, ModifyCurveLerpAttribute( src.attrib, dest.attrib, lerpFactor ), time ); *activ_curr = true; }	\
																		else					{ attrib.ImportDayPointValue( src.attrib, src.attrib, 0.f, time ); *activ_curr = activ_src; } 

#define STEP_ATTRIBUTE( attrib )										if ( activ_dest )		{ attrib.ImportDayPointValue( src.attrib, dest.attrib, ModifyCurveLerpAttribute( src.attrib, dest.attrib, lerpFactor ) > 0 ? 1.f : 0.f, time ); *activ_curr = true; }	\
																		else					{ attrib.ImportDayPointValue( src.attrib, src.attrib, 0.f, time ); *activ_curr = activ_src; } 

#define	LERP_ATTRIBUTE_VALUE( attrib, conditionAttrib )					if ( activ_dest )		{ attrib = Lerp( ModifyCurveLerpAttributeAndConditionByCondition( src.conditionAttrib, dest.conditionAttrib, lerpFactor, conditionAttrib ), src.attrib, dest.attrib ); *activ_curr = true; }	\
																		else					{ attrib = src.attrib; conditionAttrib = src.conditionAttrib; *activ_curr = activ_src; }

#define STEP_ATTRIBUTE_VALUE( attrib )									if ( activ_dest )		{ attrib = lerpFactor > 0 ? dest.attrib : src.attrib; *activ_curr = true; }	\
																		else					{ attrib = src.attrib; *activ_curr = activ_src; }

#define LERP_GROUP_END()												} while (false);

#define LERP_STRUCT_ATTRIBUTE( attrib ) attrib.ImportDayPointValue( src.attrib, dest.attrib, ModifyCurveLerpAttribute( src.attrib, dest.attrib, lerpFactor ), time );
#define STEP_STRUCT_ATTRIBUTE( attrib ) attrib.ImportDayPointValue( src.attrib, dest.attrib, ModifyCurveLerpAttribute( src.attrib, dest.attrib, lerpFactor ) > 0 ? 1.f : 0.f, time );

// --------------------------------------------------------------------------

CEnvAmbientProbesGenParameters::CEnvAmbientProbesGenParameters( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvAmbientProbesGenParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_COLORSCALED( m_colorAmbient		, Color(255,255,255), 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_colorSceneAdd		, Color(255,255,255), 0.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_colorSkyTop		, Color(255,255,255), 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_colorSkyHorizon	, Color(255,255,255), 1.f, 1.f );
	INIT_GRAPH_FLOAT(		m_skyShape			, 1.f, 4.f );
}

void CEnvAmbientProbesGenParameters::ImportDayPointValue( const CEnvAmbientProbesGenParameters &src, const CEnvAmbientProbesGenParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_colorAmbient );
		LERP_ATTRIBUTE( m_colorSceneAdd );
		LERP_ATTRIBUTE( m_colorSkyTop );
		LERP_ATTRIBUTE( m_colorSkyHorizon );
		LERP_ATTRIBUTE( m_skyShape );
	LERP_GROUP_END()
}

CEnvAmbientProbesGenParametersAtPoint::CEnvAmbientProbesGenParametersAtPoint( const CEnvAmbientProbesGenParameters& source )
	: m_activated( source.m_activated )
	, m_colorAmbient( source.m_colorAmbient.GetCachedPoint() )
	, m_colorSceneAdd( source.m_colorSceneAdd.GetCachedPoint() )
	, m_colorSkyTop( source.m_colorSkyTop.GetCachedPoint() )
	, m_colorSkyHorizon( source.m_colorSkyHorizon.GetCachedPoint() )
	, m_skyShape( source.m_skyShape.GetCachedPoint() )
{}

void CEnvAmbientProbesGenParametersAtPoint::SetPreviewPanelValues()
{
	m_activated = true;
	m_colorAmbient.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
	m_colorSceneAdd.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
	m_colorSkyTop.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
	m_colorSkyHorizon.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
	m_skyShape.SetValueScalar( 1.f );
}

// -------------------------------------------------------------------------

CEnvCameraLightParameters::CEnvCameraLightParameters( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvCameraLightParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_COLORSCALED( m_color				, Color(255,255,255), 0.f, 1.f );
	INIT_GRAPH_FLOAT( 		m_attenuation		, 0.5f, 1.f );
	INIT_GRAPH_FLOAT( 		m_radius			, 10.f, 50.f );
	INIT_GRAPH_FLOAT( 		m_offsetFront		, 0.f, 50.f );
	INIT_GRAPH_FLOAT(		m_offsetRight		, 0.f, 50.f );
	INIT_GRAPH_FLOAT(		m_offsetUp			, 0.f, 50.f );
}

void CEnvCameraLightParameters::ImportDayPointValue( const CEnvCameraLightParameters &src, const CEnvCameraLightParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_color );
		LERP_ATTRIBUTE( m_attenuation );
		LERP_ATTRIBUTE( m_radius );
		LERP_ATTRIBUTE( m_offsetFront );
		LERP_ATTRIBUTE( m_offsetRight );
		LERP_ATTRIBUTE( m_offsetUp );
	LERP_GROUP_END()
}

CEnvCameraLightParametersAtPoint::CEnvCameraLightParametersAtPoint( const CEnvCameraLightParameters& source )
	: m_activated( source.m_activated )
	, m_color( source.m_color.GetCachedPoint() )
	, m_attenuation( source.m_attenuation.GetCachedPoint() )
	, m_radius( source.m_radius.GetCachedPoint() )
	, m_offsetFront( source.m_offsetFront.GetCachedPoint() )
	, m_offsetRight( source.m_offsetRight.GetCachedPoint() )
	, m_offsetUp( source.m_offsetUp.GetCachedPoint() )
{}

void CEnvCameraLightParametersAtPoint::SetDisabledValues()
{
	m_activated = true;
	m_color.SetValue( Color ( 255, 255, 255, 255 ), 0.f );
	m_attenuation.SetValueScalar( 0.5f );
	m_radius.SetValueScalar( 10.f );
	m_offsetFront.SetValueScalar( 0.f );
	m_offsetRight.SetValueScalar( 0.f );
	m_offsetUp.SetValueScalar( 0.f );
}

// -------------------------------------------------------------------------

CEnvCameraLightsSetupParameters::CEnvCameraLightsSetupParameters( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvCameraLightsSetupParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	m_gameplayLight0.Reset( mode );
	m_gameplayLight1.Reset( mode );
	m_sceneLight0.Reset( mode );
	m_sceneLight1.Reset( mode );
	m_dialogLight0.Reset( mode );
	m_dialogLight1.Reset( mode );
	m_interiorLight0.Reset( mode );
	m_interiorLight1.Reset( mode );
	INIT_GRAPH_FLOAT( m_playerInInteriorLightsScale, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_sceneLightColorInterior0, Color::WHITE, 0.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_sceneLightColorInterior1, Color::WHITE, 0.f, 1.f );
	INIT_GRAPH_FLOAT( m_cameraLightsNonCharacterScale, 1.f, 5.f );
}

void CEnvCameraLightsSetupParameters::ImportDayPointValue( const CEnvCameraLightsSetupParameters &src, const CEnvCameraLightsSetupParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_gameplayLight0 );
		LERP_ATTRIBUTE( m_gameplayLight1 );
		LERP_ATTRIBUTE( m_sceneLight0 );
		LERP_ATTRIBUTE( m_sceneLight1 );
		LERP_ATTRIBUTE( m_dialogLight0 );
		LERP_ATTRIBUTE( m_dialogLight1 );
		LERP_ATTRIBUTE( m_interiorLight0 );
		LERP_ATTRIBUTE( m_interiorLight1 );
		LERP_ATTRIBUTE( m_playerInInteriorLightsScale );
		LERP_ATTRIBUTE( m_sceneLightColorInterior0 );
		LERP_ATTRIBUTE( m_sceneLightColorInterior1 );
		LERP_ATTRIBUTE( m_cameraLightsNonCharacterScale );
	LERP_GROUP_END()
}

CEnvCameraLightsSetupParametersAtPoint::CEnvCameraLightsSetupParametersAtPoint( const CEnvCameraLightsSetupParameters& source )
	: m_activated( source.m_activated )
	, m_gameplayLight0( source.m_gameplayLight0 )
	, m_gameplayLight1( source.m_gameplayLight1 )
	, m_sceneLight0( source.m_sceneLight0 )
	, m_sceneLight1( source.m_sceneLight1 )
	, m_dialogLight0( source.m_dialogLight0 )
	, m_dialogLight1( source.m_dialogLight1 )
	, m_interiorLight0( source.m_interiorLight0 )
	, m_interiorLight1( source.m_interiorLight1 )
	, m_playerInInteriorLightsScale( source.m_playerInInteriorLightsScale.GetCachedPoint() )
	, m_sceneLightColorInterior0( source.m_sceneLightColorInterior0.GetCachedPoint() )
	, m_sceneLightColorInterior1( source.m_sceneLightColorInterior1.GetCachedPoint() )
	, m_cameraLightsNonCharacterScale( source.m_cameraLightsNonCharacterScale.GetCachedPoint() )
{}

void CEnvCameraLightsSetupParametersAtPoint::SetDisabledValues()
{
	m_activated = true;
	m_gameplayLight0.SetDisabledValues();
	m_gameplayLight1.SetDisabledValues();
	m_sceneLight0.SetDisabledValues();
	m_sceneLight1.SetDisabledValues();
	m_dialogLight0.SetDisabledValues();
	m_dialogLight1.SetDisabledValues();
	m_interiorLight0.SetDisabledValues();
	m_interiorLight1.SetDisabledValues();
	m_playerInInteriorLightsScale.SetValueScalar( 1.f ); // neutral
	m_sceneLightColorInterior0.SetValue( Color ( 255, 255, 255, 255 ), 0.f );
	m_sceneLightColorInterior1.SetValue( Color ( 255, 255, 255, 255 ), 0.f );
	m_cameraLightsNonCharacterScale.SetValueScalar( 1.f ); //< neutral
}

namespace Helper
{
	//Copy of ECameraLightBitfield from storySceneEventCameraLight.h
	enum ECameraLightBitfield
	{
		ECLB_AbsoluteBrightness = FLAG( 0 ),
		ECLB_AbsoluteRadius		= FLAG( 1 ),
		ECLB_AbsoluteOffset		= FLAG( 2 ),
	};
};

Float CEnvCameraLightParametersAtPoint::GetRadius( const SCameraLightModifiers &lightModifier ) const
{
	return lightModifier.absoluteValues & Helper::ECLB_AbsoluteRadius ? lightModifier.radiusScale : m_radius.GetScalarClampMin( 0.f ) * Max( 0.f, lightModifier.radiusScale );
}

Vector CEnvCameraLightParametersAtPoint::GetColor( const SCameraLightModifiers &lightModifier, const Vector &interiorLightColor, Float interiorAmount ) const
{
	const Vector colorBase = Lerp( interiorAmount, m_color.GetColorScaledGammaToLinear( true ), interiorLightColor ) * lightModifier.brightnessScale;
	const Vector colorBaseCustomBrightness = Lerp( interiorAmount, m_color.GetColorGammaToLinear( true ), interiorLightColor ) * lightModifier.brightnessScale;
	const Vector colorTarget = lightModifier.GetColorOverride();
	return Lerp( Clamp( lightModifier.colorOverrideAmount, 0.f, 1.f ), lightModifier.absoluteValues & Helper::ECLB_AbsoluteBrightness ? colorBaseCustomBrightness : colorBase, colorTarget );
}

Vector CEnvCameraLightParametersAtPoint::GetOffsetForwardRightUp( const SCameraLightModifiers &lightModifier ) const
{
	const Vector& referenceVec = lightModifier.absoluteValues & Helper::ECLB_AbsoluteOffset ? Vector::ZERO_3D_POINT : Vector( m_offsetFront.GetScalar(), m_offsetRight.GetScalar(), m_offsetUp.GetScalar() );
	return referenceVec + Vector( lightModifier.offsetFront, lightModifier.offsetRight, lightModifier.offsetUp );
}

Float CEnvCameraLightParametersAtPoint::GetAttenuation( const SCameraLightModifiers &lightModifier ) const
{
	const Float attBase = m_attenuation.GetScalarClamp( 0.f, 1.f );
	const Float attTarget = Clamp( lightModifier.attenuationOverride, 0.f, 1.f );
	return Lerp( Clamp( lightModifier.attenuationOverrideAmount, 0.f, 1.f ), attBase, attTarget );
}

// --------------------------------------------------------------------------

CEnvReflectionProbesGenParameters::CEnvReflectionProbesGenParameters( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvReflectionProbesGenParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_COLORSCALED( m_colorAmbient		, Color(255,255,255), 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_colorSceneMul		, Color(255,255,255), 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_colorSceneAdd		, Color(255,255,255), 0.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_colorSkyMul		, Color(255,255,255), 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_colorSkyAdd		, Color(255,255,255), 0.f, 1.f );
	INIT_GRAPH_FLOAT(		m_remapOffset		, 0.f, 50.f );
	INIT_GRAPH_FLOAT(		m_remapStrength		, 0.f, 50.f );
	INIT_GRAPH_FLOAT(		m_remapClamp		, 500.f, 1000.f );
}

void CEnvReflectionProbesGenParameters::ImportDayPointValue( const CEnvReflectionProbesGenParameters &src, const CEnvReflectionProbesGenParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_colorAmbient );
		LERP_ATTRIBUTE( m_colorSceneMul );
		LERP_ATTRIBUTE( m_colorSceneAdd );
		LERP_ATTRIBUTE( m_colorSkyMul );
		LERP_ATTRIBUTE( m_colorSkyAdd );
		LERP_ATTRIBUTE( m_remapOffset );
		LERP_ATTRIBUTE( m_remapStrength );
		LERP_ATTRIBUTE( m_remapClamp );
	LERP_GROUP_END()
}

CEnvReflectionProbesGenParametersAtPoint::CEnvReflectionProbesGenParametersAtPoint( const CEnvReflectionProbesGenParameters& source )
: m_activated( source.m_activated )
, m_colorAmbient( source.m_colorAmbient.GetCachedPoint() )
, m_colorSceneMul( source.m_colorSceneMul.GetCachedPoint() )
, m_colorSceneAdd( source.m_colorSceneAdd.GetCachedPoint() )
, m_colorSkyMul( source.m_colorSkyMul.GetCachedPoint() )
, m_colorSkyAdd( source.m_colorSkyAdd.GetCachedPoint() )
, m_remapOffset( source.m_remapOffset.GetCachedPoint() )
, m_remapStrength( source.m_remapStrength.GetCachedPoint() )
, m_remapClamp( source.m_remapClamp.GetCachedPoint() )
{}

void CEnvReflectionProbesGenParametersAtPoint::SetPreviewPanelValues()
{
	m_activated = true;
	m_colorAmbient.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
	m_colorSceneMul.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
	m_colorSceneAdd.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
	m_colorSkyMul.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
	m_colorSkyAdd.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
	m_remapOffset.SetValueScalar( 0.f );
	m_remapStrength.SetValueScalar( 0.f );
	m_remapClamp.SetValueScalar( 500.f );
}

// --------------------------------------------------------------------------

CEnvInteriorFallbackParameters::CEnvInteriorFallbackParameters( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvInteriorFallbackParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_COLORSCALED( m_colorAmbientMul			, Color(255,255,255), 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_colorReflectionLow		, Color(255,255,255), 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_colorReflectionMiddle		, Color(255,255,255), 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_colorReflectionHigh		, Color(255,255,255), 1.f, 1.f );
}

void CEnvInteriorFallbackParameters::ImportDayPointValue( const CEnvInteriorFallbackParameters &src, const CEnvInteriorFallbackParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
	LERP_ATTRIBUTE( m_colorAmbientMul );
	LERP_ATTRIBUTE( m_colorReflectionLow );
	LERP_ATTRIBUTE( m_colorReflectionMiddle );
	LERP_ATTRIBUTE( m_colorReflectionHigh );
	LERP_GROUP_END()
}

CEnvInteriorFallbackParametersAtPoint::CEnvInteriorFallbackParametersAtPoint( const CEnvInteriorFallbackParameters& source )
	: m_activated( source.m_activated )
	, m_colorAmbientMul( source.m_colorAmbientMul.GetCachedPoint() )
	, m_colorReflectionLow( source.m_colorReflectionLow.GetCachedPoint() )
	, m_colorReflectionMiddle( source.m_colorReflectionMiddle.GetCachedPoint() )
	, m_colorReflectionHigh( source.m_colorReflectionHigh.GetCachedPoint() )
{}

// --------------------------------------------------------------------------

CEnvGlobalLightParameters::CEnvGlobalLightParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvGlobalLightParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_FLOAT_ACTIVATE_ATTRIB( m_activatedGlobalLightActivated );
	m_globalLightActivated = 1.0f;
	INIT_FLOAT_ACTIVATE_ATTRIB( m_activatedActivatedFactorLightDir );
	m_activatedFactorLightDir = 0;
	INIT_GRAPH_FLOAT(		m_forcedLightDirAnglesYaw		, 135.0f, 360.f );
	INIT_GRAPH_FLOAT(		m_forcedLightDirAnglesPitch		, 30.0f, 360.f );
	INIT_GRAPH_FLOAT(		m_forcedLightDirAnglesRoll		, 0.0f, 360.f );
	INIT_GRAPH_FLOAT(		m_forcedSunDirAnglesYaw			, 135.0f, 360.f );
	INIT_GRAPH_FLOAT(		m_forcedSunDirAnglesPitch		, 30.0f, 360.f );
	INIT_GRAPH_FLOAT(		m_forcedSunDirAnglesRoll		, 0.0f, 360.f );
	INIT_GRAPH_FLOAT(		m_forcedMoonDirAnglesYaw		, 135.0f, 360.f );
	INIT_GRAPH_FLOAT(		m_forcedMoonDirAnglesPitch		, 30.0f, 360.f );
	INIT_GRAPH_FLOAT(		m_forcedMoonDirAnglesRoll		, 0.0f, 360.f );
	INIT_GRAPH_COLORSCALED( m_sunColor							, Color::WHITE, 1.f, 1.f);
	INIT_GRAPH_COLORSCALED( m_sunColorLightSide					, Color::WHITE, 1.f, 1.f);
	INIT_GRAPH_COLORSCALED( m_sunColorLightOppositeSide			, Color::WHITE, 1.f, 1.f);
	INIT_GRAPH_FLOAT( m_sunColorCenterArea,		100.f, 100.f );
	INIT_GRAPH_FLOAT( m_sunColorSidesMargin,	100.f, 100.f );
	INIT_GRAPH_FLOAT( m_sunColorBottomHeight,	0.f, 100.f );
	INIT_GRAPH_FLOAT( m_sunColorTopHeight,		0.f, 100.f );
	INIT_GRAPH_FLOAT( m_translucencyViewDependency, 0.75f, 1.f );
	INIT_GRAPH_FLOAT( m_translucencyBaseFlatness, 0.2f, 1.f );
	INIT_GRAPH_FLOAT( m_translucencyFlatBrightness, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_translucencyGainBrightness, 0.25f, 1.f );
	INIT_GRAPH_FLOAT( m_translucencyFresnelScaleLight, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_translucencyFresnelScaleReflection, 1.f, 1.f );
	m_envProbeBaseLightingAmbient.Reset( mode );
	m_envProbeBaseLightingReflection.Reset( mode );
	INIT_GRAPH_FLOAT( m_charactersLightingBoostAmbientLight, 1.f, 2.f );
	INIT_GRAPH_FLOAT( m_charactersLightingBoostAmbientShadow, 1.f, 2.f );
	INIT_GRAPH_FLOAT( m_charactersLightingBoostReflectionLight, 1.f, 2.f );
	INIT_GRAPH_FLOAT( m_charactersLightingBoostReflectionShadow, 1.f, 2.f );
	INIT_GRAPH_COLORSCALED( m_charactersEyeBlicksColor, Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_charactersEyeBlicksShadowedScale, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_envProbeAmbientScaleLight, 1.f, 2.f );
	INIT_GRAPH_FLOAT( m_envProbeAmbientScaleShadow, 1.f, 2.f );
	INIT_GRAPH_FLOAT( m_envProbeReflectionScaleLight, 1.f, 2.f );
	INIT_GRAPH_FLOAT( m_envProbeReflectionScaleShadow, 1.f, 2.f );
	INIT_GRAPH_FLOAT( m_envProbeDistantScaleFactor, 1.f, 2.f );
}

void CEnvGlobalLightParameters::ImportDayPointValue( const CEnvGlobalLightParameters &src, const CEnvGlobalLightParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_sunColor );
		LERP_ATTRIBUTE( m_sunColorLightSide );
		LERP_ATTRIBUTE( m_sunColorLightOppositeSide );
		LERP_ATTRIBUTE( m_sunColorCenterArea );
		LERP_ATTRIBUTE( m_sunColorSidesMargin );
		LERP_ATTRIBUTE( m_sunColorBottomHeight );
		LERP_ATTRIBUTE( m_sunColorTopHeight );
		LERP_ATTRIBUTE( m_forcedLightDirAnglesYaw );		
	 	LERP_ATTRIBUTE( m_forcedLightDirAnglesPitch );		
	 	LERP_ATTRIBUTE( m_forcedLightDirAnglesRoll );		
	 	LERP_ATTRIBUTE( m_forcedSunDirAnglesYaw );		
	 	LERP_ATTRIBUTE( m_forcedSunDirAnglesPitch );		
	 	LERP_ATTRIBUTE( m_forcedSunDirAnglesRoll );	
	 	LERP_ATTRIBUTE( m_forcedMoonDirAnglesYaw );		
	 	LERP_ATTRIBUTE( m_forcedMoonDirAnglesPitch );		
	 	LERP_ATTRIBUTE( m_forcedMoonDirAnglesRoll );	
		LERP_ATTRIBUTE_VALUE( m_globalLightActivated, m_activatedGlobalLightActivated );
		LERP_ATTRIBUTE_VALUE( m_activatedFactorLightDir, m_activatedActivatedFactorLightDir );
		LERP_ATTRIBUTE( m_translucencyViewDependency );
		LERP_ATTRIBUTE( m_translucencyBaseFlatness );
		LERP_ATTRIBUTE( m_translucencyFlatBrightness );
		LERP_ATTRIBUTE( m_translucencyGainBrightness );
		LERP_ATTRIBUTE( m_translucencyFresnelScaleLight );
		LERP_ATTRIBUTE( m_translucencyFresnelScaleReflection );
		LERP_ATTRIBUTE( m_envProbeBaseLightingAmbient );
		LERP_ATTRIBUTE( m_envProbeBaseLightingReflection );
		LERP_ATTRIBUTE( m_charactersLightingBoostAmbientLight );
		LERP_ATTRIBUTE( m_charactersLightingBoostAmbientShadow );
		LERP_ATTRIBUTE( m_charactersLightingBoostReflectionLight );
		LERP_ATTRIBUTE( m_charactersLightingBoostReflectionShadow );
		LERP_ATTRIBUTE( m_charactersEyeBlicksColor );
		LERP_ATTRIBUTE( m_charactersEyeBlicksShadowedScale );
		LERP_ATTRIBUTE( m_envProbeAmbientScaleLight );
		LERP_ATTRIBUTE( m_envProbeAmbientScaleShadow );
		LERP_ATTRIBUTE( m_envProbeReflectionScaleLight );
		LERP_ATTRIBUTE( m_envProbeReflectionScaleShadow );
		LERP_ATTRIBUTE( m_envProbeDistantScaleFactor );
	LERP_GROUP_END()
}

CEnvGlobalLightParametersAtPoint::CEnvGlobalLightParametersAtPoint( const CEnvGlobalLightParameters& source )
	: m_activated( source.m_activated )
	, m_activatedGlobalLightActivated( source.m_activatedGlobalLightActivated )
	, m_globalLightActivated( source.m_globalLightActivated )
	, m_activatedActivatedFactorLightDir( source.m_activatedActivatedFactorLightDir )
	, m_activatedFactorLightDir( source.m_activatedFactorLightDir )
	, m_sunColor( source.m_sunColor.GetCachedPoint() )
	, m_sunColorLightSide( source.m_sunColorLightSide.GetCachedPoint() )
	, m_sunColorLightOppositeSide( source.m_sunColorLightOppositeSide.GetCachedPoint() )
	, m_sunColorCenterArea( source.m_sunColorCenterArea.GetCachedPoint() )
	, m_sunColorSidesMargin( source.m_sunColorSidesMargin.GetCachedPoint() )
	, m_sunColorBottomHeight( source.m_sunColorBottomHeight.GetCachedPoint() )
	, m_sunColorTopHeight( source.m_sunColorTopHeight.GetCachedPoint() )
	, m_forcedLightDirAngles( source.m_forcedLightDirAnglesRoll.GetCachedPoint().GetScalar(), source.m_forcedLightDirAnglesPitch.GetCachedPoint().GetScalar(), source.m_forcedLightDirAnglesYaw.GetCachedPoint().GetScalar() )
	, m_forcedSunDirAngles( source.m_forcedSunDirAnglesRoll.GetCachedPoint().GetScalar(), source.m_forcedSunDirAnglesPitch.GetCachedPoint().GetScalar(), source.m_forcedSunDirAnglesYaw.GetCachedPoint().GetScalar() )
	, m_forcedMoonDirAngles( source.m_forcedMoonDirAnglesRoll.GetCachedPoint().GetScalar(), source.m_forcedMoonDirAnglesPitch.GetCachedPoint().GetScalar(), source.m_forcedMoonDirAnglesYaw.GetCachedPoint().GetScalar() )
	, m_translucencyViewDependency( source.m_translucencyViewDependency.GetCachedPoint() )
	, m_translucencyBaseFlatness( source.m_translucencyBaseFlatness.GetCachedPoint() )
	, m_translucencyFlatBrightness( source.m_translucencyFlatBrightness.GetCachedPoint() )
	, m_translucencyGainBrightness( source.m_translucencyGainBrightness.GetCachedPoint() )
	, m_translucencyFresnelScaleLight( source.m_translucencyFresnelScaleLight.GetCachedPoint() )
	, m_translucencyFresnelScaleReflection( source.m_translucencyFresnelScaleReflection.GetCachedPoint() )
	, m_envProbeBaseLightingAmbient( source.m_envProbeBaseLightingAmbient )
	, m_envProbeBaseLightingReflection( source.m_envProbeBaseLightingReflection )
	, m_charactersLightingBoostAmbientLight( source.m_charactersLightingBoostAmbientLight.GetCachedPoint() )
	, m_charactersLightingBoostAmbientShadow( source.m_charactersLightingBoostAmbientShadow.GetCachedPoint() )
	, m_charactersLightingBoostReflectionLight( source.m_charactersLightingBoostReflectionLight.GetCachedPoint() )
	, m_charactersLightingBoostReflectionShadow( source.m_charactersLightingBoostReflectionShadow.GetCachedPoint() )
	, m_charactersEyeBlicksColor( source.m_charactersEyeBlicksColor.GetCachedPoint() )
	, m_charactersEyeBlicksShadowedScale( source.m_charactersEyeBlicksShadowedScale.GetCachedPoint() )
	, m_envProbeAmbientScaleLight( source.m_envProbeAmbientScaleLight.GetCachedPoint() )
	, m_envProbeAmbientScaleShadow( source.m_envProbeAmbientScaleShadow.GetCachedPoint() )
	, m_envProbeReflectionScaleLight( source.m_envProbeReflectionScaleLight.GetCachedPoint() )
	, m_envProbeReflectionScaleShadow( source.m_envProbeReflectionScaleShadow.GetCachedPoint() )
	, m_envProbeDistantScaleFactor( source.m_envProbeDistantScaleFactor.GetCachedPoint() )
{}

// --------------------------------------------------------------------------

CEnvSpeedTreeRandomColorParameters::CEnvSpeedTreeRandomColorParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvSpeedTreeRandomColorParameters::Reset( EEnvParamsResetMode mode )
{
	INIT_GRAPH_COLOR( m_luminanceWeights, RGB_LUMINANCE_WEIGHTS_COLOR );
	INIT_GRAPH_COLORSCALED( m_randomColor0, Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_randomColor1, Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_randomColor2, Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_saturation0, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_saturation1, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_saturation2, 1.f, 1.f );
}

void CEnvSpeedTreeRandomColorParameters::ImportDayPointValue( const CEnvSpeedTreeRandomColorParameters &src, const CEnvSpeedTreeRandomColorParameters &dest, Float lerpFactor, Float time )
{
	LERP_STRUCT_ATTRIBUTE( m_luminanceWeights );
	LERP_STRUCT_ATTRIBUTE( m_randomColor0 );
	LERP_STRUCT_ATTRIBUTE( m_saturation0 );
	LERP_STRUCT_ATTRIBUTE( m_randomColor1 );
	LERP_STRUCT_ATTRIBUTE( m_saturation1 );
	LERP_STRUCT_ATTRIBUTE( m_randomColor2 );
	LERP_STRUCT_ATTRIBUTE( m_saturation2 );
}

CEnvSpeedTreeRandomColorParametersAtPoint::CEnvSpeedTreeRandomColorParametersAtPoint( const CEnvSpeedTreeRandomColorParameters& source )
	: m_luminanceWeights ( source.m_luminanceWeights.GetCachedPoint() )
	, m_randomColor0 ( source.m_randomColor0.GetCachedPoint() )
	, m_saturation0 ( source.m_saturation0.GetCachedPoint() )
	, m_randomColor1 ( source.m_randomColor1.GetCachedPoint() )
	, m_saturation1 ( source.m_saturation1.GetCachedPoint() )
	, m_randomColor2 ( source.m_randomColor2.GetCachedPoint() )
	, m_saturation2 ( source.m_saturation2.GetCachedPoint() )
{
}

void CEnvSpeedTreeRandomColorParametersAtPoint::BuildShaderParams( Vector &outLumWeights, Vector &outColorParams0, Vector &outColorParams1, Vector &outColorParams2 ) const
{
	Vector col0 = m_randomColor0.GetColorScaledGammaToLinear( true );
	Vector col1 = m_randomColor1.GetColorScaledGammaToLinear( true );
	Vector col2 = m_randomColor2.GetColorScaledGammaToLinear( true );
	outColorParams0 = Vector ( 1.f - col0.X, 1.f - col0.Y, 1.f - col0.Z, m_saturation0.GetScalar() );
	outColorParams1 = Vector ( 1.f - col1.X, 1.f - col1.Y, 1.f - col1.Z, m_saturation1.GetScalar() );
	outColorParams2 = Vector ( 1.f - col2.X, 1.f - col2.Y, 1.f - col2.Z, m_saturation2.GetScalar() );
	outLumWeights = m_luminanceWeights.GetColorNotScaled( true );
}

// --------------------------------------------------------------------------

CEnvSpeedTreeParameters::CEnvSpeedTreeParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvSpeedTreeParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_COLORSCALED( m_diffuse		, Color::WHITE, 1.f, 1.f);
	INIT_GRAPH_FLOAT( m_specularScale		, 1.f, 1.f);
	INIT_GRAPH_FLOAT( m_translucencyScale	, 1.f, 1.f);
	INIT_GRAPH_COLORSCALED( m_billboardsColor,		Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_billboardsTranslucency,	1.f, 1.f );
	INIT_GRAPH_FLOAT( m_ambientOcclusionScale,		1.f, 1.f );
	m_randomColorsTrees.Reset( mode );
	m_randomColorsBranches.Reset( mode );
	m_randomColorsGrass.Reset( mode );
	INIT_GRAPH_COLORSCALED( m_randomColorsFallback, Color::WHITE, 1.f, 2.f );
	INIT_GRAPH_FLOAT( m_pigmentBrightness, 1.f, 4.f );
	INIT_GRAPH_FLOAT( m_pigmentFloodStartDist, 0.f, 100.f );
	INIT_GRAPH_FLOAT( m_pigmentFloodRange, 0.f, 100.f );
	INIT_GRAPH_FLOAT( m_billboardsLightBleed, 0.5f, 1.f );
}

void CEnvSpeedTreeParameters::ImportDayPointValue( const CEnvSpeedTreeParameters &src, const CEnvSpeedTreeParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_diffuse );
		LERP_ATTRIBUTE( m_specularScale );
		LERP_ATTRIBUTE( m_translucencyScale );
		LERP_ATTRIBUTE( m_billboardsColor );
		LERP_ATTRIBUTE( m_billboardsTranslucency );
		LERP_ATTRIBUTE( m_ambientOcclusionScale );
		LERP_ATTRIBUTE( m_randomColorsTrees );
		LERP_ATTRIBUTE( m_randomColorsBranches );
		LERP_ATTRIBUTE( m_randomColorsGrass );
		LERP_ATTRIBUTE( m_randomColorsFallback );
		LERP_ATTRIBUTE( m_pigmentBrightness );
		LERP_ATTRIBUTE( m_pigmentFloodStartDist );
		LERP_ATTRIBUTE( m_pigmentFloodRange );
		LERP_ATTRIBUTE( m_billboardsLightBleed );
	LERP_GROUP_END()
}

CEnvSpeedTreeParametersAtPoint::CEnvSpeedTreeParametersAtPoint( const CEnvSpeedTreeParameters& source )
	: m_activated( source.m_activated )
	, m_diffuse( source.m_diffuse.GetCachedPoint() )
	, m_specularScale( source.m_specularScale.GetCachedPoint() )
	, m_translucencyScale( source.m_translucencyScale.GetCachedPoint() )
	, m_billboardsColor( source.m_billboardsColor.GetCachedPoint() )
	, m_billboardsTranslucency( source.m_billboardsTranslucency.GetCachedPoint() )
	, m_ambientOcclusionScale( source.m_ambientOcclusionScale.GetCachedPoint() )
	, m_randomColorsTrees( source.m_randomColorsTrees )
	, m_randomColorsBranches( source.m_randomColorsBranches )
	, m_randomColorsGrass( source.m_randomColorsGrass )
	, m_randomColorsFallback( source.m_randomColorsFallback.GetCachedPoint() )
	, m_pigmentBrightness( source.m_pigmentBrightness.GetCachedPoint() )
	, m_pigmentFloodStartDist( source.m_pigmentFloodStartDist.GetCachedPoint() )
	, m_pigmentFloodRange( source.m_pigmentFloodRange.GetCachedPoint() )
	, m_billboardsLightBleed( source.m_billboardsLightBleed.GetCachedPoint() )
{}

// --------------------------------------------------------------------------

CEnvToneMappingCurveParameters::CEnvToneMappingCurveParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvToneMappingCurveParameters::Reset( EEnvParamsResetMode mode )
{
	INIT_GRAPH_FLOAT(		m_shoulderStrength,		0.22f, 1.f );
	INIT_GRAPH_FLOAT(		m_linearStrength,		0.30f, 1.f );
	INIT_GRAPH_FLOAT(		m_linearAngle,			0.10f, 1.f );
	INIT_GRAPH_FLOAT(		m_toeStrength,			0.20f, 1.f );
	INIT_GRAPH_FLOAT(		m_toeNumerator,			0.01f, 1.f );
	INIT_GRAPH_FLOAT(		m_toeDenominator,		0.30f, 1.f );
}

void CEnvToneMappingCurveParameters::ImportDayPointValue( const CEnvToneMappingCurveParameters &src, const CEnvToneMappingCurveParameters &dest, Float lerpFactor, Float time )
{
	STEP_STRUCT_ATTRIBUTE( m_shoulderStrength );
	STEP_STRUCT_ATTRIBUTE( m_linearStrength );
	STEP_STRUCT_ATTRIBUTE( m_linearAngle );
	STEP_STRUCT_ATTRIBUTE( m_toeStrength );
	STEP_STRUCT_ATTRIBUTE( m_toeNumerator );
	STEP_STRUCT_ATTRIBUTE( m_toeDenominator );
}

CEnvToneMappingCurveParametersAtPoint::CEnvToneMappingCurveParametersAtPoint( const CEnvToneMappingCurveParameters& source )
	: m_shoulderStrength ( source.m_shoulderStrength.GetCachedPoint() )
	, m_linearStrength ( source.m_linearStrength.GetCachedPoint() )
	, m_linearAngle ( source.m_linearAngle.GetCachedPoint() )
	, m_toeStrength ( source.m_toeStrength.GetCachedPoint() )
	, m_toeNumerator ( source.m_toeNumerator.GetCachedPoint() )
	, m_toeDenominator ( source.m_toeDenominator.GetCachedPoint() )
{}

CEnvToneMappingCurveParametersAtPoint& CEnvToneMappingCurveParametersAtPoint::SetLinearApproximate()
{
	m_shoulderStrength.SetValueScalar( 0 );
	m_linearStrength.SetValueScalar( 1 );
	m_linearAngle.SetValueScalar( 1 );
	m_toeStrength.SetValueScalar( 9999.f );
	m_toeNumerator.SetValueScalar( 0 );
	m_toeDenominator.SetValueScalar( 1 );
	return *this;
}

// --------------------------------------------------------------------------

CEnvToneMappingParameters::CEnvToneMappingParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvToneMappingParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_FLOAT(		m_skyLuminanceCustomValue,	1.f,	10.f );
	INIT_GRAPH_FLOAT(		m_skyLuminanceCustomAmount,	0.f,	1.f );
	INIT_GRAPH_FLOAT(		m_luminanceLimitShape,	1.f,	1.f );
	INIT_GRAPH_FLOAT(		m_luminanceLimitMin,	0.5f,	1.f );
	INIT_GRAPH_FLOAT(		m_luminanceLimitMax,	2.0f,	1.f );
	INIT_GRAPH_FLOAT(		m_rejectThreshold,		0.98f,	1.f );
	INIT_GRAPH_FLOAT(		m_rejectSmoothExtent,	0.f,	1.f );
	
	m_newToneMapCurveParameters.Reset( mode );
	INIT_GRAPH_FLOAT(		m_newToneMapWhitepoint, 0.65f,	1.f );
	INIT_GRAPH_FLOAT(		m_newToneMapPostScale,	1.f,	1.f );

	INIT_GRAPH_FLOAT(		m_exposureScale,	1.f, 1.f );
	INIT_GRAPH_FLOAT(		m_postScale,		1.f, 1.f );
}

void CEnvToneMappingParameters::ImportDayPointValue( const CEnvToneMappingParameters &src, const CEnvToneMappingParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_skyLuminanceCustomValue );
		LERP_ATTRIBUTE( m_skyLuminanceCustomAmount );
		LERP_ATTRIBUTE( m_luminanceLimitShape );
		LERP_ATTRIBUTE( m_luminanceLimitMin );
		LERP_ATTRIBUTE( m_luminanceLimitMax );
		LERP_ATTRIBUTE( m_rejectThreshold );
		LERP_ATTRIBUTE( m_rejectSmoothExtent );
		LERP_ATTRIBUTE( m_newToneMapCurveParameters );
		LERP_ATTRIBUTE( m_newToneMapWhitepoint );
		LERP_ATTRIBUTE( m_newToneMapPostScale );
		LERP_ATTRIBUTE( m_exposureScale );
		LERP_ATTRIBUTE( m_postScale );
	LERP_GROUP_END()
}

CEnvToneMappingParametersAtPoint::CEnvToneMappingParametersAtPoint( const CEnvToneMappingParameters& source )
	: m_activated( source.m_activated )
	, m_skyLuminanceCustomValue( source.m_skyLuminanceCustomValue.GetCachedPoint() )
	, m_skyLuminanceCustomAmount( source.m_skyLuminanceCustomAmount.GetCachedPoint() )
	, m_luminanceLimitShape( source.m_luminanceLimitShape.GetCachedPoint() )
	, m_luminanceLimitMin( source.m_luminanceLimitMin.GetCachedPoint() )
	, m_luminanceLimitMax( source.m_luminanceLimitMax.GetCachedPoint() )
	, m_rejectThreshold( source.m_rejectThreshold.GetCachedPoint() )
	, m_rejectSmoothExtent( source.m_rejectSmoothExtent.GetCachedPoint() )
	, m_newToneMapCurveParameters( source.m_newToneMapCurveParameters )
	, m_newToneMapWhitepoint( source.m_newToneMapWhitepoint.GetCachedPoint() )
	, m_newToneMapPostScale( source.m_newToneMapPostScale.GetCachedPoint() )
	, m_exposureScale( source.m_exposureScale.GetCachedPoint() )
	, m_postScale( source.m_postScale.GetCachedPoint() )
{}

// --------------------------------------------------------------------------

CEnvBloomNewParameters::CEnvBloomNewParameters( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvBloomNewParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_COLOR(		m_brightPassWeights				, RGB_LUMINANCE_WEIGHTS_COLOR_GAMMA );			
	INIT_GRAPH_COLORSCALED(	m_color							, Color::WHITE, 0.f, 1.f );
	INIT_GRAPH_COLORSCALED(	m_dirtColor						, Color::WHITE, 0.f, 1.f );
	INIT_GRAPH_FLOAT(		m_threshold						, 0.8f,  4.f );
	INIT_GRAPH_FLOAT(		m_thresholdRange				, 0.1f,  4.f );
	INIT_GRAPH_FLOAT(		m_brightnessMax					, 10.f, 20.f );
	INIT_GRAPH_COLORSCALED(	m_shaftsColor					, Color::WHITE, 0.f, 1.f );
	INIT_GRAPH_FLOAT(		m_shaftsRadius					, 1.f, 2.f );
	INIT_GRAPH_FLOAT(		m_shaftsShapeExp				, 0.f, 10.f );
	INIT_GRAPH_FLOAT(		m_shaftsShapeInvSquare			, 0.f, 100.f );
	INIT_GRAPH_FLOAT(		m_shaftsThreshold				, 0.f, 1.f );
	INIT_GRAPH_FLOAT(		m_shaftsThresholdRange			, 0.f, 1.f );
}

void CEnvBloomNewParameters::ImportDayPointValue( const CEnvBloomNewParameters &src, const CEnvBloomNewParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )		
		LERP_ATTRIBUTE( m_brightPassWeights );
		LERP_ATTRIBUTE( m_color );
		LERP_ATTRIBUTE( m_dirtColor );
		LERP_ATTRIBUTE( m_threshold );
		LERP_ATTRIBUTE( m_thresholdRange );
		LERP_ATTRIBUTE( m_brightnessMax );
		LERP_ATTRIBUTE( m_shaftsColor );
		LERP_ATTRIBUTE( m_shaftsRadius );
		LERP_ATTRIBUTE( m_shaftsShapeExp );
		LERP_ATTRIBUTE( m_shaftsShapeInvSquare );
		LERP_ATTRIBUTE( m_shaftsThreshold );
		LERP_ATTRIBUTE( m_shaftsThresholdRange );
	LERP_GROUP_END()
}

CEnvBloomNewParametersAtPoint::CEnvBloomNewParametersAtPoint( const CEnvBloomNewParameters& source )
	: m_activated( source.m_activated )
	, m_brightPassWeights( source.m_brightPassWeights.GetCachedPoint() )
	, m_color( source.m_color.GetCachedPoint() )
	, m_dirtColor( source.m_dirtColor.GetCachedPoint() )
	, m_threshold( source.m_threshold.GetCachedPoint() )
	, m_thresholdRange( source.m_thresholdRange.GetCachedPoint() )
	, m_brightnessMax( source.m_brightnessMax.GetCachedPoint() )
	, m_shaftsColor( source.m_shaftsColor.GetCachedPoint() )
	, m_shaftsRadius( source.m_shaftsRadius.GetCachedPoint() )
	, m_shaftsShapeExp( source.m_shaftsShapeExp.GetCachedPoint() )
	, m_shaftsShapeInvSquare( source.m_shaftsShapeInvSquare.GetCachedPoint() )
	, m_shaftsThreshold( source.m_shaftsThreshold.GetCachedPoint() )
	, m_shaftsThresholdRange( source.m_shaftsThresholdRange.GetCachedPoint() )
{}

inline Bool HasCameraDirtData( const SDayPointEnvironmentParams &dayPointParams )
{
	return !dayPointParams.m_cameraDirtTexture.IsNull() && dayPointParams.m_cameraDirtNumVerticalTiles > 0;
}

Bool CEnvBloomNewParametersAtPoint::IsCameraDirtExposed( const SDayPointEnvironmentParams &dayPointParams ) const
{
	return (!m_dirtColor.IsColorScaledBlack() && HasCameraDirtData( dayPointParams )) && m_brightnessMax.GetScalar() > 0;
}

Bool CEnvBloomNewParametersAtPoint::IsBloomExposed( const SDayPointEnvironmentParams &dayPointParams ) const
{
	return (!m_color.IsColorScaledBlack() || (!m_dirtColor.IsColorScaledBlack() && HasCameraDirtData( dayPointParams ))) && m_brightnessMax.GetScalar() > 0;
}

Bool CEnvBloomNewParametersAtPoint::IsShaftsExposed( const SDayPointEnvironmentParams &dayPointParams ) const
{
	return IsBloomExposed( dayPointParams ) && !m_shaftsColor.IsColorScaledBlack() && m_shaftsRadius.GetScalar() > 0;
}

// --------------------------------------------------------------------------

CEnvColorModTransparencyParameters::CEnvColorModTransparencyParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvColorModTransparencyParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated	= false;
	INIT_GRAPH_FLOAT(		m_commonFarDist			, 20.f, 200.f );
	INIT_GRAPH_COLORSCALED( m_filterNearColor		, Color::WHITE, 1.f, 2.f );
	INIT_GRAPH_COLORSCALED( m_filterFarColor		, Color::WHITE, 1.f, 2.f );
	INIT_GRAPH_FLOAT(		m_contrastNearStrength	, 1.f, 2.f);
	INIT_GRAPH_FLOAT(		m_contrastFarStrength	, 1.f, 2.f);
	m_autoHideCustom0.Reset( mode );
	m_autoHideCustom1.Reset( mode );
	m_autoHideCustom2.Reset( mode );
	m_autoHideCustom3.Reset( mode );
}

void CEnvColorModTransparencyParameters::ImportDayPointValue( const CEnvColorModTransparencyParameters &src, const CEnvColorModTransparencyParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_commonFarDist );
		LERP_ATTRIBUTE( m_filterNearColor );
		LERP_ATTRIBUTE( m_filterFarColor );
		LERP_ATTRIBUTE( m_contrastNearStrength );
		LERP_ATTRIBUTE( m_contrastFarStrength );
		LERP_ATTRIBUTE( m_autoHideCustom0 );
		LERP_ATTRIBUTE( m_autoHideCustom1 );
		LERP_ATTRIBUTE( m_autoHideCustom2 );
		LERP_ATTRIBUTE( m_autoHideCustom3 );
	LERP_GROUP_END()
}

const CEnvDistanceRangeParameters& CEnvColorModTransparencyParameters::GetParamsForAutoHideGroup( EEnvAutoHideGroup group ) const
{
	switch ( group )
	{
	case EAHG_Custom0:	return m_autoHideCustom0;
	case EAHG_Custom1:	return m_autoHideCustom1;
	case EAHG_Custom2:	return m_autoHideCustom2;
	case EAHG_Custom3:	return m_autoHideCustom3;

	case EAHG_None:		// falldown (None not allowed here)
	case EAHG_Max:		// falldown
	default:
		ASSERT( !"invalid auto hide group"); 
		return m_autoHideCustom3;
	}
}

CEnvColorModTransparencyParametersAtPoint::CEnvColorModTransparencyParametersAtPoint( const CEnvColorModTransparencyParameters& source )
	: m_activated( source.m_activated )
	, m_commonFarDist( source.m_commonFarDist.GetCachedPoint() )
	, m_filterNearColor( source.m_filterNearColor.GetCachedPoint() )
	, m_filterFarColor( source.m_filterFarColor.GetCachedPoint() )
	, m_contrastNearStrength( source.m_contrastNearStrength.GetCachedPoint() )
	, m_contrastFarStrength( source.m_contrastFarStrength.GetCachedPoint() )
	, m_autoHideCustom0( source.m_autoHideCustom0 )
	, m_autoHideCustom1( source.m_autoHideCustom1 )
	, m_autoHideCustom2( source.m_autoHideCustom2 )
	, m_autoHideCustom3( source.m_autoHideCustom3 )
{
}

const CEnvDistanceRangeParametersAtPoint& CEnvColorModTransparencyParametersAtPoint::GetParamsForAutoHideGroup( EEnvAutoHideGroup group ) const
{
	switch ( group )
	{
	case EAHG_Custom0:	return m_autoHideCustom0;
	case EAHG_Custom1:	return m_autoHideCustom1;
	case EAHG_Custom2:	return m_autoHideCustom2;
	case EAHG_Custom3:	return m_autoHideCustom3;

	case EAHG_None:		// falldown (None not allowed here)
	case EAHG_Max:		// falldown
	default:
		ASSERT( !"invalid auto hide group"); 
		return m_autoHideCustom3;
	}
}

// --------------------------------------------------------------------------

CEnvShadowsParameters::CEnvShadowsParameters ( EEnvParamsResetMode mode )		
{
	Reset( mode );
}

void CEnvShadowsParameters::Reset( EEnvParamsResetMode mode )
{	
	m_activatedAutoHide = false;
	INIT_GRAPH_FLOAT(		m_autoHideBoxSizeMin	, 1.2f,  500.f );
	INIT_GRAPH_FLOAT(		m_autoHideBoxSizeMax	, 500.f, 500.f );
	INIT_GRAPH_FLOAT(		m_autoHideBoxCompMaxX	, 40.f,  40.f );
	INIT_GRAPH_FLOAT(		m_autoHideBoxCompMaxY	, 40.f,  40.f );
	INIT_GRAPH_FLOAT(		m_autoHideBoxCompMaxZ	, 40.f,  40.f );
	INIT_GRAPH_FLOAT(		m_autoHideDistScale		, 8.f,  20.f );
}

void CEnvShadowsParameters::ImportDayPointValue( const CEnvShadowsParameters &src, const CEnvShadowsParameters &dest, Float lerpFactor, Float time )
{	
	LERP_GROUP_BEGIN( m_activatedAutoHide )
		LERP_ATTRIBUTE( m_autoHideBoxSizeMin );
		LERP_ATTRIBUTE( m_autoHideBoxSizeMax );
		LERP_ATTRIBUTE( m_autoHideBoxCompMaxX );
		LERP_ATTRIBUTE( m_autoHideBoxCompMaxY );
		LERP_ATTRIBUTE( m_autoHideBoxCompMaxZ );
		LERP_ATTRIBUTE( m_autoHideDistScale );
	LERP_GROUP_END()
}

CEnvShadowsParametersAtPoint::CEnvShadowsParametersAtPoint( const CEnvShadowsParameters& source )
	: m_activatedAutoHide( source.m_activatedAutoHide )
	, m_autoHideBoxSizeMin( source.m_autoHideBoxSizeMin.GetCachedPoint() )
	, m_autoHideBoxSizeMax( source.m_autoHideBoxSizeMax.GetCachedPoint() )
	, m_autoHideBoxCompMaxX( source.m_autoHideBoxCompMaxX.GetCachedPoint() )
	, m_autoHideBoxCompMaxY( source.m_autoHideBoxCompMaxX.GetCachedPoint() )
	, m_autoHideBoxCompMaxZ( source.m_autoHideBoxCompMaxZ.GetCachedPoint() )
	, m_autoHideDistScale( source.m_autoHideDistScale.GetCachedPoint() )
{
}

// --------------------------------------------------------------------------

CEnvGlobalFogParameters::CEnvGlobalFogParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvGlobalFogParameters::Reset( EEnvParamsResetMode mode )
{
	m_fogActivated = false;
	INIT_GRAPH_FLOAT( m_fogAppearDistance			, 0.f,		100.f );
	INIT_GRAPH_FLOAT( m_fogAppearRange				, 0.f,		100.f );
	INIT_GRAPH_COLORSCALED( m_fogColorFront		, Color (   255, 255, 128 ), 1.f, 1 );
	INIT_GRAPH_COLORSCALED( m_fogColorMiddle	, Color (   255, 255, 255 ), 1.f, 1 );
	INIT_GRAPH_COLORSCALED( m_fogColorBack		, Color (   128, 128, 255 ), 1.f, 1 );
	INIT_GRAPH_FLOAT( m_fogDensity					, 0.f,		0.01f );
	INIT_GRAPH_FLOAT( m_fogVertOffset				, 0.f,		100.f );
	INIT_GRAPH_FLOAT( m_fogVertDensity				, 0.f,		0.1f );
	INIT_GRAPH_FLOAT( m_fogVertDensityLightFront	, 1.f,		2.f );
	INIT_GRAPH_FLOAT( m_fogVertDensityLightBack		, 1.f,		2.f );
	INIT_GRAPH_FLOAT( m_fogSkyDensityScale			, 1.f,		2.f );
	INIT_GRAPH_FLOAT( m_fogCloudsDensityScale		, 1.f,		2.f );	
	INIT_GRAPH_FLOAT( m_fogSkyVertDensityLightFrontScale	, 1.f,		2.f );
	INIT_GRAPH_FLOAT( m_fogSkyVertDensityLightBackScale		, 1.f,		2.f );
	INIT_GRAPH_FLOAT( m_fogVertDensityRimRange		, 0.f,		1.f );
	INIT_GRAPH_FLOAT( m_fogDistClamp				, 5000.f,	10000.f );
	INIT_GRAPH_FLOAT( m_fogFinalExp					, 1.f,		2.f );
	INIT_GRAPH_COLORSCALED( m_fogCustomColor	, Color (   255, 255, 255 ), 1.f, 1 );
	INIT_GRAPH_FLOAT( m_fogCustomColorStart		, 0.f,		1.f );
	INIT_GRAPH_FLOAT( m_fogCustomColorRange		, 0.f,		1.f );
	INIT_GRAPH_FLOAT( m_fogCustomAmountScale	, 1.f,		1.f );
	INIT_GRAPH_FLOAT( m_fogCustomAmountScaleStart	, 0.f,		1.f );
	INIT_GRAPH_FLOAT( m_fogCustomAmountScaleRange	, 0.f,		1.f );
	INIT_GRAPH_COLORSCALED( m_aerialColorFront		, Color (   255, 255, 255 ), 1.f, 1 );
	INIT_GRAPH_COLORSCALED( m_aerialColorMiddle		, Color (   255, 255, 255 ), 1.f, 1 );
	INIT_GRAPH_COLORSCALED( m_aerialColorBack		, Color (   255, 255, 255 ), 1.f, 1 );
	INIT_GRAPH_FLOAT( m_aerialFinalExp				, 1.f,		2.f );
	INIT_GRAPH_FLOAT( m_ssaoImpactClamp				, 0.f, 1.f );
	INIT_GRAPH_FLOAT( m_ssaoImpactNearValue			, 0.f, 1.f );
	INIT_GRAPH_FLOAT( m_ssaoImpactFarValue			, 0.f, 1.f );
	INIT_GRAPH_FLOAT( m_ssaoImpactNearDistance		, 0.f, 100.f );
	INIT_GRAPH_FLOAT( m_ssaoImpactFarDistance		, 100.f, 500.f );
	INIT_GRAPH_FLOAT( m_distantLightsIntensityScale	, 1.f, 10.f );
}

void CEnvGlobalFogParameters::ImportDayPointValue( const CEnvGlobalFogParameters &src, const CEnvGlobalFogParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_fogActivated )
		LERP_ATTRIBUTE( m_fogAppearDistance );
		LERP_ATTRIBUTE( m_fogAppearRange );
		LERP_ATTRIBUTE( m_fogColorFront );
		LERP_ATTRIBUTE( m_fogColorMiddle );
		LERP_ATTRIBUTE( m_fogColorBack );
		LERP_ATTRIBUTE( m_fogDensity );
		LERP_ATTRIBUTE( m_fogVertOffset );
		LERP_ATTRIBUTE( m_fogVertDensity );
		LERP_ATTRIBUTE( m_fogVertDensityLightFront );
		LERP_ATTRIBUTE( m_fogVertDensityLightBack );
		LERP_ATTRIBUTE( m_fogSkyDensityScale );
		LERP_ATTRIBUTE( m_fogCloudsDensityScale );
		LERP_ATTRIBUTE( m_fogSkyVertDensityLightFrontScale );
		LERP_ATTRIBUTE( m_fogSkyVertDensityLightBackScale );
		LERP_ATTRIBUTE( m_fogVertDensityRimRange );
		LERP_ATTRIBUTE( m_fogDistClamp );
		LERP_ATTRIBUTE( m_fogFinalExp );	
		STEP_ATTRIBUTE( m_fogCustomColor );
		STEP_ATTRIBUTE( m_fogCustomColorStart );
		STEP_ATTRIBUTE( m_fogCustomColorRange );
		STEP_ATTRIBUTE( m_fogCustomAmountScale );
		STEP_ATTRIBUTE( m_fogCustomAmountScaleStart );
		STEP_ATTRIBUTE( m_fogCustomAmountScaleRange );
		LERP_ATTRIBUTE( m_aerialColorFront );
		LERP_ATTRIBUTE( m_aerialColorMiddle );
		LERP_ATTRIBUTE( m_aerialColorBack );
		LERP_ATTRIBUTE( m_aerialFinalExp );
		LERP_ATTRIBUTE( m_ssaoImpactClamp );
		LERP_ATTRIBUTE( m_ssaoImpactNearValue );
		LERP_ATTRIBUTE( m_ssaoImpactFarValue );
		LERP_ATTRIBUTE( m_ssaoImpactNearDistance );
		LERP_ATTRIBUTE( m_ssaoImpactFarDistance );
		LERP_ATTRIBUTE( m_distantLightsIntensityScale );
	LERP_GROUP_END()
}

CEnvGlobalFogParametersAtPoint::CEnvGlobalFogParametersAtPoint( const CEnvGlobalFogParameters& source )
	: m_fogActivated ( source.m_fogActivated )
	, m_fogAppearDistance ( source.m_fogAppearDistance.GetCachedPoint() )
	, m_fogAppearRange ( source.m_fogAppearRange.GetCachedPoint() )
	, m_fogColorFront ( source.m_fogColorFront.GetCachedPoint() )
	, m_fogColorMiddle ( source.m_fogColorMiddle.GetCachedPoint() )
	, m_fogColorBack ( source.m_fogColorBack.GetCachedPoint() )
	, m_fogDensity ( source.m_fogDensity.GetCachedPoint() )
	, m_fogVertOffset ( source.m_fogVertOffset.GetCachedPoint() )
	, m_fogVertDensity ( source.m_fogVertDensity.GetCachedPoint() )
	, m_fogVertDensityLightFront ( source.m_fogVertDensityLightFront.GetCachedPoint() )
	, m_fogVertDensityLightBack ( source.m_fogVertDensityLightBack.GetCachedPoint() )
	, m_fogSkyDensityScale ( source.m_fogSkyDensityScale.GetCachedPoint() )
	, m_fogCloudsDensityScale( source.m_fogCloudsDensityScale.GetCachedPoint() )
	, m_fogSkyVertDensityLightFrontScale ( source.m_fogSkyVertDensityLightFrontScale.GetCachedPoint() )
	, m_fogSkyVertDensityLightBackScale ( source.m_fogSkyVertDensityLightBackScale.GetCachedPoint() )
	, m_fogVertDensityRimRange ( source.m_fogVertDensityRimRange.GetCachedPoint() )
	, m_fogDistClamp ( source.m_fogDistClamp.GetCachedPoint() )
	, m_fogFinalExp ( source.m_fogFinalExp.GetCachedPoint() )
	, m_fogCustomColor ( source.m_fogCustomColor.GetCachedPoint() )
	, m_fogCustomColorStart ( source.m_fogCustomColorStart.GetCachedPoint() )
	, m_fogCustomColorRange ( source.m_fogCustomColorRange.GetCachedPoint() )
	, m_fogCustomAmountScale ( source.m_fogCustomAmountScale.GetCachedPoint() )
	, m_fogCustomAmountScaleStart ( source.m_fogCustomAmountScaleStart.GetCachedPoint() )
	, m_fogCustomAmountScaleRange ( source.m_fogCustomAmountScaleRange.GetCachedPoint() )
	, m_aerialColorFront ( source.m_aerialColorFront.GetCachedPoint() ) 
	, m_aerialColorMiddle ( source.m_aerialColorMiddle.GetCachedPoint() ) 
	, m_aerialColorBack ( source.m_aerialColorBack.GetCachedPoint() ) 
	, m_aerialFinalExp ( source.m_aerialFinalExp.GetCachedPoint() ) 
	, m_ssaoImpactClamp ( source.m_ssaoImpactClamp.GetCachedPoint() ) 
	, m_ssaoImpactNearValue ( source.m_ssaoImpactNearValue.GetCachedPoint() ) 
	, m_ssaoImpactFarValue ( source.m_ssaoImpactFarValue.GetCachedPoint() ) 
	, m_ssaoImpactNearDistance ( source.m_ssaoImpactNearDistance.GetCachedPoint() ) 
	, m_ssaoImpactFarDistance ( source.m_ssaoImpactFarDistance.GetCachedPoint() ) 
	, m_distantLightsIntensityScale ( source.m_distantLightsIntensityScale.GetCachedPoint() )
{
}

void CEnvGlobalFogParametersAtPoint::SetDisabledParams()
{
	m_fogDensity.SetValueScalar( 0 );
	m_aerialColorFront.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
	m_aerialColorMiddle.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
	m_aerialColorBack.SetValue( Color ( 255, 255, 255, 255 ), 1.f );
}

// --------------------------------------------------------------------------

CEnvGlobalSkyParameters::CEnvGlobalSkyParameters( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvGlobalSkyParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated	= false;
	INIT_FLOAT_ACTIVATE_ATTRIB( m_activatedActivateFactor );
	m_activateFactor = 1.0f;
	INIT_GRAPH_COLOR( m_skyColor,						Color( 108,155,249,255 ) );
	INIT_GRAPH_COLOR( m_skyColorHorizon,				Color( 255,190,167,255 ) );
	INIT_GRAPH_FLOAT( m_horizonVerticalAttenuation,		1.8f, 3.0f );
	INIT_GRAPH_COLOR( m_sunColorSky,					Color( 195,227,255,255 ) );
	INIT_GRAPH_FLOAT( m_sunColorSkyBrightness,			1.f, 10.f );
	INIT_GRAPH_FLOAT( m_sunAreaSkySize,					0.33f, 1.0f );
	INIT_GRAPH_COLOR( m_sunColorHorizon,				Color( 255,184,99,255 ) );
	INIT_GRAPH_FLOAT( m_sunColorHorizonHorizontalScale,	0.84f, 1.0f );
	INIT_GRAPH_COLOR( m_sunBackHorizonColor,			Color( 161,149,147,255 ) );
	INIT_GRAPH_FLOAT( m_sunInfluence,					1.f, 1.f );
	INIT_GRAPH_COLOR( m_moonColorSky,						Color( 195,227,255,255 ) );
	INIT_GRAPH_FLOAT( m_moonColorSkyBrightness,				1.f, 10.f );
	INIT_GRAPH_FLOAT( m_moonAreaSkySize,					0.33f, 1.0f );
	INIT_GRAPH_COLOR( m_moonColorHorizon,					Color( 255,184,99,255 ) );
	INIT_GRAPH_FLOAT( m_moonColorHorizonHorizontalScale,	0.84f, 1.0f );
	INIT_GRAPH_COLOR( m_moonBackHorizonColor,				Color( 161,149,147,255 ) );
	INIT_GRAPH_FLOAT( m_moonInfluence,						0.f, 1.f );
	INIT_GRAPH_FLOAT( m_globalSkyBrightness,			1.0f, 1.0f );
}

void CEnvGlobalSkyParameters::ImportDayPointValue( const CEnvGlobalSkyParameters &src, const CEnvGlobalSkyParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_skyColor );
		LERP_ATTRIBUTE( m_skyColorHorizon );
		LERP_ATTRIBUTE( m_horizonVerticalAttenuation );
		LERP_ATTRIBUTE( m_sunColorSky );
		LERP_ATTRIBUTE( m_sunColorSkyBrightness );
		LERP_ATTRIBUTE( m_sunAreaSkySize );
		LERP_ATTRIBUTE( m_sunColorHorizon );
		LERP_ATTRIBUTE( m_sunColorHorizonHorizontalScale );
		LERP_ATTRIBUTE( m_sunBackHorizonColor );
		LERP_ATTRIBUTE( m_sunInfluence );
		LERP_ATTRIBUTE( m_moonColorSky );
		LERP_ATTRIBUTE( m_moonColorSkyBrightness );
		LERP_ATTRIBUTE( m_moonAreaSkySize );
		LERP_ATTRIBUTE( m_moonColorHorizon );
		LERP_ATTRIBUTE( m_moonColorHorizonHorizontalScale );
		LERP_ATTRIBUTE( m_moonBackHorizonColor );
		LERP_ATTRIBUTE( m_moonInfluence );
		LERP_ATTRIBUTE( m_globalSkyBrightness );
		LERP_ATTRIBUTE_VALUE( m_activateFactor, m_activatedActivateFactor );
	LERP_GROUP_END()
}

CEnvGlobalSkyParametersAtPoint::CEnvGlobalSkyParametersAtPoint( const CEnvGlobalSkyParameters& source )
: m_activated( source.m_activated )
, m_skyColor( source.m_skyColor.GetCachedPoint() )
, m_skyColorHorizon( source.m_skyColorHorizon.GetCachedPoint() )
, m_horizonVerticalAttenuation( source.m_horizonVerticalAttenuation.GetCachedPoint() )
, m_sunColorSky( source.m_sunColorSky.GetCachedPoint() )
, m_sunColorSkyBrightness( source.m_sunColorSkyBrightness.GetCachedPoint() )
, m_sunAreaSkySize( source.m_sunAreaSkySize.GetCachedPoint() )
, m_sunColorHorizon( source.m_sunColorHorizon.GetCachedPoint() )
, m_sunColorHorizonHorizontalScale( source.m_sunColorHorizonHorizontalScale.GetCachedPoint() )
, m_sunBackHorizonColor( source.m_sunBackHorizonColor.GetCachedPoint() )
, m_sunInfluence( source.m_sunInfluence.GetCachedPoint() )
, m_moonColorSky( source.m_moonColorSky.GetCachedPoint() )
, m_moonColorSkyBrightness( source.m_moonColorSkyBrightness.GetCachedPoint() )
, m_moonAreaSkySize( source.m_moonAreaSkySize.GetCachedPoint() )
, m_moonColorHorizon( source.m_moonColorHorizon.GetCachedPoint() )
, m_moonColorHorizonHorizontalScale( source.m_moonColorHorizonHorizontalScale.GetCachedPoint() )
, m_moonBackHorizonColor( source.m_moonBackHorizonColor.GetCachedPoint() )
, m_moonInfluence( source.m_moonInfluence.GetCachedPoint() )
, m_globalSkyBrightness( source.m_globalSkyBrightness.GetCachedPoint() )
, m_activatedActivateFactor( source.m_activatedActivateFactor )
, m_activateFactor( source.m_activateFactor )
{
}


// --------------------------------------------------------------------------

CEnvDepthOfFieldParameters::CEnvDepthOfFieldParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvDepthOfFieldParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_FLOAT( m_nearBlurDist	, 0.f, 250.f );
	INIT_GRAPH_FLOAT( m_nearFocusDist	, 10.f, 250.f );
	INIT_GRAPH_FLOAT( m_farFocusDist	, 20.f, 250.f );
	INIT_GRAPH_FLOAT( m_farBlurDist		, 45.f, 250.f );
	INIT_GRAPH_FLOAT( m_intensity		, 0.f, 1.f );
	INIT_FLOAT_ACTIVATE_ATTRIB( m_activatedSkyThreshold );
	m_skyThreshold = 0.f;
	INIT_FLOAT_ACTIVATE_ATTRIB( m_activatedSkyRange );
	m_skyRange = 0.1f;	
}

bool CEnvDepthOfFieldParameters::IsDofExposed( Float dayCycleProgress ) const
{
	return m_intensity.GetFloatValue( dayCycleProgress ) > 0.f;
}

void CEnvDepthOfFieldParameters::ImportDayPointValue( const CEnvDepthOfFieldParameters &src, const CEnvDepthOfFieldParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_nearBlurDist );
		LERP_ATTRIBUTE( m_nearFocusDist );
		LERP_ATTRIBUTE( m_farFocusDist );
		LERP_ATTRIBUTE( m_farBlurDist );
		LERP_ATTRIBUTE( m_intensity );
		LERP_ATTRIBUTE_VALUE( m_skyThreshold, m_activatedSkyThreshold );
		LERP_ATTRIBUTE_VALUE( m_skyRange, m_activatedSkyRange );
	LERP_GROUP_END()
}

CEnvDepthOfFieldParametersAtPoint::CEnvDepthOfFieldParametersAtPoint( const CEnvDepthOfFieldParameters& source )
	: m_activated( source.m_activated )
	, m_nearBlurDist( source.m_nearBlurDist.GetCachedPoint() )
	, m_nearFocusDist( source.m_nearFocusDist.GetCachedPoint() )
	, m_farFocusDist( source.m_farFocusDist.GetCachedPoint() )
	, m_farBlurDist( source.m_farBlurDist.GetCachedPoint() )
	, m_intensity( source.m_intensity.GetCachedPoint() )
	, m_activatedSkyThreshold( source.m_activatedSkyThreshold )
	, m_skyThreshold( source.m_skyThreshold )
	, m_activatedSkyRange( source.m_activatedSkyRange )
	, m_skyRange( source.m_skyRange )
{
}

bool CEnvDepthOfFieldParametersAtPoint::IsDofExposed() const
{
	return m_intensity.GetScalar() > 0.f;
}

// --------------------------------------------------------------------------

CEnvNVSSAOParameters::CEnvNVSSAOParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvNVSSAOParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated	= false;

	INIT_GRAPH_FLOAT( m_radius					, 1.0f,		1.f );
	INIT_GRAPH_FLOAT( m_bias					, 0.1f,		1.f );
	INIT_GRAPH_FLOAT( m_detailStrength			, 1.0f,		1.f );
	INIT_GRAPH_FLOAT( m_coarseStrength			, 1.0f,		1.f );
	INIT_GRAPH_FLOAT( m_powerExponent			, 2.0f,		1.f );
	INIT_GRAPH_FLOAT( m_blurSharpness			, 0.0f,		1.f );
	INIT_GRAPH_FLOAT( m_valueClamp				, 0.1f,		1.f );
	INIT_GRAPH_COLOR( m_ssaoColor				, Color::BLACK );
	INIT_GRAPH_FLOAT( m_nonAmbientInfluence		, 0.f,		1.f );
	INIT_GRAPH_FLOAT( m_translucencyInfluence	, 1.f,		1.f );
}

void CEnvNVSSAOParameters::ImportDayPointValue( const CEnvNVSSAOParameters &src, const CEnvNVSSAOParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_radius );
		LERP_ATTRIBUTE( m_bias );
		LERP_ATTRIBUTE( m_detailStrength );
		LERP_ATTRIBUTE( m_coarseStrength );
		LERP_ATTRIBUTE( m_powerExponent );
		LERP_ATTRIBUTE( m_blurSharpness );
		LERP_ATTRIBUTE( m_valueClamp );
		LERP_ATTRIBUTE( m_ssaoColor );
		LERP_ATTRIBUTE( m_nonAmbientInfluence );
		LERP_ATTRIBUTE( m_translucencyInfluence );
	LERP_GROUP_END()
}

CEnvNVSSAOParametersAtPoint::CEnvNVSSAOParametersAtPoint( const CEnvNVSSAOParameters& source )
	: m_activated( source.m_activated )
	, m_radius( source.m_radius.GetCachedPoint() )
	, m_bias( source.m_bias.GetCachedPoint() )
	, m_detailStrength( source.m_detailStrength.GetCachedPoint() )
	, m_coarseStrength( source.m_coarseStrength.GetCachedPoint() )
	, m_powerExponent( source.m_powerExponent.GetCachedPoint() )
	, m_blurSharpness( source.m_blurSharpness.GetCachedPoint() )
	, m_valueClamp( source.m_valueClamp.GetCachedPoint() )
	, m_ssaoColor( source.m_ssaoColor.GetCachedPoint() )
	, m_nonAmbientInfluence( source.m_nonAmbientInfluence.GetCachedPoint() )
	, m_translucencyInfluence( source.m_translucencyInfluence.GetCachedPoint() )
{
}

// --------------------------------------------------------------------------

CEnvMSSSAOParameters::CEnvMSSSAOParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvMSSSAOParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated	= true;
	m_combineResolutionsBeforeBlur = true;
	m_combineResolutionsWithMul = false;

	INIT_GRAPH_FLOAT( m_noiseFilterTolerance			,	-8.0f,		1.f );
	INIT_GRAPH_FLOAT( m_blurTolerance					,  -1.25f,		1.f );
	INIT_GRAPH_FLOAT( m_hierarchyDepth					,	 3.0f,		1.f );
	INIT_GRAPH_FLOAT( m_upsampleTolerance				,	-6.0f,		1.f );
	INIT_GRAPH_FLOAT( m_rejectionFalloff				,	 7.0f,		1.f );
	INIT_GRAPH_FLOAT( m_normalAOMultiply				,    4.0f,		1.f );
	INIT_GRAPH_FLOAT( m_normalToDepthBrightnessEqualiser,  100.0f,		1.f );
	INIT_GRAPH_FLOAT( m_normalBackProjectionTolerance	,    2.0f,		1.f );
}

void CEnvMSSSAOParameters::ImportDayPointValue( const CEnvMSSSAOParameters &src, const CEnvMSSSAOParameters &dest, Float lerpFactor, Float time )
{
	m_combineResolutionsBeforeBlur = dest.m_combineResolutionsBeforeBlur;
	m_combineResolutionsWithMul = dest.m_combineResolutionsWithMul;

	LERP_GROUP_BEGIN( m_activated )
	LERP_ATTRIBUTE( m_noiseFilterTolerance );
	LERP_ATTRIBUTE( m_blurTolerance );
	LERP_ATTRIBUTE( m_hierarchyDepth );
	LERP_ATTRIBUTE( m_upsampleTolerance );
	LERP_ATTRIBUTE( m_rejectionFalloff );
	LERP_ATTRIBUTE( m_normalAOMultiply );
	LERP_ATTRIBUTE( m_normalToDepthBrightnessEqualiser );
	LERP_ATTRIBUTE( m_normalBackProjectionTolerance );
	LERP_GROUP_END()
}

CEnvMSSSAOParametersAtPoint::CEnvMSSSAOParametersAtPoint( const CEnvMSSSAOParameters& source )
	: m_activated( source.m_activated )
	, m_noiseFilterTolerance( source.m_noiseFilterTolerance.GetCachedPoint() )
	, m_blurTolerance( source.m_blurTolerance.GetCachedPoint() )
	, m_upsampleTolerance( source.m_upsampleTolerance.GetCachedPoint() )
	, m_rejectionFalloff ( source.m_rejectionFalloff.GetCachedPoint() )
	, m_normalAOMultiply( source.m_normalAOMultiply.GetCachedPoint() )
	, m_normalToDepthBrightnessEqualiser ( source.m_normalToDepthBrightnessEqualiser.GetCachedPoint() )
	, m_normalBackProjectionTolerance( source.m_normalBackProjectionTolerance.GetCachedPoint() )
	, m_combineResolutionsBeforeBlur( source.m_combineResolutionsBeforeBlur )
	, m_combineResolutionsWithMul( source.m_combineResolutionsWithMul )
	, m_hierarchyDepth( source.m_hierarchyDepth.GetCachedPoint() )
{
}

// --------------------------------------------------------------------------

CEnvParametricBalanceParameters::CEnvParametricBalanceParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvParametricBalanceParameters::Reset( EEnvParamsResetMode mode )
{	
	INIT_GRAPH_FLOAT(		m_saturation,	1.f, 2.f );
	INIT_GRAPH_COLORSCALED(	m_color,		Color::WHITE, 1.f, 1.f );
}

void CEnvParametricBalanceParameters::ImportDayPointValue( const CEnvParametricBalanceParameters &src, const CEnvParametricBalanceParameters &dest, Float lerpFactor, Float time )
{	
	LERP_STRUCT_ATTRIBUTE(		m_saturation );
	LERP_STRUCT_ATTRIBUTE(		m_color );
}

CEnvParametricBalanceParametersAtPoint::CEnvParametricBalanceParametersAtPoint( const CEnvParametricBalanceParameters& source )
	: m_saturation ( source.m_saturation.GetCachedPoint() )
	, m_color ( source.m_color.GetCachedPoint() )
{
}

// --------------------------------------------------------------------------

CEnvFinalColorBalanceParameters::CEnvFinalColorBalanceParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvFinalColorBalanceParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	m_activatedBalanceMap = false;
	m_activatedParametricBalance = false;
	
	INIT_GRAPH_FLOAT(		m_balanceMapLerp		, 0.0f, 1.f );
	INIT_GRAPH_FLOAT(		m_balanceMapAmount		, 1.0f, 1.f );
	m_balanceMap0	= NULL;
	m_balanceMap1	= NULL;
	INIT_GRAPH_FLOAT(		m_balancePostBrightness , 1.f, 1.f );
	
	INIT_GRAPH_FLOAT(		m_levelsShadows,		0.f, 1.f );
	INIT_GRAPH_FLOAT(		m_levelsMidtones,		1.f, 1.f );
	INIT_GRAPH_FLOAT(		m_levelsHighlights,		1.f, 1.f );

	INIT_GRAPH_COLOR(		m_vignetteWeights		, Color (0, 0, 0) );
	INIT_GRAPH_COLOR(		m_vignetteColor			, Color::BLACK );
	INIT_GRAPH_FLOAT(		m_vignetteOpacity		, 1.f, 1.f );
	INIT_GRAPH_FLOAT(		m_chromaticAberrationSize,	0.f, 10.f );

	INIT_GRAPH_FLOAT(		m_midtoneRangeMin,	0.3333f, 1.f );
	INIT_GRAPH_FLOAT(		m_midtoneRangeMax,	0.6666f, 1.f );
	INIT_GRAPH_FLOAT(		m_midtoneMarginMin, 0.1f, 1.f );
	INIT_GRAPH_FLOAT(		m_midtoneMarginMax, 0.1f, 1.f );

	m_parametricBalanceLow.Reset( mode );
	m_parametricBalanceMid.Reset( mode );
	m_parametricBalanceHigh.Reset( mode );
}

void CEnvFinalColorBalanceParameters::ImportDayPointValue( const CEnvFinalColorBalanceParameters &src, const CEnvFinalColorBalanceParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_vignetteWeights );
		LERP_ATTRIBUTE( m_vignetteColor );
		LERP_ATTRIBUTE( m_vignetteOpacity );
		LERP_ATTRIBUTE( m_chromaticAberrationSize );
	LERP_GROUP_END()

	LERP_GROUP_BEGIN( m_activatedBalanceMap )
		STEP_ATTRIBUTE(			m_balanceMapLerp );
		STEP_ATTRIBUTE(			m_balanceMapAmount );
		STEP_ATTRIBUTE_VALUE(	m_balanceMap0 );
		STEP_ATTRIBUTE_VALUE(	m_balanceMap1 );
		STEP_ATTRIBUTE(			m_balancePostBrightness );
	LERP_GROUP_END()

	LERP_GROUP_BEGIN( m_activatedParametricBalance )
		LERP_ATTRIBUTE(		m_levelsShadows );
		LERP_ATTRIBUTE(		m_levelsMidtones );
		LERP_ATTRIBUTE(		m_levelsHighlights );
		LERP_ATTRIBUTE(		m_midtoneRangeMin );
		LERP_ATTRIBUTE(		m_midtoneRangeMax );
		LERP_ATTRIBUTE(		m_midtoneMarginMin );
		LERP_ATTRIBUTE(		m_midtoneMarginMax );
		LERP_ATTRIBUTE(		m_parametricBalanceLow );
		LERP_ATTRIBUTE(		m_parametricBalanceMid );
		LERP_ATTRIBUTE(		m_parametricBalanceHigh );
	LERP_GROUP_END()
}

CEnvFinalColorBalanceParametersAtPoint::CEnvFinalColorBalanceParametersAtPoint( const CEnvFinalColorBalanceParameters& source )
	: m_activated( source.m_activated )
	, m_activatedBalanceMap ( source.m_activatedBalanceMap )
	, m_activatedParametricBalance ( source.m_activatedParametricBalance )
	, m_vignetteWeights( source.m_vignetteWeights.GetCachedPoint() )
	, m_vignetteColor( source.m_vignetteColor.GetCachedPoint() )
	, m_vignetteOpacity( source.m_vignetteOpacity.GetCachedPoint() )
	, m_chromaticAberrationSize( source.m_chromaticAberrationSize.GetCachedPoint() )
	, m_balanceMapLerp ( source.m_balanceMapLerp.GetCachedPoint() )
	, m_balanceMapAmount ( source.m_balanceMapAmount.GetCachedPoint() )
	, m_balanceMap0 ( source.m_balanceMap0 )
	, m_balanceMap1 ( source.m_balanceMap1 )
	, m_balancePostBrightness ( source.m_balancePostBrightness.GetCachedPoint() )
	, m_levelsShadows ( source.m_levelsShadows.GetCachedPoint() )
	, m_levelsMidtones ( source.m_levelsMidtones.GetCachedPoint() )
	, m_levelsHighlights ( source.m_levelsHighlights.GetCachedPoint() )
	, m_midtoneRangeMin ( source.m_midtoneRangeMin.GetCachedPoint() )
	, m_midtoneRangeMax ( source.m_midtoneRangeMax.GetCachedPoint() )
	, m_midtoneMarginMin ( source.m_midtoneMarginMin.GetCachedPoint() )
	, m_midtoneMarginMax ( source.m_midtoneMarginMax.GetCachedPoint() )
	, m_parametricBalanceLow ( source.m_parametricBalanceLow )
	, m_parametricBalanceMid ( source.m_parametricBalanceMid )
	, m_parametricBalanceHigh ( source.m_parametricBalanceHigh )
{
}

// --------------------------------------------------------------------------

CEnvWaterParameters::CEnvWaterParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

// --------------------------------------------------------------------------

CEnvSharpenParameters::CEnvSharpenParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvSharpenParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_FLOAT(		m_sharpenNear		, 0.f, 1.f );
	INIT_GRAPH_FLOAT(		m_sharpenFar		, 0.f, 1.f );
	INIT_GRAPH_FLOAT(		m_distanceNear		, 0.f, 1.f );
	INIT_GRAPH_FLOAT(		m_distanceFar		, 0.f, 1.f );
	INIT_GRAPH_FLOAT(		m_lumFilterOffset	, 0.025f, 1.f );
	INIT_GRAPH_FLOAT(		m_lumFilterRange	, 0.075f, 1.f );
}

void CEnvSharpenParameters::ImportDayPointValue( const CEnvSharpenParameters &src, const CEnvSharpenParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_sharpenNear );
		LERP_ATTRIBUTE( m_sharpenFar );
		LERP_ATTRIBUTE( m_distanceNear );
		LERP_ATTRIBUTE( m_distanceFar );
		LERP_ATTRIBUTE(	m_lumFilterOffset );
		LERP_ATTRIBUTE(	m_lumFilterRange );
	LERP_GROUP_END()
}

CEnvSharpenParametersAtPoint::CEnvSharpenParametersAtPoint( const CEnvSharpenParameters& source )
	: m_activated( source.m_activated )
	, m_sharpenNear( source.m_sharpenNear.GetCachedPoint() )
	, m_sharpenFar( source.m_sharpenFar.GetCachedPoint() )
	, m_distanceNear( source.m_distanceNear.GetCachedPoint() )
	, m_distanceFar( source.m_distanceFar.GetCachedPoint() )
	, m_lumFilterOffset( source.m_lumFilterOffset.GetCachedPoint() )
	, m_lumFilterRange( source.m_lumFilterRange.GetCachedPoint() )
{
}

// --------------------------------------------------------------------------

CEnvPaintEffectParameters::CEnvPaintEffectParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvPaintEffectParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_FLOAT(		m_amount			, 0.f, 1.f );
}

void CEnvPaintEffectParameters::ImportDayPointValue( const CEnvPaintEffectParameters &src, const CEnvPaintEffectParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_amount  );
	LERP_GROUP_END()
}

CEnvPaintEffectParametersAtPoint::CEnvPaintEffectParametersAtPoint( const CEnvPaintEffectParameters& source )
	: m_activated( source.m_activated )
	, m_amount( source.m_amount.GetCachedPoint() )	
{
}

// --------------------------------------------------------------------------

CEnvFlareColorParameters::CEnvFlareColorParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvFlareColorParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_COLORSCALED(		m_color0		, Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_FLOAT(			m_opacity0		, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED(		m_color1		, Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_FLOAT(			m_opacity1		, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED(		m_color2		, Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_FLOAT(			m_opacity2		, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED(		m_color3		, Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_FLOAT(			m_opacity3		, 1.f, 1.f );
}

void CEnvFlareColorParameters::ImportDayPointValue( const CEnvFlareColorParameters &src, const CEnvFlareColorParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_color0 );
		LERP_ATTRIBUTE( m_opacity0 );
		LERP_ATTRIBUTE( m_color1 );
		LERP_ATTRIBUTE( m_opacity1 );
		LERP_ATTRIBUTE( m_color2 );
		LERP_ATTRIBUTE( m_opacity2 );
		LERP_ATTRIBUTE( m_color3 );
		LERP_ATTRIBUTE( m_opacity3 );
	LERP_GROUP_END()
}

CEnvFlareColorParametersAtPoint::CEnvFlareColorParametersAtPoint( const CEnvFlareColorParameters& source )
	: m_activated( source.m_activated )
	, m_color0( source.m_color0.GetCachedPoint() )
	, m_opacity0( source.m_opacity0.GetCachedPoint() )
	, m_color1( source.m_color1.GetCachedPoint() )
	, m_opacity1( source.m_opacity1.GetCachedPoint() )
	, m_color2( source.m_color2.GetCachedPoint() )
	, m_opacity2( source.m_opacity2.GetCachedPoint() )
	, m_color3( source.m_color3.GetCachedPoint() )
	, m_opacity3( source.m_opacity3.GetCachedPoint() )
{
}

Vector CEnvFlareColorParametersAtPoint::GetColorByIndex( Uint32 curveIndex ) const
{
	const SSimpleCurvePoint *curve = NULL;
	switch ( curveIndex )
	{
	case 0:		curve = &m_color0; break;
	case 1:		curve = &m_color1; break;
	case 2:		curve = &m_color2; break;
	case 3:		curve = &m_color3; break;
	default:	ASSERT( !"Invalid" ); curve = &m_color0; break;
	}

	return curve->GetColorScaledGammaToLinear( true );
}

Float CEnvFlareColorParametersAtPoint::GetOpacityByIndex( Uint32 curveIndex ) const
{
	const SSimpleCurvePoint *curve = NULL;
	switch ( curveIndex )
	{
	case 0:		curve = &m_opacity0; break;
	case 1:		curve = &m_opacity1; break;
	case 2:		curve = &m_opacity2; break;
	case 3:		curve = &m_opacity3; break;
	default:	ASSERT( !"Invalid" ); curve = &m_opacity0; break;
	}

	return curve->GetScalarClamp( 0.f, 1.f );
}

// --------------------------------------------------------------------------

CEnvFlareColorGroupsParameters::CEnvFlareColorGroupsParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvFlareColorGroupsParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	m_default.Reset( mode );
	m_custom0.Reset( mode );
	m_custom1.Reset( mode );
	m_custom2.Reset( mode );
}

void CEnvFlareColorGroupsParameters::ImportDayPointValue( const CEnvFlareColorGroupsParameters &src, const CEnvFlareColorGroupsParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_default );
		LERP_ATTRIBUTE( m_custom0 );
		LERP_ATTRIBUTE( m_custom1 );
		LERP_ATTRIBUTE( m_custom2 );
	LERP_GROUP_END()
}

CEnvFlareColorGroupsParametersAtPoint::CEnvFlareColorGroupsParametersAtPoint( const CEnvFlareColorGroupsParameters& source )
	: m_activated( source.m_activated )
	, m_default( source.m_default )
	, m_custom0( source.m_custom0 )
	, m_custom1( source.m_custom1 )
	, m_custom2( source.m_custom2 )
{
}

// --------------------------------------------------------------------------

void CEnvWaterParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_FLOAT(		m_waterFlowIntensity	, 0.6f, 1.0f );
	INIT_GRAPH_FLOAT(		m_underwaterBrightness	, 1.0f, 1.0f );
	INIT_GRAPH_FLOAT(		m_underWaterFogIntensity, 0.95f, 1.0f );
	INIT_GRAPH_FLOAT(		m_waterFresnel	, 1.0f, 1.0f );
	INIT_GRAPH_FLOAT(		m_waterCaustics	, 1.0f, 1.0f );
	INIT_GRAPH_FLOAT(		m_waterFoamIntensity, 0.0f, 1.0f );
	INIT_GRAPH_FLOAT(		m_waterAmbientScale, 0.1f, 1.0f );
	INIT_GRAPH_FLOAT(		m_waterDiffuseScale, 0.4f, 1.0f );
	INIT_GRAPH_COLOR(		m_waterColor	, Color::BLACK );	

	INIT_GRAPH_UNDERWATER_COLOR(	 m_underWaterColor );	
}

void CEnvWaterParameters::ImportDayPointValue( const CEnvWaterParameters &src, const CEnvWaterParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_waterFlowIntensity );
		LERP_ATTRIBUTE( m_underwaterBrightness );
		LERP_ATTRIBUTE( m_underWaterFogIntensity );
		LERP_ATTRIBUTE( m_waterFresnel );
		LERP_ATTRIBUTE( m_waterCaustics );
		LERP_ATTRIBUTE( m_waterFoamIntensity );
		LERP_ATTRIBUTE( m_waterAmbientScale );
		LERP_ATTRIBUTE( m_waterDiffuseScale );
		LERP_ATTRIBUTE( m_waterColor );
		LERP_ATTRIBUTE( m_underWaterColor );	
	LERP_GROUP_END()
}

CEnvWaterParametersAtPoint::CEnvWaterParametersAtPoint( const CEnvWaterParameters& source )
	: m_activated( source.m_activated )
	, m_waterTransparency( source.m_waterFlowIntensity.GetCachedPoint() )
	, m_underwaterBrightness( source.m_underwaterBrightness.GetCachedPoint() )
	, m_underWaterFogIntensity( source.m_underWaterFogIntensity.GetCachedPoint() )
	, m_waterFresnel( source.m_waterFresnel.GetCachedPoint() )
	, m_waterCaustics( source.m_waterCaustics.GetCachedPoint() )
	, m_waterFoamIntensity( source.m_waterFoamIntensity.GetCachedPoint() )
	, m_waterColor( source.m_waterColor.GetCachedPoint() )
	, m_underWaterColor( source.m_underWaterColor.GetCachedPoint() )
	, m_waterDiffuseScale( source.m_waterDiffuseScale.GetCachedPoint() )
	, m_waterAmbientScale( source.m_waterAmbientScale.GetCachedPoint() )
{
}

//////////////////////////////////////////////////////////////////////////
// SUN AND MOON PARAMETERS
CEnvSunAndMoonParameters::CEnvSunAndMoonParameters( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvSunAndMoonParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;

	INIT_GRAPH_FLOAT( m_sunSize				, 1.f, 5.f );
	INIT_GRAPH_FLOAT( m_sunFlareSize		, 1.f, 5.f );
	INIT_GRAPH_COLORSCALED( m_sunColor		, Color::WHITE, 1.f, 1.f );
	m_sunFlareColor.Reset( mode );
	
	INIT_GRAPH_FLOAT( m_moonSize			, 1.f, 5.f );
	INIT_GRAPH_FLOAT( m_moonFlareSize		, 1.f, 5.f );
	INIT_GRAPH_COLORSCALED( m_moonColor		, Color::WHITE, 1.f, 1.f );
	m_moonFlareColor.Reset( mode );
}

void CEnvSunAndMoonParameters::ImportDayPointValue( const CEnvSunAndMoonParameters &src, const CEnvSunAndMoonParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_sunSize );
		LERP_ATTRIBUTE( m_sunColor );
		LERP_ATTRIBUTE( m_sunFlareSize );
		LERP_ATTRIBUTE( m_sunFlareColor );	
		LERP_ATTRIBUTE( m_moonSize );
		LERP_ATTRIBUTE( m_moonColor );
		LERP_ATTRIBUTE( m_moonFlareSize );
		LERP_ATTRIBUTE( m_moonFlareColor );	
	LERP_GROUP_END()
}

CEnvSunAndMoonParametersAtPoint::CEnvSunAndMoonParametersAtPoint( const CEnvSunAndMoonParameters& source )
	: m_activated( source.m_activated )
	, m_sunSize( source.m_sunSize.GetCachedPoint() )
	, m_sunColor( source.m_sunColor.GetCachedPoint() )
	, m_sunFlareSize( source.m_sunFlareSize.GetCachedPoint() )
	, m_sunFlareColor( source.m_sunFlareColor )
	, m_moonSize( source.m_moonSize.GetCachedPoint() )
	, m_moonColor( source.m_moonColor.GetCachedPoint() )
	, m_moonFlareSize( source.m_moonFlareSize.GetCachedPoint() )
	, m_moonFlareColor( source.m_moonFlareColor )	
{
	/* intentionally empty */
}
//
//////////////////////////////////////////////////////////////////////////

CEnvGameplayEffectsParameters::CEnvGameplayEffectsParameters( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvGameplayEffectsParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;

	INIT_GRAPH_FLOAT( m_catEffectBrightnessMultiply,	1.f, 1.f );
	INIT_GRAPH_FLOAT( m_behaviorAnimationMultiplier,	1.f, 1.f );
	INIT_GRAPH_FLOAT( m_specularityMultiplier,	1.f, 1.f );
	INIT_GRAPH_FLOAT( m_glossinessMultiplier,	1.f, 1.f );
}

void CEnvGameplayEffectsParameters::ImportDayPointValue( const CEnvGameplayEffectsParameters &src, const CEnvGameplayEffectsParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_catEffectBrightnessMultiply );
		LERP_ATTRIBUTE( m_behaviorAnimationMultiplier );
		LERP_ATTRIBUTE( m_specularityMultiplier );
		LERP_ATTRIBUTE( m_glossinessMultiplier );
	LERP_GROUP_END()
}

CEnvGameplayEffectsParametersAtPoint::CEnvGameplayEffectsParametersAtPoint( const CEnvGameplayEffectsParameters& source )
	: m_activated( source.m_activated )
	, m_catEffectBrightnessMultiply ( source.m_catEffectBrightnessMultiply.GetCachedPoint() )
	, m_behaviorAnimationMultiplier ( source.m_behaviorAnimationMultiplier.GetCachedPoint() )
	, m_specularityMultiplier ( source.m_specularityMultiplier.GetCachedPoint() )
	, m_glossinessMultiplier ( source.m_glossinessMultiplier.GetCachedPoint() )
{
}


// --------------------------------------------------------------------------


CEnvMotionBlurParameters::CEnvMotionBlurParameters( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvMotionBlurParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;

	INIT_GRAPH_FLOAT( m_strength,	1.0f, 1.f );	
}

void CEnvMotionBlurParameters::ImportDayPointValue( const CEnvMotionBlurParameters &src, const CEnvMotionBlurParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_strength );
	LERP_GROUP_END()
}

CEnvMotionBlurParametersAtPoint::CEnvMotionBlurParametersAtPoint( const CEnvMotionBlurParameters& source )
	: m_activated( source.m_activated )
	, m_strength ( source.m_strength.GetCachedPoint() )
{
}



// --------------------------------------------------------------------------


CEnvWindParameters::CEnvWindParameters( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvWindParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;

	INIT_GRAPH_FLOAT( m_windStrengthOverride,	1.0f, 1.f );
	INIT_GRAPH_FLOAT( m_cloudsVelocityOverride,	1.0f, 1.f );
}

void CEnvWindParameters::ImportDayPointValue( const CEnvWindParameters &src, const CEnvWindParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_windStrengthOverride );
		LERP_ATTRIBUTE( m_cloudsVelocityOverride );
	LERP_GROUP_END()
}

CEnvWindParametersAtPoint::CEnvWindParametersAtPoint( const CEnvWindParameters& source )
: m_activated( source.m_activated )
, m_windStrengthOverride ( source.m_windStrengthOverride.GetCachedPoint() )
, m_cloudsVelocityOverride ( source.m_cloudsVelocityOverride.GetCachedPoint() )
{
}

// --------------------------------------------------------------------------

CEnvColorGroupsParameters::CEnvColorGroupsParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvColorGroupsParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated = false;
	INIT_GRAPH_COLORSCALED( m_defaultGroup	,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_lightsDefault	,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_lightsDawn	,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_lightsNoon	,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_lightsEvening	,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_lightsNight	,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxDefault		,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxFire		,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxFireLight	,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxFireFlares	,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxBlood		,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxFog			,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxSmoke		,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxSmokeExplosion,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxTrails		,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxScreenParticles	,			Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxWater		,				Color::WHITE, 1.f, 1.f );

	INIT_GRAPH_COLORSCALED( m_fxSky			,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxSkyNight	,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxSkyDawn		,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxSkyNoon		,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxSkySunset	,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxSkyRain		,				Color::WHITE, 1.f, 1.f );

	INIT_GRAPH_COLORSCALED( m_mainCloudsMiddle			,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_mainCloudsFront			,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_mainCloudsBack			,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_mainCloudsRim				,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_backgroundCloudsFront		,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_backgroundCloudsBack		,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_backgroundHazeFront		,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_backgroundHazeBack		,	Color::WHITE, 1.f, 1.f );


	INIT_GRAPH_COLORSCALED( m_fxLightShaft	,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftSun,				Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftInteriorDawn,		Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftSpotlightDawn,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftReflectionLightDawn,Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftInteriorNoon,		Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftSpotlightNoon,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftReflectionLightNoon,Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftInteriorEvening,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftSpotlightEvening,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftReflectionLightEvening,Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftInteriorNight,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftSpotlightNight,	Color::WHITE, 1.f, 1.f );
	INIT_GRAPH_COLORSCALED( m_fxLightShaftReflectionLightNight,Color::WHITE, 1.f, 1.f );

	INIT_GRAPH_FLOAT( m_fxSkyAlpha										, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_fxSkyNightAlpha									, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_fxSkyDawnAlpha									, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_fxSkyNoonAlpha									, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_fxSkySunsetAlpha								, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_fxSkyRainAlpha									, 1.f, 1.f );

	INIT_GRAPH_FLOAT( m_mainCloudsMiddleAlpha							, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_mainCloudsFrontAlpha							, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_mainCloudsBackAlpha								, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_mainCloudsRimAlpha								, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_backgroundCloudsFrontAlpha						, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_backgroundCloudsBackAlpha						, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_backgroundHazeFrontAlpha						, 1.f, 1.f );
	INIT_GRAPH_FLOAT( m_backgroundHazeBackAlpha							, 1.f, 1.f );

	m_activatedCustom0 = false;
	INIT_GRAPH_COLORSCALED( m_customGroup0	, Color::WHITE, 1.f, 1.f );

	m_activatedCustom1 = false;
	INIT_GRAPH_COLORSCALED( m_customGroup1	, Color::WHITE, 1.f, 1.f );

	m_activatedCustom2 = false;
	INIT_GRAPH_COLORSCALED( m_customGroup2	, Color::WHITE, 1.f, 1.f );
}

void CEnvColorGroupsParameters::ImportDayPointValue( const CEnvColorGroupsParameters &src, const CEnvColorGroupsParameters &dest, Float lerpFactor, Float time )
{
	// named groups
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_defaultGroup );
		LERP_ATTRIBUTE( m_lightsDefault );
		LERP_ATTRIBUTE( m_lightsDawn );
		LERP_ATTRIBUTE( m_lightsNoon );
		LERP_ATTRIBUTE( m_lightsEvening );
		LERP_ATTRIBUTE( m_lightsNight );
		LERP_ATTRIBUTE( m_fxDefault );
		LERP_ATTRIBUTE( m_fxFire );
		LERP_ATTRIBUTE( m_fxFireLight );
		LERP_ATTRIBUTE( m_fxFireFlares );
		LERP_ATTRIBUTE( m_fxBlood );
		LERP_ATTRIBUTE( m_fxFog );
		LERP_ATTRIBUTE( m_fxSmoke );
		LERP_ATTRIBUTE( m_fxSmokeExplosion );
		LERP_ATTRIBUTE( m_fxTrails );
		LERP_ATTRIBUTE( m_fxScreenParticles );
		LERP_ATTRIBUTE( m_fxWater );

		LERP_ATTRIBUTE( m_fxSky );
		LERP_ATTRIBUTE( m_fxSkyNight );
		LERP_ATTRIBUTE( m_fxSkyDawn );
		LERP_ATTRIBUTE( m_fxSkyNoon );
		LERP_ATTRIBUTE( m_fxSkySunset );
		LERP_ATTRIBUTE( m_fxSkyRain );
		LERP_ATTRIBUTE( m_fxSkyAlpha );
		LERP_ATTRIBUTE( m_fxSkyNightAlpha );
		LERP_ATTRIBUTE( m_fxSkyDawnAlpha );
		LERP_ATTRIBUTE( m_fxSkyNoonAlpha );
		LERP_ATTRIBUTE( m_fxSkySunsetAlpha );
		LERP_ATTRIBUTE( m_fxSkyRainAlpha );

		LERP_ATTRIBUTE( m_mainCloudsMiddle );
		LERP_ATTRIBUTE( m_mainCloudsMiddleAlpha );
		LERP_ATTRIBUTE( m_mainCloudsFront );
		LERP_ATTRIBUTE( m_mainCloudsFrontAlpha );
		LERP_ATTRIBUTE( m_mainCloudsBack );
		LERP_ATTRIBUTE( m_mainCloudsBackAlpha );
		LERP_ATTRIBUTE( m_mainCloudsRim );
		LERP_ATTRIBUTE( m_mainCloudsRimAlpha );
		LERP_ATTRIBUTE( m_backgroundCloudsFront );
		LERP_ATTRIBUTE( m_backgroundCloudsFrontAlpha );
		LERP_ATTRIBUTE( m_backgroundCloudsBack );
		LERP_ATTRIBUTE( m_backgroundCloudsBackAlpha );
		LERP_ATTRIBUTE( m_backgroundHazeFront );
		LERP_ATTRIBUTE( m_backgroundHazeFrontAlpha );
		LERP_ATTRIBUTE( m_backgroundHazeBack );
		LERP_ATTRIBUTE( m_backgroundHazeBackAlpha );

		LERP_ATTRIBUTE( m_fxLightShaft );
		LERP_ATTRIBUTE( m_fxLightShaftSun );
		LERP_ATTRIBUTE( m_fxLightShaftInteriorDawn );
		LERP_ATTRIBUTE( m_fxLightShaftSpotlightDawn );
		LERP_ATTRIBUTE( m_fxLightShaftReflectionLightDawn );
		LERP_ATTRIBUTE( m_fxLightShaftInteriorNoon );
		LERP_ATTRIBUTE( m_fxLightShaftSpotlightNoon );
		LERP_ATTRIBUTE( m_fxLightShaftReflectionLightNoon );
		LERP_ATTRIBUTE( m_fxLightShaftInteriorEvening );
		LERP_ATTRIBUTE( m_fxLightShaftSpotlightEvening );
		LERP_ATTRIBUTE( m_fxLightShaftReflectionLightEvening );
		LERP_ATTRIBUTE( m_fxLightShaftInteriorNight );
		LERP_ATTRIBUTE( m_fxLightShaftSpotlightNight );
		LERP_ATTRIBUTE( m_fxLightShaftReflectionLightNight );
	LERP_GROUP_END()

	// custom 0
	LERP_GROUP_BEGIN( m_activatedCustom0 )
		LERP_ATTRIBUTE( m_customGroup0 );
	LERP_GROUP_END()

	// custom 1
	LERP_GROUP_BEGIN( m_activatedCustom1 )
		LERP_ATTRIBUTE( m_customGroup1 );
	LERP_GROUP_END()

	// custom 2
	LERP_GROUP_BEGIN( m_activatedCustom2 )
		LERP_ATTRIBUTE( m_customGroup2 );
	LERP_GROUP_END()
}

const SSimpleCurve& CEnvColorGroupsParameters::GetCurveForColorGroup( EEnvColorGroup group ) const
{
	switch ( group )
	{
	case ECG_Default:						return m_defaultGroup;
	case ECG_LightsDefault:					return m_lightsDefault;
	case ECG_LightsDawn:					return m_lightsDawn;
	case ECG_LightsNoon:					return m_lightsNoon;
	case ECG_LightsEvening:					return m_lightsEvening;
	case ECG_LightsNight:					return m_lightsNight;
	case ECG_FX_Default:					return m_fxDefault;
	case ECG_FX_Fire:						return m_fxFire;
	case ECG_FX_FireLight:					return m_fxFireLight;
	case ECG_FX_FireFlares:					return m_fxFireFlares;
	case ECG_FX_Smoke:						return m_fxSmoke;
	case ECG_FX_SmokeExplosion:				return m_fxSmokeExplosion;
	case ECG_FX_Trails:						return m_fxTrails;
	case ECG_FX_ScreenParticles:			return m_fxScreenParticles;

	case ECG_FX_Sky:						return m_fxSky;
	case ECG_FX_SkyNight:					return m_fxSkyNight;
	case ECG_FX_SkyDawn:					return m_fxSkyDawn;
	case ECG_FX_SkyNoon:					return m_fxSkyNoon;
	case ECG_FX_SkySunset:					return m_fxSkySunset;
	case ECG_FX_SkyRain:					return m_fxSkyRain;

	case ECG_FX_MainCloudsMiddle:			return m_mainCloudsMiddle;
	case ECG_FX_MainCloudsFront:			return m_mainCloudsFront;
	case ECG_FX_MainCloudsBack:				return m_mainCloudsBack;
	case ECG_FX_MainCloudsRim:				return m_mainCloudsRim;
	case ECG_FX_BackgroundCloudsFront:		return m_backgroundCloudsFront;
	case ECG_FX_BackgroundCloudsBack:		return m_backgroundCloudsBack;
	case ECG_FX_BackgroundHazeFront:		return m_backgroundHazeFront;
	case ECG_FX_BackgroundHazeBack:			return m_backgroundHazeBack;

	case ECG_FX_Blood:						return m_fxBlood;
	case ECG_FX_Water:						return m_fxWater;
	case ECG_FX_Fog:						return m_fxFog;
	case ECG_FX_LightShaft:					return m_fxLightShaft;
	case ECG_FX_LightShaftSun:				return m_fxLightShaftSun;
	case ECG_FX_LightShaftInteriorDawn:		return m_fxLightShaftInteriorDawn;
	case ECG_FX_LightShaftSpotlightDawn:	return m_fxLightShaftSpotlightDawn;
	case ECG_FX_LightShaftReflectionLightDawn:return m_fxLightShaftReflectionLightDawn;
	case ECG_FX_LightShaftInteriorNoon:		return m_fxLightShaftInteriorNoon;
	case ECG_FX_LightShaftSpotlightNoon:	return m_fxLightShaftSpotlightNoon;
	case ECG_FX_LightShaftReflectionLightNoon:return m_fxLightShaftReflectionLightNoon;
	case ECG_FX_LightShaftInteriorEvening:	return m_fxLightShaftInteriorEvening;
	case ECG_FX_LightShaftSpotlightEvening:	return m_fxLightShaftSpotlightEvening;
	case ECG_FX_LightShaftReflectionLightEvening:return m_fxLightShaftReflectionLightEvening;
	case ECG_FX_LightShaftInteriorNight:	return m_fxLightShaftInteriorNight;
	case ECG_FX_LightShaftSpotlightNight:	return m_fxLightShaftSpotlightNight;
	case ECG_FX_LightShaftReflectionLightNight:return m_fxLightShaftReflectionLightNight;

	case ECG_Custom0:		return m_customGroup0;
	case ECG_Custom1:		return m_customGroup1;
	case ECG_Custom2:		return m_customGroup2;
	default:				ASSERT( !"invalid group" );	return m_defaultGroup; // make compiler happy
	}
}

CEnvColorGroupsParametersAtPoint::CEnvColorGroupsParametersAtPoint( const CEnvColorGroupsParameters& source )
	: m_activated( source.m_activated )
	, m_defaultGroup( source.m_defaultGroup.GetCachedPoint() )
	, m_lightsDefault( source.m_lightsDefault.GetCachedPoint() )
	, m_lightsDawn( source.m_lightsDawn.GetCachedPoint() )
	, m_lightsNoon( source.m_lightsNoon.GetCachedPoint() )
	, m_lightsEvening( source.m_lightsEvening.GetCachedPoint() )
	, m_lightsNight( source.m_lightsNight.GetCachedPoint() )
	, m_fxDefault( source.m_fxDefault.GetCachedPoint() )
	, m_fxFire( source.m_fxFire.GetCachedPoint() )
	, m_fxFireLight( source.m_fxFireLight.GetCachedPoint() )
	, m_fxFireFlares( source.m_fxFireFlares.GetCachedPoint() )
	, m_fxSmoke( source.m_fxSmoke.GetCachedPoint() )
	, m_fxSmokeExplosion( source.m_fxSmokeExplosion.GetCachedPoint() )
	, m_fxTrails( source.m_fxTrails.GetCachedPoint() )
	, m_fxScreenParticles( source.m_fxScreenParticles.GetCachedPoint() )

	, m_fxSky( source.m_fxSky.GetCachedPoint() )
	, m_fxSkyNight( source.m_fxSkyNight.GetCachedPoint() )
	, m_fxSkyDawn( source.m_fxSkyDawn.GetCachedPoint() )
	, m_fxSkyNoon( source.m_fxSkyNoon.GetCachedPoint() )
	, m_fxSkySunset( source.m_fxSkySunset.GetCachedPoint() )
	, m_fxSkyRain( source.m_fxSkyRain.GetCachedPoint() )
	, m_fxSkyAlpha( source.m_fxSkyAlpha.GetCachedPoint() )
	, m_fxSkyNightAlpha( source.m_fxSkyNightAlpha.GetCachedPoint() )
	, m_fxSkyDawnAlpha( source.m_fxSkyDawnAlpha.GetCachedPoint() )
	, m_fxSkyNoonAlpha( source.m_fxSkyNoonAlpha.GetCachedPoint() )
	, m_fxSkySunsetAlpha( source.m_fxSkySunsetAlpha.GetCachedPoint() )
	, m_fxSkyRainAlpha( source.m_fxSkyRainAlpha.GetCachedPoint() )

	, m_mainCloudsMiddle( source.m_mainCloudsMiddle.GetCachedPoint() )
	, m_mainCloudsMiddleAlpha( source.m_mainCloudsMiddleAlpha.GetCachedPoint() )
	, m_mainCloudsFront( source.m_mainCloudsFront.GetCachedPoint() )
	, m_mainCloudsFrontAlpha( source.m_mainCloudsFrontAlpha.GetCachedPoint() )
	, m_mainCloudsBack( source.m_mainCloudsBack.GetCachedPoint() )
	, m_mainCloudsBackAlpha( source.m_mainCloudsBackAlpha.GetCachedPoint() )
	, m_mainCloudsRim( source.m_mainCloudsRim.GetCachedPoint() )
	, m_mainCloudsRimAlpha( source.m_mainCloudsRimAlpha.GetCachedPoint() )
	, m_backgroundCloudsFront( source.m_backgroundCloudsFront.GetCachedPoint() )
	, m_backgroundCloudsFrontAlpha( source.m_backgroundCloudsFrontAlpha.GetCachedPoint() )
	, m_backgroundCloudsBack( source.m_backgroundCloudsBack.GetCachedPoint() )
	, m_backgroundCloudsBackAlpha( source.m_backgroundCloudsBackAlpha.GetCachedPoint() )
	, m_backgroundHazeFront( source.m_backgroundHazeFront.GetCachedPoint() )
	, m_backgroundHazeFrontAlpha( source.m_backgroundHazeFrontAlpha.GetCachedPoint() )
	, m_backgroundHazeBack( source.m_backgroundHazeBack.GetCachedPoint() )
	, m_backgroundHazeBackAlpha( source.m_backgroundHazeBackAlpha.GetCachedPoint() )

	, m_fxBlood( source.m_fxBlood.GetCachedPoint() )
	, m_fxWater( source.m_fxWater.GetCachedPoint() )
	, m_fxFog( source.m_fxFog.GetCachedPoint() )
	, m_fxLightShaft( source.m_fxLightShaft.GetCachedPoint() )
	, m_fxLightShaftSun( source.m_fxLightShaftSun.GetCachedPoint() )
	, m_fxLightShaftInteriorDawn( source.m_fxLightShaftInteriorDawn.GetCachedPoint() )
	, m_fxLightShaftSpotlightDawn( source.m_fxLightShaftSpotlightDawn.GetCachedPoint() )
	, m_fxLightShaftReflectionLightDawn( source.m_fxLightShaftReflectionLightDawn.GetCachedPoint() )
	, m_fxLightShaftInteriorNoon( source.m_fxLightShaftInteriorNoon.GetCachedPoint() )
	, m_fxLightShaftSpotlightNoon( source.m_fxLightShaftSpotlightNoon.GetCachedPoint() )
	, m_fxLightShaftReflectionLightNoon( source.m_fxLightShaftReflectionLightNoon.GetCachedPoint() )
	, m_fxLightShaftInteriorEvening( source.m_fxLightShaftInteriorEvening.GetCachedPoint() )
	, m_fxLightShaftSpotlightEvening( source.m_fxLightShaftSpotlightEvening.GetCachedPoint() )
	, m_fxLightShaftReflectionLightEvening( source.m_fxLightShaftReflectionLightEvening.GetCachedPoint() )
	, m_fxLightShaftInteriorNight( source.m_fxLightShaftInteriorNight.GetCachedPoint() )
	, m_fxLightShaftSpotlightNight( source.m_fxLightShaftSpotlightNight.GetCachedPoint() )
	, m_fxLightShaftReflectionLightNight( source.m_fxLightShaftReflectionLightNight.GetCachedPoint() )
	, m_activatedCustom0( source.m_activatedCustom0 )
	, m_customGroup0( source.m_customGroup0.GetCachedPoint() )
	, m_activatedCustom1( source.m_activatedCustom1 )
	, m_customGroup1( source.m_customGroup1.GetCachedPoint() )
	, m_activatedCustom2( source.m_activatedCustom2 )
	, m_customGroup2( source.m_customGroup2.GetCachedPoint() )
{
}

const SSimpleCurvePoint& CEnvColorGroupsParametersAtPoint::GetCurveForColorGroup( EEnvColorGroup group ) const
{
	switch ( group )
	{
	case ECG_Default:						return m_defaultGroup;
	case ECG_LightsDefault:					return m_lightsDefault;
	case ECG_LightsDawn:					return m_lightsDawn;
	case ECG_LightsNoon:					return m_lightsNoon;
	case ECG_LightsEvening:					return m_lightsEvening;
	case ECG_LightsNight:					return m_lightsNight;
	case ECG_FX_Default:					return m_fxDefault;
	case ECG_FX_Fire:						return m_fxFire;
	case ECG_FX_FireFlares:					return m_fxFireFlares;
	case ECG_FX_FireLight:					return m_fxFireLight;
	case ECG_FX_Smoke:						return m_fxSmoke;
	case ECG_FX_SmokeExplosion:				return m_fxSmokeExplosion;
	case ECG_FX_Trails:						return m_fxTrails;
	case ECG_FX_ScreenParticles:			return m_fxScreenParticles;

	case ECG_FX_Sky:						return m_fxSky;
	case ECG_FX_SkyNight:					return m_fxSkyNight;
	case ECG_FX_SkyDawn:					return m_fxSkyDawn;
	case ECG_FX_SkyNoon:					return m_fxSkyNoon;
	case ECG_FX_SkySunset:					return m_fxSkySunset;
	case ECG_FX_SkyRain:					return m_fxSkyRain;

	case ECG_FX_MainCloudsMiddle:			return m_mainCloudsMiddle;
	case ECG_FX_MainCloudsFront:			return m_mainCloudsFront;
	case ECG_FX_MainCloudsBack:				return m_mainCloudsBack;
	case ECG_FX_MainCloudsRim:				return m_mainCloudsRim;
	case ECG_FX_BackgroundCloudsFront:		return m_backgroundCloudsFront;
	case ECG_FX_BackgroundCloudsBack:		return m_backgroundCloudsBack;
	case ECG_FX_BackgroundHazeFront:		return m_backgroundHazeFront;
	case ECG_FX_BackgroundHazeBack:			return m_backgroundHazeBack;

	case ECG_FX_Blood:						return m_fxBlood;
	case ECG_FX_Water:						return m_fxWater;
	case ECG_FX_Fog:						return m_fxFog;
	case ECG_FX_LightShaft:					return m_fxLightShaft;
	case ECG_FX_LightShaftSun:				return m_fxLightShaftSun;
	case ECG_FX_LightShaftSpotlightDawn:	return m_fxLightShaftSpotlightDawn;
	case ECG_FX_LightShaftInteriorDawn:		return m_fxLightShaftInteriorDawn;
	case ECG_FX_LightShaftReflectionLightDawn:return m_fxLightShaftReflectionLightDawn;
	case ECG_FX_LightShaftSpotlightNoon:	return m_fxLightShaftSpotlightNoon;
	case ECG_FX_LightShaftInteriorNoon:		return m_fxLightShaftInteriorNoon;
	case ECG_FX_LightShaftReflectionLightNoon:return m_fxLightShaftReflectionLightNoon;
	case ECG_FX_LightShaftSpotlightEvening:	return m_fxLightShaftSpotlightEvening;
	case ECG_FX_LightShaftInteriorEvening:	return m_fxLightShaftInteriorEvening;
	case ECG_FX_LightShaftReflectionLightEvening:return m_fxLightShaftReflectionLightEvening;
	case ECG_FX_LightShaftSpotlightNight:	return m_fxLightShaftSpotlightNight;
	case ECG_FX_LightShaftInteriorNight:	return m_fxLightShaftInteriorNight;
	case ECG_FX_LightShaftReflectionLightNight:return m_fxLightShaftReflectionLightNight;

	case ECG_Custom0:		return m_customGroup0;
	case ECG_Custom1:		return m_customGroup1;
	case ECG_Custom2:		return m_customGroup2;
	default:				ASSERT( !"invalid group" );	return m_defaultGroup; // make compiler happy
	}
}

const Float CEnvColorGroupsParametersAtPoint::GetAlphaForColorGroup( EEnvColorGroup group ) const
{
	switch ( group )
	{
		case ECG_FX_Sky:						return m_fxSkyAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_SkyNight:					return m_fxSkyNightAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_SkyDawn:					return m_fxSkyDawnAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_SkyNoon:					return m_fxSkyNoonAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_SkySunset:					return m_fxSkySunsetAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_SkyRain:					return m_fxSkyRainAlpha.GetScalarClamp( 0.0f, 1.0f );

		case ECG_FX_MainCloudsMiddle:			return m_mainCloudsMiddleAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_MainCloudsFront:			return m_mainCloudsFrontAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_MainCloudsBack:				return m_mainCloudsBackAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_MainCloudsRim:				return m_mainCloudsRimAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_BackgroundCloudsFront:		return m_backgroundCloudsFrontAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_BackgroundCloudsBack:		return m_backgroundCloudsBackAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_BackgroundHazeFront:		return m_backgroundHazeFrontAlpha.GetScalarClamp( 0.0f, 1.0f );
		case ECG_FX_BackgroundHazeBack:			return m_backgroundHazeBackAlpha.GetScalarClamp( 0.0f, 1.0f );

		default: return 1.0f;
	}
}

// --------------------------------------------------------------------------

CEnvDistanceRangeParameters::CEnvDistanceRangeParameters ( EEnvParamsResetMode mode )
{
	Reset( mode );
}

void CEnvDistanceRangeParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated	= false;
	INIT_GRAPH_FLOAT( m_distance	,45.f,	50.f );
	INIT_GRAPH_FLOAT( m_range		,5.f,	50.f );
}

void CEnvDistanceRangeParameters::ImportDayPointValue( const CEnvDistanceRangeParameters &src, const CEnvDistanceRangeParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_distance );
		LERP_ATTRIBUTE( m_range );
	LERP_GROUP_END()
}

CEnvDistanceRangeParametersAtPoint::CEnvDistanceRangeParametersAtPoint( const CEnvDistanceRangeParameters& source )
	: m_activated( source.m_activated )
	, m_distance( source.m_distance.GetCachedPoint() )
	, m_range( source.m_range.GetCachedPoint() )
{
}

// --------------------------------------------------------------------------

CAreaEnvironmentParams::CAreaEnvironmentParams ( EEnvParamsResetMode mode )
 : m_finalColorBalance		( mode )
 , m_sharpen				( mode )
 , m_paintEffect			( mode )
 , m_nvSsao					( mode )
 , m_msSsao					( mode )
 , m_globalLight			( mode )
 , m_interiorFallback		( mode )
 , m_speedTree				( mode )
 , m_toneMapping			( mode )
 , m_bloomNew				( mode )
 , m_globalFog				( mode )
 , m_sky					( mode )
 , m_depthOfField			( mode )
 , m_colorModTransparency	( mode )
 , m_shadows				( mode )
 , m_water					( mode )
 , m_colorGroups			( mode ) 
 , m_flareColorGroups		( mode ) 
 , m_sunAndMoonParams		( mode )
 , m_windParams				( mode )
 , m_gameplayEffects		( mode )
 , m_motionBlur				( mode )
 , m_cameraLightsSetup		( mode )
{}

void CAreaEnvironmentParams::Reset( EEnvParamsResetMode mode )
{
	m_finalColorBalance.Reset( mode );
	m_sharpen.Reset( mode );
	m_paintEffect.Reset( mode );
	m_nvSsao.Reset( mode );
	m_msSsao.Reset( mode );
	m_globalLight.Reset( mode );
	m_interiorFallback.Reset( mode );
	m_speedTree.Reset( mode );
	m_toneMapping.Reset( mode );
	m_bloomNew.Reset( mode );
	m_globalFog.Reset( mode );
	m_sky.Reset( mode );
	m_depthOfField.Reset( mode );
	m_colorModTransparency.Reset( mode );
	m_shadows.Reset( mode );
	m_water.Reset( mode );
	m_colorGroups.Reset( mode );	
	m_flareColorGroups.Reset( mode );	
	m_sunAndMoonParams.Reset( mode );
	m_windParams.Reset( mode );
	m_gameplayEffects.Reset( mode );
	m_motionBlur.Reset( mode );
	m_cameraLightsSetup.Reset( mode );
}

void CAreaEnvironmentParams::ImportDayPointValue( const CAreaEnvironmentParams &src, const CAreaEnvironmentParams &dest, Float lerpFactor, Float time )
{
	LERP_STRUCT_ATTRIBUTE( m_finalColorBalance );
	LERP_STRUCT_ATTRIBUTE( m_sharpen );
	LERP_STRUCT_ATTRIBUTE( m_paintEffect );
	LERP_STRUCT_ATTRIBUTE( m_nvSsao );
	LERP_STRUCT_ATTRIBUTE( m_msSsao );
	LERP_STRUCT_ATTRIBUTE( m_globalLight );
	LERP_STRUCT_ATTRIBUTE( m_interiorFallback );
	LERP_STRUCT_ATTRIBUTE( m_speedTree );
	LERP_STRUCT_ATTRIBUTE( m_toneMapping );
	LERP_STRUCT_ATTRIBUTE( m_bloomNew );
	LERP_STRUCT_ATTRIBUTE( m_globalFog );
	LERP_STRUCT_ATTRIBUTE( m_sky );
	LERP_STRUCT_ATTRIBUTE( m_depthOfField );
	LERP_STRUCT_ATTRIBUTE( m_colorModTransparency );
	LERP_STRUCT_ATTRIBUTE( m_shadows );
	LERP_STRUCT_ATTRIBUTE( m_water );
	LERP_STRUCT_ATTRIBUTE( m_colorGroups );	
	LERP_STRUCT_ATTRIBUTE( m_flareColorGroups );	
	LERP_STRUCT_ATTRIBUTE( m_sunAndMoonParams );
	LERP_STRUCT_ATTRIBUTE( m_windParams );
	LERP_STRUCT_ATTRIBUTE( m_gameplayEffects );
	LERP_STRUCT_ATTRIBUTE( m_motionBlur );
	LERP_STRUCT_ATTRIBUTE( m_cameraLightsSetup );
	LERP_STRUCT_ATTRIBUTE( m_dialogLightParams );
}

CAreaEnvironmentParamsAtPoint::CAreaEnvironmentParamsAtPoint( const CAreaEnvironmentParams& source )
	: m_finalColorBalance( source.m_finalColorBalance )
	, m_sharpen( source.m_sharpen )
	, m_paintEffect( source.m_paintEffect )
	, m_nvSsao( source.m_nvSsao )
	, m_msSsao( source.m_msSsao )
	, m_globalLight( source.m_globalLight )
	, m_interiorFallback( source.m_interiorFallback )
	, m_speedTree( source.m_speedTree )
	, m_toneMapping( source.m_toneMapping )
	, m_bloomNew( source.m_bloomNew )
	, m_globalFog( source.m_globalFog )
	, m_sky( source.m_sky )
	, m_depthOfField( source.m_depthOfField )
	, m_colorModTransparency( source.m_colorModTransparency )
	, m_shadows( source.m_shadows )
	, m_water( source.m_water )
	, m_colorGroups( source.m_colorGroups )	
	, m_flareColorGroups( source.m_flareColorGroups )	
	, m_sunParams( source.m_sunAndMoonParams )
	, m_windParams( source.m_windParams )
	, m_gameplayEffects( source.m_gameplayEffects )
	, m_motionBlurParameters( source.m_motionBlur )
	, m_cameraLightsSetup( source.m_cameraLightsSetup )
{
}

const CEnvFlareColorParametersAtPoint& CAreaEnvironmentParamsAtPoint::GetFlareColorParameters( EEnvFlareColorGroup group ) const
{
	switch ( group )
	{
	case EFCG_Default:		return m_flareColorGroups.m_default;
	case EFCG_Sun:			return m_sunParams.m_sunFlareColor;
	case EFCG_Moon:			return m_sunParams.m_moonFlareColor;
	case EFCG_Custom0:		return m_flareColorGroups.m_custom0;
	case EFCG_Custom1:		return m_flareColorGroups.m_custom1;
	case EFCG_Custom2:		return m_flareColorGroups.m_custom2;
	default:				ASSERT( !"Invalid" ); return m_flareColorGroups.m_default;
	}
}

Vector CAreaEnvironmentParamsAtPoint::GetFlareColor( EEnvFlareColorGroup group, Uint32 paramIndex ) const
{
	return GetFlareColorParameters( group ).GetColorByIndex( paramIndex );
}

Float CAreaEnvironmentParamsAtPoint::GetFlareOpacity( EEnvFlareColorGroup group, Uint32 paramIndex ) const
{
	return GetFlareColorParameters( group ).GetOpacityByIndex( paramIndex );
}

void CEnvDialogLightParameters::Reset( EEnvParamsResetMode mode )
{
	m_activated	= false;
	INIT_GRAPH_COLOR( m_lightColor , Color::WHITE );
	INIT_GRAPH_COLOR( m_lightColor2 , Color::WHITE );
	INIT_GRAPH_COLOR( m_lightColor3 , Color::WHITE );
}

void CEnvDialogLightParameters::ImportDayPointValue( const CEnvDialogLightParameters &src, const CEnvDialogLightParameters &dest, Float lerpFactor, Float time )
{
	LERP_GROUP_BEGIN( m_activated )
		LERP_ATTRIBUTE( m_lightColor );
		LERP_ATTRIBUTE( m_lightColor2 );
		LERP_ATTRIBUTE( m_lightColor3 );
	LERP_GROUP_END()
}

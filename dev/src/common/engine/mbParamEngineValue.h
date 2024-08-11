/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "materialBlock.h"


/// Engine value types
enum EMaterialEngineValueType
{
	MEVT_DistanceToCamera,			//!< Object's distance to camera
	MEVT_DistanceToCameraParallel,	//!< Object's distance to camera along z axis
	MEVT_CameraPosition,			//!< Camera world position
	MEVT_CameraVectorUp,			//!< Camera up vector
	MEVT_CameraVectorForward,		//!< Camera forward vector
	MEVT_CameraVectorRight,			//!< Camera right vector
	MEVT_CameraNear,				//!< Camera near plane
	MEVT_CameraFar,					//!< Camera far plane
	MEVT_GameTime,					//!< Game time
	MEVT_Date,						//!< Game date ( days )
	MEVT_WorldTime,					//!< World time
	MEVT_DayHour,					//!< Hour
	MEVT_GlobalLightDir,			//!< Global light direction
	MEVT_MoonDir,					//!< Moon direction
	MEVT_GlobalLightDiffuseColor,	//!< Global light diffuse color
	MEVT_GlobalLightSpecularColor,	//!< Global light specular color ( Deprecated )
	MEVT_GlobalWaterColor,			//!< Global water color
	MEVT_GlobalWaterSpecularColor,	//!< Global water specular color
	MEVT_GlobalWaterEnvCubeColor,	//!< Global water environment cube color
	MEVT_GlobalDayAmount,			//!< Global day amount
	MEVT_Custom0,					//!< Custom parameter 0 ( effect mostly )
	MEVT_Custom1,					//!< Custom parameter 1 ( effect mostly )
	MEVT_FXColor,					//!< Effect color & alpha value
	MEVT_GameplayParamsStrength,	//!< Strength of 4 gameplay params
	MEVT_GameplayFXRendererParam,	//!< Renderer-side gameplay effects parameter (eg. fade-in)
	MEVT_FoliageColor,				//!< Foliage coloring
	MEVT_RainStrength,				//!< Rain strength [normalized]
	MEVT_WetSurfaceStrength,		//!< Wet surface effect [normalized]
	MEVT_DelayedWetSurfaceStrength,	//!< Delayed wet surface effect [normalized]
	MEVT_WindStrength,				//!< Wind strength [normalized]
	MEVT_WindDirectionZ,			//!< Wind dir [rad] on Z axis
	MEVT_SkyboxWeatherBlend,		//!< Blend factor for combining skybox texture array
	MEVT_CloudsShadowIntensity,		//!< Blend factor for skybox texture array's day/night parts ( Deprecated )
	MEVT_MorphBlendRatio,			//!< Morph ratio for morphed meshes
	MEVT_LightUsageMask,			//!< Light usage mask ( Flag set in light properties )
	MEVT_TransparencyParams,		//!< Transparency params
	MEVT_SkeletalExtraData,			//!< Extra skeletal params - wetness for the player
	MEVT_ScreenVPOS					//!< Screen VPOS
};

BEGIN_ENUM_RTTI( EMaterialEngineValueType );
	ENUM_OPTION( MEVT_DistanceToCamera );
	ENUM_OPTION( MEVT_DistanceToCameraParallel );
	ENUM_OPTION( MEVT_CameraPosition );
	ENUM_OPTION( MEVT_CameraVectorUp );
	ENUM_OPTION( MEVT_CameraVectorForward );
	ENUM_OPTION( MEVT_CameraVectorRight );
	ENUM_OPTION( MEVT_CameraNear );
	ENUM_OPTION( MEVT_CameraFar );
	ENUM_OPTION( MEVT_GameTime );
	ENUM_OPTION( MEVT_Date );
	ENUM_OPTION( MEVT_WorldTime );
	ENUM_OPTION( MEVT_DayHour );
	ENUM_OPTION( MEVT_GlobalLightDir );
	ENUM_OPTION( MEVT_MoonDir );
	ENUM_OPTION( MEVT_GlobalLightDiffuseColor );
	ENUM_OPTION( MEVT_GlobalLightSpecularColor );
	ENUM_OPTION( MEVT_GlobalDayAmount );
	ENUM_OPTION( MEVT_Custom0 );
	ENUM_OPTION( MEVT_Custom1 );
	ENUM_OPTION( MEVT_FXColor );
	ENUM_OPTION( MEVT_GameplayFXRendererParam );
	ENUM_OPTION( MEVT_FoliageColor );
	ENUM_OPTION( MEVT_RainStrength );
	ENUM_OPTION( MEVT_WetSurfaceStrength );
	ENUM_OPTION( MEVT_DelayedWetSurfaceStrength );
	ENUM_OPTION( MEVT_WindStrength );
	ENUM_OPTION( MEVT_WindDirectionZ );
	ENUM_OPTION( MEVT_SkyboxWeatherBlend );
	ENUM_OPTION( MEVT_CloudsShadowIntensity );
	ENUM_OPTION( MEVT_MorphBlendRatio );
	ENUM_OPTION( MEVT_LightUsageMask );
	ENUM_OPTION( MEVT_TransparencyParams );
	ENUM_OPTION( MEVT_SkeletalExtraData );
	ENUM_OPTION( MEVT_ScreenVPOS );
END_ENUM_RTTI()

/// A block that defines scalar parameter
class CMaterialParameterEngineValue : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialParameterEngineValue, CMaterialBlock, "Input", "Engine Value" );

public:
	EMaterialEngineValueType		m_valueType;

public:
	//! Get parameter type
	RED_INLINE EMaterialEngineValueType GetValueType() const { return m_valueType; }

public:
	CMaterialParameterEngineValue();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Property was changed
	virtual void OnPropertyPostChange( IProperty *property );
	virtual String GetCaption() const;

#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	//! Compile code
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;	
#endif

	// Calculate parameter mask for mesh rendering fragment
	virtual Uint32 CalcRenderingFragmentParamMask() const;
};

BEGIN_CLASS_RTTI( CMaterialParameterEngineValue );
	PARENT_CLASS( CMaterialBlock );
	PROPERTY_EDIT( m_valueType, TXT("Type of engine value") );
END_CLASS_RTTI();



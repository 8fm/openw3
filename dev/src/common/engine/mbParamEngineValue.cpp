#include "build.h"
#include "mbParamEngineValue.h"
#include "../../../../bin/shaders/include/globalConstants.fx"
#include "../../../../bin/shaders/include/globalConstantsPS.fx"
#include "../../../../bin/shaders/include/globalConstantsVS.fx"

#include "graphConnectionRebuilder.h"
#include "materialInputSocket.h"
#include "materialOutputSocket.h"
#include "materialBlockCompiler.h"
#include "renderFragment.h"


IMPLEMENT_ENGINE_CLASS( CMaterialParameterEngineValue );
IMPLEMENT_RTTI_ENUM( EMaterialEngineValueType );
RED_DEFINE_STATIC_NAME( CustomNo );

CMaterialParameterEngineValue::CMaterialParameterEngineValue()
	: m_valueType( MEVT_GameTime )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialParameterEngineValue::OnPropertyPostChange( IProperty *property )
{
	// Pass to base class
	TBaseClass::OnPropertyPostChange( property );

	// Update layout
	OnRebuildSockets();
}

String CMaterialParameterEngineValue::GetCaption() const
{
	switch (m_valueType)
	{
		case MEVT_DistanceToCamera: return TXT("Distance to Camera");
		case MEVT_DistanceToCameraParallel: return TXT("Distance to Camera Parallel");
		case MEVT_CameraPosition: return TXT("Camera Position");
		case MEVT_CameraVectorUp: return TXT("Camera Vector Up");
		case MEVT_CameraVectorForward: return TXT("Camera Vector Forward");
		case MEVT_CameraVectorRight: return TXT("Camera Vector Right");
		case MEVT_CameraNear: return TXT("Camera Near Plane");
		case MEVT_CameraFar: return TXT("Camera Far Plane");
		case MEVT_GameTime: return TXT("Game Time");
		case MEVT_Date: return TXT("Date (Days)");
		case MEVT_WorldTime: return TXT("World Time");
		case MEVT_DayHour: return TXT("Day Hour");
		case MEVT_GlobalLightDir: return TXT("Global Light Direction");
		case MEVT_MoonDir: return TXT("Moon Direction");
		case MEVT_GlobalLightDiffuseColor: return TXT("Global Light Diffuse Color");
		case MEVT_GlobalLightSpecularColor: return TXT("Global Light Specular Color ( Deprecated )");		
		case MEVT_GlobalDayAmount: return TXT("Global Day Amount");
		case MEVT_Custom0: return TXT("Custom 0");
		case MEVT_Custom1: return TXT("Custom 1");
		case MEVT_FXColor: return TXT("FX Color");
		case MEVT_GameplayParamsStrength: return TXT("Gameplay Parameters Strength");
		case MEVT_GameplayFXRendererParam: return TXT("Gameplay FX Renderer Parameter");
		case MEVT_FoliageColor: return TXT("Foliage Color");
		case MEVT_RainStrength: return TXT("Rain Strength");
		case MEVT_WetSurfaceStrength: return TXT("Wet Surface Strength");
		case MEVT_DelayedWetSurfaceStrength: return TXT("Delayed Wet Surface Strength");
		case MEVT_WindStrength: return TXT("Wind Strength");
		case MEVT_WindDirectionZ: return TXT("Wind Direction Z");
		case MEVT_SkyboxWeatherBlend: return TXT("Skybox Weather Blend");
		case MEVT_CloudsShadowIntensity: return TXT("Clouds Shadow Intensity ( Deprecated )");
		case MEVT_MorphBlendRatio: return TXT("Morph Blend Ratio");
		case MEVT_LightUsageMask: return TXT("Light Usage Mask");
		case MEVT_TransparencyParams: return TXT("TransparencyParams");
		case MEVT_SkeletalExtraData: return TXT("SkeletalExtraData");		
		case MEVT_ScreenVPOS: return TXT("ScreenVPOS");
		default: return TXT("Engine Value");
	}
}

void CMaterialParameterEngineValue::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	
	if ( m_valueType == MEVT_LightUsageMask )
	{
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( CustomNo ) ) );
	}
	
	if ( (m_valueType >= MEVT_Custom0 && m_valueType <= MEVT_GameplayParamsStrength) ||
		  MEVT_TransparencyParams == m_valueType )
	{
		// Vector type
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Out ), Color::WHITE ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( X ), Color::RED ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("yyyy"), CNAME( Y ), Color::GREEN ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("zzzz"), CNAME( Z ), Color::BLUE ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( W ), Color( 127, 127, 127 ) ) );
	}
	else
	{
		// Scalar type
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Out ), Color::WHITE ) );
	}
}

#endif

Uint32 CMaterialParameterEngineValue::CalcRenderingFragmentParamMask() const
{
	// Some engine values requires special data to be transmitted via mesh render fragment
	switch ( m_valueType )
	{
		case MEVT_Custom0: return RFMP_CustomMaterialParameter0;
		case MEVT_Custom1: return RFMP_CustomMaterialParameter1;
		case MEVT_FoliageColor: return RFMP_FoliageColor;
	}

	// No extra params
	return 0;
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialParameterEngineValue::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Distance to camera
	if ( m_valueType == MEVT_DistanceToCamera )
	{
		CodeChunk distance = PS_DATA( "CameraDistance" );
		return distance.xxxx();
	}

	// Distance to camera parallel
	if ( m_valueType == MEVT_DistanceToCameraParallel )
	{
		CodeChunk distance = PS_DATA( "ViewZ" );
		return distance.xxxx();
	}

	// Camera world position
	if ( m_valueType == MEVT_CameraPosition )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_CameraPosition" );
		return CodeChunkOperators::Float4 ( reg.xyz(), 0.f );
	}

	if ( m_valueType == MEVT_CameraVectorUp )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_CameraVectorUp" );
		return CodeChunkOperators::Float4 ( reg.xyz(), 0.f );
	}

	if ( m_valueType == MEVT_CameraVectorForward )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_CameraVectorForward" );
		return CodeChunkOperators::Float4 ( reg.xyz(), 0.f );
	}

	if ( m_valueType == MEVT_CameraVectorRight )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_CameraVectorRight" );
		return CodeChunkOperators::Float4 ( reg.xyz(), 0.f );
	}

	if ( m_valueType == MEVT_CameraNear )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_CameraNearFar" ) ;
		return CodeChunkOperators::Float4 ( reg.xxx(), 0.f );
	}

	if ( m_valueType == MEVT_CameraFar )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_CameraNearFar" );
		return CodeChunkOperators::Float4 ( reg.yyy(), 0.f );
	}

	if ( m_valueType == MEVT_WindStrength )
	{		
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_WeatherShadingParams" );
		return reg.yyyy();
	}	

	if ( m_valueType == MEVT_WindDirectionZ )
	{		
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_WeatherShadingParams" );
		return reg.zzzz();
	}

	// Global light direction
	if ( m_valueType == MEVT_GlobalLightDir )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_GlobalLightDir" );
		return CodeChunkOperators::Float4 ( reg.xyz(), 1.f );
	}

	// Moon direction
	if ( m_valueType == MEVT_MoonDir )
	{
		CodeChunk moonDirParam = compiler.GetPS().AutomaticName();
		CodeChunk moonDirCall = CodeChunk::Printf( false, "float3 %s = skyParamsMoon.yzw;", moonDirParam.AsChar());
		compiler.GetPS().Statement(moonDirCall);
		return CodeChunkOperators::Float4 ( moonDirParam, 1.0f );
	}

	// Global day amount
	if ( m_valueType == MEVT_GlobalDayAmount )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_GlobalLightDir" );
		return CodeChunkOperators::Float4 ( reg.wwww() );
	}

	// Global light diffuse color
	if ( m_valueType == MEVT_GlobalLightDiffuseColor )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_GlobalLightColorAndSpecScale" );
		return CodeChunkOperators::Float4 ( reg.xyz(), 1.f );
	}

	// Global light specular color
	if ( m_valueType == MEVT_GlobalLightSpecularColor )
	{
		return CodeChunkOperators::Float4 ( 0.0f, 0.0f, 0.0f, 0.0f );
	}	

	// Custom material parameter 0
	if ( m_valueType == MEVT_Custom0 )
	{
		CodeChunk reg;

		if( shaderTarget == MSH_VertexShader )
		{
			reg = compiler.GetPS().ConstReg( MDT_Float4, "VSC_Custom_0" );
		}
		else
			reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_Custom0" );
		
		return reg;
	}

	// Custom material parameter 1
	if ( m_valueType == MEVT_Custom1 )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_Custom1" );
		return reg;
	}

	// FX Color
	if ( m_valueType == MEVT_FXColor )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_FXColor" );
		return reg;
	}	

	// Gameplay parameters
	if ( m_valueType == MEVT_GameplayFXRendererParam )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_GameplayEffectsRendererParameter" );
		return reg;
	}

	// Game time
	if ( m_valueType == MEVT_GameTime )
	{
		CodeChunk reg;

		if( shaderTarget == MSH_PixelShader )
		{
			reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_TimeVector" );
		}

		if( shaderTarget == MSH_VertexShader )
		{
			reg = compiler.GetVS().ConstReg( MDT_Float4, "VSC_TimeVector" );
		}
		return reg.xxxx();
	}

	// Date
	if ( m_valueType == MEVT_Date )
	{
		CodeChunk reg;

		if( shaderTarget == MSH_PixelShader )
		{
			reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_TimeVector" );
		}

		if( shaderTarget == MSH_VertexShader )
		{
			reg = compiler.GetVS().ConstReg( MDT_Float4, "VSC_TimeVector" );
		}
		return reg.yyyy();
	}

	// World time
	if ( m_valueType == MEVT_WorldTime )
	{
		CodeChunk reg;

		if( shaderTarget == MSH_PixelShader )
		{
			reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_TimeVector" );
		}

		if( shaderTarget == MSH_VertexShader )
		{
			reg = compiler.GetVS().ConstReg( MDT_Float4, "VSC_TimeVector" );
		}
		return reg.zzzz();
	}

	// Day Hour
	if ( m_valueType == MEVT_DayHour )
	{
		CodeChunk reg;
		if( shaderTarget == MSH_PixelShader )
		{
			reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_TimeVector" );
		}

		if( shaderTarget == MSH_VertexShader )
		{
			reg = compiler.GetVS().ConstReg( MDT_Float4, "VSC_TimeVector" );
		}
		return reg.wwww();
	}

	// Distance to camera parallel
	if ( m_valueType == MEVT_FoliageColor )
	{
		CodeChunk color = PS_DATA( "FoliageColor" );
		return CodeChunkOperators::Float4( color.xyz(), 1.0f );
	}

	
	// Skybox day/night blend
	if ( m_valueType == MEVT_CloudsShadowIntensity )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_WeatherAndPrescaleParams" );
		return reg.xxxx();
	}

	// Rain Strength
	if ( m_valueType == MEVT_RainStrength )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_WeatherAndPrescaleParams" );
		return reg.yyyy();
	}

	// Wet surface effect strength
	if ( m_valueType == MEVT_WetSurfaceStrength )
	{
		if( shaderTarget == MSH_VertexShader )
		{
			CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "VSC_WetSurfaceEffect" );
			return reg.xxxx();
		}
		
		if( shaderTarget == MSH_PixelShader )
		{
			CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_WeatherShadingParams" );
			return reg.xxxx();
		}		
	}	

	// Delayed wet surface effect strength
	if ( m_valueType == MEVT_DelayedWetSurfaceStrength )
	{
		if( shaderTarget == MSH_VertexShader )
		{
			CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "VSC_WetSurfaceEffect" );
			return reg.wwww();
		}

		if( shaderTarget == MSH_PixelShader )
		{
			CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_WeatherShadingParams" );
			return reg.wwww();
		}		
	}	

	// Skybox weather blend
	if ( m_valueType == MEVT_SkyboxWeatherBlend )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_WeatherAndPrescaleParams" );
		return reg.wwww();
	}

	if ( m_valueType == MEVT_MorphBlendRatio )
	{
		CodeChunk reg = compiler.GetPS().ConstReg( MDT_Float4, "PSC_MorphRatio" );
		return reg.xxxx();
	}

	// Extra skeletal data - wetness
	if ( m_valueType == MEVT_SkeletalExtraData )
	{		
		CodeChunk reg = compiler.GetPS().Data( "SkeletalExtraData" );
		return reg.xyzw();
	}	

	if ( m_valueType == MEVT_ScreenVPOS )
	{		
		CodeChunk reg = CodeChunkOperators::Float4( compiler.GetPS().Data( "ScreenVPOS" ).xy(), 0.f, 0.f );
		return reg.xyzw();
	}	

	if ( m_valueType == MEVT_LightUsageMask )
	{
		using namespace CodeChunkOperators;

		CodeChunk shaderFile = CodeChunk::Printf(true, "include_constants.fx");

		switch( shaderTarget )
		{
		case MSH_PixelShader:
			compiler.GetMaterialCompiler()->GetPixelShaderCompiler() -> Include(shaderFile);
			break;
		case MSH_VertexShader:
			compiler.GetMaterialCompiler()->GetVertexShaderCompiler() -> Include(shaderFile);
			break;
		}

		CodeChunk result = compiler.GetShader(shaderTarget).AutomaticName();
		CodeChunk worldPosition = SHADER_DATA( "WorldPosition", shaderTarget );
		CodeChunk normal = SHADER_DATA("WorldNormal", shaderTarget);
		CodeChunk lightUsageMask;
		if ( HasInput( CNAME( CustomNo ) ) )
		{
			// lightUsageMask occupy bits from 1 to 32. Bit0 is reserved for light type (Point/Spot).  So flag "LUM_Custom0" corresponds to bit1
			lightUsageMask = Clamp (  Floor( CompileInput( compiler, CNAME( CustomNo ), shaderTarget ) ),  0.0f, 31.0f );
		}
		else
		{
			lightUsageMask = SHADER_VAR_FLOAT( 0.0f, shaderTarget );
		}
		
		CodeChunk screenPosition;
		switch( shaderTarget )
		{
		case MSH_PixelShader:
			screenPosition = PS_DATA("ScreenVPOS");
			break;
		case MSH_VertexShader:
			screenPosition = compiler.GetVS().Data("FatVertex.ScreenPosition");
			break;
		}

		Vector desaturationVector = RGB_LUMINANCE_WEIGHTS_MatBlockDesaturate;
		CodeChunk desaturationWeights = SHADER_VAR_FLOAT3( Float3( desaturationVector.X, desaturationVector.Y, desaturationVector.Z ), shaderTarget );

		CodeChunk lightMask = CodeChunk::Printf(true, "float %s = CalculateMaskedLighting( %s, %s, %s, %s, %s );",
			result.AsChar(),
			worldPosition.AsChar(),
			normal.AsChar(),
			screenPosition.AsChar(),
			lightUsageMask.AsChar(),
			desaturationWeights.AsChar()
			);

		compiler.GetShader(shaderTarget).Statement( lightMask );
		return SHADER_VAR_FLOAT4( Float4( result, result, result, 1.0f ), shaderTarget );
	}
	
	if ( m_valueType == MEVT_TransparencyParams )
	{
		return compiler.GetPS().ConstReg( MDT_Float4, "PSC_TransparencyParams" );
	}

	// Not compiled
	return PS_VAR_FLOAT4( 0.0f );
}

#endif

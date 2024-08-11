#include "common.fx"

////////////////////////////////////////////////////////////////////////////////////////////////
// Global constants available in the same form for all shader stages
////////////////////////////////////////////////////////////////////////////////////////////////

START_CB( GlobalShaderConsts, 0 )
	float4		PSC_TimeVector;
	float4		PSC_ViewportSize;
	float4		PSC_ViewportSubSize;
	float4		PSC_EnvTranspFilterParams;
	float4		PSC_EnvTranspFilterDistMinColor;
	float4		PSC_EnvTranspFilterDistMaxColor;
	float4		PSC_WeatherShadingParams;
	float4		PSC_WeatherAndPrescaleParams;
	float4		PSC_SkyboxShadingParams;
	float4		PSC_GlobalLightDir;
	float4		PSC_GlobalLightColorAndSpecScale;
	float4		PSC_WaterShadingParams;
	float4		PSC_WaterShadingParamsExtra;
	float4		PSC_WaterShadingParamsExtra2;
	float4		PSC_UnderWaterShadingParams;
	float4		PSC_GameplayEffectsRendererParameter;
END_CB

/*
The below #defines are an ugly hack to make the global constants unified across all shader stages BUT leaving a window for us to rollback to the previous state of things easily.
This means that after we live with those changes for some time, we will likely just rename all this shit to SC_TimeVector, SC_ViewportSize, etc., removing the shader stage letter from the prefix.
The same comment applies to the CameraShaderConsts further below.
*/
#define VSC_TimeVector								PSC_TimeVector
#define VSC_ViewportSize							PSC_ViewportSize
#define VSC_ViewportSubSize							PSC_ViewportSubSize
#define VSC_EnvTranspFilterParams					PSC_EnvTranspFilterParams
#define VSC_EnvTranspFilterDistMinColor				PSC_EnvTranspFilterDistMinColor
#define VSC_EnvTranspFilterDistMaxColor				PSC_EnvTranspFilterDistMaxColor
#define VSC_WeatherShadingParams					PSC_WeatherShadingParams
#define VSC_WeatherAndPrescaleParams				PSC_WeatherAndPrescaleParams
#define VSC_SkyboxShadingParams						PSC_SkyboxShadingParams
#define VSC_GlobalLightDir							PSC_GlobalLightDir
#define VSC_GlobalLightColorAndSpecScale			PSC_GlobalLightColorAndSpecScale
#define VSC_WaterShadingParams						PSC_WaterShadingParams
#define VSC_WaterShadingParamsExtra					PSC_WaterShadingParamsExtra
#define VSC_WaterShadingParamsExtra2				PSC_WaterShadingParamsExtra2
#define VSC_UnderWaterShadingParams					PSC_UnderWaterShadingParams
#define VSC_GameplayEffectsRendererParameter		PSC_GameplayEffectsRendererParameter

START_CB( CameraShaderConsts, 1 )
	float4x4	PSC_WorldToScreen;
	float4x4	PSC_WorldToView;
	float4		PSC_CameraPosition;
	float4		PSC_CameraVectorRight;
	float4		PSC_CameraVectorForward;
	float4		PSC_CameraVectorUp;
	float4		PSC_ViewportParams;
	float4		PSC_WetSurfaceEffect;
	float4		PSC_RevProjCameraInfo;
	float4		PSC_CameraNearFar;
END_CB

/*
See the previous comment for explanation of the hack below
*/
#define VSC_WorldToScreen					PSC_WorldToScreen
#define VSC_WorldToView						PSC_WorldToView
#define VSC_CameraPosition					PSC_CameraPosition
#define VSC_CameraVectorRight				PSC_CameraVectorRight
#define VSC_CameraVectorForward				PSC_CameraVectorForward
#define VSC_CameraVectorUp					PSC_CameraVectorUp
#define VSC_ViewportParams					PSC_ViewportParams
#define VSC_WetSurfaceEffect				PSC_WetSurfaceEffect
#define VSC_RevProjCameraInfo				PSC_RevProjCameraInfo
#define VSC_CameraNearFar					PSC_CameraNearFar
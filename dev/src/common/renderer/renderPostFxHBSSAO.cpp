#include "build.h"
#include "renderPostProcess.h"
#include "renderPostFx.h"
#include "gameplayFx.h"
#include "renderCollector.h"
#include "renderShaderPair.h"

#include "renderScene.h"
#include "renderProxyWater.h"
#include "renderInterface.h"
#include "renderPostFxHBSSAO.h"

#include "..\gpuApiUtils\gpuApiMemory.h"


CPostFXHorizonBasedSSAO::CPostFXHorizonBasedSSAO()
{
#ifdef USE_NVSSAO
	GFSDK_SSAO_CustomHeap CustomHeap;
	CustomHeap.new_ = ::operator new;
	CustomHeap.delete_ = ::operator delete;

	GFSDK_SSAO_Status status;

#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_WINPC )
	status = GFSDK_SSAO_CreateContext_D3D11( GpuApi::Hacks::GetDevice(), &m_AOContext, &CustomHeap );
#else
#error Unsupported platform
#endif
	RED_ASSERT(status == GFSDK_SSAO_OK, TXT("HBAO+ requires feature level 11_0 or above"));
#endif
}

Bool CPostFXHorizonBasedSSAO::Apply( 
	CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, 
	ERenderTargetName applyTarget, ERenderTargetName tempTarget, const CRenderFrameInfo& info )
{	
	const Bool useSSAONormals = info.m_worldRenderSettings.m_ssaoNormalsEnable;
	const Bool useSSAOBlur = info.m_worldRenderSettings.m_ssaoBlurEnable;

	// Build renderarea
	const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );

	// Env SSAO params
	const CEnvNVSSAOParametersAtPoint &envSSAOParams = info.m_envParametersArea.m_nvSsao;

	// 		
	RED_ASSERT( !GetRenderer()->IsMSAAEnabled( info ), TXT("MSAA not supported in HBAO+") );
	GpuApi::TextureRef texNormals	= surfaces->GetRenderTargetTex( RTN_GBuffer1 );
	GpuApi::TextureRef texDepth		= surfaces->GetDepthBufferTex();

#ifdef USE_NVSSAO

#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_WINPC )
	const Int32 texWidth = GpuApi::GetTextureLevelDesc( texDepth, 0 ).width;
	const Int32 texHeight = GpuApi::GetTextureLevelDesc( texDepth, 0 ).height;


	D3D11_VIEWPORT Viewport;
	Viewport.Width = (Float)texWidth;
	Viewport.Height =(Float)texHeight;
	Viewport.TopLeftX = 0.f;
	Viewport.TopLeftY = 0.f;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;

	const Matrix& projMat = info.m_camera.GetViewToScreenRevProjAware();
	const Matrix& viewMat = info.m_camera.GetWorldToView();

	GFSDK_SSAO_InputData_D3D11 Input;
	Input.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
	Input.DepthData.pFullResDepthTextureSRV = GpuApi::Hacks::GetTextureSRV( texDepth );
	Input.DepthData.pViewport = &Viewport;
	Input.DepthData.pProjectionMatrix = projMat.AsFloat();
	Input.DepthData.ProjectionMatrixLayout = GFSDK_SSAO_ROW_MAJOR_ORDER;
	Input.DepthData.MetersToViewSpaceUnits = 1.f;

	if( useSSAONormals )
	{
		Input.NormalData.Enable = true;
		Input.NormalData.DecodeBias = -1.f;
		Input.NormalData.DecodeScale = 2.f;
		Input.NormalData.WorldToViewMatrixLayout = GFSDK_SSAO_ROW_MAJOR_ORDER;
		Input.NormalData.pWorldToViewMatrix = viewMat.AsFloat();
		Input.NormalData.pFullResNormalTextureSRV = GpuApi::Hacks::GetTextureSRV( texNormals );
	}
	else
	{
		Input.NormalData.Enable = false;
	}

	GFSDK_SSAO_Parameters_D3D11 Params;
	Params.Radius = envSSAOParams.m_radius.GetScalar();
	Params.Bias = envSSAOParams.m_bias.GetScalar();
	Params.PowerExponent = envSSAOParams.m_powerExponent.GetScalar();
	Params.Blur.Enable = useSSAOBlur;
	Params.DetailAO = envSSAOParams.m_detailStrength.GetScalar();
	Params.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_4;
	Params.Blur.Sharpness = envSSAOParams.m_blurSharpness.GetScalar();
	Params.Output.BlendMode = GFSDK_SSAO_OVERWRITE_RGB;

	ID3D11DeviceContext* d3dcontext = GpuApi::Hacks::GetDeviceContext();
	ID3D11RenderTargetView* rtv = GpuApi::Hacks::GetTextureRTV( surfaces->GetRenderTargetTex( applyTarget ) );

	GFSDK_SSAO_Status status = m_AOContext->RenderAO( d3dcontext, &Input, &Params, rtv );
	RED_ASSERT( status == GFSDK_SSAO_OK, TXT("SSAO rendering problem, status: %d, ctx: %p, rtv: %p, depth: %p, normals: %p"), status, d3dcontext, rtv, Input.DepthData.pFullResDepthTextureSRV, Input.NormalData.pFullResNormalTextureSRV );
#else
#error Unsupported platform
#endif
	// SSAO was rendered
	return true;
#else
	// SSAO was NOT rendered
	return false;
#endif
}

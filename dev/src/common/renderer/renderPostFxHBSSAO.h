#ifndef renderPostFxHBSSAO_INCLUDED
#define renderPostFxHBSSAO_INCLUDED

#ifdef USE_NVSSAO

#if defined( RED_PLATFORM_WINPC )
#include "..\\..\\..\\external\\NvidiaSSAO\\GFSDK_SSAO_PC.h"
#elif defined( RED_PLATFORM_DURANGO )
#include "..\\..\\..\\external\\NvidiaSSAO\\GFSDK_SSAO_XboxOne.h"
#else
#error Unsupported platform
#endif

#define NVSSAO_LIB_PATH "..\\..\\..\\external\\NvidiaSSAO\\"


#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_WINPC )
#include "..\gpuApiDX10\gpuApiBase.h"
#elif defined( RED_PLATFORM_ORBIS )
#include "..\gpuApiGNM\gpuApiBase.h"
#else
#error Unsupported platform
#endif


#if defined( RED_PLATFORM_DURANGO )
#pragma comment (lib, NVSSAO_LIB_PATH "GFSDK_SSAO.XboxOne.lib")
#elif defined( RED_PLATFORM_WIN32 )
#pragma comment (lib, NVSSAO_LIB_PATH "GFSDK_SSAO.win32.lib")
#elif defined( RED_PLATFORM_WIN64 )
#pragma comment (lib, NVSSAO_LIB_PATH "GFSDK_SSAO.win64.lib")
#elif defined( RED_PLATFORM_ORBIS )
#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
#pragma comment (lib, NVSSAO_LIB_PATH "GFSDK_SSAO.ps4_lcue_stub_weak.a")
#else
#pragma comment (lib, NVSSAO_LIB_PATH "GFSDK_SSAO.ps4_cue2_stub_weak.a")
#endif
#else
#error Unsupported platform
#endif


#endif // USE_NVSSAO

class CPostFXHorizonBasedSSAO
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
protected:

	TDynArray<Float>	m_weights;
	Float				m_lastVariance;


#ifdef USE_NVSSAO

#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_WINPC )
	GFSDK_SSAO_Context_D3D11* m_AOContext;
#else
#error Unsupported platform
#endif
#endif

	// Simple gauss distribution :) 
	Float Gauss( Float i, Float variance )
	{
		return 1.0f / (sqrtf(2.0f * M_PI * variance * variance ) ) * expf( (-1.0f * i * i / (2.0f*variance * variance ) ) );
	}

	void CalculateWeights( Float variance )
	{
		m_weights.Resize( 7 );

		Float sum = 0.0f;

		for ( Uint32 i = 0; i < 7; ++i )
		{
			const Float weight = Gauss((Float)i, variance );

			m_weights[i] = weight;
			sum += weight;
		}

		for ( Uint32 i = 0; i < 7; ++i )
		{
			m_weights[i] /= sum;
		}
	}

public:
	static bool IsSSAOEnabled( const CEnvDisplaySettingsParams &displaySettings, const CAreaEnvironmentParamsAtPoint &areaParams )
	{
		// don't allow ssao in non main windows, because we are using temp textures which 
		// must be exactly the same size as the renderArea 
		// (nvidia hbao don't support partial texture usage, e.g. viewport smaller than renderTarget size)

		// ace_todo: determine if ssao would have visual impast based on ssao parameters
		return displaySettings.m_allowSSAO;
	}

public:


	CPostFXHorizonBasedSSAO();


	~CPostFXHorizonBasedSSAO()
	{
		OnLostDevice();
	}

	void OnLostDevice()
	{
#ifdef USE_NVSSAO
		m_AOContext->Release();
#endif
	}

	Bool Apply( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, 
		ERenderTargetName applyTarget, ERenderTargetName tempTarget, const CRenderFrameInfo& info );
};
#endif
/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"

#include "../redMath/numericalutils.h"
#include "../redMath/mathfunctions_fpu.h"
#include "../redMath/float16compressor.h"

#include "../redMath/random/random.h"
#include "../redMath/random/standardRand.h"

#if MICROSOFT_ATG_DYNAMIC_SCALING	
//#define AC_DEBUG_DYNAMIC_SCALING
#endif

#ifdef AC_DEBUG_DYNAMIC_SCALING
int g_DSRTestCustomDynamicScaleIndex = 0;
bool g_DSRTestEnableTestDynamicScaleIndex = false;
bool g_DSRTestPerFrameChangeDynamicScaleIndex = false;
#endif

#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
#include "../../../external/BC6HBC7 DirectCompute Encoder Tool/C++/utils.h"
#include "../../../external/BC6HBC7 DirectCompute Encoder Tool/C++/EncoderBase.h"
#include "../../../external/BC6HBC7 DirectCompute Encoder Tool/C++/BC6HEncoderCS10.h"
#include "../../../external/BC6HBC7 DirectCompute Encoder Tool/C++/BC7EncoderCS10.h"
#endif

#ifdef RED_PLATFORM_CONSOLE
#	include "gpuApiDDSLoader.h"
#endif

#define	GPUAPI_BLANK2D_TEXTURE_SIZE				4
#define	GPUAPI_DEFAULT2D_TEXTURE_SIZE			16
#define	GPUAPI_DEFAULTCUBE_TEXTURE_SIZE			16
#define	GPUAPI_DISSOLVE_TEXTURE_SIZE			16
#define	GPUAPI_POISSON_ROTATION_TEXTURE_SIZE	32
#define GPUAPI_SSAO_ROTATION_TEXTURE_SIZE		4
#define GPUAPI_MIP_NOISE_TEXTURE_SIZE			64

namespace GpuApi
{
#if MICROSOFT_ATG_DYNAMIC_SCALING	

	Uint32 DynamicScalingUsageToAlignment( Uint32 usage )
	{
		Uint32 alignment = 1;
		if(usage & TEXUSAGE_DynamicScalingAlign1)				alignment = 1;
		else if(usage & TEXUSAGE_DynamicScalingAlign2)			alignment = 2;
		else if(usage & TEXUSAGE_DynamicScalingAlign4)			alignment = 4;
		else if(usage & TEXUSAGE_DynamicScalingAlignSSAO1)		alignment = 11;
		else if(usage & TEXUSAGE_DynamicScalingAlignSSAO2)		alignment = 12;
		else if(usage & TEXUSAGE_DynamicScalingAlignSSAO4)		alignment = 13;
		else if(usage & TEXUSAGE_DynamicScalingAlignSSAO8)		alignment = 14;
		else if(usage & TEXUSAGE_DynamicScalingAlignSSAO16)		alignment = 15;
		else if(usage & TEXUSAGE_DynamicScalingAlignSSAO32)		alignment = 16;
		else if(usage & TEXUSAGE_DynamicScalingAlignSSAO64)		alignment = 17;
		else { RED_FATAL( "" ); }

		return alignment;
	}

	Uint32 DynamicScalingAlignmentToUsage( Uint32 alignment )
	{
		Uint32 usage = 0;
		switch( alignment )
		{
		case 1: usage = TEXUSAGE_DynamicScalingAlign1; break;
		case 2: usage = TEXUSAGE_DynamicScalingAlign2; break;
		case 4: usage = TEXUSAGE_DynamicScalingAlign4; break;
		case 11: usage = TEXUSAGE_DynamicScalingAlignSSAO1; break;
		case 12: usage = TEXUSAGE_DynamicScalingAlignSSAO2; break;
		case 13: usage = TEXUSAGE_DynamicScalingAlignSSAO4; break;
		case 14: usage = TEXUSAGE_DynamicScalingAlignSSAO8; break;
		case 15: usage = TEXUSAGE_DynamicScalingAlignSSAO16; break;
		case 16: usage = TEXUSAGE_DynamicScalingAlignSSAO32; break;
		case 17: usage = TEXUSAGE_DynamicScalingAlignSSAO64; break;
		default: RED_FATAL( "Not handled" );
		}

		RED_FATAL_ASSERT( DynamicScalingUsageToAlignment(usage) == alignment, "" );
		return usage;
	}	

	static TextureDesc BuildDynamicScalingTestDesc( Uint32 width, Uint32 height, Uint32 alignment )
	{
		// Fill with params used with helper functions
		TextureDesc desc;
		desc.width = width;
		desc.height = height;
		desc.usage = DynamicScalingAlignmentToUsage( alignment );
		return desc;
	}

	static TextureDesc BuildTextureDescTrue1080( const TextureDesc &desc )
	{
		TextureDesc descTrue1080 = desc;
		descTrue1080.width = Uint32(floorf(float(desc.width) * (1920.0f / 1600.0f)) + 0.001f);	
		descTrue1080.height = Uint32(floorf(float(desc.height) * (1080.0f / 900.0f)) + 0.001f);

		descTrue1080.width = (descTrue1080.width + 3) & ~3;
		descTrue1080.height = (descTrue1080.height + 3) & ~3;

		descTrue1080.width = descTrue1080.width < 1920 ? descTrue1080.width : 1920;
		descTrue1080.height = descTrue1080.height < 1080 ? descTrue1080.height : 1080;

		return descTrue1080;
	}		
	
	static void GetDynamicScalingParams( const TextureDesc &desc, const TextureDesc &descTrue1080, Uint32 &outMinSizeW, Uint32 &outMinSizeH, Float &outDeltaSizeW, Float &outDeltaSizeH, Uint32 &outAlignment )
	{
		RED_FATAL_ASSERT( descTrue1080.width >= desc.width, "" );
		RED_FATAL_ASSERT( descTrue1080.height >= desc.height, "" );
		RED_FATAL_ASSERT( descTrue1080.usage == descTrue1080.usage, "" );

		const Uint32 alignment = DynamicScalingUsageToAlignment( desc.usage );
		const Uint32 _width = Uint32(floorf(float(desc.width) * (1920.0f / 1600.0f)) + 0.001f);	
		const Uint32 _height = Uint32(floorf(float(desc.height) * (1080.0f / 900.0f)) + 0.001f);

		outDeltaSizeW = float(_width - desc.width);
		outDeltaSizeH = float(_height - desc.height);
		outMinSizeW = desc.width;
		outMinSizeH = desc.height;
		outAlignment = alignment;
	}

	static Uint32 GetDynamicTargetSize(Uint32 index, Uint32 min, Float delta, Uint32 alignment, Bool isHeight)
	{
		RED_FATAL_ASSERT( alignment > 0, "" );

		Uint32 size = 0;

		if ( alignment <= 10 )
		{
			Uint32 inverseAlignment = 1;
			switch ( alignment )
			{
			case 1:		inverseAlignment = 4; break;
			case 2:		inverseAlignment = 2; break;
			case 4:		inverseAlignment = 1; break;
			default:	RED_FATAL( "" );
			}

			Float lerp = Float(index) / Float(DYANMIC_SCALING_NUM_TARGETS - 1);
			Float value = lerp * delta * inverseAlignment;
			Uint32 value2 = Uint32(value + 0.001f) & ~3;
			size = min + value2 / inverseAlignment;
			RED_FATAL_ASSERT( 0 == (min % alignment), "" );
			RED_FATAL_ASSERT( 0 == (size % alignment), "" );
		}
		else
		{
			const TextureDesc refDescMin = BuildDynamicScalingTestDesc( 1600, 900, 4 );
			const TextureDesc refDescFull = BuildTextureDescTrue1080( refDescMin );

			Uint32 refMinSizeW, refMinSizeH;
			Float refDeltaW, refDeltaH;
			Uint32 refAlign;
			GetDynamicScalingParams( refDescMin, refDescFull, refMinSizeW, refMinSizeH, refDeltaW, refDeltaH, refAlign );

			const Uint32 refSize = GetDynamicTargetSize( index, isHeight?refMinSizeH:refMinSizeW, isHeight?refDeltaH:refDeltaW, refAlign, isHeight );

			RED_FATAL_ASSERT( alignment >= 11, "" );
			const Uint32 snapping = 1 << (alignment - 11);

			// some stuff for debug to check if we're ok (final size should be more or less _value)
			//Float _lerp = Float(index) / Float(DYANMIC_SCALING_NUM_TARGETS - 1);
			//Float _value = min + _lerp * delta;

			size = (refSize + (snapping-1)) / snapping;
		}

		RED_FATAL_ASSERT( size > 0, "" );
		return size;
	}

	Uint32 g_DynamicScaleIndex = 0;				// must start off zero for textures to initialize properly
	Uint32 g_LastDynamicScaleIndex = 0;
	Uint32 g_DynamicScaleWidthFullRes = 1600;	// TODO, remove hard coding
	Uint32 g_DynamicScaleHeightFullRes = 900;	// TODO, remove hard coding
	static SDynamicScalingTextureData *g_DynamicScalingTextures = nullptr;

	static const Float g_fFrameTimeInMs = 1000.0f / 30.0f;             
	static const Int64 g_lGpuTimerFrequency = 100000000;
	static const Uint32 g_iSyncInterval = 1;
	
	void SetDynamicScaling(Uint32 index)
	{
		if(index >= DYANMIC_SCALING_NUM_TARGETS)
			return;

		g_LastDynamicScaleIndex		= g_DynamicScaleIndex;

		g_DynamicScaleIndex			= index;
		g_DynamicScaleWidthFullRes	= GetDynamicTargetSize(index, 1600, 1920.0f - 1600.0f, 4, false);		// TODO, remove hard coding
		g_DynamicScaleHeightFullRes = GetDynamicTargetSize(index, 900, 1080.0f - 900.0f, 4, true); 		// TODO, remove hard coding

		GPUAPI_LOG_WARNING(TXT("Resolution %ux%u\n"), g_DynamicScaleWidthFullRes, g_DynamicScaleHeightFullRes);

		SDynamicScalingTextureData *itr = g_DynamicScalingTextures;

		while(itr)
		{
			itr->UpdateResolution();

			itr = itr->next;
		}
	}

	void RevertToPreviousDynamicScale() 
	{
		SetDynamicScaling( g_LastDynamicScaleIndex );

		g_LastDynamicScaleIndex		= g_DynamicScaleIndex;
	}

//#define DYNAMIC_SCALING_TEST

	Bool UpdateDynamicScaling( const bool movingCamera )
	{
		g_LastDynamicScaleIndex		= g_DynamicScaleIndex;

#ifdef DYNAMIC_SCALING_TEST
		static int waitCount = 0;
		static int frameAdvice = -1;

		if(waitCount != frameAdvice)
		{
			++waitCount;
			return FALSE;
		}
		waitCount = 0;

		++g_DynamicScaleIndex;

		if(g_DynamicScaleIndex == DYANMIC_SCALING_NUM_TARGETS)
			g_DynamicScaleIndex = 0;

		SetDynamicScaling(g_DynamicScaleIndex);

		return TRUE;
#else
		LARGE_INTEGER               m_lFrequency;
		QueryPerformanceFrequency( &m_lFrequency );

		Float fMarginDropResolutionInMs = 2.0f;
		Float fMarginIncreaseResolutionInMs = 3.0f;
		Float fMargiRaiseResolutionWaitInMs = 33.0f * 4.0f;	

		// Set frame dimensions to maintain frame rate
		// This is part of the dynamic resolution implementation
		static Uint32 iFrameCount = 0;
		++iFrameCount;
		DXGIX_FRAME_STATISTICS FrameStatistics[ 3 ];
		if( iFrameCount >  _countof( FrameStatistics ) )
		{
			// We also retrieve these in RecordFrameStatistics. But that copy is just for tracking purposes,
			// and isn't part of the algorithm. The copy we retrieve here is for determining resolution, and it
			// is part of the algorithm.
			DXGIXGetFrameStatistics( _countof( FrameStatistics ), FrameStatistics );

			Uint32 iIndexMostRecentCompletedFrame = 0;
			for( ; iIndexMostRecentCompletedFrame < _countof( FrameStatistics ); ++iIndexMostRecentCompletedFrame )
			{
				if( FrameStatistics[ iIndexMostRecentCompletedFrame ].GPUTimeFlip > 0 ) 
				{
					break;
				}
			}
			GPUAPI_ASSERT( iIndexMostRecentCompletedFrame + 1 < _countof( FrameStatistics ) );
			INT64 iGpuTimeFrameComplete = FrameStatistics[ iIndexMostRecentCompletedFrame ].GPUTimeFrameComplete;	
			FLOAT fGpuFrameIncrementInMs = 1000.0f * (FLOAT) ( ( FrameStatistics[ iIndexMostRecentCompletedFrame ].GPUCountTitleUsed + FrameStatistics[ iIndexMostRecentCompletedFrame ].GPUCountSystemUsed ) / (DOUBLE) g_lGpuTimerFrequency );

			// Was the frame load due to GPU or CPU time?
			INT64 iCpuTimeFrameComplete = FrameStatistics[ iIndexMostRecentCompletedFrame ].CPUTimeFrameComplete;
			INT64 iCpuTimePresentCalled = FrameStatistics[ iIndexMostRecentCompletedFrame ].CPUTimePresentCalled;
			FLOAT fTimeCpuPresentCalledBeforeGpuFrameCompleteInMs = 1000.0f * (FLOAT) ( ( iCpuTimeFrameComplete - iCpuTimePresentCalled ) / (DOUBLE) m_lFrequency.QuadPart );

			// What is the max amount by which CPU runs ahead if we are CPU bound?
			// Overestimate is okay --- there are generally two cases:
			//  1: CPU bound: CPU will run ahead by 0-N ms, where N is the biggest "bubble" of GPU time
			//  2: GPU bound: CPU will run ahead by 1-2 frames (for swap chain count of 2)
			static FLOAT g_fCpuToGpuLatencyThresholdInMs = 10.0f;

			// If we are dropping frames, we are CPU bound --- resolution doesn't matter
			BOOL bCpuBoundIfDroppedFrame = ( fTimeCpuPresentCalledBeforeGpuFrameCompleteInMs <= g_fCpuToGpuLatencyThresholdInMs ) && movingCamera;
			BOOL bConsiderDropResolution = ( fGpuFrameIncrementInMs > (g_fFrameTimeInMs - fMarginDropResolutionInMs ) ) && ( !bCpuBoundIfDroppedFrame );
			const float margin = ( movingCamera ) ? fMarginIncreaseResolutionInMs : fMarginIncreaseResolutionInMs * 3.0f;
			BOOL bConsiderIncreaseResolution = ( fGpuFrameIncrementInMs < (g_fFrameTimeInMs - margin ) ) || bCpuBoundIfDroppedFrame;
			Uint32 dynamicScaleIndex = g_DynamicScaleIndex;

			static FLOAT fRaiseResolutionWaitTimeInMs = 0.0f;

			#ifdef AC_DEBUG_DYNAMIC_SCALING
			if ( g_DSRTestEnableTestDynamicScaleIndex )
			{
				Int32 requestedDynamicScaleIndex = g_DynamicScaleIndex;
				{
					if ( g_DSRTestPerFrameChangeDynamicScaleIndex )
					{
						requestedDynamicScaleIndex = (g_DynamicScaleIndex + 1) % DYANMIC_SCALING_NUM_TARGETS;
					}
					else
					{
						requestedDynamicScaleIndex = g_DSRTestCustomDynamicScaleIndex; 
					}
				}

				if ( requestedDynamicScaleIndex < 0 ) 
					requestedDynamicScaleIndex = 0;
				if ( requestedDynamicScaleIndex > DYANMIC_SCALING_NUM_TARGETS - 1 )
					requestedDynamicScaleIndex = DYANMIC_SCALING_NUM_TARGETS - 1;

				if ( requestedDynamicScaleIndex != g_DynamicScaleIndex )
				{
					dynamicScaleIndex = requestedDynamicScaleIndex;
					 
					SetDynamicScaling( dynamicScaleIndex );    

					return TRUE;
				}
			}
			else
			#endif
			{
				// We are GPU bound --- go to lower resolution
				if( bConsiderDropResolution )
				{
					if( dynamicScaleIndex )
					{
						--dynamicScaleIndex;

						// Frame time blown? Panic and drop extra steps
						for(float i = 1.0f; dynamicScaleIndex && ( fGpuFrameIncrementInMs > ( g_fFrameTimeInMs * i ) ); i *= 1.5f) 
						{
							--dynamicScaleIndex;
						}					
						SetDynamicScaling( dynamicScaleIndex );

						fRaiseResolutionWaitTimeInMs = 0.0f;

						return TRUE;
					}
				}
				// We have GPU time to spare --- go to higher resolution
				else if( bConsiderIncreaseResolution )
				{
					// Avoid oscillating back and forth between resolutions				
					fRaiseResolutionWaitTimeInMs += g_fFrameTimeInMs;
				
					if( fRaiseResolutionWaitTimeInMs >= fMargiRaiseResolutionWaitInMs )
					{
						if( dynamicScaleIndex < (DYANMIC_SCALING_NUM_TARGETS - 1) )
						{
							++dynamicScaleIndex;

							// seriously under time? go for extra steps
							for(float i = 1.5f; ( fGpuFrameIncrementInMs < (g_fFrameTimeInMs - ( i * margin ) ) ) && ( dynamicScaleIndex < (DYANMIC_SCALING_NUM_TARGETS - 1) ); i *= 1.5f) 
							{
								++dynamicScaleIndex;
							}
							SetDynamicScaling( dynamicScaleIndex );

							fRaiseResolutionWaitTimeInMs = 0.0f;

							return TRUE;
						}
					}
				}
			}
		}
		return FALSE;
#endif
	}
	
	void SDynamicScalingTextureData::Remove()
	{
		if(g_DynamicScalingTextures == this)
			g_DynamicScalingTextures = next;
		
		if(next)
			next->prev = prev;
		
		if(prev)
			prev->next = next;
	}

	void SDynamicScalingTextureData::Add(Uint32 minSizeW, Uint32 minSizeH, float deltaSizeW, float deltaSizeH, Uint32 alignment)
	{
		next = g_DynamicScalingTextures;
		prev = nullptr;

		if(g_DynamicScalingTextures)
			g_DynamicScalingTextures->prev = this;
		
		g_DynamicScalingTextures = this;

		for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
		{
			m_pTexture[i] = nullptr;
			m_pRenderTargetView[i] = nullptr;
			m_pShaderResourceView[i] = nullptr;
			m_pDepthStencilView[i] = nullptr;
			m_pDepthStencilViewReadOnly[i] = nullptr;
			m_pStencilShaderResourceView[i] = nullptr;
			m_pUnorderedAccessView[i] = nullptr;

			for(Uint32 j=0; j<DYNAMIC_SCALING_MAX_MIPS; ++j)
			{
				m_pp2DRenderTargetViewPerMipLevel[i][j] = nullptr;
				m_ppTex2DShaderResourceViewPerMipLevel[i][j] = nullptr;
			}
			for(Uint32 j=0; j<DYNAMIC_SCALING_MAXSLICES; ++j)
			{
				m_pRenderTargetViewsArray[i][j] = nullptr;
				m_pDepthStencilViewsArray[i][j] = nullptr;
			}
		}
		m_minSizeW			= minSizeW;
		m_minSizeH			= minSizeH;
		m_deltaSizeW		= deltaSizeW;
		m_deltaSizeH		= deltaSizeH;
		m_alignmentMinus1	= alignment - 1;
	}
	 
	void SDynamicScalingTextureData::UpdateResolution()
	{
		m_pTextureData->m_Desc.width					= GetDynamicTargetSize(g_DynamicScaleIndex, m_minSizeW, m_deltaSizeW, m_alignmentMinus1 + 1, false);
		m_pTextureData->m_Desc.height					= GetDynamicTargetSize(g_DynamicScaleIndex, m_minSizeH, m_deltaSizeH, m_alignmentMinus1 + 1, true);

		m_pTextureData->m_pTexture						= m_pTexture[g_DynamicScaleIndex];
		m_pTextureData->m_pRenderTargetView				= m_pRenderTargetView[g_DynamicScaleIndex];
		m_pTextureData->m_pShaderResourceView			= m_pShaderResourceView[g_DynamicScaleIndex];
		m_pTextureData->m_pDepthStencilView				= m_pDepthStencilView[g_DynamicScaleIndex];
		m_pTextureData->m_pDepthStencilViewReadOnly		= m_pDepthStencilViewReadOnly[g_DynamicScaleIndex];
		m_pTextureData->m_pStencilShaderResourceView	= m_pStencilShaderResourceView[g_DynamicScaleIndex];
		m_pTextureData->m_pUnorderedAccessView			= m_pUnorderedAccessView[g_DynamicScaleIndex];		

		for(Uint32 j=0; j<DYNAMIC_SCALING_MAX_MIPS; ++j)
		{
			if(m_pTextureData->m_pp2DRenderTargetViewPerMipLevel && m_pp2DRenderTargetViewPerMipLevel[g_DynamicScaleIndex][j])
			{
				m_pTextureData->m_pp2DRenderTargetViewPerMipLevel[j] = m_pp2DRenderTargetViewPerMipLevel[g_DynamicScaleIndex][j];
			}
			if(m_pTextureData->m_ppTex2DShaderResourceViewPerMipLevel && m_ppTex2DShaderResourceViewPerMipLevel[g_DynamicScaleIndex][j])
			{
				m_pTextureData->m_ppTex2DShaderResourceViewPerMipLevel[j] = m_ppTex2DShaderResourceViewPerMipLevel[g_DynamicScaleIndex][j];
			}
		}
		for(Uint32 j=0; j<DYNAMIC_SCALING_MAXSLICES; ++j)
		{
			if(m_pTextureData->m_pRenderTargetViewsArray && m_pRenderTargetViewsArray[g_DynamicScaleIndex][j])
			{
				m_pTextureData->m_pRenderTargetViewsArray[j] = m_pRenderTargetViewsArray[g_DynamicScaleIndex][j];
			}
			if(m_pTextureData->m_pDepthStencilViewsArray && m_pDepthStencilViewsArray[g_DynamicScaleIndex][j])
			{
				m_pTextureData->m_pDepthStencilViewsArray[j] = m_pDepthStencilViewsArray[g_DynamicScaleIndex][j];
			}
		}
	}
#endif

	namespace Utils
	{
		//////////////////////////////////////////////////////////////////////////
		// Random Number interface for GPUApiTexture
		Red::Math::Random::Generator< Red::Math::Random::StandardRand > GRandomNumberGenerator;


		void FillD3DTextureDesc( const TextureDesc& desc, D3D11_TEXTURE2D_DESC& outD3dDesc )
		{
			outD3dDesc.Usage = D3D11_USAGE_DEFAULT;
			outD3dDesc.Width = desc.width;
			outD3dDesc.Height = desc.height;
			outD3dDesc.MipLevels = desc.initLevels;
			outD3dDesc.ArraySize = 1;
			outD3dDesc.Format = Map(desc.format);
			outD3dDesc.SampleDesc.Count = desc.msaaLevel > 0 ? desc.msaaLevel : 1;
			outD3dDesc.SampleDesc.Quality = 0;
			outD3dDesc.BindFlags = 0;
#ifdef RED_PLATFORM_DURANGO
			outD3dDesc.ESRAMOffsetBytes = 0;
			outD3dDesc.ESRAMUsageBytes = 0;
#endif
			if (desc.usage & TEXUSAGE_Samplable)
			{
				outD3dDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			}

			if (desc.usage & TEXUSAGE_RenderTarget)
			{
				if ( desc.msaaLevel < 1 )
				{
					outD3dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
				}
				else
				{
					outD3dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
				}
			}

			if (desc.usage & TEXUSAGE_DepthStencil)
			{
				outD3dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
			}

			outD3dDesc.CPUAccessFlags = 0;
			outD3dDesc.MiscFlags = 0;

			if (desc.usage & TEXUSAGE_Dynamic)
			{
				outD3dDesc.Usage = D3D11_USAGE_DYNAMIC;
				outD3dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			}
			else if ( desc.usage & TEXUSAGE_Staging )
			{
				outD3dDesc.Usage = D3D11_USAGE_STAGING;
				outD3dDesc.CPUAccessFlags = ( ( desc.usage & TEXUSAGE_StagingWrite ) == TEXUSAGE_StagingWrite ) ? D3D11_CPU_ACCESS_WRITE : D3D11_CPU_ACCESS_READ;
			}
			else if ( desc.usage & TEXUSAGE_Immutable )
			{
				outD3dDesc.Usage = D3D11_USAGE_IMMUTABLE;
			}

			if ( TEXTYPE_CUBE == desc.type )
			{
				outD3dDesc.ArraySize = 6 * desc.sliceNum;
				outD3dDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
			}

			if ( TEXTYPE_ARRAY == desc.type )
			{
				outD3dDesc.ArraySize = desc.sliceNum;
				outD3dDesc.MiscFlags = 0;
			}

			if ( (desc.usage & TEXUSAGE_GenMip) && (desc.usage & TEXUSAGE_RenderTarget) )
			{
				outD3dDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
			}

#ifdef RED_PLATFORM_DURANGO
			if ( (desc.usage & TEXUSAGE_ESRAMResident) && desc.esramOffset >= 0 )
			{
				outD3dDesc.MiscFlags |= D3D11X_RESOURCE_MISC_ESRAM_RESIDENT;
				outD3dDesc.ESRAMOffsetBytes = static_cast<Uint32>(desc.esramOffset);
				outD3dDesc.ESRAMUsageBytes = 0; //desc.esramSize;
			}

			if (desc.usage & TEXUSAGE_NoDepthCompression )
			{
				outD3dDesc.MiscFlags |= D3D11X_RESOURCE_MISC_NO_DEPTH_COMPRESSION;
			}
#endif
		}


#ifdef RED_PLATFORM_DURANGO
		void CopyD3DTextureDescToXG( const D3D11_TEXTURE2D_DESC& d3dDesc, XG_TEXTURE2D_DESC& outXgDesc )
		{
			outXgDesc.Width					= d3dDesc.Width;
			outXgDesc.Height				= d3dDesc.Height;
			outXgDesc.MipLevels				= d3dDesc.MipLevels;
			outXgDesc.ArraySize				= d3dDesc.ArraySize;
			outXgDesc.Format				= (XG_FORMAT)d3dDesc.Format;
			outXgDesc.SampleDesc.Count		= d3dDesc.SampleDesc.Count;
			outXgDesc.SampleDesc.Quality	= d3dDesc.SampleDesc.Quality;
			outXgDesc.Usage					= (XG_USAGE)d3dDesc.Usage;
			outXgDesc.BindFlags				= d3dDesc.BindFlags;
			outXgDesc.CPUAccessFlags		= d3dDesc.CPUAccessFlags;
			outXgDesc.MiscFlags				= d3dDesc.MiscFlags;
			outXgDesc.Pitch = 0;

			// Staging textures cannot be tiled.
			if ( d3dDesc.Usage == D3D11_USAGE_STAGING )
			{
				outXgDesc.TileMode = XG_TILE_MODE_LINEAR;
			}
			else
			{
				// Always use 1D tiling for resource textures. When we get optimal from XG, it can change depending on mips and size,
				// which causes problems if we need to stream in a subset of the mips.
				outXgDesc.TileMode = XG_TILE_MODE_1D_THIN;
			}

			if ( outXgDesc.TileMode == XG_TILE_MODE_INVALID )
			{
				GPUAPI_LOG_WARNING( TXT("Cannot compute the tile mode for the texture. Defaulting to linear.") );
				outXgDesc.TileMode = XG_TILE_MODE_LINEAR;
			}
		}
#endif


		// !Note. If a texture is in-place loaded, outTextureMemory will contain the memory region HANDLE, not the raw pointer
#ifdef RED_PLATFORM_DURANGO
		void TextureFactoryD3D( const TextureDesc &desc, const TextureInitData* initData, ID3D11Resource*& outTexture, void*& outTextureMemory, Red::MemoryFramework::MemoryRegionHandle& outInplaceRegion 
# if MICROSOFT_ATG_DYNAMIC_SCALING
			, SDynamicScalingTextureData *&pDynamicScalingData
			, bool &partialResidencyAllocation
# endif
			
			)
#else
		void TextureFactoryD3D( const TextureDesc &desc, const TextureInitData* initData, ID3D11Resource*& outTexture )
#endif
		{
			//////////////////////////////////////////////////////////////////////////
			// NOTE
			// If changes are made to how textures are created on Xbox here, such as tiling mode, particularly for resource textures
			// (immutable, cookable), then matching changes should be made in /dev/internal/TexCookTools/TexCookXboxOne.cpp.
			//////////////////////////////////////////////////////////////////////////

			GPUAPI_ASSERT( IsDescSupported( desc ), TXT("TextureDesc not supported! This should have been checked before getting here!") );

			GPUAPI_ASSERT( outTexture == nullptr, TXT("Caller passed in a non-null ID3D11Resource! Things might be leaking!") );
			outTexture = nullptr;

#ifdef RED_PLATFORM_DURANGO
			GPUAPI_ASSERT( outTextureMemory == nullptr, TXT("Caller passed in non-null texture memory! Things might be leaking!") );
			outTextureMemory = nullptr;
# if MICROSOFT_ATG_DYNAMIC_SCALING
			partialResidencyAllocation = FALSE;
			pDynamicScalingData = nullptr;
# endif
#endif
			// Creating an immutable texture requires initial data.
			if ( ( desc.usage & TEXUSAGE_Immutable ) != 0 )
			{
				if ( initData == nullptr )
				{
					GPUAPI_HALT( "Creating immutable texture without any initial data." );
					return;
				}
			}

			// Creating in-place requires a full cooked buffer.
			if ( desc.IsInPlace() )
			{
				if ( initData == nullptr || !initData->m_isCooked || !initData->m_cookedData.IsValid() )
				{
					GPUAPI_HALT( "Creating in-place texture without a cooked init buffer." );
					return;
				}
			}

			if ( initData != nullptr && initData->m_isCooked )
			{
				if ( ( desc.usage & TEXUSAGE_Immutable ) == 0 )
				{
					GPUAPI_HALT( "Trying to create a non-immutable texture from cooked init data." );
					return;
				}
			}


			D3D11_TEXTURE2D_DESC dxDesc;
			FillD3DTextureDesc( desc, dxDesc );

			ID3D11Texture2D* texture2d = nullptr;

#ifdef RED_PLATFORM_DURANGO
			void* texMemory = nullptr;

			// TODO : We should be able to use placement create (and manage our own memory) for anything except ESRAM textures.
			// As-is, though, there were some problems when it came to changeable textures, and I ended up with a black screen.
			// My guess is that it's something to do with cache flushes or something...
			//
			// NOTE : If adding support for dynamic textures, see note in Release(), about IsResourcePending()!
			if ( desc.usage & TEXUSAGE_Immutable )
			{
				XG_TEXTURE2D_DESC xgTexDesc;
				CopyD3DTextureDescToXG( dxDesc, xgTexDesc );

				if ( desc.IsInPlace() )
				{
					// Since setting placement texture resource views on XB1 takes an offset from the base data address,
					// we have to set the base address to zero if we wish to change the address to arbitrary location during runtime
					//texMemory = const_cast< void* >( initData->m_cookedData.GetRawPtr() );
				}
				else
				{
					XGTextureAddressComputer* addressComputer = nullptr;
					if ( FAILED( XGCreateTexture2DComputer( &xgTexDesc, &addressComputer ) ) )
					{
						GPUAPI_HALT( "Failed to create texture address computer" );
						return;
					}

					XG_RESOURCE_LAYOUT layout;
					if( FAILED( addressComputer->GetResourceLayout( &layout ) ) )
					{
						GPUAPI_HALT( "Failed to calculate texture resource layout" );
						SAFE_RELEASE( addressComputer );
						return;
					}

					texMemory = GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, layout.SizeBytes, layout.BaseAlignmentBytes );
					if ( texMemory == nullptr )
					{
						GPUAPI_HALT( "Failed to allocate texture memory. OOM?" );
						SAFE_RELEASE( addressComputer );
						return;
					}

					if ( initData != nullptr )
					{
						// If it's fully cooked, we can just copy the data across.
						if ( initData->m_isCooked )
						{
							Red::System::MemoryCopy( texMemory, initData->m_cookedData.GetRawPtr(), layout.SizeBytes );
						}
						// Otherwise, need to copy individual pieces, dealing with possible mix of cooked/uncooked.
						else
						{
							for ( Uint32 slice = 0; slice < dxDesc.ArraySize; ++slice )
							{
								for ( Uint32 mip = 0; mip < dxDesc.MipLevels; ++mip )
								{
									const Uint32 dataIndex = mip + slice * dxDesc.MipLevels;

									if ( initData->m_mipsInitData[dataIndex].m_isCooked )
									{
										Uint64 mipOffset = layout.Plane[0].MipLayout[mip].OffsetBytes;
										Uint64 sliceSize = layout.Plane[0].MipLayout[mip].Slice2DSizeBytes;
										Uint64 sliceOffset = sliceSize * slice;
										Red::System::MemoryCopy( (Uint8*)texMemory + mipOffset + sliceOffset, initData->m_mipsInitData[dataIndex].m_data, sliceSize );
									}
									else
									{
										const Uint32 subresourceIndex = D3D11CalcSubresource( mip, slice, dxDesc.MipLevels );
										const Uint32 mipWidth	= CalculateTextureMipDimension( desc.width, mip, desc.format );
										const Uint32 mipHeight	= CalculateTextureMipDimension( desc.height, mip, desc.format );
										Uint32 pitch = CalculateTexturePitch( mipWidth, desc.format );
										Uint32 size = CalculateTextureSize( mipWidth, mipHeight, desc.format );
										addressComputer->CopyIntoSubresource( texMemory, 0, subresourceIndex, initData->m_mipsInitData[dataIndex].m_data, pitch, size );
									}
								}
							}
						}
					}

					SAFE_RELEASE( addressComputer );
				}
# if MICROSOFT_ATG_DYNAMIC_SCALING
				GPUAPI_ASSERT( !(dxDesc.MiscFlags & D3D11X_RESOURCE_MISC_ESRAM_RESIDENT), TXT("Placement not supported for ESRAM resources") );
# endif
				ID3D11DeviceX* deviceX = (ID3D11DeviceX*)GetDevice();
				deviceX->CreatePlacementTexture2D( &dxDesc, xgTexDesc.TileMode, 0, texMemory, &texture2d );
			}
			else
#endif
			{
				// TexArray with 64 slices at 2048x2048? probably enough.
				const Uint32 MAX_SUBRESOURCES = 64 * 11;
				D3D11_SUBRESOURCE_DATA subresourceData[MAX_SUBRESOURCES];
				D3D11_SUBRESOURCE_DATA* subresourceDataPtr = nullptr;

				if ( initData != nullptr )
				{
					if ( dxDesc.MipLevels * dxDesc.ArraySize > MAX_SUBRESOURCES )
					{
						GPUAPI_HALT( "Texture has too many subresources: %d mips, %d slices (max: %d)", dxDesc.MipLevels, dxDesc.ArraySize, MAX_SUBRESOURCES );
						return;
					}

#ifdef RED_PLATFORM_WINPC
					// On Xbox, cooked data will have gone through the previous section (since it must be immutable).
					if ( initData->m_isCooked )
					{
						const Uint8* dataBytePtr = static_cast< const Uint8* >( initData->m_cookedData.GetRawPtr() );

						// NOTE : Mip/Slice loop order is reverse of below. This is because the cooked data is laid out differently than
						// the order DX expects the subresource data to be.
						for ( Uint32 mip = 0; mip < dxDesc.MipLevels; ++mip )
						{
							for ( Uint32 slice = 0; slice < dxDesc.ArraySize; ++slice )
							{
								const Uint32 subresourceIndex = D3D11CalcSubresource( mip, slice, dxDesc.MipLevels );
								const Uint32 dataIndex = mip + slice * dxDesc.MipLevels;
								const Uint32 mipWidth	= CalculateTextureMipDimension( desc.width, mip, desc.format );
								const Uint32 mipHeight	= CalculateTextureMipDimension( desc.height, mip, desc.format );

								subresourceData[ subresourceIndex ].pSysMem = dataBytePtr;
								subresourceData[ subresourceIndex ].SysMemPitch = CalculateTexturePitch( mipWidth, desc.format );
								subresourceData[ subresourceIndex ].SysMemSlicePitch = CalculateTextureSize( mipWidth, mipHeight, desc.format );

								dataBytePtr += subresourceData[ subresourceIndex ].SysMemSlicePitch;
							}
						}
					}
					else
#endif
					{
						for ( Uint32 slice = 0; slice < dxDesc.ArraySize; ++slice )
						{
							for ( Uint32 mip = 0; mip < dxDesc.MipLevels; ++mip )
							{
								const Uint32 subresourceIndex = D3D11CalcSubresource( mip, slice, dxDesc.MipLevels );
								const Uint32 dataIndex = mip + slice * dxDesc.MipLevels;
								const Uint32 mipWidth	= CalculateTextureMipDimension( desc.width, mip, desc.format );
								const Uint32 mipHeight	= CalculateTextureMipDimension( desc.height, mip, desc.format );

								subresourceData[ subresourceIndex ].pSysMem = initData->m_mipsInitData[dataIndex].m_data;
								subresourceData[ subresourceIndex ].SysMemPitch = CalculateTexturePitch( mipWidth, desc.format );
								subresourceData[ subresourceIndex ].SysMemSlicePitch = CalculateTextureSize( mipWidth, mipHeight, desc.format );
							}
						}
					}

					subresourceDataPtr = subresourceData;
				}
				// Create texture
# if MICROSOFT_ATG_DYNAMIC_SCALING
				if( (desc.usage & TEXUSAGE_MASK_DynamicScaling) && (desc.width != desc.height) && (desc.width < 1920) && (desc.width > 1) && (desc.usage & (TEXUSAGE_RenderTarget | TEXUSAGE_DepthStencil)) && (desc.type != TEXTYPE_CUBE))
				{
					const Uint32 pageSize = 64 * 1024;
					const Uint32 maxPages = (32 * 1024 * 1024) / pageSize;

					GPUAPI_ASSERT( initData == nullptr, TXT("Initial data not supported for ESRAM resources") );

					const TextureDesc descTrue1080 = BuildTextureDescTrue1080( desc );
					
					Uint32 SizeAtBaseRes = CalcTextureSize(desc);
					Uint32 SizeAt1920x1080 = CalcTextureSize(descTrue1080);

					Uint32 PageAlignedSizeAtBaseRes = (SizeAtBaseRes + (pageSize - 1)) & ~(pageSize - 1);
					Uint32 PageAlignedSizeAt1920x1080 = (SizeAt1920x1080 + (pageSize - 1)) & ~(pageSize - 1);

					texMemory = VirtualAlloc(NULL, PageAlignedSizeAt1920x1080, MEM_GRAPHICS | MEM_LARGE_PAGES | MEM_RESERVE, PAGE_READONLY);

					GPUAPI_ASSERT( texMemory != nullptr, TXT("Failed to allocate memory") );

					UINT firstPage = 0;
					Uint32 numESRAMPages = 0;
					Uint32 pageOffset = 0;

					if((dxDesc.MiscFlags & D3D11X_RESOURCE_MISC_ESRAM_RESIDENT) && (desc.esramOffset <= ((32 * 1024 * 1024) - pageSize)))
					{
						firstPage = desc.esramOffset / pageSize;
						pageOffset = desc.esramOffset - (firstPage * pageSize);
						numESRAMPages = PageAlignedSizeAtBaseRes / pageSize;

						if((firstPage + numESRAMPages) >= maxPages)
						{
							numESRAMPages = maxPages - (firstPage + 1);
						}
					}
					Uint32 numDRAMPages = (PageAlignedSizeAt1920x1080 / pageSize) - numESRAMPages;

					VirtualAlloc(texMemory, numDRAMPages * pageSize, MEM_GRAPHICS | MEM_LARGE_PAGES | MEM_COMMIT, PAGE_READONLY);

					if(numESRAMPages)
					{
						UINT ESRAMPageList[ maxPages ];   // worst case size is all of ESRAM
						for( UINT i = 0; i < numESRAMPages; ++i ) 
						{
							ESRAMPageList[i] = firstPage + i;

							GPUAPI_ASSERT( ESRAMPageList[i] < 512, TXT("Page overflow") );
						}
						D3DMapEsramMemory(D3D11_MAP_ESRAM_LARGE_PAGES, (BYTE *)texMemory + (numDRAMPages * pageSize), numESRAMPages, ESRAMPageList);
					}					
					partialResidencyAllocation = TRUE;
					pDynamicScalingData = (SDynamicScalingTextureData *)VirtualAlloc(NULL, sizeof(SDynamicScalingTextureData), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

					GPUAPI_ASSERT( pDynamicScalingData != NULL, TXT("Out of memory") );
					GPUAPI_ASSERT( PageAlignedSizeAt1920x1080 - ((PageAlignedSizeAt1920x1080 - PageAlignedSizeAtBaseRes) + pageOffset) >= SizeAtBaseRes, TXT("Not enough memory allocated") );

					// Some sanity checks for half/quad res sizes calculations
					#ifdef RED_ASSERTS_ENABLED
					{
						RED_FATAL_ASSERT( !(1600 % 4) && !(900 % 4) && !(1920 % 4) && !(1080 % 4), "" );
						const TextureDesc testDescFull = BuildDynamicScalingTestDesc( 1600 / 1, 900 / 1, 4 );
						const TextureDesc testDescHalf = BuildDynamicScalingTestDesc( 1600 / 2, 900 / 2, 2 );
						const TextureDesc testDescQuarter = BuildDynamicScalingTestDesc( 1600 / 4, 900 / 4, 1 );
						RED_FATAL_ASSERT( 1600 == testDescFull.width && 900 == testDescFull.height, "" );

						const TextureDesc testDescFullTrue1080 = BuildTextureDescTrue1080( testDescFull );
						const TextureDesc testDescHalfTrue1080 = BuildTextureDescTrue1080( testDescHalf );
						const TextureDesc testDescQuarterTrue1080 = BuildTextureDescTrue1080( testDescQuarter );
						
						for ( Uint32 idx=0; idx<DYANMIC_SCALING_NUM_TARGETS; ++idx )
						{
							// 						
							float deltaSizeW;
							float deltaSizeH;
							Uint32 minSizeW;
							Uint32 minSizeH;
							Uint32 alignment;

							GetDynamicScalingParams( testDescFull, testDescFullTrue1080, minSizeW, minSizeH, deltaSizeW, deltaSizeH, alignment );
							const Uint32 currFullWidth  = GetDynamicTargetSize( idx, minSizeW, deltaSizeW, alignment, false );
							const Uint32 currFullHeight = GetDynamicTargetSize( idx, minSizeH, deltaSizeH, alignment, true );

							GetDynamicScalingParams( testDescHalf, testDescHalfTrue1080, minSizeW, minSizeH, deltaSizeW, deltaSizeH, alignment );
							const Uint32 currHalfWidth  = GetDynamicTargetSize( idx, minSizeW, deltaSizeW, alignment, false );
							const Uint32 currHalfHeight = GetDynamicTargetSize( idx, minSizeH, deltaSizeH, alignment, true );

							GetDynamicScalingParams( testDescQuarter, testDescQuarterTrue1080, minSizeW, minSizeH, deltaSizeW, deltaSizeH, alignment );
							const Uint32 currQuarterWidth  = GetDynamicTargetSize( idx, minSizeW, deltaSizeW, alignment, false );
							const Uint32 currQuarterHeight = GetDynamicTargetSize( idx, minSizeH, deltaSizeH, alignment, true );

							RED_FATAL_ASSERT( currFullWidth >= 1600 && currFullWidth <= 1920, "" );
							RED_FATAL_ASSERT( currFullHeight >= 900 && currFullHeight <= 1080, "" );
							RED_FATAL_ASSERT( currHalfWidth == currFullWidth / 2, "" );
							RED_FATAL_ASSERT( currHalfHeight == currFullHeight / 2, "" );
							RED_FATAL_ASSERT( currQuarterWidth == currFullWidth / 4, "" );
							RED_FATAL_ASSERT( currQuarterHeight == currFullHeight / 4, "" );

							RED_FATAL_ASSERT( currFullWidth <= testDescFullTrue1080.width, "" );
							RED_FATAL_ASSERT( currFullHeight <= testDescFullTrue1080.height, "" );

							if ( idx + 1 == DYANMIC_SCALING_NUM_TARGETS )
							{
								RED_FATAL_ASSERT( currFullWidth == testDescFullTrue1080.width, "" );
								RED_FATAL_ASSERT( currFullHeight == testDescFullTrue1080.height, "" );
							}
						}
					}
					#endif

					float deltaSizeW;
					float deltaSizeH;
					Uint32 minSizeW;
					Uint32 minSizeH;
					Uint32 alignment;
					GetDynamicScalingParams( desc, descTrue1080, minSizeW, minSizeH, deltaSizeW, deltaSizeH, alignment );

					// placement create textures from 1320 all the way up to 1920
					TextureDesc stepDesc = desc;
					
					pDynamicScalingData->Add(minSizeW, minSizeH, deltaSizeW, deltaSizeH, alignment);
					
					ID3D11DeviceX* deviceX = (ID3D11DeviceX*)GetDevice();
					D3D11_TEXTURE2D_DESC dxStepDesc;
					FillD3DTextureDesc( stepDesc, dxStepDesc );
					XG_TEXTURE2D_DESC xgTexDesc;
					CopyD3DTextureDescToXG( dxStepDesc, xgTexDesc );

					dxDesc.MiscFlags &= ~D3D11X_RESOURCE_MISC_ESRAM_RESIDENT;
					dxStepDesc.MiscFlags &= ~D3D11X_RESOURCE_MISC_ESRAM_RESIDENT;

					for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
					{
						stepDesc.width = GetDynamicTargetSize(i, minSizeW, deltaSizeW, alignment, false);
						stepDesc.height = GetDynamicTargetSize(i, minSizeH, deltaSizeH, alignment, true);

						dxStepDesc.Width = stepDesc.width;
						dxStepDesc.Height = stepDesc.height;

						Uint32 thisSize = CalcTextureSize(stepDesc);
						GPUAPI_ASSERT( PageAlignedSizeAt1920x1080 >= thisSize, TXT("Memory overflow") );
												
						deviceX->CreatePlacementTexture2D( &dxStepDesc, xgTexDesc.TileMode, 0, (BYTE *)texMemory + (PageAlignedSizeAt1920x1080 - thisSize) + pageOffset, &texture2d);
						pDynamicScalingData->m_pTexture[i] = texture2d;						
					}
					texture2d = (ID3D11Texture2D *)pDynamicScalingData->m_pTexture[g_DynamicScaleIndex];
				}
				else
# endif
					GetDevice()->CreateTexture2D( &dxDesc, subresourceDataPtr, &texture2d );
			}

			if ( texture2d == nullptr )
			{
				GPUAPI_HALT( "Failed to create 2d texture" );
#ifdef RED_PLATFORM_DURANGO
				if ( !desc.IsInPlace() )
				{
					GPU_API_FREE( GpuMemoryPool_Textures, MC_TextureData, texMemory );
				}
#endif
				return;
			}
			
			outTexture = texture2d;

#ifdef RED_PLATFORM_DURANGO
			// If the texture is in-place loaded, we need the handle, rather than the texture itself
			if( desc.IsInPlace() )
			{
				outInplaceRegion = initData->m_cookedData;
			}
			else
			{
				outTextureMemory = texMemory;
			}
#endif
		}


		// Returns false on failure. If failed, data may contain some successfully created objects, so the whole thing should be cleared.
		Bool CreateTextureViews( STextureData& data )
		{
			const TextureDesc& desc = data.m_Desc;
			ID3D11Resource* d3dTex = data.m_pTexture;
			HRESULT res;

			//dex++: merged code for creating SRV, RTV and DSV on both DX10 and DX11

			// When creating DSV and SRV we sometimes want to use different formats
			DXGI_FORMAT SRVfmt = MapShaderResourceView(desc.format);

			// HACK DX10 this should be inside texturefactory shit
			if(desc.usage & TEXUSAGE_RenderTarget)
			{
				DXGI_FORMAT RTVfmt = MapRenderTargetView(desc.format);

				D3D_RENDER_TARGET_VIEW_DESC rtvDesc;
				rtvDesc.Format = RTVfmt;

				if ( desc.type == TEXTYPE_2D )
				{
					if ( desc.msaaLevel < 1 )
					{
						rtvDesc.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2D;
						rtvDesc.Texture2D.MipSlice = 0;
					}
					else
					{
						rtvDesc.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2DMS;
					}
				}
				else if ( desc.type == TEXTYPE_CUBE )
				{
					rtvDesc.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2DARRAY;
					rtvDesc.Texture2DArray.MipSlice = 0;
					rtvDesc.Texture2DArray.FirstArraySlice = 0;
					rtvDesc.Texture2DArray.ArraySize = 6;
				}
				//dex++
				else if ( desc.type == TEXTYPE_ARRAY )
				{
					rtvDesc.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2DARRAY;
					rtvDesc.Texture2DArray.MipSlice = 0;
					rtvDesc.Texture2DArray.FirstArraySlice = 0;
					rtvDesc.Texture2DArray.ArraySize = desc.sliceNum;
				}
				//dex--
#if MICROSOFT_ATG_DYNAMIC_SCALING
				if(data.m_pDynamicScalingData)
				{
					for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
					{
						res = GetDevice()->CreateRenderTargetView( data.m_pDynamicScalingData->m_pTexture[i], &rtvDesc, &(data.m_pDynamicScalingData->m_pRenderTargetView[i]) );
						if ( FAILED( res ) )
						{
							GPUAPI_HALT( "Failed to create RenderTargetView" );
							return false;
						}
					}
					data.m_pRenderTargetView = data.m_pDynamicScalingData->m_pRenderTargetView[g_DynamicScaleIndex];
				}
				else
#endif
				{
					res = GetDevice()->CreateRenderTargetView( d3dTex, &rtvDesc, &(data.m_pRenderTargetView) );
					if ( FAILED( res ) )
					{
						GPUAPI_HALT( "Failed to create RenderTargetView" );
						return false;
					}
				}
#ifdef GPU_API_DEBUG_PATH
				data.m_pRenderTargetView->SetPrivateData( WKPDID_D3DDebugObjectName, 5, "tex2D" );
#endif
				if ( TEXTYPE_2D == desc.type && desc.initLevels > 1 )
				{
					GPUAPI_ASSERT( !data.m_pp2DRenderTargetViewPerMipLevel );

					data.m_pp2DRenderTargetViewPerMipLevel = new ID3D11RenderTargetView*[desc.initLevels];
					
#if MICROSOFT_ATG_DYNAMIC_SCALING
					if( data.m_pDynamicScalingData )
					{
						GPUAPI_ASSERT( desc.initLevels <= DYNAMIC_SCALING_MAX_MIPS, TXT("Error, memory about to overflow, increase DYNAMIC_SCALING_MAX_MIPS"));

						for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
						{
							data.m_pDynamicScalingData->m_pp2DRenderTargetViewPerMipLevel[i][0] = data.m_pDynamicScalingData->m_pRenderTargetView[i];
							data.m_pDynamicScalingData->m_pp2DRenderTargetViewPerMipLevel[i][0]->AddRef();

							for ( Uint32 mip_i=1; mip_i<desc.initLevels; ++mip_i )
							{
								D3D_RENDER_TARGET_VIEW_DESC rtvMipDesc = rtvDesc;
								rtvMipDesc.Texture2D.MipSlice = mip_i;
								HRESULT mipRes = GetDevice()->CreateRenderTargetView( data.m_pDynamicScalingData->m_pTexture[i], &rtvMipDesc, &data.m_pDynamicScalingData->m_pp2DRenderTargetViewPerMipLevel[i][mip_i] );					
								if ( FAILED( mipRes ) )
								{
									GPUAPI_HALT( "Failed to create RenderTargetView for mip %u", mip_i );
									return false;
								}								
							}
						}
						for ( Uint32 mip_i=0; mip_i<desc.initLevels; ++mip_i )
						{
							data.m_pp2DRenderTargetViewPerMipLevel[mip_i] = data.m_pDynamicScalingData->m_pp2DRenderTargetViewPerMipLevel[g_DynamicScaleIndex][mip_i];
						}
					}
					else
#endif
					{
						data.m_pp2DRenderTargetViewPerMipLevel[0] = data.m_pRenderTargetView;
						data.m_pp2DRenderTargetViewPerMipLevel[0]->AddRef();

						for ( Uint32 mip_i=1; mip_i<desc.initLevels; ++mip_i )
						{
							D3D_RENDER_TARGET_VIEW_DESC rtvMipDesc = rtvDesc;
							rtvMipDesc.Texture2D.MipSlice = mip_i;
							HRESULT mipRes = GetDevice()->CreateRenderTargetView( d3dTex, &rtvMipDesc, &data.m_pp2DRenderTargetViewPerMipLevel[mip_i] );					
							if ( FAILED( mipRes ) )
							{
								GPUAPI_HALT( "Failed to create RenderTargetView for mip %u", mip_i );
								return false;
							}
						}
					}
				}

				// No UAV for msaa :( 
				if ( desc.msaaLevel < 1 )
				{
					D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
					DescUAV.Format = SRVfmt;

					if ( desc.type == TEXTYPE_2D )
					{
						DescUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
						DescUAV.Texture2D.MipSlice = 0;
					}
					else if ( desc.type == TEXTYPE_CUBE )
					{
						DescUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
						DescUAV.Texture2DArray.MipSlice = 0;
						DescUAV.Texture2DArray.FirstArraySlice = 0;
						DescUAV.Texture2DArray.ArraySize = desc.sliceNum * 6;
					}
					//dex++
					else if ( desc.type == TEXTYPE_ARRAY )
					{
						DescUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
						DescUAV.Texture2DArray.MipSlice = 0;
						DescUAV.Texture2DArray.FirstArraySlice = 0;
						DescUAV.Texture2DArray.ArraySize = desc.sliceNum;
					}
					//dex--
#if MICROSOFT_ATG_DYNAMIC_SCALING
					if(data.m_pDynamicScalingData)
					{
						for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
						{
							res = GetDevice()->CreateUnorderedAccessView( data.m_pDynamicScalingData->m_pTexture[i], &DescUAV, &(data.m_pDynamicScalingData->m_pUnorderedAccessView[i]) );
							if ( FAILED( res ) )
							{
								GPUAPI_HALT( "Failed to create UAV" );
								return false;
							}
						}
						data.m_pUnorderedAccessView = data.m_pDynamicScalingData->m_pUnorderedAccessView[g_DynamicScaleIndex];
					}					
					else
#endif
					{

						res = GetDevice()->CreateUnorderedAccessView( d3dTex, &DescUAV, &(data.m_pUnorderedAccessView) );
						if ( FAILED( res ) )
						{
							GPUAPI_HALT( "Failed to create UAV" );
							return false;
						}
					}
				}

				//dex++: for texture arrays create separate render target view for each slice
				if ( desc.type == TEXTYPE_ARRAY )
				{
					GPUAPI_ASSERT( 1 == desc.initLevels, TXT( "Add support for mipmaps, as it's done for cubemaps" ) );

					// allocate rtv array
					{
						const Uint16 capacity = desc.sliceNum;
						data.m_RenderTargetViewsArraySize = capacity;
						data.m_pRenderTargetViewsArray = new ID3DRenderTargetView*[ capacity ];
						GPUAPI_ASSERT( data.m_RenderTargetViewsArraySize == capacity, TXT( "Out of range" ) );
					}
#if MICROSOFT_ATG_DYNAMIC_SCALING
					if(data.m_pDynamicScalingData)
					{
						GPUAPI_ASSERT( desc.sliceNum <= DYNAMIC_SCALING_MAXSLICES, TXT("Error, memory about to overflow, increase DYNAMIC_SCALING_MAXSLICES"));

						for ( Uint32 i=0; i<desc.sliceNum; ++i )
						{
							// describe
							D3D_RENDER_TARGET_VIEW_DESC rtvDesc;
							rtvDesc.Format = RTVfmt;
							rtvDesc.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2DARRAY;
							rtvDesc.Texture2DArray.MipSlice = 0;
							rtvDesc.Texture2DArray.FirstArraySlice = i;
							rtvDesc.Texture2DArray.ArraySize = 1;

							for(Uint32 j=0; j<DYANMIC_SCALING_NUM_TARGETS; ++j)
							{
								// create
								HRESULT res = GetDevice()->CreateRenderTargetView( data.m_pDynamicScalingData->m_pTexture[j], &rtvDesc, &(data.m_pDynamicScalingData->m_pRenderTargetViewsArray[j][i]) );
								if ( FAILED( res ) || data.m_pDynamicScalingData->m_pRenderTargetViewsArray[j][i] == NULL )
								{
									GPUAPI_HALT( "Failed to create RTV for slice %u", i );
									return false;
								}
#ifdef GPU_API_DEBUG_PATH
								data.m_pRenderTargetViewsArray[j][i]->SetPrivateData( WKPDID_D3DDebugObjectName, 5, "texAR" );
#endif
							}
							data.m_pRenderTargetViewsArray[i] = data.m_pDynamicScalingData->m_pRenderTargetViewsArray[g_DynamicScaleIndex][i];				
						}
					}
					else
#endif
					{
						for ( Uint32 i=0; i<desc.sliceNum; ++i )
						{
							// describe
							D3D_RENDER_TARGET_VIEW_DESC rtvDesc;
							rtvDesc.Format = RTVfmt;
							rtvDesc.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2DARRAY;
							rtvDesc.Texture2DArray.MipSlice = 0;
							rtvDesc.Texture2DArray.FirstArraySlice = i;
							rtvDesc.Texture2DArray.ArraySize = 1;

							// create
							HRESULT res = GetDevice()->CreateRenderTargetView( d3dTex, &rtvDesc, &(data.m_pRenderTargetViewsArray[i]) );
							if ( FAILED( res ) || data.m_pRenderTargetViewsArray[i] == NULL )
							{
								GPUAPI_HALT( "Failed to create RTV for slice %u", i );
								return false;
							}
#ifdef GPU_API_DEBUG_PATH
							data.m_pRenderTargetViewsArray[i]->SetPrivateData( WKPDID_D3DDebugObjectName, 5, "texAR" );
#endif
						}
					}
					
				}
				else if ( desc.type == TEXTYPE_CUBE )
				{
#if MICROSOFT_ATG_DYNAMIC_SCALING
					GPUAPI_ASSERT( !data.m_pDynamicScalingData, TXT("Error, wrong pointers being used"));
#endif
					GPUAPI_ASSERT( desc.initLevels > 0 );

					// allocate rtv array
					{
						const Uint16 capacity = desc.sliceNum * 6 * desc.initLevels;
						data.m_RenderTargetViewsArraySize = capacity;
						data.m_pRenderTargetViewsArray = new ID3DRenderTargetView*[ capacity ];
						GPUAPI_ASSERT( data.m_RenderTargetViewsArraySize == capacity, TXT( "Out of range" ) );
					}

					// per slice
					for ( Uint16 slice_i=0; slice_i<desc.sliceNum; ++slice_i )
					{
						// per face
						for ( Uint16 face_i=0; face_i<6; ++face_i )
						{
							// per mip
							for ( Uint16 mip_i=0; mip_i<desc.initLevels; ++mip_i )
							{
								const Int32 descSliceIndex = slice_i * 6 + face_i;
								const Uint16 entrySliceIndex = CalculateCubemapSliceIndex( desc, slice_i, face_i, mip_i );

								// describe
								D3D_RENDER_TARGET_VIEW_DESC rtvDesc;
								rtvDesc.Format = RTVfmt;
								rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
								rtvDesc.Texture2DArray.MipSlice = mip_i;
								rtvDesc.Texture2DArray.FirstArraySlice = descSliceIndex;
								rtvDesc.Texture2DArray.ArraySize = 1;

								// create the 
								HRESULT res = GetDevice()->CreateRenderTargetView( d3dTex, &rtvDesc, &(data.m_pRenderTargetViewsArray[entrySliceIndex]) );
								if ( FAILED( res ) || data.m_pRenderTargetViewsArray[entrySliceIndex] == NULL )
								{
									GPUAPI_HALT( "Failed to create RenderTargetView for slice %u, face %u, mip %u", slice_i, face_i, mip_i );
									return false;
								}
#ifdef GPU_API_DEBUG_PATH
								data.m_pRenderTargetViewsArray[entrySliceIndex]->SetPrivateData( WKPDID_D3DDebugObjectName, 7, "texCUBE" );
#endif
							}
						}
					}
				}
				//dex--
			}

			if (desc.usage & TEXUSAGE_DepthStencil)
			{
				DXGI_FORMAT DSVfmt = MapDepthStencilView( desc.format );
				SRVfmt = MapShaderResourceView( desc.format );

				D3D_DEPTH_STENCIL_VIEW_DESC dsvDesc;
				dsvDesc.Format = DSVfmt;
				dsvDesc.Flags = 0;
				if ( desc.msaaLevel < 1 )
				{
					dsvDesc.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2D;
					dsvDesc.Texture2D.MipSlice = 0;
				}
				else
				{
					dsvDesc.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2DMS;
				}
#if MICROSOFT_ATG_DYNAMIC_SCALING
				if(data.m_pDynamicScalingData)
				{
					for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
					{
						HRESULT res = GetDevice()->CreateDepthStencilView( data.m_pDynamicScalingData->m_pTexture[i], &dsvDesc, &(data.m_pDynamicScalingData->m_pDepthStencilView[i]) );

						if ( FAILED( res ) || data.m_pDynamicScalingData->m_pDepthStencilView[i] == NULL )
						{
							GPUAPI_HALT( "Failed to create DepthStencilView" );
							return false;
						}
					}
					data.m_pDepthStencilView = data.m_pDynamicScalingData->m_pDepthStencilView[g_DynamicScaleIndex];
				}
				else
#endif
				{
					HRESULT res = GetDevice()->CreateDepthStencilView( d3dTex, &dsvDesc, &(data.m_pDepthStencilView) );

					if ( FAILED( res ) || data.m_pDepthStencilView == NULL )
					{
						GPUAPI_HALT( "Failed to create DepthStencilView" );
						return false;
					}
				}
				// Create readonly DSV
				{
					D3D_DEPTH_STENCIL_VIEW_DESC dsvDescReadOnly = dsvDesc;
					GPUAPI_ASSERT ( 0 == dsvDescReadOnly.Flags );
					dsvDescReadOnly.Flags |= D3D11_DSV_READ_ONLY_DEPTH;
					if ( desc.format == TEXFMT_D24FS8 || desc.format == TEXFMT_D24S8 )
					{
						dsvDescReadOnly.Flags |= D3D11_DSV_READ_ONLY_STENCIL;
					}
#if MICROSOFT_ATG_DYNAMIC_SCALING
					if(data.m_pDynamicScalingData)
					{
						for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
						{
							res = GetDevice()->CreateDepthStencilView( data.m_pDynamicScalingData->m_pTexture[i], &dsvDescReadOnly, &(data.m_pDynamicScalingData->m_pDepthStencilViewReadOnly[i]) );

							if ( FAILED( res ) || data.m_pDynamicScalingData->m_pDepthStencilViewReadOnly[i] == NULL )
							{
								GPUAPI_HALT( "Failed to create read-only DepthStencilView" );
								return false;
							}
						}
						data.m_pDepthStencilViewReadOnly = data.m_pDynamicScalingData->m_pDepthStencilViewReadOnly[g_DynamicScaleIndex];
					}
					else
#endif
					{
						res = GetDevice()->CreateDepthStencilView( d3dTex, &dsvDescReadOnly, &(data.m_pDepthStencilViewReadOnly) );

						if ( FAILED( res ) || data.m_pDepthStencilViewReadOnly == NULL )
						{
							GPUAPI_HALT( "Failed to create read-only DepthStencilView" );
							return false;
						}
					}
				}

				// Create stencil SRV - only if the requested format supports stencil
				if ( (desc.usage & TEXUSAGE_DepthStencil) && (desc.usage & TEXUSAGE_Samplable) && (TEXTYPE_2D == desc.type) && 1 == desc.sliceNum && ( desc.format == TEXFMT_D24FS8 || desc.format == TEXFMT_D24S8 ) )
				{
					const DXGI_FORMAT SRVfmt_stencil = DXGI_FORMAT_X24_TYPELESS_G8_UINT;

					// Create the shader resource view 
					D3D11_SHADER_RESOURCE_VIEW_DESC descStencilSRV; 
					descStencilSRV.Format = SRVfmt_stencil; 
					descStencilSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					descStencilSRV.Texture2D.MipLevels = 1;
					descStencilSRV.Texture2D.MostDetailedMip = 0; 

#if MICROSOFT_ATG_DYNAMIC_SCALING
					if(data.m_pDynamicScalingData)
					{
						for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
						{
							res = GetDevice()->CreateShaderResourceView( data.m_pDynamicScalingData->m_pTexture[i], &descStencilSRV, &data.m_pDynamicScalingData->m_pStencilShaderResourceView[i] ); 
							if ( FAILED( res ) || data.m_pDynamicScalingData->m_pStencilShaderResourceView[i] == NULL )
							{
								GPUAPI_HALT( "Failed to create stencil ShaderResourceView" );
								return false;
							}
						}
						data.m_pStencilShaderResourceView = data.m_pDynamicScalingData->m_pStencilShaderResourceView[g_DynamicScaleIndex];
					}
					else
#endif
					{
						res = GetDevice()->CreateShaderResourceView( d3dTex, &descStencilSRV, &data.m_pStencilShaderResourceView ); 
						if ( FAILED( res ) || data.m_pStencilShaderResourceView == NULL )
						{
							GPUAPI_HALT( "Failed to create stencil ShaderResourceView" );
							return false;
						}
					}
				}

				//dex++: for texture arrays used as depth stencil ( shadowmaps ) create separate DSV for each slice
				if (desc.usage & TEXUSAGE_DepthStencil && desc.type == TEXTYPE_ARRAY)
				{
					GPUAPI_ASSERT( 1 == desc.initLevels, TXT( "Add support for mipmaps, as it's done for cubemaps" ) );

					// allocate rtv array
					{
						const Uint16 capacity = desc.sliceNum;
						data.m_DepthTargetViewsArraySize = capacity;
						data.m_pDepthStencilViewsArray = new ID3DDepthStencilView*[ capacity ];
						GPUAPI_ASSERT( data.m_DepthTargetViewsArraySize == capacity, TXT( "Out of range" ) );
					}
#if MICROSOFT_ATG_DYNAMIC_SCALING
					if(data.m_pDynamicScalingData)
					{
						GPUAPI_ASSERT( desc.sliceNum <= DYNAMIC_SCALING_MAXSLICES, TXT("Error, memory about to overflow, increase DYNAMIC_SCALING_MAXSLICES"));

						for ( Uint32 i=0; i<desc.sliceNum; ++i )
						{
							// describe
							D3D_DEPTH_STENCIL_VIEW_DESC rtvDesc;
							rtvDesc.Format = DSVfmt;
							rtvDesc.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2DARRAY;
							rtvDesc.Flags = 0;
							rtvDesc.Texture2DArray.MipSlice = 0;
							rtvDesc.Texture2DArray.FirstArraySlice = i;
							rtvDesc.Texture2DArray.ArraySize = 1;

							for(Uint32 j=0; j<DYANMIC_SCALING_NUM_TARGETS; ++j)
							{
								// create
								HRESULT res = GetDevice()->CreateDepthStencilView( data.m_pDynamicScalingData->m_pTexture[j], &rtvDesc, &(data.m_pDynamicScalingData->m_pDepthStencilViewsArray[j][i]) );
								if ( FAILED( res ) || data.m_pDynamicScalingData->m_pDepthStencilViewsArray[j][i] == NULL )
								{
									GPUAPI_HALT( "Failed to create DepthStencilView for slice %u", i );
									return false;
								}
							}
							data.m_pDepthStencilViewsArray[i] = data.m_pDynamicScalingData->m_pDepthStencilViewsArray[g_DynamicScaleIndex][i];
						}
					}
					else
#endif
					{
						for ( Uint32 i=0; i<desc.sliceNum; ++i )
						{
							// describe
							D3D_DEPTH_STENCIL_VIEW_DESC rtvDesc;
							rtvDesc.Format = DSVfmt;
							rtvDesc.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2DARRAY;
							rtvDesc.Flags = 0;
							rtvDesc.Texture2DArray.MipSlice = 0;
							rtvDesc.Texture2DArray.FirstArraySlice = i;
							rtvDesc.Texture2DArray.ArraySize = 1;

							// create
							HRESULT res = GetDevice()->CreateDepthStencilView( d3dTex, &rtvDesc, &(data.m_pDepthStencilViewsArray[i]) );
							if ( FAILED( res ) || data.m_pDepthStencilViewsArray[i] == NULL )
							{
								GPUAPI_HALT( "Failed to create DepthStencilView for slice %u", i );
								return false;
							}
						}
					}
				}
				//dex--
			}

			if (desc.usage & TEXUSAGE_Samplable)
			{
				D3D_SHADER_RESOURCE_VIEW_DESC srvDesc;
				srvDesc.Format = SRVfmt;

				if ( desc.type == TEXTYPE_2D )
				{
					if ( desc.msaaLevel < 1 )
					{
						srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
						srvDesc.Texture2D.MostDetailedMip = 0;
						srvDesc.Texture2D.MipLevels = desc.initLevels;
					}
					else
					{
						srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DMS;
					}
				}
				else if ( desc.type == TEXTYPE_CUBE )
				{
					srvDesc.ViewDimension = (desc.sliceNum > 1) ? D3D_SRV_DIMENSION_TEXTURECUBEARRAY : D3D_SRV_DIMENSION_TEXTURECUBE;
					if (desc.sliceNum > 1)
					{
						srvDesc.TextureCubeArray.MostDetailedMip = 0;
						srvDesc.TextureCubeArray.MipLevels = desc.initLevels;
						srvDesc.TextureCubeArray.First2DArrayFace = 0;
						srvDesc.TextureCubeArray.NumCubes = desc.sliceNum;
					}
					else
					{
						srvDesc.TextureCube.MostDetailedMip = 0;
						srvDesc.TextureCube.MipLevels = desc.initLevels;
					}
				}
				else if ( desc.type == TEXTYPE_ARRAY )
				{
					srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
					srvDesc.Texture2DArray.MostDetailedMip = 0;
					srvDesc.Texture2DArray.MipLevels = desc.initLevels;
					srvDesc.Texture2DArray.FirstArraySlice = 0;
					srvDesc.Texture2DArray.ArraySize = desc.sliceNum;
				}
#if MICROSOFT_ATG_DYNAMIC_SCALING
				if(data.m_pDynamicScalingData)
				{
					for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
					{
						HRESULT res = GetDevice()->CreateShaderResourceView( data.m_pDynamicScalingData->m_pTexture[i], &srvDesc, &(data.m_pDynamicScalingData->m_pShaderResourceView[i]) );
						if ( FAILED( res ) )
						{
							GPUAPI_HALT( "Failed to create ShaderResourceView" );
							return false;
						}
					}
					data.m_pShaderResourceView = data.m_pDynamicScalingData->m_pShaderResourceView[g_DynamicScaleIndex];
				}
				else
#endif
				{
					HRESULT res = GetDevice()->CreateShaderResourceView( d3dTex, &srvDesc, &(data.m_pShaderResourceView) );
					if ( FAILED( res ) )
					{
						GPUAPI_HALT( "Failed to create ShaderResourceView" );
						return false;
					}
				}
			}
			//dex--: DX10 and DX11 merge

			if ( 0 != (TEXUSAGE_CubeSamplablePerMipLevel & desc.usage) )
			{
#if MICROSOFT_ATG_DYNAMIC_SCALING
				GPUAPI_ASSERT( !data.m_pDynamicScalingData, TXT("Error, wrong pointers being used"));
#endif
				GPUAPI_ASSERT( TEXTYPE_CUBE == desc.type );

				GPUAPI_ASSERT( 1 == desc.sliceNum && "Not tested - won't work" );
				if ( 1 == desc.sliceNum )
				{
					GPUAPI_ASSERT( !data.m_ppCubeShaderResourceViewPerMipLevel );
					data.m_ppCubeShaderResourceViewPerMipLevel = new ID3DShaderResourceView*[desc.initLevels * desc.sliceNum];

					for ( Uint16 slice_i=0; slice_i<desc.sliceNum; ++slice_i )
					{
						for ( Uint16 level_i=0; level_i<desc.initLevels; ++level_i )
						{
							D3D_SHADER_RESOURCE_VIEW_DESC srvDesc;
							srvDesc.Format = SRVfmt;
							srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
							srvDesc.TextureCube.MostDetailedMip = level_i;
							srvDesc.TextureCube.MipLevels = 1;

							ID3DShaderResourceView *&refSRV = data.m_ppCubeShaderResourceViewPerMipLevel[ CalculateCubemapPerMipSliceIndex( desc, slice_i, level_i ) ];
							refSRV = NULL;
							HRESULT res = GetDevice()->CreateShaderResourceView( d3dTex, &srvDesc, &refSRV );
							if ( FAILED( res ) )
							{
								GPUAPI_HALT( "Failed to create ShaderResourceView for slice %u mip %u", slice_i, level_i );
								return false;
							}
						}
					}
				}
			}

			if ( 0 != (TEXUSAGE_Tex2DSamplablePerMipLevel & desc.usage) )
			{
				GPUAPI_ASSERT( TEXTYPE_2D == desc.type );

				GPUAPI_ASSERT( 1 == desc.sliceNum && "Not tested - won't work" );
				if ( 1 == desc.sliceNum )
				{
					GPUAPI_ASSERT( !data.m_ppTex2DShaderResourceViewPerMipLevel );
					data.m_ppTex2DShaderResourceViewPerMipLevel = new ID3DShaderResourceView*[desc.initLevels];

#if MICROSOFT_ATG_DYNAMIC_SCALING
					if( data.m_pDynamicScalingData )
					{
						GPUAPI_ASSERT( desc.initLevels <= DYNAMIC_SCALING_MAX_MIPS, TXT("Error, memory about to overflow, increase DYNAMIC_SCALING_MAX_MIPS"));

						for ( Uint16 level_i=0; level_i<desc.initLevels; ++level_i )
						{
							for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
							{
								D3D_SHADER_RESOURCE_VIEW_DESC srvDesc;
								srvDesc.Format = SRVfmt;
								srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
								srvDesc.Texture2D.MostDetailedMip = level_i;
								srvDesc.Texture2D.MipLevels = 1;

								ID3DShaderResourceView *&refSRV = data.m_pDynamicScalingData->m_ppTex2DShaderResourceViewPerMipLevel[i][ level_i ];
								refSRV = NULL;
								HRESULT res = GetDevice()->CreateShaderResourceView( data.m_pDynamicScalingData->m_pTexture[i], &srvDesc, &refSRV );
								if ( FAILED( res ) )
								{
									GPUAPI_HALT( "Failed to create ShaderResourceView for mip %u", level_i );
									return false;
								}
							}
							data.m_ppTex2DShaderResourceViewPerMipLevel[level_i] = data.m_pDynamicScalingData->m_ppTex2DShaderResourceViewPerMipLevel[g_DynamicScaleIndex][level_i];
						}
					}
					else
#endif
					{
						for ( Uint16 level_i=0; level_i<desc.initLevels; ++level_i )
						{
							D3D_SHADER_RESOURCE_VIEW_DESC srvDesc;
							srvDesc.Format = SRVfmt;
							srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
							srvDesc.Texture2D.MostDetailedMip = level_i;
							srvDesc.Texture2D.MipLevels = 1;

							ID3DShaderResourceView *&refSRV = data.m_ppTex2DShaderResourceViewPerMipLevel[ level_i ];
							refSRV = NULL;
							HRESULT res = GetDevice()->CreateShaderResourceView( d3dTex, &srvDesc, &refSRV );
							if ( FAILED( res ) )
							{
								GPUAPI_HALT( "Failed to create ShaderResourceView for mip %u", level_i );
								return false;
							}
						}
					}
				}
			}

			if ( 0 != (TEXUSAGE_CubeSamplablePerMipFace & desc.usage) )
			{
#if MICROSOFT_ATG_DYNAMIC_SCALING
				GPUAPI_ASSERT( !data.m_pDynamicScalingData, TXT("Error, wrong pointers being used"));
#endif
				GPUAPI_ASSERT( TEXTYPE_CUBE == desc.type );

				GPUAPI_ASSERT( 1 == desc.sliceNum && "Not tested - won't work" );
				if ( 1 == desc.sliceNum )
				{
					GPUAPI_ASSERT( !data.m_ppCubeShaderResourceViewPerMipFace );
					data.m_ppCubeShaderResourceViewPerMipFace = new ID3DShaderResourceView*[desc.initLevels * 6 * desc.sliceNum];

					for ( Uint16 slice_i=0; slice_i<desc.sliceNum; ++slice_i )
					{
						for ( Uint16 level_i=0; level_i<desc.initLevels; ++level_i )
						{
							for ( Uint16 face_i=0; face_i<6; ++face_i )
							{
								const Int32 descSliceIndex = slice_i * 6 + face_i;

								D3D_SHADER_RESOURCE_VIEW_DESC srvDesc;
								srvDesc.Format = SRVfmt;
								srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
								srvDesc.Texture2DArray.FirstArraySlice = descSliceIndex;
								srvDesc.Texture2DArray.MostDetailedMip = level_i;
								srvDesc.Texture2DArray.MipLevels = 1;
								srvDesc.Texture2DArray.ArraySize = 1;

								ID3DShaderResourceView *&refSRV = data.m_ppCubeShaderResourceViewPerMipFace[ CalculateCubemapSliceIndex( desc, slice_i, face_i, level_i ) ];
								refSRV = NULL;
								HRESULT res = GetDevice()->CreateShaderResourceView( d3dTex, &srvDesc, &refSRV );
								if ( FAILED( res ) )
								{
									GPUAPI_HALT( "Failed to create ShaderResourceView for slice %u face %u mip %u", slice_i, face_i, level_i );
									return false;
								}
							}
						}
					}
				}
			}

			return true;
		}


		typedef VOID (WINAPI *Texture2DFillFunction)( XMFLOAT4* pOut, CONST XMFLOAT2 *pTexCoord );
			
		void InitInternalTextureData2D( STextureData &td, Uint32 size, Texture2DFillFunction funcFill, Bool enablePointMips = false )
		{
			GPUAPI_ASSERT( !(enablePointMips && !Red::Math::IsPow2( size )) );
			const Uint32 mipLevels = enablePointMips ? Red::Math::MLog2( size ) + 1 : 1;
			
			D3D11_TEXTURE2D_DESC texDesc;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.Width = size;
			texDesc.Height = size;
			texDesc.MipLevels = mipLevels;
			texDesc.ArraySize = 1;
			texDesc.Format = Map(TEXFMT_R8G8B8A8);
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;
			
			td.m_Desc.type	 = TEXTYPE_2D;
			td.m_Desc.width  = size;
			td.m_Desc.height = size;
			td.m_Desc.format = TEXFMT_R8G8B8A8;
			td.m_Desc.usage	 = TEXUSAGE_Samplable;
			td.m_Desc.initLevels = static_cast<Uint16>(texDesc.MipLevels);

			const Uint32 MAX_MIPS = 12;
			GPUAPI_ASSERT( mipLevels <= MAX_MIPS );

			Uint8* dataTab[MAX_MIPS];
			Uint32 bytesPerPixel = 4;
			Uint32 pitch = bytesPerPixel * size;
			for ( Uint32 mip_i=0; mip_i<mipLevels; ++mip_i )
			{ // create init data

				dataTab[mip_i] = (Uint8*) GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, (size >> mip_i) * (size >> mip_i) * 4, 16 );
				if ( dataTab[mip_i] == nullptr )
				{
					GPUAPI_HALT( "Can't allocate texture init memory" );
					for ( Uint32 i=0; i<mip_i; ++i )
					{
						GPU_API_FREE( GpuMemoryPool_Textures, MC_TextureData, dataTab[i] );
						dataTab[i] = NULL;
					}
					return;
				}

				{
					const Uint32 mipSize	= size >> mip_i;
					const Uint32 mipPitch	= pitch >> mip_i;

					XMFLOAT4 value;
					XMFLOAT2 coord;

					for (Uint32 y = 0; y < mipSize; y++)
					{
						coord.y = (y + 0.5f) / mipSize;

						for (Uint32 x = 0; x < mipSize; x++)
						{
							coord.x = (x + 0.5f) / mipSize;

							funcFill( &value, &coord );

							Uint8* pos = dataTab[mip_i] + y * mipPitch + x * bytesPerPixel;

							pos[0] = (Uint8)(value.x * 255);
							pos[1] = (Uint8)(value.y * 255);
							pos[2] = (Uint8)(value.z * 255);
							pos[3] = (Uint8)(value.w * 255);
						}
					}
				}

			}

			// Create texture
			ID3D11Texture2D* texture2D = NULL;
			D3D11_SUBRESOURCE_DATA initDataStructTab[MAX_MIPS];
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			{
				for ( Uint32 mip_i=0; mip_i<mipLevels; ++mip_i )
				{
					initDataStructTab[mip_i].pSysMem = dataTab[mip_i];
					initDataStructTab[mip_i].SysMemPitch = pitch >> mip_i;
					initDataStructTab[mip_i].SysMemSlicePitch = 0;
				}
				
				const Bool textureCreateResult = SUCCEEDED( GetDevice()->CreateTexture2D( &texDesc, initDataStructTab, &texture2D ) );
				for ( Uint32 i=0; i<mipLevels; ++i )
				{
					GPU_API_FREE( GpuMemoryPool_Textures, MC_TextureData, dataTab[i] );
					dataTab[i] = NULL;
				}

				if ( !textureCreateResult )
				{
					return;
				}
			}
#if MICROSOFT_ATG_DYNAMIC_SCALING
			GPUAPI_ASSERT( !td.m_pDynamicScalingData, TXT("Error, wrong pointers being used"));
#endif
			td.m_pTexture = texture2D;

			srvDesc.Format = MapShaderResourceView(TEXFMT_R8G8B8A8);
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = mipLevels;
			HRESULT res = GetDevice()->CreateShaderResourceView( texture2D, &srvDesc, &(td.m_pShaderResourceView) );
			if(res != S_OK)
			{
				GPUAPI_HALT( "" );
			}
		}

		typedef VOID (WINAPI *Texture3DFillFunction)( XMFLOAT4 *pOut, CONST XMFLOAT3 *pTexCoord );

		void InitInternalTextureDataCUBE( STextureData &td, Uint32 size, Texture3DFillFunction funcFill )
		{
			D3D11_TEXTURE2D_DESC texDesc;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.Width = size;
			texDesc.Height = size;
			texDesc.MipLevels = 1;
			texDesc.ArraySize = 6;
			texDesc.Format = Map(TEXFMT_R8G8B8A8);
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

			td.m_Desc.type	 = TEXTYPE_2D;
			td.m_Desc.width  = size;
			td.m_Desc.height = size;
			td.m_Desc.format = TEXFMT_R8G8B8A8;
			td.m_Desc.usage	 = TEXUSAGE_Samplable;
			td.m_Desc.initLevels = 1;

			Uint8* data = nullptr;
			Uint32 bytesPerPixel = 4;
			Uint32 pitch = bytesPerPixel * size;
			Uint32 slicePitch = bytesPerPixel * size * size;
			{ // create init data
				XMFLOAT4 value;
				XMFLOAT3 coord, texelSize;
				Uint8* pos;

				data = (Uint8*) GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, size * size * bytesPerPixel * 6, 16 );
				if ( data == nullptr )
				{
					GPUAPI_HALT( "Can't allocate texture init memory" );
					return;
				}

				//HACK DX10 faces not filled properly, needs different coord calculation

				for ( Uint32 face = 0; face < 6; ++face )
				{
					coord.z = static_cast<Float>(face);
					for (Uint32 y = 0; y < size; y++)
					{
						coord.y = (y + 0.5f) / size;

						for (Uint32 x = 0; x < size; x++)
						{
							coord.x = (x + 0.5f) / size;

							funcFill( &value, &coord );

							//           column                row           slice
							pos = data + (x * bytesPerPixel) + (y * pitch) + (slicePitch * face);

							pos[0] = (Uint8)(value.x * 255);
							pos[1] = (Uint8)(value.y * 255);
							pos[2] = (Uint8)(value.z * 255);
							pos[3] = (Uint8)(value.w * 255);
						}
					}
				}
			}


			// Create texture
			ID3D11Texture2D* textureCube = nullptr;
			D3D11_SUBRESOURCE_DATA initDataStruct[6];
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			if(data != nullptr)
			{
				for (Uint32 i = 0; i < 6; ++i)
				{
					initDataStruct[i].pSysMem = data + slicePitch * i;
					initDataStruct[i].SysMemPitch = pitch;
					initDataStruct[i].SysMemSlicePitch = slicePitch;
				}

				if ( !SUCCEEDED( GetDevice()->CreateTexture2D( &texDesc, &(initDataStruct[0]), &textureCube ) ) )
				{
					GPU_API_FREE( GpuMemoryPool_Textures, MC_TextureData, data );
					return;
				}
				GPU_API_FREE( GpuMemoryPool_Textures, MC_TextureData, data );
				data = nullptr;
			}
			else
			{
				if ( !SUCCEEDED( GetDevice()->CreateTexture2D( &texDesc, NULL, &textureCube ) ) )
				{
					return;
				}
			}
#if MICROSOFT_ATG_DYNAMIC_SCALING
			GPUAPI_ASSERT( !td.m_pDynamicScalingData, TXT("Error, wrong pointers being used"));
#endif
			td.m_pTexture = textureCube;

			srvDesc.Format = MapShaderResourceView(TEXFMT_R8G8B8A8);
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = 1;
			srvDesc.TextureCube.MostDetailedMip = 0;
			HRESULT res = GetDevice()->CreateShaderResourceView( textureCube, &srvDesc, &(td.m_pShaderResourceView) );
			if(res != S_OK)
			{
				GPUAPI_HALT("");
			}
		}

		RED_FORCE_INLINE void DissolveCoordStep( Uint8 &value, Uint8 &outX, Uint8 &outY )
		{
			Uint8 v = (value&3);
			value >>= 2;
			Uint8 dx[] = { 0, 1, 1, 0 };
			Uint8 dy[] = { 0, 1, 0, 1 };
			outX = (outX<<1) + dx[v];
			outY = (outY<<1) + dy[v];
		}

		RED_FORCE_INLINE void DissolveCoord( Uint8 value, Uint8 &outX, Uint8 &outY )
		{
			outX = 0;
			outY = 0;
			DissolveCoordStep( value, outX, outY );
			DissolveCoordStep( value, outX, outY );
			DissolveCoordStep( value, outX, outY );
			DissolveCoordStep( value, outX, outY );
		}

		RED_FORCE_INLINE Uint8 DissolveValueForCoord( Uint32 x, Uint32 y )
		{
			GPUAPI_ASSERT( x < 16 && y < 16 );

			// lame (suboptimal)

			Uint8 val = 0;
			for ( Uint32 i=0; i<256; ++i )
			{
				Uint8 _x, _y;
				DissolveCoord( (Uint8)i, _x, _y );
				if ( x==_x && y==_y )
				{
					val = (Uint8)i;
					break;
				}
			}

			return val;
		}

		// Dissolve texture generator
		VOID WINAPI GenerateDissolveTexture( XMFLOAT4* pOut, const XMFLOAT2* pTexCoord )
		{
			Uint32 res = GPUAPI_DISSOLVE_TEXTURE_SIZE;
			Uint32 x = Red::Math::NumericalUtils::Clamp< Uint32 >( ( Uint32 ) (res * pTexCoord->x), 0, res-1 );
			Uint32 y = Red::Math::NumericalUtils::Clamp< Uint32 >( ( Uint32 ) (res * pTexCoord->y), 0, res-1 );

			// get dissolve value
			Uint8 val = DissolveValueForCoord( x, y );

			// clamp values so that 0.f clip threshold will give up full opacity, and 1.f full transparency
			val = Red::Math::NumericalUtils::Clamp< Uint8 >( val, 1, 255 );

			// build result
			*pOut = XMFLOAT4(val / 256.f,val / 256.f,val / 256.f,val / 256.f);
		}

		// Poisson rotation texture generator
		VOID WINAPI GeneratePoissonRotationTexture( XMFLOAT4* pOut, const XMFLOAT2* pTexCoord )
		{
			RED_UNUSED( pTexCoord );

			Float angle = GRandomNumberGenerator.Get< Float >( 360.0f ); // DissolveValueForCoord( x, y ) * (360.f / 256.f);
			Float dx	= sinf( DEG2RAD( angle ) );
			Float dy	= cosf( DEG2RAD( angle ) );

			pOut->x = Red::Math::NumericalUtils::Clamp(  dx * 0.5f + 0.5f,	0.f, 1.f );
			pOut->y = Red::Math::NumericalUtils::Clamp(  dy * 0.5f + 0.5f,	0.f, 1.f );
			pOut->z = Red::Math::NumericalUtils::Clamp( -dy * 0.5f + 0.5f,	0.f, 1.f );
			pOut->w = Red::Math::NumericalUtils::Clamp(  dx * 0.5f + 0.5f,	0.f, 1.f );
		}

		/// Xor fill
		VOID WINAPI BlankTextureFill( XMFLOAT4* pOut, const XMFLOAT2* pTexCoord )
		{
			RED_UNUSED( pTexCoord );
			*pOut = XMFLOAT4( 0.0f, 0.0f, 0.0f, 0.0f );
		}

		VOID WINAPI FlatNormalTextureFill( XMFLOAT4* pOut, const XMFLOAT2* pTexCoord )
		{
			RED_UNUSED( pTexCoord );
			*pOut = XMFLOAT4( 1.0f, 0.5f, 0.5f, 0.0f );
		}

		/// Xor fill
		VOID WINAPI DefaultTextureFill( XMFLOAT4* pOut, const XMFLOAT2* pTexCoord )
		{
			Uint32 x = (Uint32)(pTexCoord->x * 255);
			Uint32 y = (Uint32)(pTexCoord->y * 255);
			*pOut = XMFLOAT4( x / 255.0f, 0.0f, y / 255.0f, 1.0f );
		}

		// Cube fill
		VOID WINAPI DefaultCubeTextureFill( XMFLOAT4* pOut, const XMFLOAT3* pTexCoord )
		{
			if ( fabs(pTexCoord->z) > 2.5f )
			{
				if ( fabs(pTexCoord->z) > 3.5f )
				{
					if ( pTexCoord->z > 4.5f )
					{
						*pOut = XMFLOAT4( 1.0f, 0.5f, 0.5f, 1.0f );
					}
					else
					{
						*pOut = XMFLOAT4( 0.0f, 0.5f, 0.5f, 1.0f );
					}
				}
				else
				{
					*pOut = XMFLOAT4( 0.5f, 0.5f, 1.0f, 1.0f );
				}
			}
			else
			{
				if ( fabs(pTexCoord->z) > 0.5f )
				{
					if ( pTexCoord->z > 1.5f )
					{
						*pOut = XMFLOAT4( 0.5f, 1.0f, 0.5f, 1.0f );
					}
					else
					{
						*pOut = XMFLOAT4( 0.5f, 0.0f, 0.5f, 1.0f );
					}
				}
				else
				{
					*pOut = XMFLOAT4( 0.5f, 0.5f, 0.0f, 1.0f );
				}
			}
		}

		// SSAO rotation generator
		VOID WINAPI GenerateSSAORotationNoise( XMFLOAT4* pOut, const XMFLOAT2* pTexCoord )
		{
			const Uint8 values[16][4] =
			{
				{ 129, 253, 0, 0 },
				{  69,  69, 0, 0 },
				{   9, 221, 0, 0 },
				{ 205, 237, 0, 0 },
				{  85,  57, 0, 0 },
				{ 145,   9, 0, 0 },
				{ 221,  25, 0, 0 },
				{  25,  41, 0, 0 },
				{  41, 117, 0, 0 },
				{ 101, 205, 0, 0 },
				{ 161,  85, 0, 0 },
				{ 237, 101, 0, 0 },
				{ 253, 177, 0, 0 },
				{  57, 129, 0, 0 },
				{ 117, 145, 0, 0 },
				{ 177, 161, 0, 0 }
			};

			Uint32 res       = 4;
			Uint32 x         = Red::Math::NumericalUtils::Clamp<Uint32>( (Uint32) (res * pTexCoord->x), 0, res-1 );
			Uint32 y         = Red::Math::NumericalUtils::Clamp<Uint32>( (Uint32) (res * pTexCoord->y), 0, res-1 );
			const Uint8 *v = values[x + y * res];

			Float fX		= v[0] - 127.5f;
			Float fY		= v[1] - 127.5f;
			Float fZ		= v[2] - 127.5f;
			Float fLen		= sqrtf( fX*fX + fY*fY + fZ*fZ );
			Float fLenInv	= fLen > 0.0001f ? 1.f / fLen : 0.f;
			Float fMul		= 0.5f * fLenInv;

			pOut->x = fMul * fX + 0.5f;
			pOut->y = fMul * fY + 0.5f;
			pOut->z = fMul * fZ + 0.5f;
			pOut->w = 0;
		}

		// MipNoise
		Red::Math::Random::Generator< Red::Math::Random::StandardRand >& RefMipNoiseRandomGen()
		{
			static Red::Math::Random::Generator< Red::Math::Random::StandardRand > gen;
			return gen;
		}
		VOID WINAPI GenerateMipNoise( XMFLOAT4* pOut, const XMFLOAT2* pTexCoord )
		{
			RED_UNUSED( pTexCoord );

			//Uint32 res       = GPUAPI_MIP_NOISE_TEXTURE_SIZE;
			//Uint32 x         = Red::Math::NumericalUtils::Clamp<Uint32>( (Uint32) (res * pTexCoord->x), 0, res-1 );
			//Uint32 y         = Red::Math::NumericalUtils::Clamp<Uint32>( (Uint32) (res * pTexCoord->y), 0, res-1 );
			
			const Float randomValue = RefMipNoiseRandomGen().Get( 0.f, 1.f );

			pOut->x = randomValue;
			pOut->y = randomValue;
			pOut->z = randomValue;
			pOut->w = 0;
		}


		Bool ShouldCountTextureStats( const TextureDesc& desc )
		{
			// Only count regular samplable textures (not render targets, dynamic textures, etc)
			return ( ( desc.usage & ~TEXUSAGE_Immutable ) == GpuApi::TEXUSAGE_Samplable );
		}
	}

	// ----------------------------------------------------------------------
	// TextureDesc

	TextureDesc::TextureDesc ()
		: type( TEXTYPE_2D )
		, width( 0 )
		, height( 0 )
		, initLevels( 0 )
		, sliceNum ( 1 )
		, usage( 0 )
		, format( TEXFMT_R8G8B8A8 )
		, msaaLevel( 0 )
		, esramOffset( -1 )
		, esramSize( 0 )
		, inPlaceType( INPLACE_None )
	{}

	Uint32 TextureDesc::CalcTargetSlices() const
	{
		return type == TEXTYPE_CUBE ? sliceNum * 6 : sliceNum;
	}

	Uint32 TextureDesc::CalcTargetLevels() const
	{
		return initLevels > 0 ? initLevels : (1 + Red::Math::MLog2( Red::Math::NumericalUtils::Max( width, height ) ));
	}

	Bool TextureDesc::HasMips() const
	{
		Bool hasMips = (initLevels > 1 || 0 == initLevels && Red::Math::NumericalUtils::Max( width, height ) >= 2);
		GPUAPI_ASSERT( hasMips == (CalcTargetLevels() > 1) );
		return hasMips;
	}

	Bool TextureDesc::operator==( const TextureDesc &other ) const
	{
		return
			type == other.type &&
			width == other.width &&
			height == other.height &&
			initLevels == other.initLevels &&
			sliceNum == other.sliceNum &&
			usage == other.usage &&
			format == other.format &&
			msaaLevel == other.msaaLevel &&
			esramOffset == other.esramOffset &&
			esramSize == other.esramSize &&
			inPlaceType == other.inPlaceType;
	}

	// ----------------------------------------------------------------------

	TextureDataDesc::TextureDataDesc()
		: width( 0 )
		, height( 0 )
		, format( TEXFMT_R8G8B8A8 )
		, rowPitch( 0 )
		, slicePitch( 0 )
		, data( NULL )
	{}

	// ----------------------------------------------------------------------
	ID3D11Resource* GetD3DTextureBase( const TextureRef &ref )
	{
		GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Textures.Data(ref).m_pTexture) );
		return ref ? GetDeviceData().m_Textures.Data(ref).m_pTexture : NULL;
	}

	ID3D11Texture2D* GetD3DTexture2D( const TextureRef &ref )
	{
		GPUAPI_ASSERT( !(ref && (!GetDeviceData().m_Textures.Data(ref).m_pTexture ) ) );
		return ref ? static_cast<ID3D11Texture2D*>( GetDeviceData().m_Textures.Data(ref).m_pTexture ) : NULL;
	}

	ID3D11Texture2D* GetD3DTextureCube( const TextureRef &ref )
	{
		GPUAPI_ASSERT( !(ref && (!GetDeviceData().m_Textures.Data(ref).m_pTexture || TEXTYPE_CUBE!=GetDeviceData().m_Textures.Data(ref).m_Desc.type)) );
		return ref ? static_cast<ID3D11Texture2D*>( GetDeviceData().m_Textures.Data(ref).m_pTexture ) : NULL;
	}

	ID3D11RenderTargetView* GetD3DRenderTargetView( const TextureRef &ref, int sliceID/*=-1*/ )
	{
		//dex++: added slice support
		const GpuApi::STextureData& texData = GetDeviceData().m_Textures.Data(ref);
		if ( ref )
		{
			if ( sliceID == -1 )
			{
				GPUAPI_ASSERT( texData.m_pRenderTargetView != NULL );
				return texData.m_pRenderTargetView;
			}
			else if ( texData.m_pp2DRenderTargetViewPerMipLevel )
			{
				GPUAPI_ASSERT( TEXTYPE_2D == texData.m_Desc.type );
				GPUAPI_ASSERT( (Uint32)sliceID < texData.m_Desc.initLevels );
				return texData.m_pp2DRenderTargetViewPerMipLevel[ sliceID ];
			}
			else if ( 0 == sliceID )
			{
				GPUAPI_ASSERT( texData.m_pRenderTargetView != NULL );
				return texData.m_pRenderTargetView;
			}
			else
			{
				if ( texData.m_pRenderTargetViewsArray == nullptr )
				{
					GPUAPI_HALT( "No rendertarget view array in array texture" );
					return nullptr;
				}

				GPUAPI_ASSERT( sliceID < texData.m_RenderTargetViewsArraySize );
				GPUAPI_ASSERT( texData.m_pRenderTargetViewsArray[sliceID] != NULL, TXT( "Rendertarget view not created" ) );
				return texData.m_pRenderTargetViewsArray[sliceID];
			}
		}
		else
		{
			return NULL;
		}
		//dex--
	}

	ID3D11UnorderedAccessView* GetD3DUnorderedAccessView( const TextureRef &ref )
	{
		GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Textures.Data(ref).m_pUnorderedAccessView) );
		return ref ? GetDeviceData().m_Textures.Data(ref).m_pUnorderedAccessView : NULL;
	}

	ID3D11DepthStencilView* GetD3DDepthStencilView( const TextureRef &ref, int sliceID/*=-1*/, Bool isReadOnly/*=false*/ )
	{
		//dex++: added slice support
		const GpuApi::STextureData& texData = GetDeviceData().m_Textures.Data(ref);
		if ( ref )
		{
			if ( sliceID == -1 )
			{
				GPUAPI_ASSERT( NULL != texData.m_pDepthStencilView );
				GPUAPI_ASSERT( NULL != texData.m_pDepthStencilViewReadOnly );

				return isReadOnly ? texData.m_pDepthStencilViewReadOnly : texData.m_pDepthStencilView;
			}
			else
			{
				GPUAPI_ASSERT( !isReadOnly, TXT( "Only non-sliced readonly dsv supported" ) );

				if ( texData.m_pDepthStencilViewsArray == nullptr )
				{
					GPUAPI_HALT( "No depthstencil view array in array texture" );
					return nullptr;
				}

				//GPUAPI_ASSERT( sliceID < texData.m_depthTargetViewsArraySize );
				GPUAPI_ASSERT( texData.m_pDepthStencilViewsArray[sliceID] != NULL, TXT( "Stencil view not created" ) );
				GPUAPI_ASSERT( sliceID >= 0 && sliceID < (int)texData.m_Desc.sliceNum );
				return texData.m_pDepthStencilViewsArray[sliceID];
			}
		}
		else
		{
			return NULL;
		}
		//dex--
	}

	ID3D11ShaderResourceView* GetD3DShaderResourceView( const TextureRef &ref )
	{
		GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Textures.Data(ref).m_pShaderResourceView) );
		return ref ? GetDeviceData().m_Textures.Data(ref).m_pShaderResourceView : NULL;
	}

	// ----------------------------------------------------------------------

	void AddRef( const TextureRef &texture )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(texture) );
		GetDeviceData().m_Textures.IncRefCount( texture );
	}

	Int32 Release( const TextureRef &texture )
	{
#if WAITING_FOR_DEX_TO_FIX_GLOBAL_TEXTURES
		if ( GetDeviceData().m_DeviceShutDone )
		{
			GPUAPI_ASSERT( !GetDeviceData().m_Textures.IsInUse(texture) );
			return;
		}
#endif

		//

		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(texture) );

		TextureRef parentRef;
		Int32 refCount = 0;
		// Release given texture
		{
			SDeviceData &dd = GetDeviceData();
			
			// Release and optionally destroy
			GPUAPI_ASSERT( texture );
			GPUAPI_ASSERT( dd.m_Textures.GetRefCount( texture ) >= 1 );

			refCount = dd.m_Textures.DecRefCount( texture );
			if ( 0 == refCount )
			{	
				STextureData &data = dd.m_Textures.Data( texture );

				// Take ownership over parent reference (will be released later)
				parentRef = data.parentTexId;
				data.parentTexId = TextureRef::Null();

				// TODO: stats shouldn't be handled on final, but they're used for budeting
				// Count memory usage
				if ( Utils::ShouldCountTextureStats( data.m_Desc ) )
				{
					const Int32 textureMemory = CalcTextureSize( data.m_Desc );
					dd.m_TextureStats.RemoveTexture( textureMemory, data.m_Group );
				}

#ifdef RED_PLATFORM_DURANGO
				if ( data.m_inplaceMemoryRegion.IsValid() || data.m_textureBaseMemory != nullptr )
				{
					// queue for destruction once we're sure the GPU has finished with it
					GpuApi::QueueForDestroy( texture );
				}
				else
#endif
				{
					GpuApi::Destroy( texture );
				}
			}
		}

		// Release parent reference
		if ( parentRef )
		{
			Release( parentRef );
			parentRef = TextureRef::Null();
		}

		return refCount;
	}


	void Destroy( const TextureRef& ref )
	{
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_Textures.GetRefCount( ref ) == 0 );
		STextureData& data = dd.m_Textures.Data( ref );

#ifdef RED_PLATFORM_DURANGO
		// We shouldn't be destroying textures with placement memory that are still pending in a command queue.
		if ( data.m_inplaceMemoryRegion.IsValid() || data.m_textureBaseMemory )
		{
			RED_FATAL_ASSERT( !((ID3D11DeviceX*)GetDevice())->IsResourcePending( data.m_pTexture ), "About to destroy a texture in use?!" );
		}
#endif

#if MICROSOFT_ATG_DYNAMIC_SCALING
		if(data.m_pDynamicScalingData)
		{
			for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
			{
				SAFE_RELEASE( data.m_pDynamicScalingData->m_pTexture[i] );
				SAFE_RELEASE( data.m_pDynamicScalingData->m_pShaderResourceView[i] );
			}
			data.m_pTexture = nullptr;
			data.m_pShaderResourceView = nullptr;
		}
		else
#endif
		{
			SAFE_RELEASE( data.m_pTexture );
			SAFE_RELEASE( data.m_pShaderResourceView );
		}
		if ( data.m_ppTex2DShaderResourceViewPerMipLevel )
		{
#if MICROSOFT_ATG_DYNAMIC_SCALING
			if(data.m_pDynamicScalingData)
			{
				for ( Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i )
				{
					for ( Uint32 j=0; j<DYNAMIC_SCALING_MAX_MIPS; ++j )
					{
						SAFE_RELEASE( data.m_pDynamicScalingData->m_ppTex2DShaderResourceViewPerMipLevel[i][j] );
					}
				}
			}
			else
#endif
			{
				const Int32 num = data.m_Desc.initLevels;
				for ( Int32 i=0; i<num; ++i )
				{
					SAFE_RELEASE( data.m_ppTex2DShaderResourceViewPerMipLevel[i] );
				}
			}
			delete[] data.m_ppTex2DShaderResourceViewPerMipLevel;
			data.m_ppTex2DShaderResourceViewPerMipLevel = nullptr;
		}

		if ( data.m_ppCubeShaderResourceViewPerMipLevel )
		{
			const Int32 num = data.m_Desc.initLevels * data.m_Desc.sliceNum;
			for ( Int32 i=0; i<num; ++i )
			{
				SAFE_RELEASE( data.m_ppCubeShaderResourceViewPerMipLevel[i] );
			}

			delete[] data.m_ppCubeShaderResourceViewPerMipLevel;
			data.m_ppCubeShaderResourceViewPerMipLevel = nullptr;
		}

		if ( data.m_ppCubeShaderResourceViewPerMipFace )
		{
			const Int32 num = 6 * data.m_Desc.initLevels * data.m_Desc.sliceNum;
			for ( Int32 i=0; i<num; ++i )
			{
				SAFE_RELEASE( data.m_ppCubeShaderResourceViewPerMipFace[i] );
			}

			delete[] data.m_ppCubeShaderResourceViewPerMipFace;
			data.m_ppCubeShaderResourceViewPerMipFace = nullptr;
		}
#if MICROSOFT_ATG_DYNAMIC_SCALING
		if(data.m_pDynamicScalingData)
		{
			for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
			{
				SAFE_RELEASE( data.m_pDynamicScalingData->m_pUnorderedAccessView[i] );
				SAFE_RELEASE( data.m_pDynamicScalingData->m_pRenderTargetView[i] );
				SAFE_RELEASE( data.m_pDynamicScalingData->m_pDepthStencilView[i] );
				SAFE_RELEASE( data.m_pDynamicScalingData->m_pDepthStencilViewReadOnly[i] );
				SAFE_RELEASE( data.m_pDynamicScalingData->m_pStencilShaderResourceView[i] );
			}
			data.m_pUnorderedAccessView = nullptr;
			data.m_pRenderTargetView = nullptr;
			data.m_pDepthStencilView = nullptr;
			data.m_pDepthStencilViewReadOnly = nullptr;
			data.m_pStencilShaderResourceView = nullptr;
		}
		else
#endif
		{
			SAFE_RELEASE( data.m_pUnorderedAccessView );
			SAFE_RELEASE( data.m_pRenderTargetView );
			SAFE_RELEASE( data.m_pDepthStencilView );
			SAFE_RELEASE( data.m_pDepthStencilViewReadOnly );
			SAFE_RELEASE( data.m_pStencilShaderResourceView );
		}
		// release renderTargetViewArray related resources
		{
			GPUAPI_ASSERT( (data.m_RenderTargetViewsArraySize > 0) == (nullptr != data.m_pRenderTargetViewsArray) );
#if MICROSOFT_ATG_DYNAMIC_SCALING
			if(data.m_pDynamicScalingData)
			{
				for ( Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i )
				{
					for ( Uint32 j=0; j<DYNAMIC_SCALING_MAXSLICES; ++j )
					{
						SAFE_RELEASE( data.m_pDynamicScalingData->m_pRenderTargetViewsArray[i][j] );
					}
				}
			}
			else
#endif
			{
				for ( Uint32 i=0; i<data.m_RenderTargetViewsArraySize; ++i )
				{
					SAFE_RELEASE( data.m_pRenderTargetViewsArray[i] );
				}
			}
			delete[] data.m_pRenderTargetViewsArray;
			data.m_pRenderTargetViewsArray = nullptr;
			data.m_RenderTargetViewsArraySize = 0;
		}

		// release depthTargetViewArray related resources
		{
			GPUAPI_ASSERT( (data.m_DepthTargetViewsArraySize > 0) == (nullptr != data.m_pDepthStencilViewsArray) );

#if MICROSOFT_ATG_DYNAMIC_SCALING
			if(data.m_pDynamicScalingData)
			{
				for ( Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i )
				{
					for ( Uint32 j=0; j<DYNAMIC_SCALING_MAXSLICES; ++j )
					{
						SAFE_RELEASE( data.m_pDynamicScalingData->m_pDepthStencilViewsArray[i][j] );
					}
				}
			}
			else
#endif
			{
				for ( Uint32 i=0; i<data.m_DepthTargetViewsArraySize; ++i )
				{
					SAFE_RELEASE( data.m_pDepthStencilViewsArray[i] );
				}
			}
			delete[] data.m_pDepthStencilViewsArray;
			data.m_pDepthStencilViewsArray = nullptr;
			data.m_DepthTargetViewsArraySize = 0;
		}

		//
		if ( data.m_pp2DRenderTargetViewPerMipLevel )
		{
			GPUAPI_ASSERT( TEXTYPE_2D == data.m_Desc.type && data.m_Desc.initLevels > 1 );
#if MICROSOFT_ATG_DYNAMIC_SCALING
			if(data.m_pDynamicScalingData)
			{
				for ( Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i )
				{
					for ( Uint32 j=0; j<DYNAMIC_SCALING_MAX_MIPS; ++j )
					{
						SAFE_RELEASE( data.m_pDynamicScalingData->m_pp2DRenderTargetViewPerMipLevel[i][j] );
					}
				}
			}
			else
#endif
			{
				for ( Uint32 i=0; i<data.m_Desc.initLevels; ++i )
				{
					SAFE_RELEASE( data.m_pp2DRenderTargetViewPerMipLevel[i] );
				}
			}
			delete[] data.m_pp2DRenderTargetViewPerMipLevel;
			data.m_pp2DRenderTargetViewPerMipLevel = nullptr;
		}


#ifdef RED_PLATFORM_DURANGO
		// Free the texture memory
		if ( data.m_inplaceMemoryRegion.IsValid() )
		{
			GPUAPI_ASSERT( data.m_Desc.IsInPlace() );
			ReleaseInPlaceMemoryRegion( data.m_Desc.inPlaceType, data.m_inplaceMemoryRegion );
		}
# if MICROSOFT_ATG_DYNAMIC_SCALING
		else if ( data.m_partialResidencyAllocation )
		{
			VirtualFree( data.m_textureBaseMemory, 0, MEM_RELEASE );

			data.m_textureBaseMemory = NULL;

			data.m_pDynamicScalingData->Remove();

			VirtualFree( data.m_pDynamicScalingData, 0, MEM_RELEASE );

			data.m_pDynamicScalingData = NULL;
		}
# endif
		else if ( data.m_textureBaseMemory )
		{
			GPU_API_FREE( GpuMemoryPool_Textures, MC_TextureData, data.m_textureBaseMemory );
		}
		data.m_textureBaseMemory = nullptr;
		data.m_inplaceMemoryRegion = nullptr;
#endif

		// Destroy texture
		dd.m_Textures.Destroy( ref );
	}


	void GetTextureDesc( const TextureRef &ref, TextureDesc &outDesc )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		outDesc = GetDeviceData().m_Textures.Data(ref).m_Desc;
	}

	const TextureDesc& GetTextureDesc( const TextureRef &ref )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		return GetDeviceData().m_Textures.Data(ref).m_Desc;
	}

	void SetTextureDebugPath( const TextureRef &ref, const char* debugPath )
	{
#ifdef GPU_API_DEBUG_PATH
		if (!ref.isNull())
		{
			GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
			STextureData &data = GetDeviceData().m_Textures.Data(ref);
			Red::System::StringCopy( data.m_debugPath, debugPath, ARRAYSIZE(data.m_debugPath) );

			Uint32 pathLen = ( Uint32 )Red::System::StringLength( data.m_debugPath );

			// Destroy previous data
			data.m_pTexture->SetPrivateData( WKPDID_D3DDebugObjectName, 0, NULL );
			if (data.m_pShaderResourceView)
			{
				data.m_pShaderResourceView->SetPrivateData( WKPDID_D3DDebugObjectName, 0, NULL );
			}

			if (pathLen > 0)
			{
				data.m_pTexture->SetPrivateData( WKPDID_D3DDebugObjectName, pathLen, data.m_debugPath );
				if (data.m_pShaderResourceView)
				{
					data.m_pShaderResourceView->SetPrivateData( WKPDID_D3DDebugObjectName, pathLen, data.m_debugPath );
				}
			}
		}
#endif
	}

	void GetTextureLevelDesc( const TextureRef &ref, Uint16 level, TextureLevelDesc &outDesc )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		const STextureData &data = GetDeviceData().m_Textures.Data(ref);

		outDesc.width	= CalculateTextureMipDimension( data.m_Desc.width, ( Uint8 )level, data.m_Desc.format );
		outDesc.height	= CalculateTextureMipDimension( data.m_Desc.height, ( Uint8 )level, data.m_Desc.format );


		//GPUAPI_ASSERT( level < data.m_Desc.CalcTargetLevels() );

		//// Get d3d texture level desc
		//D3DSURFACE_DESC d3dDesc;
		//switch ( data.m_Desc.type )
		//{
		//case TEXTYPE_2D:
		//	{
		//		GPUAPI_ASSERT( data.m_pTexture || data.m_pSurface );
		//		if ( data.m_pTexture )
		//		{
		//			static_cast<IDirect3DTexture9*>(data.m_pTexture)->GetLevelDesc( level, &d3dDesc );
		//		}
		//		else
		//		{
		//			data.m_pSurface->GetDesc( &d3dDesc );
		//		}
		//	}
		//	break;

		//case TEXTYPE_CUBE:
		//	{
		//		GPUAPI_ASSERT( data.m_pTexture );
		//		static_cast<IDirect3DCubeTexture9*>(data.m_pTexture)->GetLevelDesc( level, &d3dDesc );
		//	}
		//	break;

		//default:
		//	GPUAPI_HALT( TXT( "invalid" ) );
		//	return; // keep compiler happy (uninit variable)
		//}

		//// Get data
		//GPUAPI_ASSERT( d3dDesc.Width <= data.m_Desc.width && d3dDesc.Height <= data.m_Desc.height );
		//outDesc.width = d3dDesc.Width;
		//outDesc.height = d3dDesc.Height;		
	}

	TextureLevelDesc GetTextureLevelDesc( const TextureRef &ref, Uint16 level )
	{
		TextureLevelDesc desc;
		GetTextureLevelDesc( ref, level, desc );
		return desc;
	}

	const TextureStats *GetTextureStats()
	{
		return &GpuApi::GetDeviceData().m_TextureStats;
	}

#ifndef NO_TEXTURE_IMPORT
	Bool ImportTexture( const wchar_t* importPath, GpuApi::eTextureImportFormat format, /*out*/void*& dst, /*out*/Uint32& rowPitch, /*out*/Uint32& depthPitch, /*out*/TextureDesc& desc )
	{
		if ( !IsInit() )
		{
			return false;
		}

		// Read bitmap info
		DirectX::TexMetadata metadata;
		Red::System::MemorySet( &metadata, 0, sizeof( metadata ) );

		HRESULT hr = S_FALSE;
		switch ( format )
		{
		case TIF_DDS:
			hr = DirectX::GetMetadataFromDDSFile( importPath, DirectX::DDS_FLAGS_NONE, metadata );		
			break;
		case TIF_TGA:
			hr = DirectX::GetMetadataFromTGAFile( importPath, metadata );
			break;
		case TIF_WIC:
			// PNGs (and possibly others?) can be imported in BGR order, but WIC_FLAGS_FORCE_RGB forces it into RGB, which is good for us!
			hr = DirectX::GetMetadataFromWICFile( importPath, DirectX::WIC_FLAGS_FORCE_RGB, metadata );
			break;
		}
		
		if ( FAILED( hr ) )
		{			
			GPUAPI_HALT( "Unknown texture import format!" );
			return false;
		}

		// Texture size should be power of 2 !
		
		Bool isPow2Width = Red::Math::IsPow2( static_cast< Int32 >( metadata.width ) );
		Bool isPow2Height = Red::Math::IsPow2( static_cast< Int32 >( metadata.height ) );

		if ( format == TIF_DDS )
		{
			if ( metadata.width < 1 || metadata.height < 1 || (metadata.width & 3) != 0 || (metadata.height & 3 ) != 0 )
			{
				GPUAPI_HALT( "DDS Texture size should be a multiple of four" );
				return false;
			}
		}
		else if ( !isPow2Width || !isPow2Height )
		{
			GPUAPI_HALT( "Texture size should be power of two" );
			return false;
		}

		DirectX::ScratchImage image;
		hr = S_FALSE;
		switch ( format )
		{
		case TIF_DDS:
			hr = DirectX::LoadFromDDSFile( importPath, DirectX::DDS_FLAGS_NONE, NULL, image );
			break;
		case TIF_TGA:
			hr = DirectX::LoadFromTGAFile( importPath, NULL, image );
			break;
		case TIF_WIC:
			hr = DirectX::LoadFromWICFile( importPath, DirectX::WIC_FLAGS_FORCE_RGB, NULL, image );
			break;
		}
		
		if ( FAILED( hr ) )
		{
			GPUAPI_HALT( "Unable to create texture from file" );
			return false;
		}

		const DirectX::Image* loadedImage = image.GetImage(0, 0, 0);
		desc.width	= static_cast< GpuApi::Uint32 >( metadata.width );
		desc.height = static_cast< GpuApi::Uint32 >( metadata.height );
		desc.format = Map( metadata.format );
		rowPitch	= static_cast< GpuApi::Uint32 >( loadedImage->rowPitch );
		depthPitch	= static_cast< GpuApi::Uint32 >( loadedImage->slicePitch );

		GPUAPI_ASSERT( !dst, TXT( "Destination memory already allocated" ) );
		dst = GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, loadedImage->slicePitch, 16 );
		GPUAPI_ASSERT( dst, TXT( "Destination memory can't be allocated" ) );

		Red::System::MemoryCopy( dst, loadedImage->pixels, static_cast< GpuApi::Uint32 >( loadedImage->slicePitch ) );

		return true;
	}
#endif


	TextureRef CreateTextureAlias( const TextureDesc &desc, const TextureRef& texture )
	{
		GPUAPI_HALT("NOT IMPLEMENTED!"); 
		RED_UNUSED (desc);
		RED_UNUSED (texture);
		return TextureRef::Null();
	}


	TextureRef CreateTexture( const TextureDesc &desc, eTextureGroup group, const TextureInitData* initData /*= nullptr*/ )
	{
		if ( !IsInit() )
		{
			// ace_fix!!!!! przywrocic ten fatal error (teraz jest bo niektore shity sa tak wrzucone w silniku)		
			// GPUAPI_HALT( TXT( "Not init during attempt to create texture" ) );
			return TextureRef::Null();
		}

		if ( !IsDescSupported( desc ) )
		{
			GPUAPI_HALT( "Unsupported TextureDesc. Failed to create texture." );
			return TextureRef::Null();
		}

		// Creating an immutable texture requires initial data.
		if ( ( desc.usage & TEXUSAGE_Immutable ) != 0 )
		{
			if ( initData == nullptr )
			{
				GPUAPI_HALT( "Creating immutable texture without any initial data." );
				return TextureRef::Null();
			}
		}

		if ( desc.IsInPlace() )
		{
			if ( ( desc.usage & TEXUSAGE_Immutable ) == 0 )
			{
				GPUAPI_HALT( "In-place texture creation is only supported for immutable textures." );
				return TextureRef::Null();
			}
		}


		// Create d3d texture
		ID3D11Resource* d3dTex = nullptr;
#ifdef RED_PLATFORM_DURANGO
		void* textureMemory = nullptr;
		Red::MemoryFramework::MemoryRegionHandle inplaceTextureMemory = nullptr;
# if MICROSOFT_ATG_DYNAMIC_SCALING
		bool partialResidencyAllocation;
		SDynamicScalingTextureData *pDynamicScalingData = nullptr;
		Utils::TextureFactoryD3D( desc, initData, d3dTex, textureMemory, inplaceTextureMemory, pDynamicScalingData, partialResidencyAllocation );
# else
		Utils::TextureFactoryD3D( desc, initData, d3dTex, textureMemory, inplaceTextureMemory );
# endif
#else
		Utils::TextureFactoryD3D( desc, initData, d3dTex );
#endif

		if ( d3dTex == nullptr )
		{
#ifdef RED_PLATFORM_DURANGO
			GPUAPI_ASSERT( textureMemory == nullptr, TXT("TextureFactoryD3D failed, but we got textureMemory?") );
#endif
			return TextureRef::Null();
		}

		// Allocate resource
		SDeviceData &dd = GetDeviceData();
		Uint32 texId = dd.m_Textures.Create( 1 );
		if ( !texId )
		{
			GPUAPI_HALT( "Failed to reserve TextureRef" );
			SAFE_RELEASE( d3dTex );
#ifdef RED_PLATFORM_DURANGO
			if ( !desc.IsInPlace() )
			{
				GPU_API_FREE( GpuMemoryPool_Textures, MC_TextureData, textureMemory );
			}
#endif
			return TextureRef::Null();
		}

		// Init resource
		GpuApi::STextureData &data = dd.m_Textures.Data( texId );
		data.m_pTexture = d3dTex;
		data.m_Desc = desc;
		data.m_Group = group;
#ifdef RED_PLATFORM_DURANGO
		data.m_textureBaseMemory = textureMemory;
		data.m_inplaceMemoryRegion = inplaceTextureMemory;
# if MICROSOFT_ATG_DYNAMIC_SCALING
		data.m_pDynamicScalingData = pDynamicScalingData;
		data.m_partialResidencyAllocation = partialResidencyAllocation;
# endif
#endif
		// Initialize handle
		GPUAPI_ASSERT( texId && dd.m_Textures.GetRefCount(texId) );
		TextureRef texRef( texId );

		if ( !Utils::CreateTextureViews( data ) )
		{
			SafeRelease( texRef );
			return TextureRef::Null();
		}

		// TODO: stats shouldn't be handled on final, but they're used for budeting
		// If this is a simple texture, count used memory
		if ( Utils::ShouldCountTextureStats( data.m_Desc ) )
		{
			const Uint32 textureSize = CalcTextureSize( texRef );
			dd.m_TextureStats.AddTexture( textureSize, group );
		}

#ifdef RED_PLATFORM_WINPC
		// Since we can't use in-place creation on PC, we need to free the memory, so the caller doesn't need to worry about the
		// differences between platforms.
		if ( desc.IsInPlace() && initData != nullptr && initData->m_isCooked && initData->m_cookedData.IsValid() )
		{
			ReleaseInPlaceMemoryRegion( INPLACE_Texture, initData->m_cookedData );
		}
#endif

#if MICROSOFT_ATG_DYNAMIC_SCALING
		if(data.m_pDynamicScalingData)
		{
			data.m_pDynamicScalingData->m_pTextureData = &data;
			data.m_pDynamicScalingData->UpdateResolution();
		}
#endif
		if (initData)
		{
			if (initData->m_isCooked)
			{
				SetTextureDebugPath( texRef, "Cooked" );
			}
			else
			{
				SetTextureDebugPath( texRef, "NonCooked" );
			}
		}
		else
		{
			SetTextureDebugPath( texRef, "NoInitData" );
		}

		// Return handle
		return texRef;
	}


	void GetTextureDescFromMemoryFile( const void *memoryFile, Uint32 fileSize, TextureDesc *outDesc /* = NULL */ )
	{
		if ( !IsInit() )
		{
			// ace_fix!!!!! przywrocic ten fatal error (teraz jest bo niektore shity sa tak wrzucone w silniku)		
			// GPUAPI_HALT( TXT( "Not init during attempt to create texture" ) );
			return;
		}
#ifndef RED_PLATFORM_CONSOLE
		bool isCube = false;
		TextureDesc localDesc;
		{
			DirectX::TexMetadata metaData;
			HRESULT res = DirectX::GetMetadataFromDDSMemory(memoryFile, fileSize, DirectX::DDS_FLAGS_NONE, metaData);
			RED_UNUSED(res);

			localDesc.initLevels = static_cast< GpuApi::Uint16 >( metaData.mipLevels );
			localDesc.format     = GpuApi::Map( metaData.format );
			localDesc.height     = static_cast< GpuApi::Uint32 >( metaData.height );
			localDesc.width      = static_cast< GpuApi::Uint32 >( metaData.width );
			localDesc.type       = GpuApi::TEXTYPE_2D;
			localDesc.usage      = GpuApi::TEXUSAGE_Samplable;
			isCube				 = ((DirectX::TEX_MISC_TEXTURECUBE & metaData.miscFlags) != 0);
		}

		if ( !IsDescSupported( localDesc ) )
		{
			return;
		}
		if ( outDesc )
		{
			*outDesc = localDesc;
		}
#endif
	}

	TextureRef CreateTextureFromMemoryFile( const void *memoryFile, Uint32 fileSize, eTextureGroup group, TextureDesc *outDesc )
	{
		if ( !IsInit() )
		{
			// ace_fix!!!!! przywrocic ten fatal error (teraz jest bo niektore shity sa tak wrzucone w silniku)		
			// GPUAPI_HALT( TXT( "Not init during attempt to create texture" ) );
			return TextureRef::Null();
		}

		SDeviceData &dd = GetDeviceData();
		TextureDesc localDesc;
		ID3D11Resource	*d3dTexBase	= NULL;
#ifndef RED_PLATFORM_CONSOLE
		bool isCube = false;
		{
			DirectX::TexMetadata metaData;
			HRESULT res = DirectX::GetMetadataFromDDSMemory(memoryFile, fileSize, DirectX::DDS_FLAGS_NONE, metaData);
			RED_UNUSED(res);

			localDesc.initLevels = static_cast< GpuApi::Uint16 >( metaData.mipLevels );
			localDesc.format     = GpuApi::Map( metaData.format );
			localDesc.height     = static_cast< GpuApi::Uint32 >( metaData.height );
			localDesc.width      = static_cast< GpuApi::Uint32 >( metaData.width );
			localDesc.type       = GpuApi::TEXTYPE_2D;
			localDesc.usage      = GpuApi::TEXUSAGE_Samplable;
			isCube				 = ((DirectX::TEX_MISC_TEXTURECUBE & metaData.miscFlags) != 0);
		}
		if ( !IsDescSupported( localDesc ) )
		{
			return TextureRef::Null();
		}
		if ( outDesc )
		{
			*outDesc = localDesc;
		}

		// Create d3d texture
		ID3D11Resource		*d3dTex	= NULL;

		DirectX::TexMetadata metaData;
		DirectX::ScratchImage* scratchImage = new DirectX::ScratchImage(); //allocate on the heap
		HRESULT texRes = DirectX::LoadFromDDSMemory( memoryFile, fileSize, DirectX::DDS_FLAGS_NONE, &metaData, *scratchImage);
		if ( !SUCCEEDED( texRes ) )
		{
			GPUAPI_ASSERT( d3dTex == NULL );
		}

		DirectX::CreateTexture(dd.m_pDevice, scratchImage->GetImages(), scratchImage->GetImageCount(), metaData, &d3dTex);
		scratchImage->Release();
		scratchImage = NULL;

		d3dTexBase = d3dTex;
#else
		HRESULT textureResult = CreateDDSTextureFromMemory( dd.m_pDevice, (Uint8*)memoryFile, fileSize, &d3dTexBase, nullptr );

		D3D11_RESOURCE_DIMENSION dim;
		d3dTexBase->GetType( &dim );
		if ( D3D11_RESOURCE_DIMENSION_TEXTURE2D == dim )
		{
			ID3D11Texture2D* texture2D = (ID3D11Texture2D*)d3dTexBase;
			D3D11_TEXTURE2D_DESC descriptor;
			texture2D->GetDesc(&descriptor);

			localDesc.initLevels = descriptor.MipLevels;
			localDesc.format     = GpuApi::Map( descriptor.Format );
			localDesc.height     = static_cast< GpuApi::Uint32 >( descriptor.Height );
			localDesc.width      = static_cast< GpuApi::Uint32 >( descriptor.Width );
			localDesc.type       = GpuApi::TEXTYPE_2D;
			localDesc.usage      = GpuApi::TEXUSAGE_Samplable;
		}

#endif

		if ( !d3dTexBase )
		{
			return TextureRef::Null();
		}
		
		// Allocate resource
		Uint32 texId = dd.m_Textures.Create( 1 );
		if ( !texId )
		{
			// Release texBase.
			// Other textures are ignored here, because I hold only one reference, in texBase.
			if ( d3dTexBase )
			{
				d3dTexBase->Release();
			}

			return TextureRef::Null();
		}

		// Init resource
		GpuApi::STextureData &data = dd.m_Textures.Data( texId );
		data.m_pTexture = d3dTexBase;
		data.m_Desc = localDesc;
		data.m_Group = group;

		// Initialize handle
		GPUAPI_ASSERT( texId && dd.m_Textures.GetRefCount(texId) );
		TextureRef texRef( texId );

		// If this is a simple texture, count used memory
		if ( Utils::ShouldCountTextureStats( data.m_Desc ) )
		{
			//const Uint32 textureSize = CalcTextureSize( texRef );
			//dd.m_TextureStats.AddTexture( textureSize, group );
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		// HACK DX10 only samplable depth format is D32
		if (localDesc.format == TEXFMT_D24S8)
		{
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else
		{
			srvDesc.Format = MapShaderResourceView( localDesc.format );
		}

		if ( localDesc.type == TEXTYPE_2D )
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = localDesc.initLevels;
		}
		else if ( localDesc.type == TEXTYPE_CUBE )
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.MipLevels = localDesc.initLevels;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = 6;
		}

		HRESULT res = GetDevice()->CreateShaderResourceView( d3dTexBase, &srvDesc, &(data.m_pShaderResourceView) );
		if(res != S_OK)
		{
			GPUAPI_HALT( "DUPA!" );
		}

		// Return handle
		return texRef;
	}

	Uint32 CalcTextureSize( const TextureDesc &texDesc )
	{
		GPUAPI_ASSERT( TEXTYPE_2D == texDesc.type || TEXTYPE_CUBE == texDesc.type || TEXTYPE_ARRAY == texDesc.type, TXT("Unsupported texture type") );

#if defined( RED_PLATFORM_WINPC )
		Uint32 totalPixels = 0;

		// Calculate number of pixels in texture
		// dex_todo: this does not account for alignment, compression, etc....
		const Uint32 levelsCount = texDesc.CalcTargetLevels();
		for ( Uint32 lvl_i=0; lvl_i<levelsCount; ++lvl_i )
		{				
			const Uint32 mipWidth	= CalculateTextureMipDimension( texDesc.width, lvl_i, texDesc.format );
			const Uint32 mipHeight	= CalculateTextureMipDimension( texDesc.height, lvl_i, texDesc.format );
			totalPixels += mipWidth * mipHeight;
		}

		// Cubemap has 6 faces :)
		if ( TEXTYPE_CUBE == texDesc.type )
		{
			totalPixels *= 6;
		}
		if ( TEXTYPE_ARRAY == texDesc.type )
		{
			totalPixels *= texDesc.sliceNum;
		}

		// Calculate 
		const Uint32 totalBits = totalPixels * Utils::GetTextureFormatPixelSize( texDesc.format );
		return totalBits / 8;
#elif defined( RED_PLATFORM_DURANGO )
		XG_RESOURCE_LAYOUT resourceLayout;
		XG_FORMAT format = (XG_FORMAT)Map( texDesc.format ); // XG_FORMAT is identical to DXGI_FORMAT on XB1
		Uint32 bindFlags = 0;
		Uint32 msaaLevel = texDesc.msaaLevel > 0 ? texDesc.msaaLevel : 1;

		if (texDesc.usage & TEXUSAGE_Samplable)
		{
			bindFlags |= XG_BIND_SHADER_RESOURCE;
		}
		if (texDesc.usage & TEXUSAGE_RenderTarget)
		{
			if ( texDesc.msaaLevel < 1 )
			{
				bindFlags |= XG_BIND_RENDER_TARGET;
			}
			else
			{
				bindFlags |= XG_BIND_RENDER_TARGET;
			}
		}
		if (texDesc.usage & TEXUSAGE_DepthStencil)
		{
			bindFlags |= XG_BIND_DEPTH_STENCIL;
		}

		Uint32 cpuAccessFlags = 0;
		if ( texDesc.usage & TEXUSAGE_Dynamic || texDesc.usage & TEXUSAGE_StagingWrite )
		{
			cpuAccessFlags = XG_CPU_ACCESS_WRITE;
		}
		else if ( texDesc.usage & TEXUSAGE_Staging )
		{
			cpuAccessFlags = XG_CPU_ACCESS_READ;
		}

		Uint32 miscFlags = 0;
		if ( texDesc.usage & TEXUSAGE_ESRAMResident )
		{
			miscFlags |= XG_RESOURCE_MISC_ESRAM_RESIDENT;
		}

		XG_USAGE usage = XG_USAGE_DEFAULT;
		if ( texDesc.usage & TEXUSAGE_Dynamic )
		{
			usage = XG_USAGE_DYNAMIC;
		}
		if ( texDesc.usage & TEXUSAGE_Staging )
		{
			usage = XG_USAGE_STAGING;
		}

		XG_TILE_MODE tileMode;
		XG_TILE_MODE stencilTileMode;


		switch ( texDesc.type )
		{
		case TEXTYPE_2D:
		case TEXTYPE_ARRAY:
		case TEXTYPE_CUBE:
			{
				if ( texDesc.usage & TEXUSAGE_DepthStencil )
				{
					XGComputeOptimalDepthStencilTileModes(	format, 
															texDesc.width, 
															texDesc.height, 
															texDesc.sliceNum, 
															msaaLevel, 
															false, 
															&tileMode,
															&stencilTileMode );
				}
				// Creating immutable textures goes through the CopyD3DTextureDescToXG function above, so need to
				// select the same tiling as there.
				else if ( (texDesc.usage & TEXUSAGE_Immutable) )
				{
					// Always use 1D tiling for resource textures. When we get optimal from XG, it can change depending on mips and size,
					// which causes problems if we need to stream in a subset of the mips.
					tileMode = XG_TILE_MODE_1D_THIN;
				}
				else if ( (texDesc.usage & TEXUSAGE_Staging) )
				{
					// Staging textures must have linear tile mode.
					tileMode = XG_TILE_MODE_LINEAR;
				}
				else
				{
					tileMode = XGComputeOptimalTileMode(	XG_RESOURCE_DIMENSION_TEXTURE2D,
												format,
												texDesc.width,
												texDesc.height,
												texDesc.sliceNum,
												msaaLevel,
												bindFlags);
				}

				XG_TEXTURE2D_DESC xgDesc;
				xgDesc.Width = texDesc.width;
				xgDesc.Height = texDesc.height;
				xgDesc.MipLevels = texDesc.initLevels;
				xgDesc.ArraySize = texDesc.sliceNum;
				if( texDesc.type == TEXTYPE_CUBE )
				{
					xgDesc.ArraySize *= 6;
				}
				xgDesc.Format = format;
				xgDesc.SampleDesc.Count = msaaLevel;
				xgDesc.SampleDesc.Quality = 0;
				xgDesc.Usage = usage;
				xgDesc.BindFlags = bindFlags;
				xgDesc.CPUAccessFlags = cpuAccessFlags;
				xgDesc.MiscFlags = 0;
				xgDesc.ESRAMOffsetBytes = 0; //texDesc.esramOffset;
				xgDesc.ESRAMUsageBytes = 0; //texDesc.esramSize;
				xgDesc.TileMode = tileMode;
				xgDesc.Pitch = 0; //CalculateTexturePitch( texDesc.width, texDesc.format );

				HRESULT result;

				XGTextureAddressComputer* computer = nullptr;
				XGCreateTexture2DComputer( &xgDesc, &computer );
				result = computer->GetResourceLayout( &resourceLayout );
				computer->Release();

				/*
				result = XGComputeTexture2DLayout( &xgDesc, &resourceLayout );
				*/
				GPUAPI_MUST_SUCCEED( result );
			}
			break;

		default:
			GPUAPI_HALT( "Unsupported texture type" );
			return 0;
		}

		Uint32 size = (Uint32)resourceLayout.SizeBytes;

		//if ( resourceLayout.Planes > 1 )
		//{
		//	for ( Uint32 i = 0; i < resourceLayout.Planes; ++i )
		//	{
		//		size += resourceLayout.Plane[i].SizeBytes;
		//	}
		//}

		return size;
#else
#error unsupported platform
#endif
	}

	Uint32 CalcTextureSize( const TextureRef &tex )
	{
		if ( tex )
		{
			const TextureDesc &texDesc = GetDeviceData().m_Textures.Data( tex ).m_Desc;
			return CalcTextureSize( texDesc );
		}

		return 0;
	}

	Bool CopyTextureData( const TextureRef& destRef, Uint32 destMipLevel, Uint32 destArraySlice, const TextureRef& srcRef, Uint32 srcMipLevel, Uint32 srcArraySlice )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(srcRef), TXT("Invalid source texture reference") );
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(destRef), TXT("Invalid destination texture reference") );
		if ( !GetDeviceData().m_Textures.IsInUse(srcRef) || !GetDeviceData().m_Textures.IsInUse(destRef) )
		{
			return false;
		}

		const STextureData &srcData = GetDeviceData().m_Textures.Data(srcRef);
		const STextureData &destData = GetDeviceData().m_Textures.Data(destRef);

		GPUAPI_ASSERT( srcData.m_pTexture != nullptr, TXT("Source texture doesn't exist") );
		GPUAPI_ASSERT( destData.m_pTexture != nullptr, TXT("Destination texture doesn't exist") );
		if ( destData.m_pTexture == nullptr ||  srcData.m_pTexture == nullptr )
		{
			return false;
		}

		Uint32 srcLevels = srcData.m_Desc.CalcTargetLevels();
		Uint32 dstLevels = destData.m_Desc.CalcTargetLevels();
		Uint32 srcSlices = srcData.m_Desc.CalcTargetSlices();
		Uint32 dstSlices = destData.m_Desc.CalcTargetSlices();

		GPUAPI_ASSERT( srcMipLevel < srcLevels, TXT("Source mip level out of range: %u >= %u"), srcMipLevel, srcLevels );
		GPUAPI_ASSERT( destMipLevel < dstLevels, TXT("Destination mip level out of range: %u >= %u"), destMipLevel, dstLevels );
		GPUAPI_ASSERT( srcArraySlice < srcSlices, TXT("Source slice out of range: %u >= %u"), srcArraySlice, srcSlices );
		GPUAPI_ASSERT( destArraySlice < dstSlices, TXT("Destination slice out of range: %u >= %u"), destArraySlice, dstSlices );
		if ( srcMipLevel >= srcLevels || destMipLevel >= dstLevels || srcArraySlice >= srcSlices || destArraySlice >= dstSlices )
		{
			return false;
		}

		Uint32 destSubresource = D3D11CalcSubresource(destMipLevel, destArraySlice, destData.m_Desc.initLevels);
		Uint32 srcSubresource = D3D11CalcSubresource(srcMipLevel, srcArraySlice, srcData.m_Desc.initLevels);

		GetDeviceContext()->CopySubresourceRegion( destData.m_pTexture, destSubresource, 0, 0, 0, srcData.m_pTexture, srcSubresource, nullptr );
		return true;
	}

	Bool CopyTextureDataAsync( const TextureRef& destRef, Uint32 destMipLevel, Uint32 destArraySlice, const TextureRef& srcRef, Uint32 srcMipLevel, Uint32 srcArraySlice, void* deferredContext )
	{
		GPUAPI_ASSERT( deferredContext != nullptr, TXT("Null deferred context") );
		if ( deferredContext == nullptr )
		{
			return false;
		}

		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(srcRef), TXT("Invalid source texture reference") );
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(destRef), TXT("Invalid destination texture reference") );
		if ( !GetDeviceData().m_Textures.IsInUse(srcRef) || !GetDeviceData().m_Textures.IsInUse(destRef) )
		{
			return false;
		}

		const STextureData &srcData = GetDeviceData().m_Textures.Data(srcRef);
		const STextureData &destData = GetDeviceData().m_Textures.Data(destRef);

		GPUAPI_ASSERT( srcData.m_pTexture != nullptr, TXT("Source texture doesn't exist") );
		GPUAPI_ASSERT( destData.m_pTexture != nullptr, TXT("Destination texture doesn't exist") );
		if ( destData.m_pTexture == nullptr ||  srcData.m_pTexture == nullptr )
		{
			return false;
		}

		Uint32 srcLevels = srcData.m_Desc.CalcTargetLevels();
		Uint32 dstLevels = destData.m_Desc.CalcTargetLevels();
		Uint32 srcSlices = srcData.m_Desc.CalcTargetSlices();
		Uint32 dstSlices = destData.m_Desc.CalcTargetSlices();

		GPUAPI_ASSERT( srcMipLevel < srcLevels, TXT("Source mip level out of range: %u >= %u"), srcMipLevel, srcLevels );
		GPUAPI_ASSERT( destMipLevel < dstLevels, TXT("Destination mip level out of range: %u >= %u"), destMipLevel, dstLevels );
		GPUAPI_ASSERT( srcArraySlice < srcSlices, TXT("Source slice out of range: %u >= %u"), srcArraySlice, srcSlices );
		GPUAPI_ASSERT( destArraySlice < dstSlices, TXT("Destination slice out of range: %u >= %u"), destArraySlice, dstSlices );
		if ( srcMipLevel >= srcLevels || destMipLevel >= dstLevels || srcArraySlice >= srcSlices || destArraySlice >= dstSlices )
		{
			return false;
		}

		Uint32 destSubresource = D3D11CalcSubresource(destMipLevel, destArraySlice, destData.m_Desc.initLevels);
		Uint32 srcSubresource = D3D11CalcSubresource(srcMipLevel, srcArraySlice, srcData.m_Desc.initLevels);

		((ID3D11DeviceContext*)deferredContext)->CopySubresourceRegion( destData.m_pTexture, destSubresource, 0, 0, 0, srcData.m_pTexture, srcSubresource, nullptr );
		return true;
	}


	Bool CopyRect( const TextureRef &sourceRef, const Rect& sourceRect, Uint32 sourceArraySlice, const TextureRef &destRef, Uint32 destX, Uint32 destY, Uint32 destArraySlice )
	{
		SDeviceData &dd = GetDeviceData();

		const TextureDesc& sourceDesc = dd.m_Textures.Data( sourceRef ).m_Desc;
		const TextureDesc& destDesc = dd.m_Textures.Data( destRef ).m_Desc;

		GPUAPI_ASSERT( sourceArraySlice < sourceDesc.CalcTargetSlices(), TXT("Invalid source array slice: %u"), sourceArraySlice );
		GPUAPI_ASSERT( destArraySlice < destDesc.CalcTargetSlices(), TXT("Invalid destination array slice: %u"), destArraySlice );

		Bool isFullSource = sourceRect.left == 0 && sourceRect.top == 0 && sourceRect.right == (Int32)sourceDesc.width && sourceRect.bottom == (Int32)sourceDesc.height;
		Bool isFullDest = destX == 0 && destY == 0 && sourceRect.right == (Int32)destDesc.width && sourceRect.bottom == (Int32)destDesc.height;

		if (   sourceDesc.format != destDesc.format									// Formats match
			|| sourceDesc.msaaLevel != destDesc.msaaLevel							// Multisample levels match
			|| ( ( !isFullSource || !isFullDest ) && sourceDesc.msaaLevel > 0 )		// If multisampled, must copy entire texture
			|| sourceRect.left < 0 || sourceRect.top < 0 || sourceRect.right > (Int32)sourceDesc.width || sourceRect.bottom > (Int32)sourceDesc.height	// Must be within texture
			|| destX + (sourceRect.right - sourceRect.left) > destDesc.width || destY + (sourceRect.bottom - sourceRect.top) > destDesc.height
			)
		{
			// Can't copy here, caller should use a fallback.
			return false;
		}

		D3D11_BOX sourceBox;
		sourceBox.left = sourceRect.left;
		sourceBox.top = sourceRect.top;
		sourceBox.back = 1;
		sourceBox.right = sourceRect.right;
		sourceBox.bottom = sourceRect.bottom;
		sourceBox.front = 0;

		STextureData& destTex = dd.m_Textures.Data(destRef);

		//this is a failsafe, it shouldn't happen but something is wrong with resizing (the whole viewport system is a fuckup) and it happens
		// TODO : probably not actually needed, since we check for off-texture rectangles above?
		if ( sourceRect.right - sourceRect.left > static_cast<Int64>(destTex.m_Desc.width) )
		{
			sourceBox.right = sourceBox.left + destTex.m_Desc.width;
		}
		if ( sourceRect.bottom - sourceRect.top > static_cast<Int64>(destTex.m_Desc.height) )
		{
			sourceBox.bottom = sourceBox.top + destTex.m_Desc.height;
		}

		D3D11_BOX* boxPtr = isFullSource ? nullptr : &sourceBox;
		Uint32 destSubresource = D3D11CalcSubresource(0, destArraySlice, destDesc.initLevels);
		Uint32 srcSubresource = D3D11CalcSubresource(0, sourceArraySlice, sourceDesc.initLevels);
		GetDeviceContext()->CopySubresourceRegion(dd.m_Textures.Data(destRef).m_pTexture, destSubresource, destX, destY, 0, dd.m_Textures.Data(sourceRef).m_pTexture, srcSubresource, boxPtr);

		return true;
	}

	void CopyTextureDataDMA( const TextureRef& destRef, Uint32 destMipLevel, Uint32 destArraySlice, const TextureRef& srcRef, Uint32 srcMipLevel, Uint32 srcArraySlice )
	{
#ifdef RED_PLATFORM_DURANGO
		SDeviceData &dd = GetDeviceData();

		// DMA out from ESRAM
		GPUAPI_ASSERT( dd.m_Textures.Data( srcRef ).m_Desc.usage & TEXUSAGE_ESRAMResident && !(dd.m_Textures.Data( destRef ).m_Desc.usage & TEXUSAGE_ESRAMResident), TXT("DMA copy only available from ESRAM to DRAM") );

		const STextureData &srcData = GetDeviceData().m_Textures.Data(srcRef);
		const STextureData &destData = GetDeviceData().m_Textures.Data(destRef);

		ID3DResource* sourceTexture = srcData.m_pTexture;
		ID3DResource* destTexture = destData.m_pTexture;

		Uint32 srcSubresource = D3D11CalcSubresource(srcMipLevel, srcArraySlice, srcData.m_Desc.initLevels);
		Uint32 destSubresource = D3D11CalcSubresource(destMipLevel, destArraySlice, destData.m_Desc.initLevels);

		// Writeback is typically used for resources that are written by the GPU. Thus we need to ensure that 
		//  the GPU is done using this resource before we start the DMA operation. We insert a GPU fence and wait
		//  on it with the DMA engine. Once we get an API to access the internal write fence for a given resource,  
		//  we can wait on that instead.
		dd.m_pImmediateContext->FlushGpuCaches( sourceTexture );
		Uint64 fence = dd.m_pImmediateContext->InsertFence( 0 );
		dd.m_dmaContext1->InsertWaitOnFence( 0, fence );

		dd.m_dmaContext1->CopySubresourceRegion( destTexture, destSubresource, 0, 0, 0, sourceTexture, srcSubresource, nullptr, 0 );

		// Insert a fence after the copy and kickoff the DMA engine
		// DO NOT REMOVE THIS IF YOU DON'T KNOW WHY IT'S HERE!!!
		// TODO moradin:expose fence through the texture descriptor
		dd.m_dmaContext1->InsertFence( 0 );
#else
		GPUAPI_HALT("DMA is not available on this platform");
#endif
	}

	// A general purpose memory move that leverages DMA requests inserted into draw command buffer
	void DMAMemory( void* dst, void* src, Uint32 numBytes, Bool isBlocking )
	{
#ifdef RED_PLATFORM_DURANGO
		SDeviceData &dd = GetDeviceData();

		GPUAPI_FATAL_ASSERT( ( (Uint8*)src >= (Uint8*)dst + numBytes )
			|| ( (Uint8*)dst >= (Uint8*)src + numBytes ), "Overlapped copy!" );

		GPUAPI_FATAL_ASSERT( ( numBytes & 3 ) == 0, "Size must be multiple of 4: %u", numBytes );

		dd.m_pImmediateContext->CopyMemoryToMemory( dst, src, numBytes );
		dd.m_pImmediateContext->FlushGpuCacheRange( D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, dst, numBytes );

		// Use this to verify that the dma works
		//Red::System::MemoryMove( dst, src, numBytes );
#else
		GPUAPI_HALT("DMA is not available on this platform");
#endif
	}

	void BatchedDMAMemory( SDMABatchElement* batch, Uint32 batchCount, Bool isBlocking )
	{
#ifdef RED_PLATFORM_DURANGO

		GPUAPI_ASSERT( batchCount > 0, TXT("zero length batch is not supported") );

		SDeviceData &dd = GetDeviceData();

		/*

		if ( isBlocking )
		{
			Uint64 afterFence = dd.m_pImmediateContext->InsertFence( 0 );
			while ( dd.m_pDevice->IsFencePending( afterFence ) )
			{
				// block the calling thread until the whole batch is done
				continue;
			}
		}

		for ( Uint32 i = 0; i < batchCount; ++i )
		{
			SDMABatchElement& batchElement = batch[i];
			Red::System::MemoryCopy( batchElement.dst, batchElement.src, batchElement.size );
		}

		/*/

		for ( Uint32 i = 0; i < batchCount; ++i )
		{
			SDMABatchElement& batchElement = batch[i];

			GPUAPI_FATAL_ASSERT( ( (Uint8*)batchElement.src >= (Uint8*)batchElement.dst + batchElement.size )
				|| ( (Uint8*)batchElement.dst >= (Uint8*)batchElement.src + batchElement.size ), "Overlapped copy!" );

			GPUAPI_FATAL_ASSERT( ( batchElement.size & 3 ) == 0, "Size must be multiple of 4: %u", batchElement.size );

			dd.m_pImmediateContext->CopyMemoryToMemory( batchElement.dst, batchElement.src, batchElement.size );

			// Need to flush caches after copying. Needs to be between each copy, because maybe one overlaps with a prior one.
			dd.m_pImmediateContext->FlushGpuCacheRange( D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, batchElement.dst, batchElement.size );
		}

		// this is completely utterly absolutely unreliable shit

		//if ( isBlocking )
		//{
		//	dd.m_pImmediateContext->InsertWaitUntilIdle(0);
		//	Uint64 afterFence = dd.m_pImmediateContext->InsertFence( 0 );
		//	while ( dd.m_pDevice->IsFencePending( afterFence ) )
		//	{
		//		// block the calling thread until the whole batch is done
		//		continue;
		//	}
		//}

		//*/

		//__faststorefence();	 // Ensure any previous stores are flushed before we attempt DMA
#else
		GPUAPI_HALT("DMA is not available on this platform");
#endif
	}

	void* LockLevel( const TextureRef& ref, Uint32 level, Uint32 slice, Uint32 lockFlags, Uint32& outPitch )
	{
		const STextureData &data = GetDeviceData().m_Textures.Data(ref);

		GPUAPI_ASSERT( !(lockFlags & BLF_DoNotWait) || ( data.m_Desc.usage & TEXUSAGE_Staging ), TXT("Do not wait is only possible for staging textures") );

		if (data.m_Desc.usage & (TEXUSAGE_Dynamic | TEXUSAGE_Staging))
		{
			D3D11_MAP		mappedType  = (D3D11_MAP)MapBuffLockFlagsToD3DLockType( lockFlags );
			D3D11_MAP_FLAG	mappedFlags = (D3D11_MAP_FLAG)MapBuffLockFlagsToD3DLockFlags( lockFlags );

			D3D11_MAPPED_SUBRESOURCE mapped;
			mapped.pData = nullptr;
			Uint32 subresource = D3D11CalcSubresource( level, slice, data.m_Desc.initLevels );
			
			HRESULT hr = GetDeviceContext()->Map( data.m_pTexture, subresource, mappedType, mappedFlags, &mapped );

			// We are still waiting for the resource, allowed situation
			if ( ( lockFlags & BLF_DoNotWait ) && ( data.m_Desc.usage & TEXUSAGE_Staging ) && ( hr == DXGI_ERROR_WAS_STILL_DRAWING ) )
			{
				//GPUAPI_ASSERT( !mapped.pData, TXT( "Still waiting for the resource" ) );
				return nullptr;
			}

			if ( FAILED( hr ) )
			{
				GPUAPI_HALT(  "Failed to map texture %0x subresource %d. Error code: %d", data.m_pTexture, subresource, hr );
			}

			outPitch = mapped.RowPitch;
			return mapped.pData;
		}
		else
		{
			return nullptr;
		}
	}

	void UnlockLevel( const TextureRef& ref, Uint32 level, Uint32 slice )
	{
		const STextureData &data = GetDeviceData().m_Textures.Data(ref);
		if (data.m_Desc.usage & (TEXUSAGE_Dynamic | TEXUSAGE_Staging))
		{
			Uint32 subresource = D3D11CalcSubresource( level, slice, data.m_Desc.initLevels );
			GetDeviceContext()->Unmap( data.m_pTexture, subresource );
		}
	}

	RED_INLINE void BindShaderResourceView( Uint32 slot, ID3D11ShaderResourceView* resourceView, eShaderType shaderStage, Bool inPlace = false, const Red::MemoryFramework::MemoryRegion* memoryRegion = nullptr )
	{
#ifdef RED_PLATFORM_DURANGO
		if ( inPlace )
		{
			ID3D11DeviceContextX* contextX = (ID3D11DeviceContextX*)GetDeviceContext();
			
			switch (shaderStage)
			{
			case PixelShader:
				{
					contextX->PSSetPlacementShaderResource( slot, resourceView, memoryRegion->GetRawPtr() );
					break;
				}
			case VertexShader:
				{
					contextX->VSSetPlacementShaderResource( slot, resourceView, memoryRegion->GetRawPtr() );
					break;
				}
			case GeometryShader:
				{
					contextX->GSSetPlacementShaderResource( slot, resourceView, memoryRegion->GetRawPtr() );
					break;
				}
			case HullShader:
				{
					contextX->HSSetPlacementShaderResource( slot, resourceView, memoryRegion->GetRawPtr() );
					break;
				}
			case DomainShader:
				{
					contextX->DSSetPlacementShaderResource( slot, resourceView, memoryRegion->GetRawPtr() );
					break;
				}
			case ComputeShader:
				{
					contextX->CSSetPlacementShaderResource( slot, resourceView, memoryRegion->GetRawPtr() );
					break;
				}
			}
		}
		else
#endif
		{
			switch (shaderStage)
			{
			case PixelShader:
				{
					GetDeviceContext()->PSSetShaderResources( slot, 1, &resourceView );
					break;
				}
			case VertexShader:
				{
					GetDeviceContext()->VSSetShaderResources( slot, 1, &resourceView );
					break;
				}
			case GeometryShader:
				{
					GetDeviceContext()->GSSetShaderResources( slot, 1, &resourceView );
					break;
				}
			case HullShader:
				{
					GetDeviceContext()->HSSetShaderResources( slot, 1, &resourceView );
					break;
				}
			case DomainShader:
				{
					GetDeviceContext()->DSSetShaderResources( slot, 1, &resourceView );
					break;
				}
			case ComputeShader:
				{
					GetDeviceContext()->CSSetShaderResources( slot, 1, &resourceView );
					break;
				}
			}
		}
	}

	void BindTextureCubeMipLevel( Uint32 slot, const TextureRef &texture, Uint32 sliceIndex, Uint32 mipIndex, eShaderType shaderStage )
	{
		ID3D11ShaderResourceView *srv = NULL;

		if ( texture )
		{
			const STextureData &tdata = GetDeviceData().m_Textures.Data(texture);
			if ( tdata.m_ppCubeShaderResourceViewPerMipLevel )
			{
				srv = tdata.m_ppCubeShaderResourceViewPerMipLevel[ CalculateCubemapPerMipSliceIndex( GetTextureDesc( texture ), (Uint16)sliceIndex, (Uint16)mipIndex ) ];
			}
			GPUAPI_ASSERT( srv );

#ifdef RED_PLATFORM_DURANGO
			BindShaderResourceView( slot, srv, shaderStage, tdata.m_Desc.IsInPlace(), tdata.m_inplaceMemoryRegion.GetRegionInternal() );
			return;
#endif
		}

		BindShaderResourceView( slot, srv, shaderStage );
	}

	void BindTextureCubeMipFace( Uint32 slot, const TextureRef &texture, Uint32 sliceIndex, Uint32 mipIndex, Uint32 faceIndex, eShaderType shaderStage )
	{
		ID3D11ShaderResourceView *srv = NULL;

		if ( texture )
		{
			const STextureData &tdata = GetDeviceData().m_Textures.Data(texture);
			if ( tdata.m_ppCubeShaderResourceViewPerMipFace )
			{
				srv = tdata.m_ppCubeShaderResourceViewPerMipFace[ CalculateCubemapSliceIndex( GetTextureDesc( texture ), (Uint16)sliceIndex, (Uint16)faceIndex, (Uint16)mipIndex ) ];
			}
			GPUAPI_ASSERT( srv );

#ifdef RED_PLATFORM_DURANGO
			BindShaderResourceView( slot, srv, shaderStage, tdata.m_Desc.IsInPlace(), tdata.m_inplaceMemoryRegion.GetRegionInternal() );
			return;
#endif
		}

		BindShaderResourceView( slot, srv, shaderStage );
	}

	void BindTextureMipLevel( Uint32 slot, const TextureRef &texture, Uint32 sliceIndex, Uint32 mipIndex, eShaderType shaderStage )
	{
		RED_ASSERT( 0 == sliceIndex );

		ID3D11ShaderResourceView *srv = NULL;

		if ( texture )
		{
			const STextureData &tdata = GetDeviceData().m_Textures.Data(texture);
			if ( tdata.m_ppTex2DShaderResourceViewPerMipLevel )
			{
				RED_ASSERT( mipIndex < tdata.m_Desc.initLevels );
				srv = tdata.m_ppTex2DShaderResourceViewPerMipLevel[ mipIndex ];
			}
			GPUAPI_ASSERT( srv );

#ifdef RED_PLATFORM_DURANGO
			BindShaderResourceView( slot, srv, shaderStage, tdata.m_Desc.IsInPlace(), tdata.m_inplaceMemoryRegion.GetRegionInternal() );
			return;
#endif
		}
		BindShaderResourceView( slot, srv, shaderStage );
	}

	void BindTextureMipLevelUAV( Uint32 slot, const TextureRef& texture, Uint32 sliceIndex, Uint32 mipIndex )
	{
		// this is currently only required for LoadTextureData2D fallback route on Orbis
		GPUAPI_HALT("NOT IMPLEMENTED!"); 

		RED_UNUSED (slot);
		RED_UNUSED (texture);
		RED_UNUSED (sliceIndex);
		RED_UNUSED (mipIndex);
	}


	void BindTextureStencil( Uint32 slot, const TextureRef &texture, eShaderType shaderStage )
	{
		ID3D11ShaderResourceView *srv = NULL;

		if ( texture )
		{
			const STextureData &tdata = GetDeviceData().m_Textures.Data(texture);
#if MICROSOFT_ATG_DYNAMIC_SCALING
			srv = tdata.m_pDynamicScalingData ? tdata.m_pDynamicScalingData->m_pStencilShaderResourceView[g_DynamicScaleIndex] : tdata.m_pStencilShaderResourceView;
#else
			srv = tdata.m_pStencilShaderResourceView;
#endif
			GPUAPI_ASSERT( srv );
#ifdef RED_PLATFORM_DURANGO
			BindShaderResourceView( slot, srv, shaderStage, tdata.m_Desc.IsInPlace(), tdata.m_inplaceMemoryRegion.GetRegionInternal() );
			return;
#endif
		}
		BindShaderResourceView( slot, srv, shaderStage );
	}

	void BindTextures( Uint32 startSlot, Uint32 numTextures, const TextureRef *textures, eShaderType shaderStage )
	{
		GPUAPI_ASSERT( startSlot + numTextures <= GpuApi::MAX_PS_SAMPLERS );

		if ( textures != nullptr )
		{
			for ( Uint32 i=0; i<numTextures; ++i )
			{
				TextureRef currRef = textures[i];
				GPUAPI_ASSERT( !currRef || (GetDeviceData().m_Textures.Data(currRef).m_Desc.usage & TEXUSAGE_Samplable) );
				ID3D11ShaderResourceView* srv = nullptr;

				if (!currRef.isNull())
				{
					srv = GetD3DShaderResourceView(currRef);
					STextureData &tdata = GetDeviceData().m_Textures.Data(currRef);
#ifdef RED_PLATFORM_DURANGO
					BindShaderResourceView( startSlot + i, srv, shaderStage, tdata.m_Desc.IsInPlace(), tdata.m_inplaceMemoryRegion.GetRegionInternal() );
#else
					BindShaderResourceView( startSlot + i, srv, shaderStage );
#endif
				}
				else
				{
					srv = GetD3DShaderResourceView( GpuApi::GetInternalTexture( GpuApi::INTERTEX_Default2D) ); // TODO: this is invalid in case we're dealing with CUBE texture declared in shader
					BindShaderResourceView( startSlot + i, srv, shaderStage );
				}
			}
		}
		else
		{
			for ( Uint32 i=0; i<numTextures; ++i )
			{
				BindShaderResourceView( startSlot + i, nullptr, shaderStage );
			}
		}
	}

#if defined(RED_PLATFORM_DURANGO)
	static const D3D11X_SET_FAST SET_FAST_TYPES[eShaderType::ShaderTypeMax] =
	{
		D3D11X_SET_FAST_VS,
		D3D11X_SET_FAST_PS,
		D3D11X_SET_FAST_GS,
		D3D11X_SET_FAST_HS,
		D3D11X_SET_FAST_DS,
		D3D11X_SET_FAST_CS,
	};
#endif

	void BindTexturesFast( Uint32 startSlot, Uint32 numTextures, const TextureRef *textures, eShaderType shaderStage )
	{
#if defined(RED_PLATFORM_DURANGO)
		GPUAPI_ASSERT( startSlot + numTextures <= GpuApi::MAX_PS_SAMPLERS );

		ID3D11DeviceContextX* contextX = (ID3D11DeviceContextX*)GetDeviceContext();
		D3D11X_SET_FAST fastType = SET_FAST_TYPES[shaderStage];

		SDeviceData  &dd = GetDeviceData();

		for ( Uint32 i=0; i<numTextures; ++i )
		{
			TextureRef currRef = textures[i];
			GPUAPI_ASSERT( !currRef || (GetDeviceData().m_Textures.Data(currRef).m_Desc.usage & TEXUSAGE_Samplable) );
			GPUAPI_ASSERT(currRef.isNull() == false);

			if (!currRef.isNull())
			{
				STextureData& tdata = GetDeviceData().m_Textures.Data(currRef);
				ID3D11ShaderResourceView* srv = GetD3DShaderResourceView(currRef);
				GPUAPI_ASSERT(srv != nullptr);

				if ( tdata.m_Desc.IsInPlace() )
				{
					BindShaderResourceView( startSlot + i, srv, shaderStage, true, tdata.m_inplaceMemoryRegion.GetRegionInternal());
				}
				else
				{
					contextX->SetFastResources(fastType, startSlot + i, srv, 0);
				}
			}
		}
#else
		BindTextures(startSlot, numTextures, textures, shaderStage);
#endif		
	}

	void BindTextureUAVs( Uint32 startSlot, Uint32 numTextures, const TextureRef* textures )
	{
		GPUAPI_ASSERT( startSlot + numTextures <= GpuApi::MAX_PS_SAMPLERS );

		ID3D11UnorderedAccessView* d3dTextures[ MAX_PS_SAMPLERS ];

		for ( Uint32 i=0; i<numTextures; ++i )
		{
			if ( textures != nullptr )
			{
				TextureRef currRef = textures[i];
				GPUAPI_ASSERT( !currRef || (GetDeviceData().m_Textures.Data(currRef).m_Desc.usage & TEXUSAGE_RenderTarget) );

				if (!currRef.isNull())
				{
					d3dTextures[i] = GetD3DUnorderedAccessView(currRef);
				}
				else
				{
					d3dTextures[i] = nullptr;
				}
			}
			else
			{
				d3dTextures[i] = nullptr;
			}
		}

		GetDeviceContext()->CSSetUnorderedAccessViews( startSlot, numTextures, &(d3dTextures[0]), 0 );
	}

	Bool IsDescSupported( const TextureDesc &desc )
	{
		if ( !IsTextureSizeValidForFormat( desc.width, desc.height, desc.format ) )
		{
			return false;
		}

		if ( TEXTYPE_2D != desc.type && TEXTYPE_CUBE != desc.type && TEXTYPE_ARRAY != desc.type )
		{
			return false;
		}

		// Cannot create a backbuffer this way.
		if ( (desc.usage & TEXUSAGE_BackBuffer) != 0 )
		{
			return false;
		}

		// Can only have one of the following
		const Uint32 usageAccessMask = TEXUSAGE_Dynamic | TEXUSAGE_Staging | TEXUSAGE_StagingWrite | TEXUSAGE_Immutable;
		if ( ( desc.usage & usageAccessMask ) != 0 )
		{
			Bool usageMaskOk = false;
			usageMaskOk |= ( ( desc.usage & usageAccessMask ) == TEXUSAGE_Dynamic );
			usageMaskOk |= ( ( desc.usage & usageAccessMask ) == TEXUSAGE_Staging );
			usageMaskOk |= ( ( desc.usage & usageAccessMask ) == TEXUSAGE_StagingWrite );
			usageMaskOk |= ( ( desc.usage & usageAccessMask ) == TEXUSAGE_Immutable );
			if ( !usageMaskOk )
			{
				return false;
			}
		}

		// Creating an immutable texture has a few extra limitations.
		if ( ( desc.usage & TEXUSAGE_Immutable ) != 0 )
		{
			if ( desc.msaaLevel > 1 )
			{
				return false;
			}

			const Uint32 unsupportedUsages = TEXUSAGE_RenderTarget | TEXUSAGE_DepthStencil | TEXUSAGE_Dynamic | TEXUSAGE_Staging | TEXUSAGE_StagingWrite | TEXUSAGE_GenMip;
			if ( ( desc.usage & unsupportedUsages ) != 0 )
			{
				return false;
			}
		}

		// TODO : Should probably add more checks (like, can't use staging for render target and such.

		return true;
	}

	void LoadTextureData2D( const TextureRef &destTex, Uint32 mipLevel, Uint32 arraySlice, const Rect* destRect, const void *srcMemory, Uint32 srcPitch )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse( destTex ), TXT("Invalid texture reference") );
		if ( !GetDeviceData().m_Textures.IsInUse( destTex ) )
		{
			return;
		}

		const STextureData& texData = GetDeviceData().m_Textures.Data( destTex );
		GPUAPI_ASSERT( texData.m_pTexture != nullptr, TXT("Destination texture doesn't exist") );
		if ( texData.m_pTexture == nullptr )
		{
			return;
		}

		Uint32 texLevels = texData.m_Desc.CalcTargetLevels();
		Uint32 texSlices = texData.m_Desc.CalcTargetSlices();

		GPUAPI_ASSERT( mipLevel < texLevels, TXT("Mip level out of range: %u >= %u"), mipLevel, texLevels );
		GPUAPI_ASSERT( arraySlice < texSlices, TXT("Array slice out of range: %u >= %u"), arraySlice, texSlices );
		if ( mipLevel >= texLevels || arraySlice >= texSlices )
		{
			return;
		}

		GPUAPI_ASSERT( !(texData.m_Desc.usage & TEXUSAGE_Immutable), TXT("Cannot load data into immutable texture") );
		if ( texData.m_Desc.usage & TEXUSAGE_Immutable )
		{
			return;
		}

		D3D11_BOX destBox;
		if ( destRect )
		{
			destBox.left = destRect->left;
			destBox.top = destRect->top;
			destBox.right = destRect->right;
			destBox.bottom = destRect->bottom;
			destBox.front = 0;
			destBox.back = 1;
		}

		Uint32 subresource = D3D11CalcSubresource( mipLevel, arraySlice, texData.m_Desc.initLevels );
		GetDeviceContext()->UpdateSubresource( texData.m_pTexture, subresource, destRect ? &destBox : nullptr, srcMemory, srcPitch, 0 );
	}

	void LoadTextureData2DAsync( const TextureRef &destTex, Uint32 mipLevel, Uint32 arraySlice, const Rect* destRect, const void *srcMemory, Uint32 srcPitch, void* deferredContext )
	{
		GPUAPI_ASSERT( deferredContext != nullptr, TXT("Null deferred context") );
		if ( deferredContext == nullptr )
		{
			return;
		}
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(destTex), TXT("Invalid texture reference") );
		if ( !GetDeviceData().m_Textures.IsInUse(destTex) )
		{
			return;
		}

		const STextureData& texData = GetDeviceData().m_Textures.Data(destTex);
		GPUAPI_ASSERT( texData.m_pTexture != nullptr, TXT("Destination texture doesn't exist") );
		if ( texData.m_pTexture == nullptr )
		{
			return;
		}

		// Get texture data
		GPUAPI_ASSERT( mipLevel < texData.m_Desc.CalcTargetLevels() );
		GPUAPI_ASSERT( arraySlice < texData.m_Desc.CalcTargetSlices() );

		Uint32 texLevels = texData.m_Desc.CalcTargetLevels();
		Uint32 texSlices = texData.m_Desc.CalcTargetSlices();

		GPUAPI_ASSERT( mipLevel < texLevels, TXT("Mip level out of range: %u >= %u"), mipLevel, texLevels );
		GPUAPI_ASSERT( arraySlice < texSlices, TXT("Array slice out of range: %u >= %u"), arraySlice, texSlices );
		if ( mipLevel >= texLevels || arraySlice >= texSlices )
		{
			return;
		}

		GPUAPI_ASSERT( !(texData.m_Desc.usage & TEXUSAGE_Immutable), TXT("Cannot load data into immutable texture") );
		if ( texData.m_Desc.usage & TEXUSAGE_Immutable )
		{
			return;
		}

		D3D11_BOX destBox;
		if ( destRect )
		{
			destBox.left = destRect->left;
			destBox.top = destRect->top;
			destBox.right = destRect->right;
			destBox.bottom = destRect->bottom;
			destBox.front = 0;
			destBox.back = 1;
		}

		Uint32 subresource = D3D11CalcSubresource(mipLevel, arraySlice, texData.m_Desc.initLevels);
		((ID3D11DeviceContext*)deferredContext)->UpdateSubresource( texData.m_pTexture, subresource, destRect ? &destBox : nullptr, srcMemory, srcPitch, 0 );
	}


	Bool GrabTexturePixels( const TextureRef &texture, Uint32 grabX, Uint32 grabY, Uint32 grabWidth, Uint32 grabHeight, Uint8 *outDataRGBA, Uint32 stride, Bool forceFullAlpha )
	{
		GPUAPI_ASSERT( outDataRGBA );

		if ( !texture || grabWidth < 1 || grabHeight < 1 || stride < 4 )
		{
			return false;
		}

		// Get internal data
		SDeviceData  &dd = GetDeviceData();
		STextureData &td = dd.m_Textures.Data( texture );

		// Test if pixel grab is possible for given texture
		if ( grabX >= td.m_Desc.width				||	// Checking for the top left side avoids misses
			 grabY >= td.m_Desc.height				||	// caused from integer overflow when adding the
			 grabX + grabWidth > td.m_Desc.width	||	// width and height
			 grabY + grabHeight > td.m_Desc.height )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempted to grab data from invalid area." ) );
			return false;
		}

		// Test whether we support grabbing pixels from given texture format
		if ( TEXTYPE_2D != td.m_Desc.type || (TEXFMT_R8G8B8A8 != td.m_Desc.format && TEXFMT_R8G8B8X8 != td.m_Desc.format) )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
			return false;
		}

		// Create staging texture
		ID3D11Texture2D *stagingTexture = NULL;
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Usage = D3D11_USAGE_STAGING;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		texDesc.ArraySize = 1;
		texDesc.BindFlags = 0;
		texDesc.MipLevels = 1;
		texDesc.MiscFlags = 0;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;

		texDesc.Format = Map(td.m_Desc.format);
		texDesc.Width = td.m_Desc.width;
		texDesc.Height = td.m_Desc.height;

		HRESULT hr = GetDevice()->CreateTexture2D(&texDesc, 0, &stagingTexture);
		if ( !SUCCEEDED(hr) )
		{
			GPUAPI_HALT( "Failed to create staging texture for PixelsGrabbing" );
			GPUAPI_ASSERT( !stagingTexture );
			return false;
		}

		RED_UNUSED(hr);
#ifdef GPU_API_DEBUG_PATH
		const char* debugName = "tempTex";
		stagingTexture->SetPrivateData( WKPDID_D3DDebugObjectName, 7, debugName );
#endif

		// Copy data
		if ( td.m_pTexture == nullptr )
		{
			GPUAPI_HALT( "Source texture doesn't exist" );
			return false;
		}
		GetDeviceContext()->CopyResource(stagingTexture, td.m_pTexture);

		// Build lock rect
		RECT realRect;
		realRect.left   = grabX;
		realRect.top    = grabY;
		realRect.right  = grabX + grabWidth;
		realRect.bottom = grabY + grabHeight;

		// Lock destination surface
		D3D11_MAPPED_SUBRESOURCE mappedTexture;
		if ( !SUCCEEDED( GetDeviceContext()->Map( stagingTexture, 0, D3D11_MAP_READ, 0, &mappedTexture ) ) )
		{
			GPUAPI_LOG_WARNING( TXT( "Texture pixels grab internal failure : lock rect failed." ) );
			stagingTexture->Release();
			return false;
		}

		Uint8* data = (Uint8*)mappedTexture.pData + grabY * mappedTexture.RowPitch + grabX * 4;

		// Copy data to pixel buffer
		if ( forceFullAlpha )
		{
			for ( Uint32 y=0; y<grabHeight; y++ )
			{
				Uint8 *src  = data + y * mappedTexture.RowPitch;
				Uint8 *dest = outDataRGBA + stride * (y * grabWidth);
				for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=stride )
				{
					dest[0] = src[0];
					dest[1] = src[1];
					dest[2] = src[2];
					dest[3] = 255;
				}
			}
		}
		else
		{
			for ( Uint32 y=0; y<grabHeight; y++ )
			{
				Uint8 *src  = data + y * mappedTexture.RowPitch;
				Uint8 *dest = outDataRGBA + stride * (y * grabWidth);
				for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=stride )
				{
					dest[0] = src[0];
					dest[1] = src[1];
					dest[2] = src[2];
					dest[3] = src[3];
				}
			}
		}

		// Unlock and release
		GetDeviceContext()->Unmap( stagingTexture, 0 );
		
		stagingTexture->Release();

		// Return :)
		return true;
	}

	Bool GrabTexturePixels( const TextureRef &texture, Uint32 grabX, Uint32 grabY, Uint32 grabWidth, Uint32 grabHeight, Float *outData )
	{
		GPUAPI_ASSERT( outData );

		if ( !texture || grabWidth < 1 || grabHeight < 1 )
		{
			return false;
		}

		// Get internal data
		SDeviceData  &dd = GetDeviceData();
		STextureData &td = dd.m_Textures.Data( texture );

		// Test if pixel grab is possible for given texture
		if ( grabX + grabWidth > td.m_Desc.width || grabY + grabHeight > td.m_Desc.height )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempted to grab data from invalid area." ) );
			return false;
		}		

		// Test whether we support grabbing pixels from given texture format
		if ( TEXTYPE_2D != td.m_Desc.type || (TEXFMT_Float_R16G16B16A16 != td.m_Desc.format && TEXFMT_Float_R32G32B32A32 != td.m_Desc.format && TEXFMT_Float_R32 != td.m_Desc.format) )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
			return false;
		}

		// Create staging texture
		ID3D11Texture2D *stagingTexture = NULL;
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Usage = D3D11_USAGE_STAGING;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		texDesc.ArraySize = 1;
		texDesc.BindFlags = 0;
		texDesc.MipLevels = 1;
		texDesc.MiscFlags = 0;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;

		texDesc.Format = Map(td.m_Desc.format);
		texDesc.Width = td.m_Desc.width;
		texDesc.Height = td.m_Desc.height;

		HRESULT hr = GetDevice()->CreateTexture2D(&texDesc, 0, &stagingTexture);
		RED_UNUSED(hr);
#ifdef GPU_API_DEBUG_PATH
		const char* debugName = "tempTex";
		stagingTexture->SetPrivateData( WKPDID_D3DDebugObjectName, 7, debugName );
#endif

		// Copy data
		if ( td.m_pTexture == nullptr )
		{
			GPUAPI_HALT( "Source texture doesn't exist" );
			return false;
		}
		GetDeviceContext()->CopyResource(stagingTexture, td.m_pTexture);

		// Build lock rect
		RECT realRect;
		realRect.left   = grabX;
		realRect.top    = grabY;
		realRect.right  = grabX + grabWidth;
		realRect.bottom = grabY + grabHeight;

		// Lock destination surface
		D3D11_MAPPED_SUBRESOURCE mappedTexture;
		if ( !SUCCEEDED( GetDeviceContext()->Map( stagingTexture, 0, D3D11_MAP_READ, 0, &mappedTexture ) ) )
		{
			GPUAPI_LOG_WARNING( TXT( "Texture pixels grab internal failure : lock rect failed." ) );
			stagingTexture->Release();
			return false;
		}

		// Grab the data
		switch ( td.m_Desc.format )
		{
		case TEXFMT_Float_R16G16B16A16:
			{
				const Uint16* data = (const Uint16*)((const Uint8*)mappedTexture.pData + grabY * mappedTexture.RowPitch + grabX * 4 * sizeof(Uint16));

				// Copy data to pixel buffer
				for ( Uint32 y=0; y<grabHeight; y++ )
				{	
					const Uint16 *src  = (const Uint16*)((Uint8*)data + y * mappedTexture.RowPitch);
					Float *dest = outData + 4 * (y * grabWidth);
					for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=4 )
					{
						dest[0] = Float16Compressor::Decompress( src[0] );
						dest[1] = Float16Compressor::Decompress( src[1] );
						dest[2] = Float16Compressor::Decompress( src[2] );
						dest[3] = 1.f;
					}
				}
			}
			break;

		case TEXFMT_Float_R32G32B32A32:
			{
				Float* data = (Float*)((Uint8*)mappedTexture.pData + grabY * mappedTexture.RowPitch + grabX * 4 * sizeof(Float));

				// Copy data to pixel buffer
				for ( Uint32 y=0; y<grabHeight; y++ )
				{
					Float *src  = (Float*)((Uint8*)data + y * mappedTexture.RowPitch);
					Float *dest = outData + 4 * (y * grabWidth);
					for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=4 )
					{
						dest[0] = src[0];
						dest[1] = src[1];
						dest[2] = src[2];
						dest[3] = 1.f;
					}
				}
			}
			break;

		case TEXFMT_Float_R32:
			{
				Float* data = (Float*)((Uint8*)mappedTexture.pData + grabY * mappedTexture.RowPitch + grabX * sizeof(Float));

				// Copy data to pixel buffer
				for ( Uint32 y=0; y<grabHeight; ++y )
				{
					const Float *src  = (const Float*)((const Uint8*)data + y * mappedTexture.RowPitch);
					Float *dest = outData + (y * grabWidth);
					Red::System::MemoryCopy( dest, src, 4 * grabWidth );
				}
			}
			break;

		default:
			GPUAPI_HALT( "Format not handled" );
		}
		
		// Unlock and release
		GetDeviceContext()->Unmap( stagingTexture, 0 );

		stagingTexture->Release();

		// Return :)
		return true;
	}

	Bool SaveTexturePixels( const TextureRef& , Uint32 , Uint32 , Uint32 , Uint32 , const Char* , eTextureSaveFormat  )
	{
#if 0
		//HACK DX10 no texture saving
		if ( !texture || grabWidth < 1 || grabHeight < 1 || !fileName )
		{
			return false;
		}

		// Get internal data
		SDeviceData  &dd = GetDeviceData();
		STextureData &td = dd.m_Textures.Data( texture );

		// Test if pixel grab is possible for given texture
		if ( grabX + grabWidth > td.m_Desc.width || grabY + grabHeight > td.m_Desc.height )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempted to grab data from invalid area." ) );
			return false;
		}		

		// Test whether we support grabbing pixels from given texture format
		if ( TEXTYPE_2D != td.m_Desc.type || ( TEXFMT_A8R8G8B8 != td.m_Desc.format && format != SAVE_FORMAT_DDS ) )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
			return false;
		}

		// Create software surface
		IDirect3DSurface9 *softSurf = NULL;
		D3DFORMAT softSurfFormat = D3DFMT_A8R8G8B8;
		
		if ( format == SAVE_FORMAT_DDS )
		{
			if ( td.m_Desc.format == TEXFMT_Float_R32 )
			{
				softSurfFormat = D3DFMT_R32F;
			}
			else if ( td.m_Desc.format == TEXFMT_Float_A32R32G32B32 || td.m_Desc.format == TEXFMT_Float_A16R16G16B16 || td.m_Desc.format == TEXFMT_Float_A2R10G10B10 )
			{
				softSurfFormat = D3DFMT_A32B32G32R32F;
			}
			else if ( td.m_Desc.format == TEXFMT_Float_G16R16 )
			{
				softSurfFormat = D3DFMT_G32R32F;
			}
			else if ( td.m_Desc.format == TEXFMT_A8 )
			{
				softSurfFormat = D3DFMT_A8;
			}
			else if ( td.m_Desc.format == TEXFMT_L8 )
			{
				softSurfFormat = D3DFMT_L8;
			}
			else if ( td.m_Desc.format == TEXFMT_A8L8 )
			{
				softSurfFormat = D3DFMT_A8L8;
			}
			else
			{
				GPUAPI_LOG_WARNING( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
				return false;
			}
		}

		// Build lock rect
		RECT realRect;
		realRect.left   = grabX;
		realRect.top    = grabY;
		realRect.right  = grabX + grabWidth;
		realRect.bottom = grabY + grabHeight;

		D3DXIMAGE_FILEFORMAT destFormat = Map( format );

		HRESULT hr = D3DXSaveSurfaceToFile( fileName, destFormat, td.m_pSurface, NULL, &realRect );

		// Return :)
		return SUCCEEDED( hr );
#else
		return false;
#endif
	}

	Bool SaveTextureToMemory( const Uint8* textureData, const size_t textureDataSize, const size_t width, size_t height, const eTextureFormat format, const size_t pitch, const eTextureSaveFormat saveFormat, void** buffer, size_t& size )
	{
#ifndef RED_PLATFORM_CONSOLE
		DirectX::Image image;
		image.pixels = const_cast< Uint8* >( textureData );
		image.rowPitch = pitch;
		image.slicePitch = textureDataSize;
		image.width = width;
		image.height = height;
		image.format = Map( format );

		DirectX::Image srcImage = image;
		DirectX::ScratchImage img;
		if ( DirectX::IsCompressed( Map( format ) ) )
		{
			// decompress first
			GPUAPI_MUST_SUCCEED( DirectX::Decompress( image, Map( TEXFMT_R8G8B8A8 ), img ) );
			srcImage = *img.GetImage(0, 0, 0);
		}
		
		DirectX::Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = width;
		rect.h = height;
		DirectX::ScratchImage* tempImage = new DirectX::ScratchImage();
		HRESULT hr = tempImage->Initialize2D(Map( TEXFMT_R8G8B8A8 ), width, height, 1, 1);
		if ( FAILED( hr ) )
		{
			GPUAPI_HALT( "SaveTextureToMemory failed on image initialization." );
			return false;
		}
		hr = DirectX::CopyRectangle( srcImage, rect, *tempImage->GetImage(0,0,0), DirectX::TEX_FILTER_DEFAULT, 0, 0 );
		if ( FAILED( hr ) )
		{
			GPUAPI_HALT( "SaveTextureToMemory failed on copying rectangle." );
			return false;
		}

		DirectX::Blob blob;
		switch ( saveFormat )
		{
		case SAVE_FORMAT_DDS:
			hr = DirectX::SaveToDDSMemory(tempImage->GetImages(), tempImage->GetImageCount(), tempImage->GetMetadata(), DirectX::DDS_FLAGS_NONE, blob);
			break;
		case SAVE_FORMAT_BMP:
			hr = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), blob );
			break;
		case SAVE_FORMAT_JPG:
			hr = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG), blob );
			break;
		case SAVE_FORMAT_PNG:
			hr = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), blob );
			break;
		case SAVE_FORMAT_TGA:
			hr = DirectX::SaveToTGAMemory(*tempImage->GetImage(0,0,0), blob );
			break;
		}

		if ( FAILED( hr ) )
		{
			GPUAPI_HALT( "SaveTextureToMemory failed." );
			return false;
		}

		size = static_cast< size_t >( blob.GetBufferSize() );

		*buffer = GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, size, 16 );
		Red::System::MemoryCopy( *buffer, blob.GetBufferPointer(), size );

		blob.Release();
		tempImage->Release();
#endif
		return true;
	}

	Bool SaveTextureToMemory( const TextureRef &texture, eTextureSaveFormat format, const Rect* sourceRect, void** buffer, Uint32& size )
	{
#ifndef RED_PLATFORM_CONSOLE
		Uint32 width = sourceRect->right - sourceRect->left;
		Uint32 height = sourceRect->bottom - sourceRect->top;

		if ( !texture || width < 1 || height < 1 )
		{
			return false;
		}

		// Get internal data
		SDeviceData  &dd = GetDeviceData();
		STextureData &td = dd.m_Textures.Data( texture );

		// Test if save is possible for given texture
		if ( sourceRect->right > (Int32)td.m_Desc.width || sourceRect->bottom > (Int32)td.m_Desc.height )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempted to save data from invalid area." ) );
			return false;
		}		

		//// Test whether we support saving from given texture format
		//if ( TEXTYPE_2D != td.m_Desc.type || ( TEXFMT_R8G8B8A8 != td.m_Desc.format && format != SAVE_FORMAT_DDS ) )
		//{
		//	GPUAPI_LOG_WARNING( TXT( "Attempt to save data from unsupported (for grabbing) format detected." ) );
		//	return false;
		//}

		DirectX::ScratchImage* scratchImage = new DirectX::ScratchImage();
		HRESULT captureRes = DirectX::CaptureTexture(GetDevice(), GetDeviceContext(), td.m_pTexture, *scratchImage);
		if (captureRes == S_OK)
		{
			DirectX::Rect rect;
			rect.x = 0;
			rect.y = 0;
			rect.w = width;
			rect.h = height;
			DirectX::ScratchImage* tempImage = new DirectX::ScratchImage();
			tempImage->Initialize2D(Map( TEXFMT_R8G8B8A8 ), width, height, 1, 1);
			DirectX::CopyRectangle( *scratchImage->GetImage(0, 0, 0), rect, *tempImage->GetImage(0,0,0), DirectX::TEX_FILTER_DEFAULT, 0, 0 );

			DirectX::Blob blob;
			HRESULT saveRes;
			switch(format)
			{
			case SAVE_FORMAT_DDS:
				{
					saveRes = DirectX::SaveToDDSMemory(tempImage->GetImages(), tempImage->GetImageCount(), tempImage->GetMetadata(), DirectX::DDS_FLAGS_NONE, blob);
				}
			case SAVE_FORMAT_BMP:
				{
					saveRes = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), blob );
				}
			case SAVE_FORMAT_JPG:
				{
					saveRes = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG), blob );
				}
			case SAVE_FORMAT_PNG:
				{
					saveRes = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), blob );
				}
			}

			size = static_cast< GpuApi::Uint32 >( blob.GetBufferSize() );

			*buffer = GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, size, 16 );
			Red::System::MemoryCopy(*buffer, blob.GetBufferPointer(), size);

			blob.Release();
			tempImage->Release();
		}
		else
		{
			GPUAPI_HALT( "Texture saving failed" );
		}
		scratchImage->Release();
#endif
		// Done
		return true;
	}

#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
	Bool CompressBC6HBC7( /* in */ const TextureDataDesc& srcImage, /* in-out */ TextureDataDesc& compressedImage )
	{
		D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
		GetDevice()->CheckFeatureSupport( D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts) );
		if ( !hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x )
		{
			GPUAPI_LOG_WARNING( TXT( "Sorry your driver and/or video card doesn't support DirectCompute 4.x" ) );
			return false;
		}

		EncoderBase* encoder = NULL;
		if ( compressedImage.format == TEXFMT_BC6H )
		{
			encoder = new CGPUBC6HEncoder();
		}
		else if ( compressedImage.format == TEXFMT_BC7 )
		{
			encoder = new CGPUBC7Encoder();
		}
		else
		{
			GPUAPI_HALT( "Should never get here." );
			return false;
		}

		GPUAPI_ASSERT( encoder );
		encoder->Initialize( GetDevice(), GetDeviceContext() );

		DirectX::Image image;
		image.format		= Map( srcImage.format );
		image.width			= srcImage.width;
		image.height		= srcImage.height;
		image.pixels		= *const_cast<Uint8**>( srcImage.data );
		image.rowPitch		= srcImage.rowPitch;
		image.slicePitch	= srcImage.slicePitch;

		DirectX::TexMetadata metadata;
		metadata.arraySize	= 1;
		metadata.depth		= 1;
		metadata.dimension	= DirectX::TEX_DIMENSION_TEXTURE2D;
		metadata.format		= Map( srcImage.format );
		metadata.height		= srcImage.height;
		metadata.width		= srcImage.width;
		metadata.mipLevels	= 1;
		metadata.miscFlags	= 0;

		ID3D11Texture2D* sourceTexture = NULL;
		HRESULT hr = DirectX::CreateTexture( GetDevice(), &image, 1, metadata, (ID3D11Resource**)&sourceTexture );
		GPUAPI_ASSERT( SUCCEEDED( hr ) );

		DirectX::Image* cImage = new DirectX::Image();
		hr = encoder->GPU_EncodeAndReturn( sourceTexture, Map( compressedImage.format ), cImage );
		if ( SUCCEEDED( hr ) )
		{
			Red::System::MemoryCopy( *compressedImage.data, cImage->pixels, cImage->slicePitch );
			compressedImage.width = cImage->width;
			compressedImage.height = cImage->height;
			compressedImage.rowPitch = cImage->rowPitch;
			compressedImage.slicePitch = cImage->slicePitch;
		}

		// cleanup
		free( cImage->pixels ); // memory was allocated inside GPU_EncodeAndReturn
		delete cImage;
		cImage = NULL;
		delete encoder;
		encoder = NULL;

		return SUCCEEDED( hr );
	}
#endif

	Bool CompressImage( /* in */ const TextureDataDesc& srcImage, /* in-out */ TextureDataDesc& compressedImage, EImageCompressionHint compressionHint, Float alphaThreshold /*= 0.5f*/ )
	{
#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
		if ( compressedImage.format == TEXFMT_BC6H || compressedImage.format == TEXFMT_BC7 )
		{
			if ( CompressBC6HBC7( srcImage, compressedImage ) )
			{
				return true;
			}
		}
#endif
#ifndef RED_PLATFORM_CONSOLE
		// feed the structure with source data
		DirectX::Image image;
		image.format = Map( srcImage.format );
		image.width = srcImage.width;
		image.height = srcImage.height;
		image.pixels = *const_cast<Uint8**>( srcImage.data );
		image.rowPitch = srcImage.rowPitch;
		image.slicePitch = srcImage.slicePitch;

		// build flags
		DWORD compressionFlags = DirectX::TEX_COMPRESS_DEFAULT;
		if ( CIH_NormalmapRGB == compressionHint )
		{
			compressionFlags |= DirectX::TEX_COMPRESS_UNIFORM;
		}

		// preform compression
		DirectX::ScratchImage tempImage;
		// TODO: integrate OpenMP and use DirectX::TEX_COMPRESS_PARALLEL
		HRESULT hr = DirectX::Compress( image, Map( compressedImage.format ), compressionFlags, alphaThreshold, tempImage );
		if ( FAILED( hr ) )
		{
			GPUAPI_LOG_WARNING( TXT( "Unable to compress image" ) );
			return false;
		}

		if ( compressedImage.data == nullptr )
		{
			GPUAPI_HALT( "No data in compressed image, compression failed." );
			return false;
		}

		const DirectX::Image* img = tempImage.GetImage(0, 0, 0);
		compressedImage.width = img->width;
		compressedImage.height = img->height;
		compressedImage.rowPitch = img->rowPitch;
		compressedImage.slicePitch = img->slicePitch;
		compressedImage.format = Map( img->format );

		if ( !*compressedImage.data )
		{
			*compressedImage.data = (GpuApi::Uint8*)GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, img->slicePitch, 16 );
		}

		Red::System::MemoryCopy( *compressedImage.data, img->pixels, img->slicePitch );
#endif
		return true;
	}

	Bool DecompressImage( /* in */ const TextureDataDesc& srcImage, /* in-out */ TextureDataDesc& decompressedImage )
	{
#ifndef RED_PLATFORM_CONSOLE
		DirectX::Image image;
		image.format		= Map( srcImage.format );
		image.width			= srcImage.width;
		image.height		= srcImage.height;
		image.pixels		= *const_cast<Uint8**>( srcImage.data );
		image.rowPitch		= srcImage.rowPitch;
		image.slicePitch	= srcImage.slicePitch;

		DirectX::ScratchImage tempImage;
		HRESULT hr = DirectX::Decompress( image, Map( decompressedImage.format ), tempImage );
		if ( FAILED( hr ) )
		{
			GPUAPI_LOG_WARNING( TXT( "Unable to compress image" ) );
			return false;
		}

		const DirectX::Image* img		= tempImage.GetImage(0, 0, 0);
		decompressedImage.width			= img->width;
		decompressedImage.height		= img->height;
		decompressedImage.format		= Map( img->format );
		decompressedImage.rowPitch		= img->rowPitch;
		decompressedImage.slicePitch	= img->slicePitch;
		
		if ( !*decompressedImage.data )
		{
			 *decompressedImage.data = (GpuApi::Uint8*)GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, img->slicePitch, 16 );
		}
		
		Red::System::MemoryCopy( *decompressedImage.data, img->pixels, img->slicePitch );
#endif
		return true;
	}

	TextureRef GetInternalTexture( eInternalTexture internalTexture )
	{
		if( INTERTEX_Max == internalTexture )
		{
			GPUAPI_HALT( "internal texture index invalid" );
			return TextureRef::Null();
		}
		const TextureRef &ref = GetDeviceData().m_InternalTextures[internalTexture];
		GPUAPI_ASSERT( ref, TXT( "Internal texture reference is invalid" ) );
#ifdef NO_GPU_ASSERTS
		RED_UNUSED( ref );
#endif
		return ref;
	}

	void InitInternalTextures( bool assumeRefsPresent )
	{	
		SDeviceData &dd = GetDeviceData();

		// Pre check
		for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		{
			bool isPresent = !dd.m_InternalTextures[i].isNull();
#ifdef NO_GPU_ASSERTS
			RED_UNUSED( isPresent );
			RED_UNUSED( assumeRefsPresent );
#endif
			GPUAPI_ASSERT( isPresent == assumeRefsPresent );
		}

		// Create resources
		for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		{
			if ( !dd.m_InternalTextures[i] )
			{
				dd.m_InternalTextures[i] = TextureRef( dd.m_Textures.Create( 1 ) );
				GPUAPI_ASSERT( dd.m_InternalTextures[i] != NULL );
			}
		}

		// Create internal textures
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_Blank2D] ),			GPUAPI_BLANK2D_TEXTURE_SIZE,			Utils::BlankTextureFill );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_FlatNormal2D] ),		GPUAPI_BLANK2D_TEXTURE_SIZE,			Utils::FlatNormalTextureFill );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_Default2D] ),		GPUAPI_DEFAULT2D_TEXTURE_SIZE,			Utils::DefaultTextureFill );
		Utils::InitInternalTextureDataCUBE( dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_DefaultCUBE] ),		GPUAPI_DEFAULTCUBE_TEXTURE_SIZE,		Utils::DefaultCubeTextureFill );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_DissolvePattern] ),	GPUAPI_DISSOLVE_TEXTURE_SIZE,			Utils::GenerateDissolveTexture );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_PoissonRotation] ),	GPUAPI_POISSON_ROTATION_TEXTURE_SIZE,	Utils::GeneratePoissonRotationTexture );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_SSAORotation] ),		GPUAPI_SSAO_ROTATION_TEXTURE_SIZE,		Utils::GenerateSSAORotationNoise );
		Utils::RefMipNoiseRandomGen().Seed( 0xf112 );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_MipNoise] ),			GPUAPI_MIP_NOISE_TEXTURE_SIZE,			Utils::GenerateMipNoise, true );

		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_Blank2D], "BlankTexture" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_FlatNormal2D], "FlatTexture" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_Default2D], "DefaultTexture" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_DefaultCUBE], "DefaultCube" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_DissolvePattern], "DissolvePattern" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_PoissonRotation], "PoissonRotation" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_SSAORotation], "SSAORotation" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_MipNoise], "MipNoise" );

		//// Post check
		//for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		//{
		//	const STextureData &td = dd.m_Textures.Data( dd.m_InternalTextures[i] );
		//	GPUAPI_ASSERT( dd.m_InternalTextures[i] && "Not all internal texture were created!" );
		//	GPUAPI_ASSERT( NULL != td.m_pTexture );
		//	//GPUAPI_ASSERT( (NULL != td.m_pSurface) == (TEXTYPE_2D == td.m_Desc.type) );
		//}
	}

	void ShutInternalTextures( bool dropRefs )
	{
		SDeviceData &dd = GetDeviceData();

		// Release d3d resources

		for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		{
			TextureRef ref = dd.m_InternalTextures[i];
			if ( !ref )
			{
				continue;
			}

			STextureData &data = dd.m_Textures.Data( ref );
			//if ( data.m_pSurface )
			//{
			//	data.m_pSurface->Release();
			//	data.m_pSurface = NULL;
			//}
#if MICROSOFT_ATG_DYNAMIC_SCALING
			if(data.m_pDynamicScalingData)
			{
				for(Uint32 i=0; i<DYANMIC_SCALING_NUM_TARGETS; ++i)
				{
					data.m_pDynamicScalingData->m_pTexture[i]->Release();
					data.m_pDynamicScalingData->m_pTexture[i] = NULL;
				}
			}
			else
#endif
			{
				if ( data.m_pTexture )
				{
					data.m_pTexture->Release();
					data.m_pTexture = NULL;
				}
			}
		}

		// Drop resources

		if ( dropRefs )
		{
			for ( Uint32 i=0; i<INTERTEX_Max; ++i )
			{
				SafeRelease( dd.m_InternalTextures[i] );
			}
		}
	}

	//dex++: dynamic texture debug code
	Uint32 GetNumDynamicTextures()
	{
		return GetDeviceData().m_NumDynamicTextures;
	}

	const char* GetDynamicTextureName( Uint32 index )
	{
		if ( index < GetDeviceData().m_NumDynamicTextures )
		{
			return GetDeviceData().m_DynamicTextures[index].m_Name;
		}
		else
		{
			return NULL;
		}
	}

	TextureRef GetDynamicTextureRef( Uint32 index )
	{
		if ( index < GetDeviceData().m_NumDynamicTextures )
		{
			return GetDeviceData().m_DynamicTextures[index].m_Texture;
		}
		else
		{
			return TextureRef::Null();
		}
	}

	void AddDynamicTexture( TextureRef tex, const char* name )
	{
		if ( GetDeviceData().m_NumDynamicTextures < ARRAYSIZE( GetDeviceData().m_DynamicTextures ) )
		{
			GetDeviceData().m_DynamicTextures[ GetDeviceData().m_NumDynamicTextures ].m_Name = name;
			GetDeviceData().m_DynamicTextures[ GetDeviceData().m_NumDynamicTextures ].m_Texture = tex;
			GetDeviceData().m_NumDynamicTextures += 1;
		}
	}

	void RemoveDynamicTexture( TextureRef tex )
	{
		for ( Uint32 i=0; i<GetDeviceData().m_NumDynamicTextures; ++i )
		{
			if ( GetDeviceData().m_DynamicTextures[ i ].m_Texture == tex )
			{
				// copy rest of the textures
				for ( Uint32 j=i+1; j<GetDeviceData().m_NumDynamicTextures; ++j )
				{
					GetDeviceData().m_DynamicTextures[j-1] = GetDeviceData().m_DynamicTextures[j];
				}
				
				// remove from list
				GetDeviceData().m_NumDynamicTextures -= 1;
				break;
			}
		}
	}
	//dex--


	Bool CalculateCookedTextureMipOffsetAndSize( const TextureDesc& texDesc, Uint32 mip, Uint32 slice, Uint32* outOffset, Uint32* outSize )
	{
#ifdef RED_PLATFORM_DURANGO
		D3D11_TEXTURE2D_DESC dxTexDesc;
		Utils::FillD3DTextureDesc( texDesc, dxTexDesc );

		XG_TEXTURE2D_DESC xgTexDesc;
		Utils::CopyD3DTextureDescToXG( dxTexDesc, xgTexDesc );

		XG_RESOURCE_LAYOUT layout;
		HRESULT hr = XGComputeTexture2DLayout( &xgTexDesc, &layout );
		if ( FAILED( hr ) )
		{
			return false;
		}

		if ( mip >= dxTexDesc.MipLevels || slice >= dxTexDesc.ArraySize )
		{
			return false;
		}

		if ( outOffset != nullptr )
		{
			*outOffset = ( Uint32 )( layout.Plane[0].MipLayout[mip].OffsetBytes + layout.Plane[0].MipLayout[mip].Slice2DSizeBytes * slice );
		}
		
		if ( outSize != nullptr )
		{
			*outSize = ( Uint32 )( layout.Plane[0].MipLayout[mip].Slice2DSizeBytes );
		}

		return true;

#else
		const Uint32 texMips = texDesc.CalcTargetLevels();
		const Uint32 texSlices = texDesc.CalcTargetSlices();

		if ( mip >= texMips || slice >= texSlices )
		{
			return false;
		}

		if ( outOffset != nullptr )
		{
			Uint32 offset = 0;

			for ( Uint32 i = 0; i <= mip; ++i )
			{
				Uint32 mipWidth		= CalculateTextureMipDimension( texDesc.width, i, texDesc.format );
				Uint32 mipHeight	= CalculateTextureMipDimension( texDesc.height, i, texDesc.format );
				Uint32 mipSize		= CalculateTextureSize( mipWidth, mipHeight, texDesc.format );

				if ( i < mip )
				{
					offset += mipSize * texSlices;
				}
				else
				{
					offset += mipSize * slice;
				}
			}

			*outOffset = offset;
		}

		if ( outSize != nullptr )
		{
			const Uint32 mipWidth	= CalculateTextureMipDimension( texDesc.width, mip, texDesc.format );
			const Uint32 mipHeight	= CalculateTextureMipDimension( texDesc.height, mip, texDesc.format );
			*outSize				= CalculateTextureSize( mipWidth, mipHeight, texDesc.format );
		}

		return true;
#endif
	}

	Uint32 CalculateCookedTextureSize( const TextureDesc& texDesc )
	{
#ifdef RED_PLATFORM_DURANGO
		D3D11_TEXTURE2D_DESC dxTexDesc;
		Utils::FillD3DTextureDesc( texDesc, dxTexDesc );

		XG_TEXTURE2D_DESC xgTexDesc;
		Utils::CopyD3DTextureDescToXG( dxTexDesc, xgTexDesc );


		XG_RESOURCE_LAYOUT layout;
		HRESULT hr = XGComputeTexture2DLayout( &xgTexDesc, &layout );
		if ( FAILED( hr ) )
		{
			return 0;
		}

		return ( Uint32 )layout.SizeBytes;

#else
		const Uint32 texMips = texDesc.CalcTargetLevels();
		const Uint32 texSlices = texDesc.CalcTargetSlices();

		Uint32 sliceSize = 0;
		for ( Uint32 i = 0; i < texMips; ++i )
		{
			const Uint32 mipWidth	= CalculateTextureMipDimension( texDesc.width, i, texDesc.format );
			const Uint32 mipHeight	= CalculateTextureMipDimension( texDesc.height, i, texDesc.format );
			const Uint32 mipSize	= CalculateTextureSize( mipWidth, mipHeight, texDesc.format );

			sliceSize += mipSize;
		}

		return sliceSize * texSlices;
#endif
	}


	void IncrementInFlightTextureMemory( Uint32 textureSize )
	{
		GpuApi::GetDeviceData().m_TextureStats.IncrementTextureMemoryInFlight( textureSize );
	}

	void DecrementInFlightTextureMemory( Uint32 textureSize )
	{
		GpuApi::GetDeviceData().m_TextureStats.DecrementTextureMemoryInFlight( textureSize );
	}


	Red::MemoryFramework::MemoryRegionHandle GetTextureInPlaceMemory( const TextureRef& texture )
	{
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_Textures.IsInUse( texture ), TXT("Invalid texture ref") );

#ifdef RED_PLATFORM_DURANGO

		const STextureData& texData = GetDeviceData().m_Textures.Data( texture );
		if ( !texData.m_Desc.IsInPlace() )
		{
			return Red::MemoryFramework::MemoryRegionHandle();
		}

		return texData.m_inplaceMemoryRegion;

#else

		GPUAPI_HALT( "In-place textures not supported on this platform!" );
		return Red::MemoryFramework::MemoryRegionHandle();

#endif
	}

}

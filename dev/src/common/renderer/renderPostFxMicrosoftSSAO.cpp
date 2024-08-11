#include "build.h"
#include "renderPostProcess.h"
#include "renderPostFx.h"
#include "renderShaderPair.h"
#include "renderPostFxMicrosoftSSAO.h"

#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_WINPC )
#include "..\gpuApiDX10\gpuApiBase.h"
#include "../../common/gpuApiDX10/gpuApiMapping.h"
#elif defined( RED_PLATFORM_ORBIS )
#include "..\gpuApiGnm\gpuApiBase.h"
#include "../../common/gpuApiGnm/gpuApiMapping.h"
#else
#error Unsupported platform
#endif

class CRenderSurfaces;

/*
namespace Config
{
	// This is necessary to filter out pixel shimmer due to bilateral upsampling with too much lost resolution.  High
	// frequency detail can sometimes not be reconstructed, and the noise filter fills in the missing pixels with the
	// result of the higher resolution SSAO.
	TConfigVar< Float >	cvNoiseFilterTolerance( "Rendering/MSSSAO", "NoiseFilterTolerance", -3.0f, eConsoleVarFlag_Save ); // suggested values -8 to 0
	TConfigVar< Float >	cvBlurTolerance( "Rendering/MSSSAO", "BlurTolerance", -6.0f, eConsoleVarFlag_Save );			   // suggested values -8.0f to -1.0f
	TConfigVar< Float >	cvUpsampleTolerance( "Rendering/MSSSAO", "UpsampleTolerance", -3.0f, eConsoleVarFlag_Save );	   // suggested values -12.0f to -1.0f

	// Controls how aggressive to fade off samples that occlude spheres but by so much as to be unreliable.
	// This is what gives objects a dark halo around them when placed in front of a wall.  If you want to
	// fade off the halo, boost your rejection falloff.  The tradeoff is that it reduces overall AO.
	TConfigVar< Float >	cvRejectionFalloff( "Rendering/MSSSAO", "RejectionFalloff", 8.0f, eConsoleVarFlag_Save ); // suggested values 1.0f to 10.0f

	// The higher quality modes blend wide and narrow sampling patterns.  The wide
	// pattern is due to deinterleaving and requires blurring.  The narrow pattern is
	// not on a deinterleaved buffer, but it only samples every other pixel.  The blur
	// on it is optional.  If you combine the two before blurring, the narrow will get
	// blurred as well.  This creates a softer effect but can remove any visible noise
	// from having 50% sample coverage.
	TConfigVar< Bool > cvCombineResolutionsBeforeBlur( "Rendering/MSSSAO", "CombineResolutionsBeforeBlur", true, eConsoleVarFlag_Save );

	// When combining the wide and narrow patterns, a mul() operation can be used or
	// a min() operation.  Multiplication exaggerates the result creating even darker
	// creases.  This is an artistic choice.  I think it looks less natural, but often
	// art teams prefer more exaggerated contrast.  For me, it's more about having the
	// right AO falloff so that it's a smooth gradient rather than falling off precipitously
	// and forming overly dark recesses.
	TConfigVar< Bool > cvCombineResolutionsWithMul( "Rendering/MSSSAO", "CombineResolutionsWithMul", false, eConsoleVarFlag_Save );

	TConfigVar< Int32, Validation::IntRange<1, 4> > cvHierarchyDepth( "Rendering/MSSSAO", "HierarchyDepth", 2, eConsoleVarFlag_Save ); // valid values 1 to 4
	TConfigVar< Float >	cvNormalAOMultiply( "Rendering/MSSSAO", "NormalAOMultiply", 1.7f, eConsoleVarFlag_Save ); // more to darken
	TConfigVar< Float >	cvNormalToDepthBrightnessEqualiser( "Rendering/MSSSAO", "NormalToDepthBrightnessEqualiser", 100.0f, eConsoleVarFlag_Save ); // includes compensation for fade by depth, possibly ought to be a far plane multiply?

	TConfigVar< Float > cvNormalBackProjectionTolerance( "Rendering/MSSSAO", "NormalBackProjectionTolerance", 3.0f, eConsoleVarFlag_Save ); //angle in degrees
}
*/

namespace
{
#ifdef RED_PLATFORM_DURANGO
	inline void InsertComputeBarrier( ID3D11DeviceContextX* context )
	{
		context->GpuSendPipelinedEvent(D3D11X_GPU_PIPELINED_EVENT_CS_PARTIAL_FLUSH);
	}
#endif

	template <typename T> RED_FORCE_INLINE T AlignUpWithMask( T value, size_t mask )
	{
		return static_cast<T>((static_cast<size_t>(value) + mask) & ~mask);
	}

	template <typename T> RED_FORCE_INLINE T AlignDownWithMask( T value, size_t mask )
	{
		return static_cast<T>(static_cast<size_t>(value) & ~mask);
	}

	template <typename T> RED_FORCE_INLINE T AlignUp( T value, size_t alignment )
	{
		return AlignUpWithMask(value, alignment - 1);
	}

	template <typename T> RED_FORCE_INLINE T AlignDown( T value, size_t alignment )
	{
		return AlignDownWithMask(value, alignment - 1);
	}

	template <typename T> RED_FORCE_INLINE bool IsAligned( T value, size_t alignment )
	{
		return 0 == ((size_t)value & (alignment - 1));
	}

	template <typename T> RED_FORCE_INLINE T DivideByMultiple( T value, size_t alignment )
	{
		return (T)((value + alignment - 1) / alignment);
	}

	inline void DispatchFullScreenCompute( CRenderShaderCompute* shader, Uint32 ImageWidth, Uint32 ImageHeight, Uint32 GroupWidth = 8, Uint32 GroupHeight = 8 )
	{
		shader->Dispatch( DivideByMultiple(ImageWidth, GroupWidth), DivideByMultiple(ImageHeight, GroupHeight), 1 );
	}

	inline void Dispatch2D( CRenderShaderCompute* shader, Uint32 ImageWidth, Uint32 ImageHeight, Uint32 GroupWidth, Uint32 GroupHeight )
	{
		shader->Dispatch( DivideByMultiple(ImageWidth, GroupWidth), DivideByMultiple(ImageHeight, GroupHeight), 1 );
	}

	inline void Dispatch3D( CRenderShaderCompute* shader, Uint32 ImageWidth, Uint32 ImageHeight, Uint32 ImageDepth, Uint32 GroupWidth, Uint32 GroupHeight, Uint32 GroupDepth )
	{
		shader->Dispatch(
			DivideByMultiple(ImageWidth, GroupWidth),
			DivideByMultiple(ImageHeight, GroupHeight),
			DivideByMultiple(ImageDepth, GroupDepth) );
	}

	GpuApi::TextureRef Create2DTexture( Uint32 width, Uint32 height, GpuApi::eTextureFormat format, const char* debugName, Uint32 esramOffset, Uint32 usageExtra )
	{
		GpuApi::TextureDesc textureDesc;
		textureDesc.usage = GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_Samplable | usageExtra;
		textureDesc.initLevels = 1;
		textureDesc.width = width;
		textureDesc.height = height;
		textureDesc.sliceNum = 1;
		textureDesc.type = GpuApi::TEXTYPE_2D;
		textureDesc.format = format;

		if (esramOffset != 0)
		{
			textureDesc.esramOffset = esramOffset;
			textureDesc.usage |= GpuApi::TEXUSAGE_ESRAMResident;
		}

		GpuApi::TextureRef texture = GpuApi::CreateTexture( textureDesc, GpuApi::TEXG_System, nullptr );
		GpuApi::SetTextureDebugPath( texture, debugName );
		return texture;
	}

	GpuApi::TextureRef CreateArrayTexture( Uint32 width, Uint32 height, Uint32 sliceCount, GpuApi::eTextureFormat format, const char* debugName, Uint32 esramOffset, Uint32 usageExtra )
	{
		GpuApi::TextureDesc textureDesc;
		textureDesc.usage = GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_Samplable | usageExtra;
		textureDesc.initLevels = 1;
		textureDesc.width = width;
		textureDesc.height = height;
		textureDesc.sliceNum = 16;
		textureDesc.type = GpuApi::TEXTYPE_ARRAY;
		textureDesc.format = format;

		if (esramOffset != 0)
		{
			textureDesc.esramOffset = esramOffset;
			textureDesc.usage |= GpuApi::TEXUSAGE_ESRAMResident;
		}

		GpuApi::TextureRef texture = GpuApi::CreateTexture( textureDesc, GpuApi::TEXG_System, nullptr );
		GpuApi::SetTextureDebugPath( texture, debugName );
		return texture;
	}
}

void CPostFXMicrosoftSSAO::WriteCommonCBData( Float *dest, Uint32 BufferWidth, Uint32 BufferHeight )
{
	// These are the weights that are multiplied against the samples because not all samples are
	// equally important.  The farther the sample is from the center location, the less they matter.
	// We use the thickness of the sphere to determine the weight.  The scalars in front are the number
	// of samples with this weight because we sum the samples together before multiplying by the weight,
	// so as an aggregate all of those samples matter more.  After generating this table, the weights
	// are normalized.
	dest[ 0] = 4.0f * m_SampleThickness[ 0];	// Axial
	dest[ 1] = 4.0f * m_SampleThickness[ 1];	// Axial
	dest[ 2] = 4.0f * m_SampleThickness[ 2];	// Axial
	dest[ 3] = 4.0f * m_SampleThickness[ 3];	// Axial
	dest[ 4] = 4.0f * m_SampleThickness[ 4];	// Diagonal
	dest[ 5] = 8.0f * m_SampleThickness[ 5];	// L-shaped
	dest[ 6] = 8.0f * m_SampleThickness[ 6];	// L-shaped
	dest[ 7] = 8.0f * m_SampleThickness[ 7];	// L-shaped
	dest[ 8] = 4.0f * m_SampleThickness[ 8];	// Diagonal
	dest[ 9] = 8.0f * m_SampleThickness[ 9];	// L-shaped
	dest[10] = 8.0f * m_SampleThickness[10];	// L-shaped
	dest[11] = 4.0f * m_SampleThickness[11];	// Diagonal

	//#define SAMPLE_EXHAUSTIVELY

	// If we aren't using all of the samples, delete their weights before we normalize.
#ifndef SAMPLE_EXHAUSTIVELY
	dest[0] = 0.0f;
	dest[2] = 0.0f;
	dest[5] = 0.0f;
	dest[7] = 0.0f;
	dest[9] = 0.0f;
#endif

	// Normalize the weights by dividing by the sum of all weights
	float totalWeight = 0.0f;
	for (int i = 0; i < 12; ++i)
		totalWeight += dest[i];
	for (int i = 0; i < 12; ++i)
		dest[i] /= totalWeight;

	dest[12] = 1.0f / BufferWidth;
	dest[13] = 1.0f / BufferHeight;
	dest[14] = 1.0f / m_RejectionFalloff;
	dest[15] = m_NormalToDepthBrightnessEqualiser * m_NormalAOMultiply;

	dest[16] = cosf(0.01745329f * (90.f + m_NormalBackProjectionTolerance));
	dest[17] = 1.0f / (1.0f + m_NormalBackProjectionTolerance);
}

void CPostFXMicrosoftSSAO::UpdateCBs( const float TanHalfFovH, float zMagicA, float zMagicB )
{
	Int32 currentWidth;
	Int32 currentHeight;
	GetScreenSize( currentWidth, currentHeight );

	Bool forceUpdate = false;

	// Force update in case resolution changed
	RED_FATAL_ASSERT( currentWidth > 0 && currentHeight > 0, "Expected resolution info" );
	if ( currentWidth > 0 && currentHeight > 0 )
	{
#ifdef RED_ASSERTS_ENABLED
		{
			const CRenderSurfaces *surfaces = GetRenderer()->GetSurfaces();
			RED_FATAL_ASSERT( surfaces, "Expected renderSurfaces" );
			if ( surfaces )
			{
				RED_FATAL_ASSERT( currentWidth == (Int32)surfaces->GetWidth( false ), "Expected the same width" );
				RED_FATAL_ASSERT( currentHeight == (Int32)surfaces->GetHeight( false ), "Expected the same height" );
			}
		}
#endif

		if ( -1 == m_LastScreenWidth || -1 == m_LastScreenHeight || currentWidth != m_LastScreenWidth || currentHeight != m_LastScreenHeight )
		{
			m_LastScreenWidth = currentWidth;
			m_LastScreenHeight = currentHeight;
			forceUpdate = true;
		}
	}


	if ( forceUpdate ) 
	{
		m_verticalResolution = currentWidth;

		const uint32_t bufferWidth2 = (currentWidth + 3) / 4;
		const uint32_t bufferHeight2 = (currentHeight + 3) / 4;

		m_CBData->m_InverseRTSizeData[0] = 1.0f / bufferWidth2;
		m_CBData->m_InverseRTSizeData[1] = 1.0f / bufferHeight2;

		void* data = GpuApi::LockBuffer( m_InverseRTSizeCB, GpuApi::BLF_Discard, 0, 4 * sizeof(Float) );
		Red::System::MemoryCopy( data, m_CBData->m_InverseRTSizeData, 4 * sizeof(Float) );
		GpuApi::UnlockBuffer( m_InverseRTSizeCB );


		uint32_t TileCountX = AlignUp(DivideByMultiple(AlignUp(currentWidth, 16), 8), 16);
		uint32_t TileCountY = AlignUp(DivideByMultiple(AlignUp(currentHeight, 8), 8), 2);

		m_CBData->m_DepthDecompressCBData[0].TileCountX	= TileCountX;
		m_CBData->m_DepthDecompressCBData[0].TileCountY	= TileCountY;
		m_CBData->m_DepthDecompressCBData[1].TileCountX	= TileCountX;
		m_CBData->m_DepthDecompressCBData[1].TileCountY	= TileCountY;
	}

	if( (TanHalfFovH != m_LastTanHalfFovH) || (m_RejectionFalloff != m_LastRejectionFalloff) || (m_LastNormalToDepthBrightnessEqualiser != m_NormalToDepthBrightnessEqualiser) ||
		(m_NormalAOMultiply != m_LastNormalAOMultiply) || (m_NormalBackProjectionTolerance != m_LastNormalBackProjectionTolerance) || forceUpdate )
	{
		m_LastTanHalfFovH		 = TanHalfFovH;
		m_LastRejectionFalloff	 = m_RejectionFalloff;
		m_LastNormalAOMultiply	 = m_NormalAOMultiply;
		m_LastNormalBackProjectionTolerance = m_NormalBackProjectionTolerance;
		m_LastNormalToDepthBrightnessEqualiser = m_NormalToDepthBrightnessEqualiser;
		m_CurrentAODoubleBuffer	^= 1;

		GpuApi::TextureDesc tdesc;

		for(int i=0; i<m_DepthDownSizeCount; ++i)
		{
			uint32_t BufferWidth, BufferHeight, ArrayCount;

			for(int j=0; j<2; ++j)
			{
				switch(j)
				{
				case 0:
					GpuApi::GetTextureDesc( m_DepthTiled[i], tdesc );
					BufferWidth		= tdesc.width;
					BufferHeight	= tdesc.height;
					ArrayCount		= tdesc.sliceNum;
					break;

				case 1:
					GpuApi::GetTextureDesc( m_DepthDownsize[i], tdesc );
					BufferWidth		= tdesc.width;
					BufferHeight	= tdesc.height;
					ArrayCount		= tdesc.sliceNum;
					break;

				default:
					assert(0);
					break;
				}
				// Here we compute multipliers that convert the center depth value into (the reciprocal of)
				// sphere thicknesses at each sample location.  This assumes a maximum sample radius of 5
				// units, but since a sphere has no thickness at its extent, we don't need to sample that far
				// out.  Only samples whole integer offsets with distance less than 25 are used.  This means
				// that there is no sample at (3, 4) because its distance is exactly 25 (and has a thickness of 0.)

				// The shaders are set up to sample a circular region within a 5-pixel radius.
				const float ScreenspaceDiameter = 10.0f;

				// SphereDiameter = CenterDepth * ThicknessMultiplier.  This will compute the thickness of a sphere centered
				// at a specific depth.  The ellipsoid scale can stretch a sphere into an ellipsoid, which changes the
				// characteristics of the AO.
				// TanHalfFovH:  Radius of sphere in depth units if its center lies at Z = 1
				// ScreenspaceDiameter:  Diameter of sample sphere in pixel units
				// ScreenspaceDiameter / BufferWidth:  Ratio of the screen width that the sphere actually covers
				// Note about the "2.0f * ":  Diameter = 2 * Radius
				float ThicknessMultiplier = 2.0f * TanHalfFovH * ScreenspaceDiameter / BufferWidth;

				if (ArrayCount == 1)
					ThicknessMultiplier *= 2.0f;

				// This will transform a depth value from [0, thickness] to [0, 1].
				float InverseRangeFactor = 1.0f / ThicknessMultiplier;

				// The thicknesses are smaller for all off-center samples of the sphere.  Compute thicknesses relative
				// to the center sample.
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][ 0] = InverseRangeFactor / m_SampleThickness[ 0];
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][ 1] = InverseRangeFactor / m_SampleThickness[ 1];
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][ 2] = InverseRangeFactor / m_SampleThickness[ 2];
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][ 3] = InverseRangeFactor / m_SampleThickness[ 3];
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][ 4] = InverseRangeFactor / m_SampleThickness[ 4];
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][ 5] = InverseRangeFactor / m_SampleThickness[ 5];
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][ 6] = InverseRangeFactor / m_SampleThickness[ 6];
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][ 7] = InverseRangeFactor / m_SampleThickness[ 7];
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][ 8] = InverseRangeFactor / m_SampleThickness[ 8];
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][ 9] = InverseRangeFactor / m_SampleThickness[ 9];
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][10] = InverseRangeFactor / m_SampleThickness[10];
				m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][11] = InverseRangeFactor / m_SampleThickness[11];

				WriteCommonCBData( &m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][i][j][12], BufferWidth, BufferHeight );
				WriteCommonCBData( &m_CBData->m_WithNormalsCBData[m_CurrentAODoubleBuffer][i][j][0], BufferWidth, BufferHeight );
			}
		}
	}

	if( (m_BlurTolerance != m_LastBlurTolerance) || (m_UpsampleTolerance != m_LastUpsampleTolerance) || (m_NoiseFilterTolerance != m_LastNoiseFilterTolerance) || forceUpdate )
	{
		m_LastBlurTolerance			 = m_BlurTolerance;
		m_LastUpsampleTolerance		 = m_UpsampleTolerance;
		m_LastNoiseFilterTolerance	 = m_NoiseFilterTolerance;
		m_CurrentBlurDoubleBuffer	^= 1;

		for(int i=0; i<BlurCBData::Count; ++i)
		{
			GpuApi::TextureRef HiResDepth;
			GpuApi::TextureRef LoResDepth;

			switch(i)
			{
			case BlurCBData::Depth0ToLinear:
				LoResDepth = m_DepthDownsize[0];
				HiResDepth = m_LinearDepth;
				break;

			case BlurCBData::Depth1to0:
				LoResDepth = m_DepthDownsize[1];
				HiResDepth = m_DepthDownsize[0];
				break;

			case BlurCBData::Depth2to1:
				LoResDepth = m_DepthDownsize[2];
				HiResDepth = m_DepthDownsize[1];
				break;

			case BlurCBData::Depth3to2:
				LoResDepth = m_DepthDownsize[3];
				HiResDepth = m_DepthDownsize[2];
				break;

			default:
				RED_HALT( "Unknown blur pass" );
				break;
			}

			GpuApi::TextureDesc loresdesc = GpuApi::GetTextureDesc( LoResDepth );
			GpuApi::TextureDesc hiresdesc = GpuApi::GetTextureDesc( HiResDepth );

			Uint32 LoWidth  = loresdesc.width;
			Uint32 LoHeight = loresdesc.height;
			Uint32 HiWidth  = hiresdesc.width;
			Uint32 HiHeight = hiresdesc.height;
			
			float kBlurTolerance = 1.0f - powf(10.0f, m_BlurTolerance) * m_verticalResolution / (float)LoWidth;
			kBlurTolerance *= kBlurTolerance;
			float kUpsampleTolerance = powf(10.0f, m_UpsampleTolerance);
			float kNoiseFilterWeight = 1.0f / (powf(10.0f, m_NoiseFilterTolerance) + kUpsampleTolerance);

			m_CBData->m_BlurCBData[m_CurrentBlurDoubleBuffer][i][0] = 1.0f / LoWidth;
			m_CBData->m_BlurCBData[m_CurrentBlurDoubleBuffer][i][1] = 1.0f / LoHeight;
			m_CBData->m_BlurCBData[m_CurrentBlurDoubleBuffer][i][2] = 1.0f / HiWidth;
			m_CBData->m_BlurCBData[m_CurrentBlurDoubleBuffer][i][3] = 1.0f / HiHeight;
			m_CBData->m_BlurCBData[m_CurrentBlurDoubleBuffer][i][4] = kNoiseFilterWeight;
			m_CBData->m_BlurCBData[m_CurrentBlurDoubleBuffer][i][5] = m_verticalResolution / (float)LoWidth;
			m_CBData->m_BlurCBData[m_CurrentBlurDoubleBuffer][i][6] = kBlurTolerance;
			m_CBData->m_BlurCBData[m_CurrentBlurDoubleBuffer][i][7] = kUpsampleTolerance;
		}
	}
	m_CBData->m_DepthDecompressCBData[m_CurrentFrameIndex].zMagicA	= zMagicA;
	m_CBData->m_DepthDecompressCBData[m_CurrentFrameIndex].zMagicB	= zMagicB;
}

void CPostFXMicrosoftSSAO::ComputeAO( CRenderShaderCompute* shader, const GpuApi::TextureRef& Destination, const Int32 depthCount, const Int32 tiledIndex, const GpuApi::TextureRef& DepthBuffer, const Bool useNormals )
{
	//ID3D11DeviceContextX* d3dContext = (ID3D11DeviceContextX *)GpuApi::Hacks::GetDeviceContext();

	if( useNormals )
	{
		Uint32 cbSize = 20 * sizeof(Float);
		void* data = GpuApi::LockBuffer( m_WithNormalsCB, GpuApi::BLF_Discard, 0, cbSize );
		Red::System::MemoryCopy( data, m_CBData->m_WithNormalsCBData[m_CurrentAODoubleBuffer][depthCount][tiledIndex], cbSize );
		GpuApi::UnlockBuffer( m_WithNormalsCB );
		GpuApi::BindConstantBuffer( 3, m_WithNormalsCB, GpuApi::ComputeShader );
		//d3dContext->CSSetPlacementConstantBuffer(0, GpuApi::Hacks::GetBuffer( m_WithNormalsCB ), m_CBData->m_WithNormalsCBData[m_CurrentAODoubleBuffer][depthCount][tiledIndex]);
	}
	else
	{
		Uint32 cbSize = 32 * sizeof(Float);
		void* data = GpuApi::LockBuffer( m_DepthOnlyCB, GpuApi::BLF_Discard, 0, cbSize );
		Red::System::MemoryCopy( data, m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][depthCount][tiledIndex], cbSize );
		GpuApi::UnlockBuffer( m_DepthOnlyCB );
		GpuApi::BindConstantBuffer( 3, m_DepthOnlyCB, GpuApi::ComputeShader );
		//d3dContext->CSSetPlacementConstantBuffer(0, GpuApi::Hacks::GetBuffer( m_DepthOnlyCB ), m_CBData->m_DepthOnlySsaoCBData[m_CurrentAODoubleBuffer][depthCount][tiledIndex]);
	}

	GpuApi::BindTextures( 0, 1, &DepthBuffer, GpuApi::ComputeShader );
	GpuApi::BindTextureUAVs( 0, 1, &Destination );

	//d3dContext->CSSetShaderResources( 0, 1, DepthBuffer );
	//d3dContext->CSSetUnorderedAccessViews( 0, 1, Destination.GetUAVPtr(), nullptr );

	GpuApi::TextureDesc depthDesc = GpuApi::GetTextureDesc( DepthBuffer );

	uint32_t BufferWidth	= depthDesc.width;
	uint32_t BufferHeight	= depthDesc.height;
	uint32_t ArrayCount		= depthDesc.sliceNum;

	if (ArrayCount == 1)
		Dispatch3D(shader, BufferWidth, BufferHeight, ArrayCount, 16, 16, 1);
	else
		Dispatch3D(shader, BufferWidth, BufferHeight, ArrayCount, 8, 8, 1);

	GpuApi::BindTextureUAVs( 0, 1, nullptr );
}


void CPostFXMicrosoftSSAO::BlurAndUpsample( GpuApi::TextureRef Destination, 
										   const GpuApi::TextureRef& HiResDepth, 
										   const GpuApi::TextureRef& LoResDepth, 
										   const BlurCBData::Enum cbIndex, 
										   GpuApi::TextureRef InterleavedAO, 
										   GpuApi::TextureRef HighQualityAO, 
										   GpuApi::TextureRef HiResAO
										   )
{
	CRenderShaderCompute* shader = nullptr;
	if (HiResAO.isNull())
	{
		shader = HighQualityAO.isNull() ? m_BlurUpsampleFinal[0] : (m_CombineResolutionsBeforeBlur ?
			(m_CombineResolutionsWithMul ? m_BlurUpsampleFinal[2] : m_BlurUpsampleFinal[1]) :
			(m_CombineResolutionsWithMul ? m_BlurUpsampleFinal[4] : m_BlurUpsampleFinal[3]));
	}
	else
	{
		shader = HighQualityAO.isNull() ? m_BlurUpsampleBlend[0] : (m_CombineResolutionsBeforeBlur ?
			(m_CombineResolutionsWithMul ? m_BlurUpsampleBlend[2] : m_BlurUpsampleBlend[1]) :
			(m_CombineResolutionsWithMul ? m_BlurUpsampleBlend[4] : m_BlurUpsampleBlend[3]));
	}

	Uint32 cbSize = 20 * sizeof(Float);
	void* data = GpuApi::LockBuffer( m_WithNormalsCB, GpuApi::BLF_Discard, 0, cbSize );
	Red::System::MemoryCopy( data, &m_CBData->m_BlurCBData[m_CurrentBlurDoubleBuffer][cbIndex][0], cbSize );
	GpuApi::UnlockBuffer( m_WithNormalsCB );
	GpuApi::BindConstantBuffer( 3, m_WithNormalsCB, GpuApi::ComputeShader );

	//d3dContext->CSSetPlacementConstantBuffer(0, GpuApi::Hacks::GetBuffer( m_WithNormalsCB ), &m_CBData->m_BlurCBData[m_CurrentBlurDoubleBuffer][cbIndex][0]);

	GpuApi::TextureRef resources[5] = {LoResDepth, HiResDepth, InterleavedAO, HighQualityAO, HiResAO};
	GpuApi::BindTextures( 0, 5, resources, GpuApi::ComputeShader );

	GpuApi::BindTextureUAVs( 0, 1, &Destination );

	GpuApi::TextureDesc depthDesc = GpuApi::GetTextureDesc( HiResDepth );

	Uint32 HiWidth  = depthDesc.width;
	Uint32 HiHeight = depthDesc.height;

	Dispatch2D( shader, HiWidth+2, HiHeight+2, 16, 16 );

	GpuApi::BindTextureUAVs( 0, 1, nullptr );

#ifdef RED_PLATFORM_DURANGO
	ID3D11DeviceContextX* d3dContext = (ID3D11DeviceContextX *)GpuApi::Hacks::GetDeviceContext();
	InsertComputeBarrier( d3dContext );
#endif
}


CPostFXMicrosoftSSAO::CPostFXMicrosoftSSAO()
{
	m_CurrentAODoubleBuffer				= 0;
	m_CurrentBlurDoubleBuffer			= 0;
	m_CurrentFrameIndex					= 0;

	m_QualityLevel						= kSsaoQualityHigh; //kSsaoQualityVeryHigh;
	m_NoiseFilterTolerance				= -3.0f;			//Config::cvNoiseFilterTolerance.Get();
	m_BlurTolerance						= -6.0f;			//Config::cvBlurTolerance.Get();
	m_UpsampleTolerance					= -3.0f;			//Config::cvUpsampleTolerance.Get();
	m_RejectionFalloff					= 8.0f;				//Config::cvRejectionFalloff.Get();
	m_CombineResolutionsBeforeBlur		= true;				//Config::cvCombineResolutionsBeforeBlur.Get();
	m_CombineResolutionsWithMul			= false;			//Config::cvCombineResolutionsWithMul.Get();
	m_HierarchyDepth					= 2;				//Config::cvHierarchyDepth.Get();
	m_NormalAOMultiply					= 1.7f;				//Config::cvNormalAOMultiply.Get();

	m_LastTanHalfFovH					= 0.0f;
	m_LastRejectionFalloff				= m_RejectionFalloff - 1.0f;
	m_LastNormalAOMultiply				= m_NormalAOMultiply - 1.0f;
	m_LastBlurTolerance					= m_BlurTolerance - 1.0f;
	m_LastUpsampleTolerance				= m_UpsampleTolerance - 1.0f;
	m_LastNoiseFilterTolerance			= m_NoiseFilterTolerance - 1.0f;
	m_LastNormalBackProjectionTolerance	= m_NormalBackProjectionTolerance - 1.0f;

	m_NormalToDepthBrightnessEqualiser	= 100.0f;	//Config::cvNormalToDepthBrightnessEqualiser.Get();	// includes compensation for fade by depth, possibly ought to be a far plane multiply?
	m_LastNormalToDepthBrightnessEqualiser = m_NormalToDepthBrightnessEqualiser - 1.0f;
	m_NormalBackProjectionTolerance		= 0.0f;		//Config::cvNormalBackProjectionTolerance.Get();

	m_SampleThickness[ 0]				= sqrt(1.0f - 0.2f * 0.2f);
	m_SampleThickness[ 1]				= sqrt(1.0f - 0.4f * 0.4f);
	m_SampleThickness[ 2]				= sqrt(1.0f - 0.6f * 0.6f);
	m_SampleThickness[ 3]				= sqrt(1.0f - 0.8f * 0.8f);
	m_SampleThickness[ 4]				= sqrt(1.0f - 0.2f * 0.2f - 0.2f * 0.2f);
	m_SampleThickness[ 5]				= sqrt(1.0f - 0.2f * 0.2f - 0.4f * 0.4f);
	m_SampleThickness[ 6]				= sqrt(1.0f - 0.2f * 0.2f - 0.6f * 0.6f);
	m_SampleThickness[ 7]				= sqrt(1.0f - 0.2f * 0.2f - 0.8f * 0.8f);
	m_SampleThickness[ 8]				= sqrt(1.0f - 0.4f * 0.4f - 0.4f * 0.4f);
	m_SampleThickness[ 9]				= sqrt(1.0f - 0.4f * 0.4f - 0.6f * 0.6f);
	m_SampleThickness[10]				= sqrt(1.0f - 0.4f * 0.4f - 0.8f * 0.8f);
	m_SampleThickness[11]				= sqrt(1.0f - 0.6f * 0.6f - 0.6f * 0.6f);

	m_CBData							= nullptr;

	m_LastScreenWidth = -1;
	m_LastScreenHeight = -1;

	Init();
}

//template <size_t TBinarySize>
//void CPostFXMicrosoftSSAO::CreateShader( ID3D11ComputeShader *&shader, const uint8_t (&shaderBinary)[TBinarySize] )
//{
//	ID3D11DeviceX* device = (ID3D11DeviceX *)GpuApi::Hacks::GetDevice();
//
//	GPUAPI_MUST_SUCCEED(device->CreateComputeShader(shaderBinary, TBinarySize, NULL, &shader));
//}

void CPostFXMicrosoftSSAO::Init()
{
	//// Samplers

#ifdef RED_PLATFORM_DURANGO
	ID3D11DeviceX* device = (ID3D11DeviceX *)GpuApi::Hacks::GetDevice();

	D3D11_SAMPLER_DESC SamplerDesc;
	ZeroMemory(&SamplerDesc, sizeof(SamplerDesc));
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 0.0f;
	GPUAPI_MUST_SUCCEED( device->CreateSamplerState( &SamplerDesc, (ID3D11SamplerState **)&m_Samplers[Samplers::PointBorder] ) );

	ZeroMemory(&SamplerDesc, sizeof(SamplerDesc));
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.MinLOD = -FLT_MAX;
	SamplerDesc.MaxLOD = FLT_MAX;
	SamplerDesc.MipLODBias = 0.0f;
	SamplerDesc.MaxAnisotropy = 16;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	GPUAPI_MUST_SUCCEED( device->CreateSamplerState( &SamplerDesc, (ID3D11SamplerState **)&m_Samplers[Samplers::LinearClamp] ) );

	D3D11X_SAMPLER_DESC SamplerDescX;
	ZeroMemory(&SamplerDescX, sizeof(SamplerDescX));
	SamplerDescX.ClampX = D3D11X_TEX_CLAMP_BORDER;
	SamplerDescX.ClampY = D3D11X_TEX_CLAMP_BORDER;
	SamplerDescX.ClampZ = D3D11X_TEX_CLAMP_BORDER;
	SamplerDescX.MaxAnisotropicRatio = D3D11X_TEX_ANISO_RATIO_4;
	SamplerDescX.DepthCompareFunction = D3D11X_TEX_DEPTH_COMPARE_NEVER;
	SamplerDescX.ForceUnnormalized = FALSE;
	SamplerDescX.AnisotropicThreshold = 0; // [0,7]
	SamplerDescX.MCCoordTrunc = FALSE;
	SamplerDescX.ForceDegamma = FALSE;
	SamplerDescX.AnisotropicBias = D3DFloatToUnsigned_1_5(0.0f);
	SamplerDescX.TruncateCoordinates = FALSE;
	SamplerDescX.DisableCubemapWrap = FALSE; // disable seamless cubemap edges/corners?
	SamplerDescX.FilterMode = D3D11X_TEX_FILTER_MODE_MAX;
	SamplerDescX.MinLOD = D3DFloatToUnsigned_4_8(0.0f);
	SamplerDescX.MaxLOD = D3DFloatToUnsigned_4_8(16.0f);
	SamplerDescX.PerfMip = 0;
	SamplerDescX.PerfZ = 0;
	SamplerDescX.LODBias = D3DFloatToSigned_5_8(0.0f);
	SamplerDescX.LODBiasSecondary = D3DFloatToSigned_1_4(0.0f);
	SamplerDescX.XYMagFilter = D3D11X_TEX_XY_FILTER_BILINEAR; // point, bilinear, aniso_point, aniso_bilinear
	SamplerDescX.XYMinFilter = D3D11X_TEX_XY_FILTER_ANISO_BILINEAR;
	SamplerDescX.ZFilter = D3D11X_TEX_Z_FILTER_NONE; // none, point, linear
	SamplerDescX.MipFilter = D3D11X_TEX_MIP_FILTER_LINEAR; // none, point, linear
	SamplerDescX.MipPointPreclamp = FALSE;
	SamplerDescX.DisableLSBCeil = FALSE;
	SamplerDescX.FilterPrecisionFix = FALSE;
	SamplerDescX.BorderColorIndex = 0;
	SamplerDescX.BorderColorType = D3D11X_TEX_BORDER_COLOR_TRANS_BLACK; // trans_black, opaque_black, opaque_white, index

	GPUAPI_MUST_SUCCEED( device->CreateSamplerStateX(&SamplerDescX, (ID3D11SamplerState **)&m_Samplers[Samplers::MaxBorder] ) );
#else
	GpuApi::SamplerStateDesc samplerDesc;
	samplerDesc.filterMin = GpuApi::TEXFILTERMIN_Point;
	samplerDesc.filterMag = GpuApi::TEXFILTERMAG_Point;
	samplerDesc.filterMip = GpuApi::TEXFILTERMIP_Point;
	samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = GpuApi::TEXADDR_Border;
	samplerDesc.borderColor[0] = samplerDesc.borderColor[1] = samplerDesc.borderColor[2] = samplerDesc.borderColor[3] = 0.0f;
	m_Samplers[Samplers::PointBorder] = GpuApi::RequestSamplerState( samplerDesc );

	samplerDesc.filterMin = GpuApi::TEXFILTERMIN_Linear;
	samplerDesc.filterMag = GpuApi::TEXFILTERMAG_Linear;
	samplerDesc.filterMip = GpuApi::TEXFILTERMIP_Linear;
	samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = GpuApi::TEXADDR_Clamp;
	m_Samplers[Samplers::LinearClamp] = GpuApi::RequestSamplerState( samplerDesc );

	samplerDesc.filterMin = GpuApi::TEXFILTERMIN_Linear;
	samplerDesc.filterMag = GpuApi::TEXFILTERMAG_Linear;
	samplerDesc.filterMip = GpuApi::TEXFILTERMIP_Linear;
	samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = GpuApi::TEXADDR_Border;
	samplerDesc.borderColor[0] = samplerDesc.borderColor[1] = samplerDesc.borderColor[2] = samplerDesc.borderColor[3] = 0.0f;
	m_Samplers[Samplers::MaxBorder] = GpuApi::RequestSamplerState( samplerDesc );
#endif

	m_DepthPrepare1WithNormalsCS =			GetRenderer()->m_AoPrepareDepthAndNormalBuffers1CS;
	m_DepthPrepare1CS =						GetRenderer()->m_AoPrepareDepthBuffers1CS;
	m_DepthPrepare2CS =						GetRenderer()->m_AoPrepareDepthBuffers2CS;
	m_LinearizeDepthCS =					GetRenderer()->m_AoLinearizeDepthCS;
	m_Render1CS =							GetRenderer()->m_AoRender1CS;
	m_Render2CS =							GetRenderer()->m_AoRender2CS;
	m_RenderNormals1CS =					GetRenderer()->m_AoRenderNormals1CS;
	m_RenderNormals2CS =					GetRenderer()->m_AoRenderNormals2CS;
	m_BlurUpsampleBlend[0] =				GetRenderer()->m_AoBlurUpsampleBlendOutCS;
	m_BlurUpsampleBlend[1] =				GetRenderer()->m_AoBlurUpsamplePreMinBlendOutCS;
	m_BlurUpsampleBlend[2] =				GetRenderer()->m_AoBlurUpsamplePreMulBlendOutCS;
	m_BlurUpsampleBlend[3] =				GetRenderer()->m_AoBlurUpsamplePostMinBlendOutCS;
	m_BlurUpsampleBlend[4] =				GetRenderer()->m_AoBlurUpsamplePostMulBlendOutCS;
	m_BlurUpsampleFinal[0] =				GetRenderer()->m_AoBlurUpsampleCS;
	m_BlurUpsampleFinal[1] =				GetRenderer()->m_AoBlurUpsamplePreMinCS;
	m_BlurUpsampleFinal[2] =				GetRenderer()->m_AoBlurUpsamplePreMulCS;
	m_BlurUpsampleFinal[3] =				GetRenderer()->m_AoBlurUpsamplePostMinCS;
	m_BlurUpsampleFinal[4] =				GetRenderer()->m_AoBlurUpsamplePostMulCS;

	// constant buffer data
	m_CBData = (ConstantBufferData *) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RenderData, sizeof(ConstantBufferData) ); //VirtualAlloc(nullptr, sizeof(ConstantBufferData), MEM_GRAPHICS | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE | PAGE_GPU_COHERENT);

	m_DepthOnlyCB = GpuApi::CreateBuffer( 32 * sizeof(Float), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
	m_WithNormalsCB = GpuApi::CreateBuffer( 20 * sizeof(Float), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
	m_DepthDecompressCB = GpuApi::CreateBuffer( sizeof(DepthData), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
	m_DepthToPosCB = GpuApi::CreateBuffer( sizeof(DepthToPosData), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
	m_InverseRTSizeCB = GpuApi::CreateBuffer( 4 * sizeof(Float), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
};

void CPostFXMicrosoftSSAO::Deinit()
{
	// constant buffers
	GpuApi::SafeRelease( m_DepthOnlyCB );
	GpuApi::SafeRelease( m_WithNormalsCB );
	GpuApi::SafeRelease( m_DepthDecompressCB );
	GpuApi::SafeRelease( m_DepthToPosCB );
	GpuApi::SafeRelease( m_InverseRTSizeCB );

	//constant data
	RED_MEMORY_FREE( MemoryPool_Default, MC_RenderData, m_CBData );

	// samplers
#ifdef RED_PLATFORM_DURANGO
	if( m_Samplers[Samplers::MaxBorder] ) ((ID3D11SamplerState *)m_Samplers[Samplers::MaxBorder])->Release();
	if( m_Samplers[Samplers::LinearClamp] ) ((ID3D11SamplerState *)m_Samplers[Samplers::LinearClamp])->Release();
	if( m_Samplers[Samplers::PointBorder] ) ((ID3D11SamplerState *)m_Samplers[Samplers::PointBorder])->Release();
#else
	GpuApi::SafeRelease( m_Samplers[Samplers::MaxBorder] );
	GpuApi::SafeRelease( m_Samplers[Samplers::LinearClamp] );
	GpuApi::SafeRelease( m_Samplers[Samplers::PointBorder] );
#endif

	ReleaseTextures();
}


Bool CPostFXMicrosoftSSAO::EnsureTexturesAllocated( class CRenderSurfaces* surfaces )
{
	const Int32 width = surfaces->GetWidth( true );
	const Int32 height = surfaces->GetHeight( true );

	if ( width <= 0 || height <= 0 )
	{
		ReleaseTextures();
		return false;
	}


#if MICROSOFT_ATG_DYNAMIC_SCALING
	// With dynamic scaling, only create once with the full unscaled size.
	if ( m_LastScreenWidth > 0 || m_LastScreenHeight > 0 )
	{
		return true;
	}
#endif

	{
		Int32 currentWidth;
		Int32 currentHeight;
		GetScreenSize( currentWidth, currentHeight );
		if ( width == currentWidth && height == currentHeight )
		{
			return true;
		}
	}


	ReleaseTextures();


	// buffers
	const uint32_t bufferWidth1 = (width + 1) / 2;
	const uint32_t bufferWidth2 = (width + 3) / 4;
	const uint32_t bufferWidth3 = (width + 7) / 8;
	const uint32_t bufferWidth4 = (width + 15) / 16;
	const uint32_t bufferWidth5 = (width + 31) / 32;
	const uint32_t bufferWidth6 = (width + 63) / 64;
	const uint32_t bufferHeight1 = (height + 1) / 2;
	const uint32_t bufferHeight2 = (height + 3) / 4;
	const uint32_t bufferHeight3 = (height + 7) / 8;
	const uint32_t bufferHeight4 = (height + 15) / 16;
	const uint32_t bufferHeight5 = (height + 31) / 32;
	const uint32_t bufferHeight6 = (height + 63) / 64;

	// we will allocate into the last 8 mb of ESRAM, this is the only area that is not used for something else at the same time
	// 24 mb offset
	Uint32 esramOffset = 0x18D0000;

	GpuApi::eTextureFormat depthFormat = GpuApi::TEXFMT_Float_R32;

	m_NormalBufferDownsize = Create2DTexture( bufferWidth1, bufferHeight1, GpuApi::TEXFMT_Float_R11G11B10, "NormalBufferDownsize", esramOffset, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO2 );
	esramOffset = AlignUp( esramOffset + GpuApi::CalcTextureSize( m_NormalBufferDownsize ), 64 * 1024 );

	m_NormalBufferTiled = CreateArrayTexture( bufferWidth3, bufferHeight3, 16, GpuApi::TEXFMT_Float_R11G11B10, "NormalBufferTiled", esramOffset, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO8 );
	esramOffset = AlignUp( esramOffset + GpuApi::CalcTextureSize( m_NormalBufferTiled ), 64 * 1024 );

	//--------------------------------------------

	m_DepthDownsize[0] = Create2DTexture( bufferWidth1, bufferHeight1, depthFormat, "DepthDownsize[0]", esramOffset, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO2 );
	esramOffset = AlignUp( esramOffset + GpuApi::CalcTextureSize( m_DepthDownsize[0] ), 64 * 1024 );

	m_DepthTiled[0] = CreateArrayTexture( bufferWidth3, bufferHeight3, 16, depthFormat, "DepthTiled[0]", esramOffset, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO8 );
	esramOffset = AlignUp( esramOffset + GpuApi::CalcTextureSize( m_DepthTiled[0] ), 64 * 1024 );

	m_AOMerged[0] = Create2DTexture( bufferWidth1, bufferHeight1, GpuApi::TEXFMT_L8, "AOMerged[0]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO2 );
	m_AOHighQuality[0] = Create2DTexture( bufferWidth1, bufferHeight1, GpuApi::TEXFMT_L8, "AOHighQuality[0]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO2 );
	m_AOSmooth[0] = Create2DTexture( bufferWidth1, bufferHeight1, GpuApi::TEXFMT_L8, "AOSmooth[0]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO2 );

	//--------------------------------------------

	m_DepthDownsize[1] = Create2DTexture( bufferWidth2, bufferHeight2, depthFormat, "DepthDownsize[1]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO4 );
	m_DepthTiled[1] = CreateArrayTexture( bufferWidth4, bufferHeight4, 16, depthFormat, "DepthTiled[1]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO16 );
	m_AOMerged[1] = Create2DTexture( bufferWidth2, bufferHeight2, GpuApi::TEXFMT_L8, "AOMerged[1]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO4 );
	m_AOHighQuality[1] = Create2DTexture( bufferWidth2, bufferHeight2, GpuApi::TEXFMT_L8, "AOHighQuality[1]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO4 );
	m_AOSmooth[1] = Create2DTexture( bufferWidth2, bufferHeight2, GpuApi::TEXFMT_L8, "AOSmooth[1]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO4 );

	//--------------------------------------------

	m_DepthDownsize[2] = Create2DTexture( bufferWidth3, bufferHeight3, depthFormat, "DepthDownsize[2]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO8 );
	m_DepthTiled[2] = CreateArrayTexture( bufferWidth5, bufferHeight5, 16, depthFormat, "DepthTiled[2]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO32 );
	m_AOMerged[2] = Create2DTexture( bufferWidth3, bufferHeight3, GpuApi::TEXFMT_L8, "AOMerged[2]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO8 );
	m_AOHighQuality[2] = Create2DTexture( bufferWidth3, bufferHeight3, GpuApi::TEXFMT_L8, "AOHighQuality[2]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO8 );
	m_AOSmooth[2] = Create2DTexture( bufferWidth3, bufferHeight3, GpuApi::TEXFMT_L8, "AOSmooth[2]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO8 );

	//--------------------------------------------

	m_DepthDownsize[3] = Create2DTexture( bufferWidth4, bufferHeight4, depthFormat, "DepthDownsize[3]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO16 );
	m_DepthTiled[3] = CreateArrayTexture( bufferWidth6, bufferHeight6, 16, depthFormat, "DepthTiled[3]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO64 );
	m_AOHighQuality[3] = Create2DTexture( bufferWidth4, bufferHeight4, GpuApi::TEXFMT_L8, "AOHighQuality[3]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO16 );
	m_AOMerged[3] = Create2DTexture( bufferWidth4, bufferHeight4, GpuApi::TEXFMT_L8, "AOMerged[3]", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO16 );

	//--------------------------------------------

	m_LinearDepth = Create2DTexture( width, height, depthFormat, "LinearDepth", 0, GpuApi::TEXUSAGE_DynamicScalingAlignSSAO1 );

	return true;
}

void CPostFXMicrosoftSSAO::ReleaseTextures()
{
	GpuApi::SafeRelease( m_LinearDepth );
	GpuApi::SafeRelease( m_NormalBufferDownsize );
	GpuApi::SafeRelease( m_NormalBufferTiled );
	GpuApi::SafeRelease( m_DepthDownsize[0] );
	GpuApi::SafeRelease( m_DepthDownsize[1] );
	GpuApi::SafeRelease( m_DepthDownsize[2] );
	GpuApi::SafeRelease( m_DepthDownsize[3] );
	GpuApi::SafeRelease( m_DepthTiled[0] );
	GpuApi::SafeRelease( m_DepthTiled[1] );
	GpuApi::SafeRelease( m_DepthTiled[2] );
	GpuApi::SafeRelease( m_DepthTiled[3] );
	GpuApi::SafeRelease( m_AOMerged[0] );
	GpuApi::SafeRelease( m_AOMerged[1] );
	GpuApi::SafeRelease( m_AOMerged[2] );
	GpuApi::SafeRelease( m_AOMerged[3] );
	GpuApi::SafeRelease( m_AOHighQuality[0] );
	GpuApi::SafeRelease( m_AOHighQuality[1] );
	GpuApi::SafeRelease( m_AOHighQuality[2] );
	GpuApi::SafeRelease( m_AOHighQuality[3] );
	GpuApi::SafeRelease( m_AOSmooth[0] );
	GpuApi::SafeRelease( m_AOSmooth[1] );
	GpuApi::SafeRelease( m_AOSmooth[2] );

	m_LastScreenWidth = -1;
	m_LastScreenHeight = -1;
}


void CPostFXMicrosoftSSAO::GetScreenSize( Int32& outWidth, Int32& outHeight )
{
	if ( m_LinearDepth.isNull() )
	{
		outWidth = -1;
		outHeight = -1;
		return;
	}

	const GpuApi::TextureDesc &depthDesc = GpuApi::GetTextureDesc( m_LinearDepth );
	RED_FATAL_ASSERT( depthDesc.width > 0 && depthDesc.height > 0, "Expected non zero resolution" );

	// Get current resolution
	outWidth = (Int32)depthDesc.width;
	outHeight = (Int32)depthDesc.height;
}


void CPostFXMicrosoftSSAO::Apply( CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, ERenderTargetName applyTarget, ERenderTargetName tempTarget, const CRenderFrameInfo& info )
{
	// Make sure we have render target textures sized appropriately with the current resolution.
	if ( !EnsureTexturesAllocated( surfaces ) )
	{
		ERR_RENDERER( TXT("Could not set up SSAO render targets. %ux%u"), surfaces->GetWidth(), surfaces->GetHeight() );
		return;
	}


	// TODO, add a function called on depth buffer change
	GpuApi::TextureRef texDepth	= surfaces->GetDepthBufferTex();

	const CEnvMSSSAOParametersAtPoint& ssaoParams = info.m_envParametersArea.m_msSsao;
	m_NoiseFilterTolerance				= ssaoParams.m_noiseFilterTolerance.GetScalar();				//Config::cvNoiseFilterTolerance.Get();
	m_BlurTolerance						= ssaoParams.m_blurTolerance.GetScalar();						//Config::cvBlurTolerance.Get();
	m_UpsampleTolerance					= ssaoParams.m_upsampleTolerance.GetScalar();					//Config::cvUpsampleTolerance.Get();
	m_RejectionFalloff					= ssaoParams.m_rejectionFalloff.GetScalar();					//Config::cvRejectionFalloff.Get();
	{
		// ttp#131233 hacky fix
		//
		// We had a gpu crash on gtx760 (plus on other gtx7** cards, but didn't see it myself) with m_CombineResolutionsBeforeBlur==false.
		// I'm unable to properly fix it at this moment since it looks like some deep driver issue, so we agreed with lighting artists to 
		// force the value to true. Seems like it was setup this way everywhere anyway, and the bug showed itself with some test environment.
		//
		// I'm forcing value of *withMul parameter here also, but it's not related to the gpu crash. I'm doing it to prevent SSAO popping when 
		// entering/leaving environments with different values. It might have happen because both of these parameters are not blendable and 
		// shouldn't even be in the environments.

		m_CombineResolutionsBeforeBlur		= true; //ssaoParams.m_combineResolutionsBeforeBlur;					//Config::cvCombineResolutionsBeforeBlur.Get();
		m_CombineResolutionsWithMul			= true; //ssaoParams.m_combineResolutionsWithMul;						//Config::cvCombineResolutionsWithMul.Get();
	}
	m_HierarchyDepth					= (Int32)ssaoParams.m_hierarchyDepth.GetScalar();						//Config::cvHierarchyDepth.Get();
	m_NormalAOMultiply					= ssaoParams.m_normalAOMultiply.GetScalar();					//Config::cvNormalAOMultiply.Get();
	m_NormalBackProjectionTolerance     = ssaoParams.m_normalBackProjectionTolerance.GetScalar();		//Config::cvNormalBackProjectionTolerance.Get();
	
	const Bool UseNormals = info.m_worldRenderSettings.m_ssaoNormalsEnable;

	CRenderCamera camera = info.m_camera;

	// There is some huge precision problem with this when camera is away from (0,0,0). Low fov values make this even worse.
	// So the solution is to move the camera to the world origin and let ssao operate on this.
	// Note that this happens only when using normals for ssao, but I'm doing it always for consistency.
	// Camera is kept as initialized by the original camera since there is some stuff that we might want to keep (subpixel offsets or other stuff).
	camera.Set( Vector ( 0, 0, 0, 1 ), info.m_camera.GetRotation(), info.m_camera.GetFOV(), info.m_camera.GetAspect(), info.m_camera.GetNearPlane(), info.m_camera.GetFarPlane(), info.m_camera.GetZoom(), info.m_camera.IsReversedProjection() );

	// Load first element of projection matrix which is the cotangent of the horizontal FOV divided by 2.
	const float* pProjMat = reinterpret_cast<const float*>(&camera.GetViewToScreenRevProjAware());
	const float TanHalfFovH = 1.0f / pProjMat[0];
	const float Near = camera.GetNearPlane();
	const float Far = camera.GetFarPlane();
	m_NormalToDepthBrightnessEqualiser = Far / 50.0f;
	const float rDelta = 1.0f / (Far - Near);
	const float zMagicA = Far * rDelta;
	const float zMagicB = (-Far * Near) * rDelta;
	const float zMagic = (Far - Near) / Near;

	UpdateCBs( TanHalfFovH, zMagic, 0.0f );

	// Phase 1:  Decompress, linearize, downsample, and deinterleave the depth buffer

	Uint32 cbSize = sizeof(DepthData);
	void* data = GpuApi::LockBuffer( m_DepthDecompressCB, GpuApi::BLF_Discard, 0, cbSize );
	Red::System::MemoryCopy( data, &m_CBData->m_DepthDecompressCBData[m_CurrentFrameIndex], cbSize );
	GpuApi::UnlockBuffer( m_DepthDecompressCB );
	GpuApi::BindConstantBuffer( 3, m_DepthDecompressCB, GpuApi::ComputeShader );

	//d3dContext->CSSetPlacementConstantBuffer(0, GpuApi::Hacks::GetBuffer(m_DepthDecompressCB), &m_CBData->m_DepthDecompressCBData[m_CurrentFrameIndex]);

#ifdef RED_PLATFORM_DURANGO
	ID3D11DeviceContextX* d3dContext = (ID3D11DeviceContextX *)GpuApi::Hacks::GetDeviceContext();

	d3dContext->CSSetSamplers( 0, 2, (ID3D11SamplerState **)m_Samplers );
#else
	GpuApi::SetSamplerState( 0, m_Samplers[0], GpuApi::ComputeShader );
	GpuApi::SetSamplerState( 1, m_Samplers[1], GpuApi::ComputeShader );
#endif

	//set empty setup
	GpuApi::RenderTargetSetup rtSetup;
	GpuApi::SetupRenderTargets( rtSetup );
	//d3dContext->OMSetRenderTargets(0, nullptr, nullptr);

	if( UseNormals )
	{
		GpuApi::TextureRef uavs[ 7 ] = { m_LinearDepth, m_DepthDownsize[0], m_DepthTiled[0], m_DepthDownsize[1], m_DepthTiled[1], m_NormalBufferDownsize, m_NormalBufferTiled };
		GpuApi::BindTextureUAVs( 0, 7, uavs );
	}
	else
	{
		GpuApi::TextureRef uavs[ 5 ] = { m_LinearDepth, m_DepthDownsize[0], m_DepthTiled[0], m_DepthDownsize[1], m_DepthTiled[1] };
		GpuApi::BindTextureUAVs( 0, 5, uavs );
	}

#ifdef RED_PLATFORM_DURANGO
	// Make sure writes to the depth buffer complete first.
	d3dContext->InsertWaitUntilIdle(0);
#endif

	if( UseNormals )
	{
		GpuApi::TextureRef textures[2] = { texDepth, surfaces->GetRenderTargetTex( RTN_GBuffer1 )};

		GpuApi::BindTextures( 0, 2, textures, GpuApi::ComputeShader );
		//SetShaderResources( d3dContext, depthSrv, GpuApi::Hacks::GetTextureSRV( texNormals ) );
	}
	else
	{
		GpuApi::BindTextures( 0, 1, &texDepth, GpuApi::ComputeShader );
		//SetShaderResources( d3dContext, depthSrv );
	}
	const GpuApi::TextureDesc &depthTiled1Desc = GpuApi::GetTextureDesc( m_DepthTiled[1] );
	DispatchFullScreenCompute( UseNormals ? m_DepthPrepare1WithNormalsCS : m_DepthPrepare1CS, depthTiled1Desc.width * 8, depthTiled1Desc.height * 8 );

#ifdef RED_PLATFORM_DURANGO
	InsertComputeBarrier( d3dContext );
#endif

	if ((int)m_HierarchyDepth > 2)
	{
		GpuApi::BindConstantBuffer( 3, m_InverseRTSizeCB, GpuApi::ComputeShader );
		//d3dContext->CSSetFastConstantBuffer(0, m_InverseRTSizeCB);

		GpuApi::TextureRef uavs[ 4 ] = { m_DepthDownsize[2], m_DepthTiled[2], m_DepthDownsize[3], m_DepthTiled[3] };
		GpuApi::BindTextureUAVs( 0, 4, uavs );
		//SetUnorderedAccessViews( d3dContext, m_DepthDownsize[2].GetUAV(), m_DepthTiled[2].GetUAV(), m_DepthDownsize[3].GetUAV(), m_DepthTiled[3].GetUAV() );

		GpuApi::BindTextures( 0, 1, &m_DepthDownsize[1], GpuApi::ComputeShader );
		//SetShaderResources( d3dContext, m_DepthDownsize[1].GetSRV() );

		const GpuApi::TextureDesc &depthTiled3Desc = GpuApi::GetTextureDesc( m_DepthTiled[3] );
		DispatchFullScreenCompute( m_DepthPrepare2CS, depthTiled3Desc.width * 8, depthTiled3Desc.height * 8 );
#ifdef RED_PLATFORM_DURANGO
		InsertComputeBarrier( d3dContext );
#endif
	}

	GpuApi::BindTextureUAVs( 0, 7, nullptr );
	//SetUnorderedAccessViews( d3dContext, nullptr );

	// Phase 2:  Render SSAO for each sub-tile
	if ((int)m_HierarchyDepth > 3)
	{
		ComputeAO( m_Render1CS, m_AOMerged[3], 3, 0, m_DepthTiled[3] );
		if (m_QualityLevel >= kSsaoQualityLow)
		{
			ComputeAO( m_Render2CS, m_AOHighQuality[3], 3, 1, m_DepthDownsize[3] );
		}
	}
	if ((int)m_HierarchyDepth > 2)
	{
		ComputeAO( m_Render1CS, m_AOMerged[2], 2, 0, m_DepthTiled[2] );
		if (m_QualityLevel >= kSsaoQualityMedium)  
		{
			ComputeAO( m_Render2CS, m_AOHighQuality[2], 2, 1, m_DepthDownsize[2] );
		}
	}
	if ((int)m_HierarchyDepth > 1)
	{
		ComputeAO( m_Render1CS, m_AOMerged[1], 1, 0, m_DepthTiled[1] );
		if (m_QualityLevel >= kSsaoQualityHigh)	   
		{
			ComputeAO( m_Render2CS, m_AOHighQuality[1], 1, 1, m_DepthDownsize[1] );
		}
	}

	if(UseNormals)
	{
		Vector corners[3];

		// Calc corners for far plane
		corners[0] = camera.GetScreenToWorld().TransformVectorWithW( Vector(-1,-1,+1,+1) );
		corners[1] = camera.GetScreenToWorld().TransformVectorWithW( Vector(+1,-1,+1,+1) );
		corners[2] = camera.GetScreenToWorld().TransformVectorWithW( Vector(-1,+1,+1,+1) );
		corners[0].Div4( corners[0].W );
		corners[1].Div4( corners[1].W );
		corners[2].Div4( corners[2].W );

		m_CBData->m_DepthToPosData[m_CurrentFrameIndex].cameraPos = camera.GetPosition();
		m_CBData->m_DepthToPosData[m_CurrentFrameIndex].origin = corners[0];
		m_CBData->m_DepthToPosData[m_CurrentFrameIndex].hDelta = corners[1] - corners[0];
		m_CBData->m_DepthToPosData[m_CurrentFrameIndex].vDelta = corners[2] - corners[0];

		Uint32 cbSize = sizeof(DepthToPosData);
		void* data = GpuApi::LockBuffer( m_DepthToPosCB, GpuApi::BLF_Discard, 0, cbSize );
		Red::System::MemoryCopy( data, &m_CBData->m_DepthToPosData[m_CurrentFrameIndex], cbSize );
		GpuApi::UnlockBuffer( m_DepthToPosCB );
		GpuApi::BindConstantBuffer( 1, m_DepthToPosCB, GpuApi::ComputeShader );
		//d3dContext->CSSetPlacementConstantBuffer(1, m_DepthToPosCB, &m_CBData->m_DepthToPosData[m_CurrentFrameIndex]);

		GpuApi::BindTextures( 1, 1, &m_NormalBufferTiled, GpuApi::ComputeShader );
		//d3dContext->CSSetShaderResources( 1, 1, m_NormalBufferTiled.GetSRVPtr() );
		ComputeAO( m_RenderNormals1CS, m_AOMerged[0], 0, 0, m_DepthTiled[0], true );
		if (m_QualityLevel >= kSsaoQualityVeryHigh)
		{
			GpuApi::BindTextures( 1, 1, &m_NormalBufferDownsize, GpuApi::ComputeShader );
			//d3dContext->CSSetShaderResources( 1, 1, m_NormalBufferDownsize.GetSRVPtr() );
			ComputeAO( m_RenderNormals2CS, m_AOHighQuality[0], 0, 1, m_DepthDownsize[0], true );
		}
	}
	else
	{
		ComputeAO( m_Render1CS, m_AOMerged[0], 0, 0, m_DepthTiled[0] );
		if (m_QualityLevel >= kSsaoQualityVeryHigh)
		{
			ComputeAO( m_Render2CS, m_AOHighQuality[0], 0, 1, m_DepthDownsize[0] );
		}
	}

#ifdef RED_PLATFORM_DURANGO
	InsertComputeBarrier( d3dContext );
#endif

	GpuApi::BindTextureUAVs( 0, 7, nullptr );
	//d3dContext->CSSetUnorderedAccessViews( 0, 1, &NullUAV, nullptr );

#ifdef RED_PLATFORM_DURANGO
	d3dContext->CSSetSamplers( 0, 1, (ID3D11SamplerState **)&m_Samplers[Samplers::LinearClamp] );
#else
	GpuApi::SetSamplerState( 0, m_Samplers[Samplers::LinearClamp], GpuApi::ComputeShader );
#endif	

	// Phase 4:  Iteratively blur and upsample, combining each result
	GpuApi::TextureRef NextSRV = m_AOMerged[3];

	// 120 x 68 -> 240 x 135
	if ((int)m_HierarchyDepth > 3)
	{
		BlurAndUpsample( m_AOSmooth[2], m_DepthDownsize[2], m_DepthDownsize[3], BlurCBData::Enum::Depth3to2, NextSRV,
			m_QualityLevel >= kSsaoQualityLow ? m_AOHighQuality[3] : GpuApi::TextureRef::Null(), m_AOMerged[2] );

		NextSRV = m_AOSmooth[2];
	}
	else
		NextSRV = m_AOMerged[2];


	// 240 x 135 -> 480 x 270
	if ((int)m_HierarchyDepth > 2)
	{
		BlurAndUpsample( m_AOSmooth[1], m_DepthDownsize[1], m_DepthDownsize[2], BlurCBData::Enum::Depth2to1, NextSRV,
			m_QualityLevel >= kSsaoQualityMedium ? m_AOHighQuality[2] : GpuApi::TextureRef::Null(), m_AOMerged[1] );

		NextSRV = m_AOSmooth[1];
	}
	else
		NextSRV = m_AOMerged[1];


	// 480 x 270 -> 960 x 540
	if ((int)m_HierarchyDepth > 1)
	{
		BlurAndUpsample( m_AOSmooth[0], m_DepthDownsize[0], m_DepthDownsize[1], BlurCBData::Enum::Depth1to0, NextSRV,
			m_QualityLevel >= kSsaoQualityHigh ? m_AOHighQuality[1] : GpuApi::TextureRef::Null(), m_AOMerged[0] );

		NextSRV = m_AOSmooth[0];
	}
	else
		NextSRV = m_AOMerged[0];

	// 960 x 540 -> 1920 x 1080
	BlurAndUpsample( surfaces->GetRenderTargetTex( applyTarget ), m_LinearDepth, m_DepthDownsize[0], BlurCBData::Enum::Depth0ToLinear, NextSRV,
		m_QualityLevel >= kSsaoQualityVeryHigh ? m_AOHighQuality[0] : GpuApi::TextureRef::Null(), GpuApi::TextureRef::Null() );

	GpuApi::BindTextures( 0, 5, nullptr, GpuApi::ComputeShader );
	//SetShaderResources(d3dContext, nullptr);

	GpuApi::BindTextureUAVs( 0, 7, nullptr );
	//d3dContext->CSSetUnorderedAccessViews( 0, 1, &NullUAV, nullptr );

	m_CurrentFrameIndex ^= 1;
}

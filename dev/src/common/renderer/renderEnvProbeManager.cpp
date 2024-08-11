/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderEnvProbeManager.h"
#include "renderPostProcess.h"
#include "renderShaderPair.h"
#include "renderRenderSurfaces.h"
#include "renderEnvProbe.h"
#include "renderCollector.h"
#include "renderScene.h"
#include "renderProxyLight.h"
#include "../redMath/float16compressor.h"
#include "../engine/textureCache.h"
#include "renderCube.h"

namespace Config
{
	TConfigVar< Float >		cvEnvProbeLightsMaxDistance( "Rendering", "EnvProbeLightsMaxDistance", 125, eConsoleVarFlag_Developer );
	TConfigVar< Float >		cvEnvProbeAllocFailureRecoverDelay( "Rendering", "EnvProbeAllocFailureRecoverDelay", 5, eConsoleVarFlag_Developer );
}

using namespace PostProcessUtilities;
#include "renderPostFx.h"
#include "../engine/renderSettings.h"

#define ENVPROBE_FADE_IN_DURATION_DEFAULT		0.250f
#define ENVPROBE_FADE_OUT_DURATION_DEFAULT		0.125f
#define ENVPROBE_BLENDOVER_DURATION				0.75f

// can't be lower than blur extent during filtering
#define ENVPROBE_FILTER_TEMP_MARGIN		5

namespace
{
	const Bool  facesFlipHorizontal[6]	= { 0, 0, 1, 1, 0, 0 };
	const Bool  facesFlipVertical[6]	= { 1, 1, 0, 0, 1, 1 };
	const Int32 facesRotation[6]		= { 0, 0, 0, 0, 0, 0 };
	const Uint16 facesMapping[6]		= { 1, 0, 3, 2, 5, 4 };
}

extern void	BuildEnvProbeCamera( CRenderCamera &camera, const Vector &origin, Uint32 faceIndex );
extern void RenderEnvProbeFace( const CRenderCollector &mainRenderCollector, MeshDrawingStats &meshDrawingStats, const CRenderFrameInfo &info, const CRenderSceneEx *scene, CRenderInterface &renderer, CRenderEnvProbe &envProbe, const CRenderEnvProbeManager::SCubeSlotData &envProbeSlot, Uint32 faceIndex, const GpuApi::TextureRef &texRenderTarget, const GpuApi::TextureRef &texTempShadowMask, Uint32 renderResolution, const void *tiledConstantBuffer );

static void CopyTextureOneToOne( GpuApi::TextureRef texSampler, Uint32 sampleOffsetX, Uint32 sampleOffsetY, Bool flipHorizontal, Bool flipVertical, Int32 rotation )
{
	// Bind textures
	GpuApi::BindTextures( 0, 1, &texSampler, GpuApi::PixelShader );

	// Setup draw context
	CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

	// Render
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( -((Float)sampleOffsetX - GpuApi::GetViewport().x), -((Float)sampleOffsetY - GpuApi::GetViewport().y), 1, 0 ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)flipHorizontal, (Float)flipVertical, (Float)rotation, 0 ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( (Float)0, (Float)0, 0, 0 ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( (Float)GpuApi::GetViewport().width, 0, 0, 0 ) );
	GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeUnwrap );

	// Unbind textures
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
}

static void CopyReflectionAtlas( const GpuApi::TextureRef &texSource, const GpuApi::TextureRef &texTarget, Int32 segmentIndex, Float blendAlpha )
{
	if ( blendAlpha <= 0 )
	{
		return;
	}

	blendAlpha = Min( 1.f, blendAlpha );

	const GpuApi::TextureLevelDesc &texDesc = GpuApi::GetTextureLevelDesc( texTarget, 0 );
	const Uint32 viewportWidth = texDesc.width;
	const Uint32 viewportHeight = texDesc.height / (-1 != segmentIndex ? CRenderEnvProbeManager::CUBE_ARRAY_CAPACITY : 1);
	const Uint32 offsetY = -1 != segmentIndex ? segmentIndex * viewportHeight : 0;

	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, texTarget );
	rtSetup.SetViewport( viewportWidth, viewportHeight, 0, offsetY );
	GpuApi::SetupRenderTargets( rtSetup );

	// Setup draw context
	CGpuApiScopedDrawContext drawContext ( blendAlpha < 1.f ? GpuApi::DRAWCONTEXT_PostProcBlend : GpuApi::DRAWCONTEXT_PostProcSet );

	// Bind textures
	GpuApi::BindTextures( 0, 1, &texSource, GpuApi::PixelShader );

	// Render
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( 0, 0, 0, blendAlpha ) );
	GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterCopy );

	// Unbind textures
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
}

Uint32 GetEnvProbeResolution( EEnvProbeType probeType )
{
	switch ( probeType )
	{
	case ENVPROBETYPE_Ambient:		return CRenderEnvProbeManager::AMBIENT_CUBE_RESOLUTION;
	case ENVPROBETYPE_Reflection:	return CRenderEnvProbeManager::REFLECTION_CUBE_RESOLUTION;
	default:						ASSERT( !"invalid" ); return 0;
	}
}

Uint16 CRenderEnvProbeManager::GetEnvProbeTypeNumMips( EEnvProbeType probeType )
{
	Uint16 numMips = 0;
	switch ( probeType )
	{
	case ENVPROBETYPE_Ambient:		numMips = AMBIENT_CUBE_NUM_MIPS; break;
	case ENVPROBETYPE_Reflection:	numMips = REFLECTION_CUBE_NUM_MIPS; break;
	default:						ASSERT( !"invalid" );
	}

	ASSERT( numMips <= (Uint16)MLog2( GetEnvProbeResolution( probeType ) ) + 1 );
	return numMips;
}

struct SScopedEnvProbesTable
{
	SScopedEnvProbesTable ( Bool build )
	{
		for ( Uint32 i=0; i<ARRAY_COUNT(m_pProbesTable); ++i )
		{
			m_pProbesTable[i] = nullptr;
		}

		if ( build )
		{
			Build();
		}
	}

	~SScopedEnvProbesTable ()
	{
		Release();
	}

	CRenderEnvProbe* operator[]( Uint32 index ) const
	{
		RED_ASSERT( index < ARRAY_COUNT(m_pProbesTable) );
		return m_pProbesTable[index];
	}

	void Release()
	{
		for ( Uint32 i=0; i<ARRAY_COUNT(m_pProbesTable); ++i )
		{
			SAFE_RELEASE( m_pProbesTable[i] );
		}
	}

	void Set( Uint32 slotIndex, CRenderEnvProbe *probe )
	{
		if ( slotIndex >= ARRAY_COUNT(m_pProbesTable) )
		{
			RED_ASSERT( !"Invalid slot index" );
			return;
		}

		if ( probe )
		{
			probe->AddRef();
		}

		SAFE_RELEASE( m_pProbesTable[slotIndex] );
		m_pProbesTable[slotIndex] = probe;
	}	

	Int32 Find( const CRenderEnvProbe *probe ) const
	{
		for ( Uint32 i=0; i<ARRAY_COUNT(m_pProbesTable); ++i )
		{
			if ( probe == m_pProbesTable[i] )
			{
				return (Int32)i;
			}
		}

		return -1;
	}

	void Build()
	{
		Release();

		struct Visitor
		{
			Visitor ( SScopedEnvProbesTable *table )
				: m_pTable( table )
			{}

			void Visit( CRenderEnvProbe *probe )
			{
				const Int32 slotIndex = probe->GetDynamicData().m_arraySlotIndex;
				if ( -1 != slotIndex )
				{
					RED_ASSERT( !(*m_pTable)[slotIndex] );
					m_pTable->Set( slotIndex, probe );
				}
			}

			SScopedEnvProbesTable *m_pTable;
		};

		Visitor visitor ( this );
		CRenderEnvProbe::VisitResourcesAll( visitor );
	}

private:
	CRenderEnvProbe* m_pProbesTable[CRenderEnvProbeManager::CUBE_ARRAY_CAPACITY];
};

#ifndef NO_EDITOR

RED_INLINE Float DeprojectDepth( const CRenderCamera &camera, Float depth )
{
	Vector v = camera.GetScreenToView().TransformVectorWithW( Vector ( 0.f, 0.f, depth, 1.f ) );
	ASSERT( v.Z > 0 && v.W > 0 );
	const Float deprojected = v.Z / v.W;
	return deprojected;
}

Bool GrabEnvProbeFaceBuffers( const CRenderCamera &camera, const GpuApi::Rect* srcRect, Uint32 supersampleFactor, void* bufferData, Bool grabNonShadowData, Int32 shadowChannelIndex, Int32 shadowBitIndex, TDynArray<Float> &refSuperSamplingWeights )
{
	ASSERT( 0 == (srcRect->right - srcRect->left) % supersampleFactor );
	ASSERT( 0 == (srcRect->bottom - srcRect->top) % supersampleFactor );
	const Uint32 sourceWidth = ( srcRect->right - srcRect->left);
	const Uint32 sourceHeight = ( srcRect->bottom - srcRect->top );
	const Uint32 targetWidth = ( sourceWidth / supersampleFactor );
	const Uint32 targetHeight = ( sourceHeight / supersampleFactor );

	if ( supersampleFactor < 1 || targetWidth <= 0 || targetHeight <= 0 || 0 != (sourceWidth % supersampleFactor) || 0 != (sourceHeight % supersampleFactor) )
	{
		return false;
	}

	ASSERT( !(((Uint64)bufferData) & 3),  TXT("Alignment broken? Float cast may fail.") );
	Uint8 * const bufferDataTyped = static_cast<Uint8*>( bufferData );
	const Uint32 sourceSingleBufferSize = 4 * sourceWidth * sourceHeight;
	const Uint32 targetSingleBufferSize = 4 * targetWidth * targetHeight;

	// allocate temp buffer
	TDynArray<Uint8> tempBuffer ( sourceSingleBufferSize );

	// grab depth and generate sampling weights
	if ( grabNonShadowData )
	{
		refSuperSamplingWeights.Resize( sourceWidth * sourceHeight );

		// grab depth buffer
		{
			Float *tempDepthBuffer = reinterpret_cast<Float*>( tempBuffer.Data() );
			if ( !GpuApi::GrabTexturePixels( GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer3Depth ),	srcRect->left, srcRect->top, sourceWidth, sourceHeight, tempDepthBuffer ) )
			{
				return false;
			}

			TDynArray<Float> depthsTable;
			for ( Uint32 i=0; i<targetWidth; ++i )
				for ( Uint32 j=0; j<targetHeight; ++j )
				{
					// Build weights
					{
						Uint32 numSceneSamples = 0;
						for ( Uint32 ii=0; ii<supersampleFactor; ++ii )
							for ( Uint32 jj=0; jj<supersampleFactor; ++jj )
							{
								Uint32 offset = supersampleFactor * i + ii + (supersampleFactor * j + jj) * sourceWidth;
								Float depth = tempDepthBuffer[ offset ];
								numSceneSamples += depth < 1 ? 1 : 0;
							}

							if ( numSceneSamples > 0 )
							{
								depthsTable.ClearFast();

								for ( Uint32 ii=0; ii<supersampleFactor; ++ii )
									for ( Uint32 jj=0; jj<supersampleFactor; ++jj )
									{
										Uint32 offset = supersampleFactor * i + ii + (supersampleFactor * j + jj) * sourceWidth;
										Float depth = tempDepthBuffer[ offset ];
										if ( depth >= 1 )
										{
											continue;
										}

										depthsTable.PushBack( depth );
									}

									for ( Uint32 ii=0; ii+1<depthsTable.Size(); ++ii )
										for ( Uint32 jj=ii+1; jj<depthsTable.Size(); ++jj )
										{
											if ( depthsTable[ii] > depthsTable[jj] )
											{
												Swap( depthsTable[ii], depthsTable[jj] );
											}
										}

										const Float refDeprojDepth = DeprojectDepth( camera, depthsTable[ depthsTable.Size() / 2 ] );
										const Float toleranceOffset = 1 + 0.15f * refDeprojDepth;
										const Float toleranceRange = 1 + 0.25f * refDeprojDepth;
										for ( Uint32 ii=0; ii<supersampleFactor; ++ii )
											for ( Uint32 jj=0; jj<supersampleFactor; ++jj )
											{
												Uint32 offset = supersampleFactor * i + ii + (supersampleFactor * j + jj) * sourceWidth;
												Float depth = tempDepthBuffer[ offset ];
												if ( depth < 1 )
												{
													const Float currDeprojDepth = DeprojectDepth( camera, depth );
													refSuperSamplingWeights[offset] = Clamp( 1.f - Abs(currDeprojDepth - refDeprojDepth) / toleranceRange, 0.f, 1.f );
												}
												else
												{
													refSuperSamplingWeights[offset] = 0.f;
												}
											}
							}
							else
							{
								for ( Uint32 ii=0; ii<supersampleFactor; ++ii )
									for ( Uint32 jj=0; jj<supersampleFactor; ++jj )
									{
										const Uint32 weightsOffset = supersampleFactor * i + ii + (supersampleFactor * j + jj) * sourceWidth;
										refSuperSamplingWeights[weightsOffset] = 1.f;
									}
							}
					}

					Float resultValue = 0.f;				
					Float weightsSum = 0.f;
					for ( Uint32 ii=0; ii<supersampleFactor; ++ii )
						for ( Uint32 jj=0; jj<supersampleFactor; ++jj )
						{
							const Uint32 weightsOffset = supersampleFactor * i + ii + (supersampleFactor * j + jj) * sourceWidth;
							const Uint32 srcOffset = weightsOffset;
							const Float &src = tempDepthBuffer[ srcOffset ];
							const Float currWeight = refSuperSamplingWeights[ weightsOffset ];

							resultValue += currWeight * src;
							weightsSum += currWeight;
						}
						if ( weightsSum > 0 )
						{
							resultValue /= weightsSum;
						}

						Uint16 &dst = *(Uint16*)( bufferDataTyped + GetEnvProbeDataSourceBufferOffset( ENVPROBEBUFFERTEX_Depth ) + 2 * (i + j * targetWidth) );
						dst = Float16Compressor::Compress( resultValue );
				}
		}		
	}

	ASSERT( refSuperSamplingWeights.Size() == sourceWidth * sourceHeight );

	// grab non shadow / non depth
	if ( grabNonShadowData )
	{
		// grab albedo buffer
		{
			if ( !GpuApi::GrabTexturePixels( GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer0 ),		srcRect->left, srcRect->top, sourceWidth, sourceHeight, tempBuffer.TypedData(), 4, false ) )
			{
				return false;
			}

			for ( Uint32 i=0; i<targetWidth; ++i )
				for ( Uint32 j=0; j<targetHeight; ++j )
				{
					Float resultValue0 = 0.f;				
					Float resultValue1 = 0.f;
					Float resultValue2 = 0.f;
					Float weightsSum = 0.f;
					for ( Uint32 ii=0; ii<supersampleFactor; ++ii )
						for ( Uint32 jj=0; jj<supersampleFactor; ++jj )
						{
							const Uint32 weightsOffset = (supersampleFactor * i + ii + (supersampleFactor * j + jj) * sourceWidth);
							const Uint32 srcOffset = 4 * weightsOffset;
							const Uint8 *src = &tempBuffer[ srcOffset ];
							const Float currWeight = refSuperSamplingWeights[ weightsOffset ];
							
							resultValue0 += currWeight * src[0];
							resultValue1 += currWeight * src[1];
							resultValue2 += currWeight * src[2];

							weightsSum += currWeight;
						}

						Uint8 *dst = bufferDataTyped + GetEnvProbeDataSourceBufferOffset( ENVPROBEBUFFERTEX_Albedo ) + 2 * (i + j * targetWidth);

						if ( weightsSum > 0 )
						{
							Float resultMul = 1.f / (weightsSum * 255.f);
							resultValue0 *= resultMul;
							resultValue1 *= resultMul;
							resultValue2 *= resultMul;

							// pack ycbcr
							Float packed[3] = {
								0.299f * resultValue0 + 0.587f * resultValue1 + 0.114f * resultValue2,  
								0.5f + (-0.168f * resultValue0 - 0.331f * resultValue1 + 0.5f * resultValue2),
								0.5f + (0.5f * resultValue0 - 0.418f * resultValue1 - 0.081f * resultValue2)
							};

							dst[0] = (Uint8)Clamp( packed[0] * 255.f, 0.f, 255.f );
							dst[1] = (Uint8)Clamp( packed[1 + j % 2] * 255.f, 0.f, 255.f );
						}
						else
						{
							dst[0] = 0;
							dst[1] = 255 / 2; //< most neutral chrominance component value
						}
				}
		}

		// grab normals buffer
		{
			if ( !GpuApi::GrabTexturePixels( GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer1 ),		srcRect->left, srcRect->top, sourceWidth, sourceHeight, tempBuffer.TypedData(), 4, false ) )
			{
				return false;
			}

			for ( Uint32 i=0; i<targetWidth; ++i )
				for ( Uint32 j=0; j<targetHeight; ++j )
				{
					Vector v_accum ( 0, 0, 0 );			
					for ( Uint32 ii=0; ii<supersampleFactor; ++ii )
						for ( Uint32 jj=0; jj<supersampleFactor; ++jj )
						{
							const Uint32 weightsOffset = (supersampleFactor * i + ii + (supersampleFactor * j + jj) * sourceWidth);
							const Uint32 srcOffset = 4 * weightsOffset;
							const Uint8 *src = &tempBuffer[ srcOffset ];
							const Float currWeight = refSuperSamplingWeights[ weightsOffset ];

							v_accum += Vector ( src[0] - 127.5f, src[1] - 127.5f, src[2] - 127.5f ).Normalized3() * currWeight;
						}
						v_accum.Normalize3();

						Uint8 *dst = bufferDataTyped + GetEnvProbeDataSourceBufferOffset( ENVPROBEBUFFERTEX_Normals ) + 3 * (i + j * targetWidth);
						dst[0] = (Uint8)Clamp( (v_accum.X + 1.0f) * 127.5f, 0.f, 255.f );
						dst[1] = (Uint8)Clamp( (v_accum.Y + 1.0f) * 127.5f, 0.f, 255.f );
						dst[2] = (Uint8)Clamp( (v_accum.Z + 1.0f) * 127.5f, 0.f, 255.f );
				}
		}
	}

	// grab shadow
	{
		Uint8 * const destShadowData = bufferDataTyped + GetEnvProbeDataSourceBufferOffset( ENVPROBEBUFFERTEX_GlobalShadow );

		ASSERT( shadowChannelIndex >= 0 && shadowChannelIndex < 4 );
		ASSERT( shadowBitIndex >= 0 && shadowBitIndex < 8 );

		TDynArray<Uint8> shadowBuffer ( sourceSingleBufferSize );
		if ( !GpuApi::GrabTexturePixels( GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ),	srcRect->left, srcRect->top, sourceWidth, sourceHeight, shadowBuffer.TypedData(), 4, false ) )
		{
			return false;
		}

		for ( Uint32 i=0; i<targetWidth; ++i )
			for ( Uint32 j=0; j<targetHeight; ++j )
			{
				Float resultValue = 0.f;	
				Float weightsSum = 0.f;
				for ( Uint32 ii=0; ii<supersampleFactor; ++ii )
					for ( Uint32 jj=0; jj<supersampleFactor; ++jj )
					{
						const Uint32 weightsOffset = (supersampleFactor * i + ii + (supersampleFactor * j + jj) * sourceWidth);
						const Uint32 srcOffset = 4 * weightsOffset;
						const Uint8 *src = &shadowBuffer[ srcOffset ];
						const Float currWeight = refSuperSamplingWeights[ weightsOffset ];

						COMPILE_ASSERT( GLOBAL_SHADOW_BUFFER_CHANNEL_SHADOW < 4 );
						resultValue += currWeight * src[GLOBAL_SHADOW_BUFFER_CHANNEL_SHADOW];

						weightsSum += currWeight;
					}
					if ( weightsSum > 0 )
					{
						resultValue /= weightsSum;
					}

					Uint8 b = FLAG( shadowBitIndex );
					Uint8 &v = destShadowData[4 * (i + j * targetWidth) + shadowChannelIndex];
					v = resultValue > 127 ? (v | b) : (v & ~b);
			}
	}

	return true;
}

#endif

SRenderEnvProbeDynamicData::SRenderEnvProbeDynamicData ()
	: m_lastUpdateTime ( -1 )
	, m_lastFullUpdateStartUpdateTime ( -1 )
	, m_lastFullUpdateFinishUpdateTime ( -1 )
	, m_arraySlotIndex ( -1 )
	, m_isArraySlotInit ( false )
	, m_isFaceTexturesInit ( false )
	, m_isFaceTexturesUnpacked ( false )
	, m_prefetchFailed ( false )
	, m_failereRecoveryTimeStamp ( -1 )
{}

SRenderEnvProbeDynamicData::~SRenderEnvProbeDynamicData ()
{
	// empty
}

Bool SRenderEnvProbeDynamicData::IsReadyToDisplay( const CRenderEnvProbeManager &manager ) const
{
	ASSERT( !(-1 == m_arraySlotIndex && m_isArraySlotInit) );
	return m_isArraySlotInit && manager.GetCubeSlotData( m_arraySlotIndex ).m_isInReflectionAtlas;
}

void SRenderEnvProbeDynamicData::DiscardArraySlot()
{
	m_arraySlotIndex = -1;
	m_isArraySlotInit = false;
	m_isFaceTexturesInit = false;
	m_isFaceTexturesUnpacked = false;
	m_prefetchFailed = false;
	m_failereRecoveryTimeStamp = -1;
}

// ---

CRenderEnvProbeManager::SCubeSlotData::SCubeSlotData ()
{
	m_weight = 0;
	m_needsReflectionBaseUpdate = false;
	m_isInReflectionAtlas = false;
	m_reflectionBaseUpdateCounter = 0;
}

void CRenderEnvProbeManager::SCubeSlotData::ImportRenderEnvProbeData( const CRenderEnvProbe &envProbe )
{
	m_probeParams = envProbe.GetProbeParams();
}

const GpuApi::TextureRef& CRenderEnvProbeManager::SCubeSlotData::GetFaceTexture( eEnvProbeBufferTexture texType ) const
{
	RED_ASSERT( m_faceTextures[texType], TXT("Texture not present - trying to use it before it was created or after it was destroyed?") );
	return m_faceTextures[texType];
}

// ---

CRenderEnvProbeManager::CRenderEnvProbeManager ()
	: m_pCurrGenProbe ( NULL )
	, m_currGenStartUpdateTime ( -1 )
	, m_currGenProgress ( -1 )
	, m_currGenBlendoverFactor ( -1 )
	, m_currGenBlendoverAccum ( 0 )
	, m_lastUpdateTime ( -1 )
	, m_lastUpdateGameTime ( -1 )
	, m_isHelperTexturesInit ( false )
	, m_finalCompositionCounter ( 0 )
	, m_isDuringPrefetch ( false )
	, m_isPostPrefetch( false )
	, m_needsPostPrefetchUpdate ( false )
	, m_prefetchPosition ( Vector::ZERO_3D_POINT )
{
	const GpuApi::eTextureFormat probesTexFmt = GpuApi::TEXFMT_Float_R11G11B10;

	LOG_RENDERER( TXT("Allocating envProbeManager textures") );

	// Allocate temp face textures
	{
		GpuApi::TextureDesc desc;
		{
			desc.type		= GpuApi::TEXTYPE_2D;
			desc.width		= MAX_CUBE_RESOLUTION;
			desc.height		= MAX_CUBE_RESOLUTION;
			desc.initLevels	= 1;
			desc.format		= probesTexFmt;
			desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
		}

		m_tempFaceTexture = GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_tempFaceTexture, "envProbeTempFace" );
	}
	
	// Reflection cube
	{
		GpuApi::TextureDesc desc;
		{
			desc.type		= GpuApi::TEXTYPE_CUBE;
			desc.width		= REFLECTION_CUBE_RESOLUTION;
			desc.height		= REFLECTION_CUBE_RESOLUTION;
			desc.initLevels	= 1;
			desc.sliceNum	= 1;
			desc.format		= probesTexFmt;
			desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
		}
			 
		m_intermediateReflectionCube = GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_intermediateReflectionCube, "envProbeReflCube" );
	}

	// Create shadow mask
	{
		GpuApi::TextureDesc desc;
		desc.type		= GpuApi::TEXTYPE_2D;
		desc.initLevels	= 1;
		desc.width		= MAX_CUBE_RESOLUTION;
		desc.height		= MAX_CUBE_RESOLUTION;
		
		desc.usage	= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
		desc.format	= GpuApi::TEXFMT_R8G8B8A8;
		m_surfaceShadowMask	= GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_surfaceShadowMask, "envProbeShadowMask" );
	}

	// Init nesting order
	for ( Uint32 slot_i=0; slot_i<CUBE_ARRAY_CAPACITY; ++slot_i )
	{
		m_nestingOrder[slot_i] = slot_i;
	}

	// Allocate cube slots data (textures are being created during loading)
	m_cubeSlotsData.Resize( CUBE_ARRAY_CAPACITY );
	for ( Uint32 slot_i=0; slot_i<CUBE_ARRAY_CAPACITY; ++slot_i )
	{
		m_nestingOrder[slot_i] = slot_i;		
	}

	// Create temp paraboloid textures
	for ( Uint32 i=0; i<ARRAY_COUNT(m_tempParaboloid); ++i )
	{
		const Uint32 sizeDiv = 1;		// this can be set to 2 in case no 0 level mipmap filtering will be performed

		GpuApi::TextureDesc desc;
		desc.type		= GpuApi::TEXTYPE_2D;
		desc.width		= 2 * (2 * ENVPROBE_FILTER_TEMP_MARGIN + GetEnvProbeResolution( ENVPROBETYPE_Reflection ) / sizeDiv);
		desc.height		= CUBE_ARRAY_CAPACITY * (2 * ENVPROBE_FILTER_TEMP_MARGIN + GetEnvProbeResolution( ENVPROBETYPE_Reflection ) / sizeDiv);
		desc.initLevels	= 1;
		desc.sliceNum	= 1;
		desc.format		= probesTexFmt;
		desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;

		m_tempParaboloid[i] = GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_tempParaboloid[i], "envProbeTempParaboloid" );
	}

	// Create ambient atlas textures
	{
		const Uint32 numMips = GetEnvProbeTypeNumMips( ENVPROBETYPE_Ambient );
		const Uint32 extent = (1 << (GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection ) - 1)) >> MLog2( GetEnvProbeResolution( ENVPROBETYPE_Reflection ) / GetEnvProbeResolution( ENVPROBETYPE_Ambient ) );
		
		// temp scene atlas
		{
			GpuApi::TextureDesc desc;
			desc.type		= GpuApi::TEXTYPE_2D;
			desc.width		= 2 * (GetEnvProbeResolution( ENVPROBETYPE_Ambient ) + 2 * extent);
			desc.height		= CUBE_ARRAY_CAPACITY * (GetEnvProbeResolution( ENVPROBETYPE_Ambient ) + 2 * extent);
			desc.initLevels	= (Uint16)numMips;
			desc.sliceNum	= 1;
			desc.format		= probesTexFmt;
			desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;

			m_cachedUnpackedAmbientScene	= GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
			GpuApi::SetTextureDebugPath( m_cachedUnpackedAmbientScene, "envProbeCachedUnpackedAmbient" );
		}

		// temp sky atlas
		{
			GpuApi::TextureDesc desc;
			desc.type		= GpuApi::TEXTYPE_2D;
			desc.width		= 2 * (GetEnvProbeResolution( ENVPROBETYPE_Ambient ) + 2 * extent);
			desc.height		= CUBE_ARRAY_CAPACITY * (GetEnvProbeResolution( ENVPROBETYPE_Ambient ) + 2 * extent);
			desc.initLevels	= (Uint16)numMips;
			desc.sliceNum	= 1;
			desc.format		= GpuApi::TEXFMT_Float_R16;
			desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;

			m_cachedUnpackedAmbientSkyFactor = GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
			GpuApi::SetTextureDebugPath( m_cachedUnpackedAmbientSkyFactor, "envProbeCachedUnpackedAmbientSkyFactor" );
		}

		// result atlas
		{
			GpuApi::TextureDesc desc;
			desc.type		= GpuApi::TEXTYPE_2D;
			desc.width		= 2 * (GetEnvProbeResolution( ENVPROBETYPE_Ambient ) + 2 * extent);
			desc.height		= CUBE_ARRAY_CAPACITY * (GetEnvProbeResolution( ENVPROBETYPE_Ambient ) + 2 * extent);
			desc.initLevels	= (Uint16)numMips;
			desc.sliceNum	= 1;
			desc.format		= probesTexFmt;
			desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;

			m_ambientAtlas	= GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
			GpuApi::SetTextureDebugPath( m_ambientAtlas, "envProbeAmbientAtlas" );
		}
	}

	// Create reflection atlas texture
	{
		const Uint32 numMips = GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection );
		const Uint32 extent = (1 << (numMips - 1));

		GpuApi::TextureDesc desc;
		desc.type		= GpuApi::TEXTYPE_2D;
		desc.width		= 2 * (GetEnvProbeResolution( ENVPROBETYPE_Reflection ) + 2 * extent);
		desc.height		= CUBE_ARRAY_CAPACITY * (GetEnvProbeResolution( ENVPROBETYPE_Reflection ) + 2 * extent);
		desc.initLevels	= (Uint16)numMips;
		desc.sliceNum	= 1;
		desc.format		= probesTexFmt;
		desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;

		m_reflectionAtlas = GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_reflectionAtlas, "envProbeReflectionAtlas" );
		
		desc.format			= GpuApi::TEXFMT_Float_R32;
		m_mergeSumTexture	= GpuApi::CreateTexture( desc, GpuApi::TEXG_System );		// ace_optimize: this texture doesn't have to be repeated for every segment. results are the same for each segment and paraboloid face..
		GpuApi::SetTextureDebugPath( m_mergeSumTexture, "envProbeMergeSum" );
	}

	// Create reflection related atlases
	{
		// ace_optimize : extent not needed here (it's for simplicity only)
		const Uint32 extent = (1 << (GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection ) - 1));

		GpuApi::TextureDesc desc;
		desc.type		= GpuApi::TEXTYPE_2D;
		desc.width		= 2 * (GetEnvProbeResolution( ENVPROBETYPE_Reflection ) + 2 * extent);
		desc.height		= CUBE_ARRAY_CAPACITY * (GetEnvProbeResolution( ENVPROBETYPE_Reflection ) + 2 * extent);
		desc.initLevels	= 1;
		desc.sliceNum	= 1;
		desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;

		desc.format						= probesTexFmt;
		m_tempReflectionAtlas			= GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_tempReflectionAtlas, "envProbeTempReflection" );
		m_reflectionAtlasBaseCurrent	= GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_reflectionAtlasBaseCurrent, "envProbeReflBaseCurrent" );
		m_reflectionAtlasBaseBlendSrc	= GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_reflectionAtlasBaseBlendSrc, "envProbeReflBlendSrc" );
		m_reflectionAtlasBaseBlendDest	= GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_reflectionAtlasBaseBlendDest, "envProbeReflBlendDst" );

		desc.format						= probesTexFmt;
		m_cachedUnpackedAlbedo			= GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_cachedUnpackedAlbedo, "envProbeCachedUnpackedAlbedo" );

		desc.format						= probesTexFmt;
		m_cachedUnpackedNormals			= GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_cachedUnpackedNormals, "envProbeCachedUnpackedNormals" );

		desc.format						= GpuApi::TEXFMT_Float_R16G16;
		m_cachedUnpackedDepthAndSky		= GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_cachedUnpackedDepthAndSky, "envProbeCachedUnpackedDepth" );
	}
}

CRenderEnvProbeManager::~CRenderEnvProbeManager ()
{
	// Release data sources
	{
		IEnvProbeDataSource::tScopedLock lock ( IEnvProbeDataSource::GetCommunicationLock() );

		// Release current data source
		if ( m_currLoadingDataSource )
		{
			m_currLoadingDataSource->CancelLoading();
			m_currLoadingDataSource = NULL;
		}

		// Release cancelled data source
		if ( m_cancelledLoadingDataSource )
		{
			m_cancelledLoadingDataSource->CancelLoading();
			m_cancelledLoadingDataSource = NULL;
		}
	}

	// Release temp face textures
	GpuApi::SafeRelease( m_tempFaceTexture );

	// Release face textures
	{
		for ( Uint32 slot_i=0; slot_i<m_cubeSlotsData.Size(); ++slot_i )
		{
			for ( Uint32 ftt_i=0; ftt_i<ENVPROBEBUFFERTEX_MAX; ++ftt_i )
			{
				for ( Uint32 face_i=0; face_i<6; ++face_i )
				{
					GpuApi::SafeRelease( RefCubeSlotData(slot_i).m_faceTextures[ftt_i] );
				}
			}
		}
		m_cubeSlotsData.Clear();
	}

	// Release intermediate textures
	{
		GpuApi::SafeRelease( m_intermediateReflectionCube );
	}

	// Release paraboloid textures
	for ( Uint32 i=0; i<ARRAY_COUNT(m_tempParaboloid); ++i )
	{
		GpuApi::SafeRelease( m_tempParaboloid[i] );		
	}

	GpuApi::SafeRelease( m_mergeSumTexture );

	// Release atlases
	GpuApi::SafeRelease( m_cachedUnpackedAmbientScene );
	GpuApi::SafeRelease( m_cachedUnpackedAmbientSkyFactor );
	GpuApi::SafeRelease( m_cachedUnpackedAlbedo );
	GpuApi::SafeRelease( m_cachedUnpackedDepthAndSky );
	GpuApi::SafeRelease( m_cachedUnpackedNormals );
	GpuApi::SafeRelease( m_ambientAtlas );
	GpuApi::SafeRelease( m_reflectionAtlas );
	GpuApi::SafeRelease( m_reflectionAtlasBaseCurrent );
	GpuApi::SafeRelease( m_reflectionAtlasBaseBlendSrc );
	GpuApi::SafeRelease( m_reflectionAtlasBaseBlendDest );
	GpuApi::SafeRelease( m_tempReflectionAtlas );

	// Release surfaces
	GpuApi::SafeRelease( m_surfaceShadowMask );
	
	//
	SAFE_RELEASE( m_pCurrGenProbe );
	m_currGenProgress = -1;
	m_currGenBlendoverFactor = -1;
	m_currGenBlendoverAccum = 0;
}

void CRenderEnvProbeManager::UpdateBlendover( Float updateTimeDelta )
{
	if ( updateTimeDelta <= 0 )
	{
		return;
	}

	Float blendOverAddValue = 0;
	{
		m_currGenBlendoverAccum += updateTimeDelta / ENVPROBE_BLENDOVER_DURATION;

		const Float minFactorStep = 1.f / 100.f;
		if ( m_currGenBlendoverAccum >= minFactorStep )
		{
			blendOverAddValue = m_currGenBlendoverAccum;
			m_currGenBlendoverAccum = 0;
		}
	}

	if ( -1 == m_currGenBlendoverFactor )
	{
		Uint32 numSlotsReady = 0;
		Uint32 numSlotsReadyNeeded = 0;

		for ( Uint32 i=0; i<CUBE_ARRAY_CAPACITY; ++i )
		{
			SCubeSlotData &slot = RefCubeSlotData( i );
			if ( slot.m_needsReflectionBaseUpdate )
			{
				numSlotsReadyNeeded += 1;
				//ASSERT( slot.m_reflectionBaseUpdateCounter <= 1 ); disabled because it may happen (and it's valid) in case of fastUpdate
				numSlotsReady += slot.m_reflectionBaseUpdateCounter > 0 ? 1 : 0;
			}
		}

		if ( numSlotsReadyNeeded > 0 && numSlotsReadyNeeded == numSlotsReady )
		{
			Swap( m_reflectionAtlasBaseBlendSrc, m_reflectionAtlasBaseCurrent );
			
			m_currGenBlendoverFactor = 0;

			// Blending will start in next frame (we don't want perform blending in the same frame
			// in which we are also doing reflection base preparation, in order to prevent hitches)
			return;
		}
	}	

	if ( m_currGenBlendoverFactor >= 0 && m_currGenBlendoverFactor < 1 )
	{
		const Float prevBlendOverFactor = m_currGenBlendoverFactor;
		m_currGenBlendoverFactor = Min( 1.f, m_currGenBlendoverFactor + blendOverAddValue );
		
		if ( m_currGenBlendoverFactor > prevBlendOverFactor )
		{
			Float blendAlpha = 1.f;
			if ( m_currGenBlendoverFactor < 0.99f )
			{
				blendAlpha = (m_currGenBlendoverFactor - prevBlendOverFactor) / (1.f - prevBlendOverFactor);
				blendAlpha = Clamp( blendAlpha, 0.f, 1.f );
			}
			else
			{
				m_currGenBlendoverFactor = 1;
			}

			if ( m_currGenBlendoverFactor >= 1 )
			{
				Swap( m_reflectionAtlasBaseBlendSrc, m_reflectionAtlasBaseBlendDest );
			}
			else
			{
				CopyReflectionAtlas( m_reflectionAtlasBaseBlendSrc, m_reflectionAtlasBaseBlendDest, -1, blendAlpha );
			}
		}
	}
	else if ( 1 == m_currGenBlendoverFactor )
	{
		for ( Uint32 i=0; i<CUBE_ARRAY_CAPACITY; ++i )
		{
			RefCubeSlotData( i ).m_reflectionBaseUpdateCounter = 0;
		}

		m_currGenBlendoverFactor = -1;
	}
}

void CRenderEnvProbeManager::UpdateNestingOrder()
{
	PC_SCOPE_RENDER_LVL1( UpdateNestingOrder );

	COMPILE_ASSERT( ARRAY_COUNT(m_nestingOrder) == CUBE_ARRAY_CAPACITY );
	ASSERT( ARRAY_COUNT(m_nestingOrder) == m_cubeSlotsData.Size() );

	// Get sort keys
	Vector sortKeys[CUBE_ARRAY_CAPACITY];
	for ( Uint32 slot_i=0; slot_i<CUBE_ARRAY_CAPACITY; ++slot_i )
	{
		const CRenderEnvProbeManager::SCubeSlotData &src = GetCubeSlotData( slot_i );
		const Vector &srcPos = src.m_probeParams.m_probeOrigin;
		m_nestingOrder[slot_i] = slot_i;
		sortKeys[slot_i].Set4( (Float)src.m_probeParams.m_nestingLevel, srcPos.X, srcPos.Y, srcPos.Z );
	}

	// Superfast bubblesort ;)
	// Global slot stays unmodified.
	COMPILE_ASSERT( 0 == GLOBAL_SLOT_INDEX );
	for ( Uint32 i=1; i+1<CUBE_ARRAY_CAPACITY; ++i )
	for ( Uint32 j=i+1; j<CUBE_ARRAY_CAPACITY; ++j )
	{
		const Vector &v0 = sortKeys[ m_nestingOrder[i] ];
		const Vector &v1 = sortKeys[ m_nestingOrder[j] ];
		if ( v0.X < v1.X || ( v0.X == v1.X && (v0.Y < v1.Y || ( v0.Y == v1.Y && (v0.Z < v1.Z || ( v0.Z == v1.Z && ( v0.W < v1.W || ( v0.W == v1.W && GetCubeSlotData( m_nestingOrder[i] ).m_probeParams.m_debugId < GetCubeSlotData( m_nestingOrder[j] ).m_probeParams.m_debugId ) ) ) ) ) ) ) )
		{
			Swap( m_nestingOrder[i], m_nestingOrder[j] );
		}		
	}
}

void CRenderEnvProbeManager::InitHelperTextures()
{
	LOG_RENDERER( TXT("Initializing envProbeManager helper textures") );

	// Clear various textures
	{
		GetRenderer()->ClearColorTarget( m_cachedUnpackedAmbientScene, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_cachedUnpackedAmbientSkyFactor, Vector::ZEROS );
		for ( Uint32 i=0; i<ARRAY_COUNT(m_tempParaboloid); ++i )
		{
			GetRenderer()->ClearColorTarget( m_tempParaboloid[i], Vector::ZEROS );
		}
		GetRenderer()->ClearColorTarget( m_mergeSumTexture, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_cachedUnpackedAmbientScene, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_cachedUnpackedAmbientSkyFactor, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_cachedUnpackedAlbedo, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_cachedUnpackedNormals, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_cachedUnpackedDepthAndSky, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_reflectionAtlasBaseCurrent, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_reflectionAtlasBaseBlendSrc, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_reflectionAtlasBaseBlendDest, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_ambientAtlas, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_reflectionAtlas, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_tempReflectionAtlas, Vector::ZEROS );
		GetRenderer()->ClearColorTarget( m_surfaceShadowMask, Vector::ONES );
	}

	// Init mergeSumTexture
	{
		GetRenderer()->ClearColorTarget( m_mergeSumTexture, Vector::ONES );
		CopyToTempParaboloid( m_mergeSumTexture );
		BlurTempParaboloid( m_mergeSumTexture, -1, 0, true );
		FilterReflectionAtlas( m_mergeSumTexture, -1, true );
	}
}

void CRenderEnvProbeManager::Update( CRenderCollector &collector )
{
	PC_SCOPE_RENDER_LVL1( EnvProbeManagerUpdate );

	if ( !collector.m_scene || !collector.m_info )
	{
		return;
	}

	if ( !m_isHelperTexturesInit )
	{
		InitHelperTextures();
		m_isHelperTexturesInit = true;
	}

	if ( IsDuringPrefetch() )
	{
		CancelPrefetch();
	}

	// Progress time
	Bool needsFastUpdate = m_isPostPrefetch;
	const Float currUpdateTime = collector.GetRenderFrameInfo().m_engineTime;
	const Float updateTimeDelta = -1 != m_lastUpdateTime ? currUpdateTime - m_lastUpdateTime : 0.0333f;
	const Float currUpdateGameTime = collector.GetRenderFrameInfo().m_gameTime;
	RED_ASSERT( currUpdateGameTime >= 0 );
	if ( -1 != m_lastUpdateGameTime && !needsFastUpdate )
	{	
		const Float fastUpdateMinutesThreshold = 20; //< how much gametime need to differ from the one from last update in order to issue a fast update

		Float dayTimeRange = 24 * 60 * 60;
		Float v0 = Min( currUpdateGameTime, m_lastUpdateGameTime );
		Float v1 = Max( currUpdateGameTime, m_lastUpdateGameTime );
		v0 -= floorf( v0 / dayTimeRange ) * dayTimeRange;
		v1 -= floorf( v1 / dayTimeRange ) * dayTimeRange;
		if ( (v1 - v0) >= 0.5f * dayTimeRange )
		{
			v0 += dayTimeRange;
		}

		const Float gameTimeDiff = fabsf( v1 - v0 );
		needsFastUpdate |= gameTimeDiff >= fastUpdateMinutesThreshold * 60;
	}
	m_lastUpdateTime = currUpdateTime;
	m_lastUpdateGameTime = currUpdateGameTime;

	// Cancel current envprobe update if fast update is needed
	if ( needsFastUpdate )
	{
		CancelCurrProbeUpdate();
	}

	// Perform fast envprobes update
	if ( m_needsPostPrefetchUpdate || needsFastUpdate )
	{
		m_needsPostPrefetchUpdate = false;

		SScopedEnvProbesTable probesTable ( true );
		for ( Uint32 probe_i=0; probe_i<CUBE_ARRAY_CAPACITY; ++probe_i )
		{
			CRenderEnvProbe *probe = probesTable[probe_i];
			if ( probe && probe->GetDynamicData().m_isFaceTexturesInit && (!probe->GetDynamicData().m_isFaceTexturesUnpacked || needsFastUpdate) ) // !probe->GetDynamicData().m_isFaceTexturesUnpacked is needed for update that comes from postPrefetchUpdate
			{
				UpdateEnvProbes( collector, probe, updateTimeDelta, true );
				CancelCurrProbeUpdate();
			}
		}
	}
	
	// Update slots and envprobes
	UpdateSlots( collector.GetRenderFrameInfo().m_camera.GetPosition(), updateTimeDelta, false );
	UpdateEnvProbes( collector, nullptr, updateTimeDelta, false );
	
	// Update blendover
	if ( collector.GetRenderFrameInfo().m_envParametersGame.m_displaySettings.m_allowEnvProbeUpdate )
	{
		Uint32 numComposition = 1;
		if ( needsFastUpdate )
		{
			CopyReflectionAtlas( m_reflectionAtlasBaseCurrent, m_reflectionAtlasBaseBlendSrc, -1, 1 );
			CopyReflectionAtlas( m_reflectionAtlasBaseCurrent, m_reflectionAtlasBaseBlendDest, -1, 1 );
			m_finalCompositionCounter = 0;
			numComposition = 3;
		}
		else
		{
			UpdateBlendover( collector.GetRenderFrameInfo().m_envParametersGame.m_displaySettings.m_enableEnvProbeInstantUpdate ? ENVPROBE_BLENDOVER_DURATION : updateTimeDelta );
		}

		for ( Uint32 composition_i=0; composition_i<numComposition; ++composition_i )
		{
			switch ( m_finalCompositionCounter )
			{
			case 0:
				MergeAmbientAtlas( currUpdateGameTime, collector.m_info->m_envParametersArea.m_globalLight.m_envProbeBaseLightingAmbient );
				MergeReflectionAtlas( collector, m_tempReflectionAtlas, m_reflectionAtlasBaseBlendDest );
				m_finalCompositionCounter = 1;
				break;

			case 1:
				CopyToTempParaboloid( m_tempReflectionAtlas );
				BlurTempParaboloid( m_tempReflectionAtlas, -1, 0, false );
				m_finalCompositionCounter = 2;
				break;

			case 2:
				CopyReflectionAtlas( m_tempReflectionAtlas, m_reflectionAtlas, -1, 1 );
				FilterReflectionAtlas( m_reflectionAtlas, -1, false );
				m_finalCompositionCounter = 0;
				for ( Uint32 slot_i=0; slot_i<CUBE_ARRAY_CAPACITY; ++slot_i )
				{
					SCubeSlotData &slot = m_cubeSlotsData[slot_i];
					if ( slot.m_reflectionBaseUpdateCounter > 0 )
					{
						slot.m_isInReflectionAtlas = true;
					}
				}
				break;

			default:
				ASSERT( !"invalid" );
				m_finalCompositionCounter = 0;
			}
		}

		if ( needsFastUpdate )
		{
			SScopedEnvProbesTable probesTable ( true );
			for ( Uint32 i=0; i<CUBE_ARRAY_CAPACITY; ++i )
			{
				SCubeSlotData &slotData = RefCubeSlotData( i );				
				if ( slotData.m_reflectionBaseUpdateCounter > 0 && probesTable[i] && probesTable[i]->GetDynamicData().IsReadyToDisplay( *this ) )
				{
					slotData.m_weight = 1;
				}
				slotData.m_reflectionBaseUpdateCounter = 0;
			}

			m_currGenBlendoverFactor = -1;
		}
	}

	// Post prefetch stuff done
	m_isPostPrefetch = false;

	// Update stats
	UpdateRenderingStats();
}

void CRenderEnvProbeManager::UpdateRenderingStats()
{
#ifndef RED_FINAL_BUILD

	extern SceneRenderingStats GRenderingStats;

	// Reset stats
	GRenderingStats.m_totalEnvProbeObjects = 0;
	GRenderingStats.m_totalGlobalEnvProbeObjects = 0;
	GRenderingStats.m_numEnvProbeStats = 0;

	// Init slot related stats
	COMPILE_ASSERT( SceneRenderingStats::ENVPROBE_STATS_CAPACITY == CUBE_ARRAY_CAPACITY );
	GRenderingStats.m_numEnvProbeStats = Min( SceneRenderingStats::ENVPROBE_STATS_CAPACITY, CRenderEnvProbeManager::CUBE_ARRAY_CAPACITY );
	for ( Uint32 slot_i=0; slot_i<GRenderingStats.m_numEnvProbeStats; ++slot_i )
	{
		const SCubeSlotData &slotData = GetCubeSlotData( slot_i );
		SceneRenderingStats::EnvProbeStats &dst = GRenderingStats.m_envProbeStats[slot_i];

		dst.Reset();
		dst.m_debugId = slotData.m_probeParams.m_debugId;
		dst.m_isGlobalProbe = slotData.m_probeParams.IsGlobalProbe();
		dst.m_isDuringUpdate = m_pCurrGenProbe && m_pCurrGenProbe->GetDynamicData().m_arraySlotIndex == (Int32)slot_i;
		dst.m_weightOneThousands = (Uint32)( 1000 * slotData.m_weight );
		dst.m_hasProbeObject = false;
		dst.m_lastUpdateDelayMillis = 0;
		dst.m_lastUpdateDurationMillis = 0;
	}

	// Init probe object related stuff
	{
		struct Visitor
		{
			Visitor ( Float lastUpdateTime )
				: m_lastUpdateTime( lastUpdateTime )
			{}

			void Visit( CRenderEnvProbe *probe )
			{
				++GRenderingStats.m_totalEnvProbeObjects;
				if ( probe->IsGlobalProbe() )
				{
					++GRenderingStats.m_totalGlobalEnvProbeObjects;
				}

				Int32 slotIdx = probe->GetDynamicData().m_arraySlotIndex;
				if ( -1 != slotIdx )
				{
					ASSERT( slotIdx >= 0 && slotIdx < (Int32)GRenderingStats.m_numEnvProbeStats );
					if ( slotIdx >= 0 && slotIdx < (Int32)GRenderingStats.m_numEnvProbeStats )
					{
						SceneRenderingStats::EnvProbeStats &dst = GRenderingStats.m_envProbeStats[slotIdx];

						ASSERT( !dst.m_hasProbeObject && "Slot duplicated in probes - there should be at most on probe assigned to the slot" );
						dst.m_hasProbeObject = true;

						dst.m_lastUpdateDelayMillis = (Uint32)( 1000 * Max( 0.f, m_lastUpdateTime - probe->GetDynamicData().m_lastFullUpdateFinishUpdateTime ) );
						dst.m_lastUpdateDurationMillis = (Uint32)( 1000 * Max( 0.f, probe->GetDynamicData().m_lastFullUpdateFinishUpdateTime - probe->GetDynamicData().m_lastFullUpdateStartUpdateTime ) );
					}
				}
			}

			Float m_lastUpdateTime;
		};

		Visitor visitor ( m_lastUpdateTime );
		CRenderEnvProbe::VisitResourcesAll( visitor );
	}

#endif
}

Uint32 SelectEnvProbes_AddRef( const Vector &refPosition, Uint32 capacity, Float *outDistSquared, CRenderEnvProbe **outProbes, SScopedEnvProbesTable *outOccupyingTable, Bool *outIsGlobalCubesValid )
{
	PC_SCOPE_RENDER_LVL1( SelectEnvProbes_AddRef );

	RED_ASSERT( capacity > 0 );

	if ( outOccupyingTable )
	{
		outOccupyingTable->Release();
	}

	if ( outIsGlobalCubesValid )
	{
		*outIsGlobalCubesValid = false;
	}
	
	Uint32 numGlobalProbes = 0;
	
	Uint32				numSelected = 0;
	Float				*selectedDistSq = outDistSquared;
	CRenderEnvProbe		**selectedProbes = outProbes;

	// Clear selection tables
	for ( Uint32 i=0; i<capacity; ++ i )
	{
		selectedDistSq[i] = 0;
		selectedProbes[i] = NULL;
	}

	// Build selected probes table
	Bool isGlobalProbeSelected = false;
	for ( TRenderResourceIterator< CRenderEnvProbe > it; it; ++it )
	{
		const Bool isCurrentProbeGlobal = it->IsGlobalProbe();

		// Update global probes counter
		numGlobalProbes += isCurrentProbeGlobal ? 1 : 0;

		// Update occupying table
		if ( outOccupyingTable )
		{
			const Int32 slotIndex = it->GetDynamicData().m_arraySlotIndex;
			if ( -1 != slotIndex )
			{
				RED_ASSERT( !(*outOccupyingTable)[slotIndex] );
				outOccupyingTable->Set( slotIndex, *it );
			}
		}

		// Calculate square dist to probe
		Float currDistSq = -1;
		if ( isCurrentProbeGlobal )
		{
			currDistSq = -1;
		}
		else if ( it->GetProbeParams().m_genParams.m_isInteriorFallback )
		{
			currDistSq = -0.5f;
		}
		else
		{
			currDistSq = it->GetProbeParams().SquaredDistance( refPosition );
			RED_ASSERT( currDistSq >= 0 );
		}

		// Find best selection slot
		Int32 bestSelectionSlot = -1;
		if ( !(isCurrentProbeGlobal && isGlobalProbeSelected) )
		{
			if ( numSelected == capacity )
			{
				Int32 slotIndex = 0;
				for ( Uint32 i=1; i<numSelected; ++i )
				{
					if ( selectedDistSq[i] > selectedDistSq[slotIndex] )
					{
						slotIndex = i;
					}
				}

				if ( currDistSq < selectedDistSq[slotIndex] )
				{
					bestSelectionSlot = slotIndex;
				}
			}
			else
			{
				bestSelectionSlot = numSelected;
				++numSelected;
			}
		}

		// Update selection
		if ( -1 != bestSelectionSlot )
		{
			isGlobalProbeSelected |= isCurrentProbeGlobal;
			selectedDistSq[bestSelectionSlot] = currDistSq;
			CRenderEnvProbe *&refSelProbe = selectedProbes[bestSelectionSlot];
			SAFE_RELEASE( refSelProbe );
			refSelProbe = *it;
			refSelProbe->AddRef();
		}
	}

	// Output whether global cubes are valid
	if ( outIsGlobalCubesValid )
	{
		*outIsGlobalCubesValid = (1 == numGlobalProbes);
	}

	return numSelected;
}

void CRenderEnvProbeManager::UpdateSlots( const Vector &cameraPosition, Float updateTimeDelta, Bool enableFastSelection )
{
	PC_SCOPE_RENDER_LVL1( UpdateSlots );
	
	// Build probes tables
	Bool isGlobalCubesValid = false;
	SScopedEnvProbesTable tableCurrent ( false ); //< probes currently assigned to slots
	SScopedEnvProbesTable tableSelected ( false ); //< currently selected probes to be shown
	{
		Uint32				numSelected = 0;
		Float				selectedDistSq[CUBE_ARRAY_CAPACITY];
		CRenderEnvProbe		*selectedProbes[CUBE_ARRAY_CAPACITY];

		// Build selected probes table
		numSelected = SelectEnvProbes_AddRef( cameraPosition, CUBE_ARRAY_CAPACITY, selectedDistSq, selectedProbes, &tableCurrent, &isGlobalCubesValid );

		// Sort selected probes (most important first)
		for ( Uint32 i=0; i+1<numSelected; ++i )
		for ( Uint32 j=i+1; j<numSelected; ++j )
		{
			RED_ASSERT( selectedProbes[i] && selectedProbes[j] && selectedProbes[i] != selectedProbes[j] );
			if ( selectedDistSq[i] > selectedDistSq[j] )
			{
				Swap( selectedDistSq[i], selectedDistSq[j] );
				Swap( selectedProbes[i], selectedProbes[j] );
			}
		}

		// Put selected probes to selected table
		for ( Uint32 i=0; i<numSelected; ++i )
		{
			RED_ASSERT( selectedProbes[i] );
			RED_ASSERT( !(i>0 && selectedProbes[i]->IsGlobalProbe()) );
			tableSelected.Set( i, selectedProbes[i] );
		}

		// Release selected probes
		for ( Uint32 i=0; i<numSelected; ++i )
		{
			SAFE_RELEASE( selectedProbes[i] );
		}
	}

	// In case of fast selection drop non selected, active slots immedately
	if ( enableFastSelection )
	{
		for ( Uint32 slot_i=0; slot_i<CUBE_ARRAY_CAPACITY; ++slot_i )
		{
			CRenderEnvProbe *probe = tableCurrent[slot_i];

			// In case of slot without an envprobe set zero weight...
			if ( !probe )
			{
				m_cubeSlotsData[slot_i].m_weight = 0; //< set zero weight so that we wouldn't have to wait for the fadeout
				continue;
			}

			// In case of a slot has a probe, but it wasn't selected as needed anymore...
			if ( -1 == tableSelected.Find( probe ) )
			{
				// Perform a fast discard of that probe
				probe->RefDynamicData().DiscardArraySlot(); //< keep this before clearing tableCurrent in case the only ref would be held by tableCurrent
				probe = nullptr;
				tableCurrent.Set( slot_i, nullptr );
				m_cubeSlotsData[slot_i].m_weight = 0; //< set zero weight so that we wouldn't have to wait for the fadeout
				continue;
			}
		}
	}

	// Build slots active tab and slots ready to show tab
	Bool slotsActiveTab[CUBE_ARRAY_CAPACITY];
	Bool slotsReadyToShowTab[CUBE_ARRAY_CAPACITY];
	{
		memset( slotsActiveTab, 0, sizeof(slotsActiveTab) );
		memset( slotsReadyToShowTab, 0, sizeof(slotsReadyToShowTab) );

		for ( Uint32 slot_i=0; slot_i<CUBE_ARRAY_CAPACITY; ++slot_i )
		{
			// Mark fading out slots (no probe needed here - could have been destroyed)
			if ( m_cubeSlotsData[slot_i].m_weight > 0 )
			{
				slotsActiveTab[slot_i] = true;
			}

			// Mark slots in use by currently assigned envprobes
			CRenderEnvProbe *slotProbe = tableCurrent[slot_i];
			if ( slotProbe )
			{
				RED_ASSERT( slot_i == slotProbe->GetDynamicData().m_arraySlotIndex );
				if ( !((GLOBAL_SLOT_INDEX == slot_i && !isGlobalCubesValid) || (GLOBAL_SLOT_INDEX == slot_i) != slotProbe->IsGlobalProbe()) )
				{
					slotsActiveTab[slot_i] = true;

					if ( slotProbe->GetDynamicData().IsReadyToDisplay( *this ) )
					{
						slotsReadyToShowTab[slot_i] = true;
					}
				}
			}
		}
	}

	// Remove from selected probes these which wouldn't be active anyway because of some data changes etc
	// Note that this may limit number of probes visible at once (hence we selected on #CUBE_ARRAY_CAPACITY), but this situation
	// won't happen normally and will be only in the editor during changing whether some probes are global or not, or the global probes are not setup correctly (there is more that one global probe).
	{
		// Remove global probes if global probes are invalid
		if ( !isGlobalCubesValid )
		{
			for ( Uint32 i=0; i<CUBE_ARRAY_CAPACITY; ++i )
			{
				CRenderEnvProbe *probe = tableSelected[i];
				if ( probe && probe->IsGlobalProbe() )
				{
					tableSelected.Set( i, nullptr );
				}
			}
		}

		// Remove probes which were not succesfully activated
		for ( Uint32 i=0; i<CUBE_ARRAY_CAPACITY; ++i )
		{
			CRenderEnvProbe *probe = tableSelected[i];
			if ( probe && -1 != probe->GetDynamicData().m_arraySlotIndex && !slotsActiveTab[probe->GetDynamicData().m_arraySlotIndex] )
			{
				tableSelected.Set( i, nullptr );
			}
		}
	}

	// Drop slots for non selected probes
	for ( Uint32 i=0; i<CUBE_ARRAY_CAPACITY; ++i )
	{
		CRenderEnvProbe *probe = tableCurrent[i];
		if ( probe && -1 == tableSelected.Find( probe ) )
		{
			probe->RefDynamicData().DiscardArraySlot();
			tableCurrent.Set( i, nullptr );
		}
	}

	// Allocate slots
	for ( Uint32 sel_slot_i=0, unusedIterator=0; sel_slot_i<CUBE_ARRAY_CAPACITY; ++sel_slot_i )
	{
		CRenderEnvProbe *probe = tableSelected[sel_slot_i];
		if ( !probe )
		{
			continue;
		}

		if ( -1 != probe->GetDynamicData().m_arraySlotIndex )
		{
			RED_ASSERT( probe->GetDynamicData().m_arraySlotIndex == tableCurrent.Find( probe ) );
			continue;
		}

		Int32 newSlotIndex = -1;
		if ( probe->IsGlobalProbe() )
		{
			if ( isGlobalCubesValid && !slotsActiveTab[GLOBAL_SLOT_INDEX] )
			{
				newSlotIndex = GLOBAL_SLOT_INDEX;
			}
		}
		else
		{
			while ( unusedIterator < CUBE_ARRAY_CAPACITY && (slotsActiveTab[unusedIterator] || GLOBAL_SLOT_INDEX == unusedIterator) )
			{
				++unusedIterator;
			}

			if ( unusedIterator < CUBE_ARRAY_CAPACITY )
			{	
				newSlotIndex = unusedIterator;
				++unusedIterator;
			}
		}

		if ( -1 != newSlotIndex )
		{
			RED_ASSERT( !slotsActiveTab[newSlotIndex] );
			RED_ASSERT( !tableCurrent[newSlotIndex] );
			probe->RefDynamicData().m_arraySlotIndex = newSlotIndex;
			tableCurrent.Set( newSlotIndex, probe );
			slotsActiveTab[newSlotIndex] = true;
		}
	}

	// Update slots
	for ( Uint32 slot_i=0; slot_i<CUBE_ARRAY_CAPACITY; ++slot_i )
	{
		SCubeSlotData &slot = m_cubeSlotsData[slot_i];
		CRenderEnvProbe *probe = tableCurrent[slot_i];
		if ( probe )
		{
			slot.ImportRenderEnvProbeData( *probe );
			slot.m_needsReflectionBaseUpdate = true;
		}
		else
		{
			slot.m_needsReflectionBaseUpdate = false;
			slot.m_reflectionBaseUpdateCounter = 0;
			slot.m_isInReflectionAtlas = false;
		}
	}

	// Update slot weights
	for ( Uint32 slot_i=0; slot_i<CUBE_ARRAY_CAPACITY; ++slot_i )
	{	
		Bool isFadeIn = slotsReadyToShowTab[slot_i];
		Float fadeDuration = isFadeIn ? m_cubeSlotsData[slot_i].m_probeParams.m_genParams.m_fadeInDuration : m_cubeSlotsData[slot_i].m_probeParams.m_genParams.m_fadeOutDuration;		
		if ( fadeDuration < 0 )
		{
			fadeDuration = isFadeIn ? ENVPROBE_FADE_IN_DURATION_DEFAULT : ENVPROBE_FADE_OUT_DURATION_DEFAULT;
		}

		const Float speed = (isFadeIn ? 1.f : -1.f)  / Max( 0.001f, fadeDuration );
		const Float weightAdd = GLOBAL_SLOT_INDEX == slot_i ? (speed > 0 ? 1.f : -1.f) : speed * updateTimeDelta;
		m_cubeSlotsData[slot_i].m_weight = Clamp( m_cubeSlotsData[slot_i].m_weight + weightAdd, 0.f, 1.f );
	}

	// Update nesting order
	UpdateNestingOrder();
}

Bool CRenderEnvProbeManager::ProbeQualifiesForPrefetch( const CRenderEnvProbe &envProbe ) const
{
	return 
		!envProbe.GetDynamicData().m_prefetchFailed &&
		-1 != envProbe.GetDynamicData().m_arraySlotIndex &&
		envProbe.GetFacesDataSource() &&
		envProbe.GetFacesDataSource()->IsLoadable();
}

Bool CRenderEnvProbeManager::ProbeQualifiesForUpdate( const CRenderFrameInfo &info, const CRenderEnvProbe &envProbe ) const
{
	return 
		info.m_envParametersGame.m_displaySettings.m_allowEnvProbeUpdate &&
		-1 != envProbe.GetDynamicData().m_arraySlotIndex &&
		envProbe.GetFacesDataSource() &&
		envProbe.GetFacesDataSource()->IsLoadable();
}

void CRenderEnvProbeManager::CancelCurrProbeUpdate()
{
	if ( m_currLoadingDataSource )
	{
		RED_FATAL_ASSERT( !m_cancelledLoadingDataSource, "Why do we have a loading data source when the previous one is not finished?" );

		IEnvProbeDataSource::tScopedLock lock ( IEnvProbeDataSource::GetCommunicationLock() );

		m_cancelledLoadingDataSource = m_currLoadingDataSource;
		m_cancelledLoadingDataSource->RequestFastLoadingFinish();
		m_currLoadingDataSource = NULL;		
	}
	
	SAFE_RELEASE( m_pCurrGenProbe );
	m_currGenStartUpdateTime = -1;
	m_currGenProgress = -1;
	
	for ( Uint32 face_i=0; face_i<6; ++face_i )
	{
		m_currGenPerFaceData[face_i].Reset();
	}
}

void CRenderEnvProbeManager::UpdateEnvProbes( const CRenderCollector &mainRenderCollector, CRenderEnvProbe *forcedProbeToRender, Float updateTimeDelta, Bool forceInstantUpdate )
{
	PC_SCOPE_RENDER_LVL1( UpdateEnvProbes );

	const CRenderFrameInfo &info = mainRenderCollector.GetRenderFrameInfo();
	const Float currUpdateTime = m_lastUpdateTime;
	ASSERT( currUpdateTime >= 0 );
	
	// Drop current probe update if probe not valid anymore
	if ( nullptr != m_pCurrGenProbe && (!ProbeQualifiesForUpdate( info, *m_pCurrGenProbe ) || ( forcedProbeToRender && forcedProbeToRender != m_pCurrGenProbe ) ) )
	{
		CancelCurrProbeUpdate();
	}

	// Find probe to update
	if ( NULL == m_pCurrGenProbe )
	{
		PC_SCOPE_RENDER_LVL1( UpdateEnvProbes_ChooseProbe );

		CRenderEnvProbe *probeToUpdate = NULL;
		if ( forcedProbeToRender )
		{
			if ( ProbeQualifiesForUpdate( info, *forcedProbeToRender ) )
			{
				probeToUpdate = forcedProbeToRender;
				probeToUpdate->AddRef();
			}
		}
		else
		{
			for ( TRenderResourceIterator< CRenderEnvProbe > it; it; ++it )
			{
				CRenderEnvProbe *currProbe = *it;
				const Float currProbeTime = currProbe->GetDynamicData().m_lastUpdateTime;

				if ( !ProbeQualifiesForUpdate( info, *currProbe ) )
				{
					continue;
				}

				const Bool currIsForcedGlobal = currProbe->IsGlobalProbe() && -1 == currProbe->GetDynamicData().m_lastUpdateTime;
				if ( !currIsForcedGlobal && NULL != probeToUpdate && currProbeTime >= probeToUpdate->GetDynamicData().m_lastUpdateTime )
				{
					continue;
				}

				const SCubeSlotData &slotData = GetCubeSlotData( currProbe->GetDynamicData().m_arraySlotIndex );
				if ( !currIsForcedGlobal && currProbe != m_pCurrGenProbe && slotData.m_reflectionBaseUpdateCounter >= 1 )
				{
					continue;
				}

				SAFE_RELEASE( probeToUpdate );
				probeToUpdate = currProbe;
				probeToUpdate->AddRef();

				if ( currIsForcedGlobal )
				{
					break;
				}
			}
		}

		// Replace currently chosen updateProbe if needed
		if ( probeToUpdate != m_pCurrGenProbe )
		{
			CancelCurrProbeUpdate();

			if ( NULL != probeToUpdate )
			{
				m_pCurrGenProbe = probeToUpdate;
				m_pCurrGenProbe->AddRef();
				m_pCurrGenProbe->RefDynamicData().m_lastUpdateTime = currUpdateTime;
				m_currGenRenderFrameInfo = CRenderFrameInfo::BuildEnvProbeFaceRenderInfoBase( info );
				m_currGenStartUpdateTime = currUpdateTime;
				m_currGenProgress = 0;
			}
		}

		// Release probe reference
		SAFE_RELEASE( probeToUpdate );
	}

	// Update the probe
	if ( NULL != m_pCurrGenProbe )
	{
		PC_SCOPE_RENDER_LVL1( UpdateEnvProbes_UpdateTheProbe );

		Bool finalForceInstantUpdate = info.m_envParametersGame.m_displaySettings.m_enableEnvProbeInstantUpdate;
		m_currGenRenderFrameInfo.m_envParametersGame.m_displaySettings.m_enableEnvProbeInstantUpdate = finalForceInstantUpdate;
		
		Int32 allowedUpdatesLeft = 1;
		Float adaptedUpdateTimeDelta = updateTimeDelta;

		// Force instant update for global probe after init
		if ( m_pCurrGenProbe->IsGlobalProbe() && -1 == m_pCurrGenProbe->GetDynamicData().m_lastFullUpdateFinishUpdateTime )
		{
			finalForceInstantUpdate = true;
		}

		/*
		// Force instant update if new probe need to be loaded
		if ( !m_pCurrGenProbe->GetDynamicData().m_isArraySlotInit )
		{
			finalForceInstantUpdate = true;
		}
		*/

		//
		if ( Config::cvForceInstantEnvProbeUpdate.Get() )
		{
			finalForceInstantUpdate = true;
		}

		//
		if ( forceInstantUpdate )
		{
			finalForceInstantUpdate = true;
		}

		// Every probe is now update instantly right after it appears on the scene
		if ( finalForceInstantUpdate )
		{			
			allowedUpdatesLeft = NumericLimits<Int32>::Max();
			adaptedUpdateTimeDelta = 10;
		}
		
		// Update the probe
		if ( adaptedUpdateTimeDelta > 0 ) // timeDelta may be zero in case the game is paused (window out of focus etc).
		{
			while ( NULL != m_pCurrGenProbe && allowedUpdatesLeft-- > 0 )
			{
				if ( !UpdateEnvProbe( mainRenderCollector, *GetRenderer(), adaptedUpdateTimeDelta ) )
				{
					break;
				}
			}
		}
	}
}

void CRenderEnvProbeManager::MergeAmbientAtlas( Float gameTime, const CEnvAmbientProbesGenParametersAtPoint &ambientParams )
{
	PC_SCOPE_RENDER_LVL1( MergeAmbientAtlas );

	for ( Uint32 mip_i=0; mip_i<2; ++mip_i )
	{
		const Uint32 mergeWidth = GpuApi::GetTextureLevelDesc( m_ambientAtlas, 0 ).width >> mip_i;
		const Uint32 mergeHeight = GpuApi::GetTextureLevelDesc( m_ambientAtlas, 0 ).height >> mip_i;

		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, m_ambientAtlas, (Uint16)mip_i );
		rtSetup.SetViewport( mergeWidth, mergeHeight );
		GpuApi::SetupRenderTargets( rtSetup );

		//
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

		//
		const Uint32 segmentSize = mergeWidth/2;
		const Uint32 extent = (segmentSize - (GetEnvProbeResolution( ENVPROBETYPE_Ambient ) >> mip_i)) / 2;
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, ambientParams.m_colorAmbient.GetColorScaledGammaToLinear( true ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, ambientParams.m_colorSceneAdd.GetColorScaledGammaToLinear( true ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, ambientParams.m_colorSkyTop.GetColorScaledGammaToLinear( true ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, ambientParams.m_colorSkyHorizon.GetColorScaledGammaToLinear( true ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_4, Vector ( ambientParams.m_skyShape.GetScalarClampMin(0), (Float)mip_i, (Float)segmentSize, (Float)extent ) );
		ASSERT( CUBE_ARRAY_CAPACITY == m_cubeSlotsData.Size() );
		for ( Uint32 slot_i=0; slot_i<CUBE_ARRAY_CAPACITY; ++slot_i )
		{
			const Bool isInteriorFallback = m_cubeSlotsData[slot_i].m_probeParams.m_genParams.m_isInteriorFallback;
			Vector ambientColor = m_cubeSlotsData[slot_i].m_probeParams.m_genParams.GetAmbientColorLinear( gameTime ) * (isInteriorFallback ? m_currGenRenderFrameInfo.m_envParametersArea.m_interiorFallback.m_colorAmbientMul.GetColorScaledGammaToLinear(true) : Vector::ONES);
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_5 + slot_i, Vector ( ambientColor.X, ambientColor.Y, ambientColor.Z, isInteriorFallback ? 1.f : 0.f ) );
		}
		
		// Bind textures
		GpuApi::TextureRef tex[] = { m_cachedUnpackedAmbientScene, m_cachedUnpackedAmbientSkyFactor };
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

		//
		GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterAmbientMerge );

		// Unbind textures
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
	}
}

void CRenderEnvProbeManager::MergeReflectionAtlas( CRenderCollector &collector, const GpuApi::TextureRef &texReflectionAtlas, const GpuApi::TextureRef &texMergeSource )
{
	PC_SCOPE_RENDER_LVL1( MergeReflectionAtlas );
	
	const Uint32 mergeWidth = GpuApi::GetTextureLevelDesc( texReflectionAtlas, 0 ).width;
	const Uint32 mergeHeight = GpuApi::GetTextureLevelDesc( texReflectionAtlas, 0 ).height;

	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, texReflectionAtlas );
	rtSetup.SetViewport( mergeWidth, mergeHeight );
	GpuApi::SetupRenderTargets( rtSetup );

	//
	CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

	//
	const Uint32 segmentSize = mergeWidth/2;
	const Uint32 margin = (segmentSize - GetEnvProbeResolution( ENVPROBETYPE_Reflection )) / 2;
	const CEnvReflectionProbesGenParametersAtPoint &colorModParams = collector.GetRenderFrameInfo().m_envParametersArea.m_globalLight.m_envProbeBaseLightingReflection;
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)segmentSize, (Float)margin, 0, 0 ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, colorModParams.m_colorAmbient.GetColorScaledGammaToLinear( true ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, colorModParams.m_colorSceneMul.GetColorScaledGammaToLinear( true ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, colorModParams.m_colorSceneAdd.GetColorScaledGammaToLinear( true ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_4, colorModParams.m_colorSkyMul.GetColorScaledGammaToLinear( true ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_5, colorModParams.m_colorSkyAdd.GetColorScaledGammaToLinear( true ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_6, Vector ( colorModParams.m_remapOffset.GetScalarClampMin(0), colorModParams.m_remapStrength.GetScalarClampMin(0), colorModParams.m_remapClamp.GetScalarClampMin(0), 0.f ) );

	ASSERT( CUBE_ARRAY_CAPACITY == m_cubeSlotsData.Size() );
	for ( Uint32 slot_i=0; slot_i<CUBE_ARRAY_CAPACITY; ++slot_i )
	{
		const SEnvProbeGenParams &slotGenParams = m_cubeSlotsData[slot_i].m_probeParams.m_genParams;
		const Float interiorFallbackFactor = slotGenParams.m_isInteriorFallback ? 1.f : 0.f;
		Vector customParamsValue = Vector ( 
			Clamp( slotGenParams.m_dimmerFactor, 0.f, 1.f ), 
			Clamp( slotGenParams.m_fogAmount, 0.f, 1.f ),
			interiorFallbackFactor, 
			0 );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_7 + slot_i, customParamsValue );
	}

	// Bind textures
	GpuApi::TextureRef tex[] = { texMergeSource, m_cachedUnpackedAlbedo, m_cachedUnpackedNormals, m_cachedUnpackedDepthAndSky, m_ambientAtlas };
	GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearMip, GpuApi::PixelShader );

	//
	GetRenderer()->CalculateSharedConstants( collector.GetRenderFrameInfo(), mergeWidth, mergeHeight, -1, -1 ); // original renderinfo because we need current fog settings
	GetRenderer()->BindSharedConstants( GpuApi::PixelShader );

	//
	GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterReflectionMerge );

	//
	GetRenderer()->UnbindSharedConstants( GpuApi::PixelShader );

	// Unbind textures
	GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
}

void CRenderEnvProbeManager::InitCachedUnpackedAmbientTextures( const SRenderEnvProbeDynamicData &dynData, CRenderEnvProbeManager::SCubeSlotData &slotData )
{
	PC_SCOPE_RENDER_LVL1( InitCachedUnpackedAmbientTextures );

	// ace_optimize: don't generate mips which we don't need
	// ace_todo: a lot of duplicated code here

	const SCubeSlotData &envProbeSlot = RefCubeSlotData( dynData.m_arraySlotIndex );
	const Uint32 baseResolution = GetEnvProbeDataSourceResolution();
	ASSERT( dynData.m_arraySlotIndex >= 0 && dynData.m_arraySlotIndex < CUBE_ARRAY_CAPACITY );
	
	for ( Uint32 bufftype_i=0; bufftype_i<2; ++bufftype_i )
	{
		const Bool isSkyBuffType = 0!=bufftype_i;

		const Uint32 cubeRowIndex = dynData.m_arraySlotIndex;

		{
			const Uint32 tempExtent				= (1 << (GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection ) - 1));
			const Uint32 tempSphereResolution	= GetEnvProbeResolution( ENVPROBETYPE_Reflection );
			const Uint32 tempSphereOffsetY		= cubeRowIndex * (tempSphereResolution + 2 * tempExtent);
			const Uint32 tempSpheresAreaWidth	= 2 * (tempSphereResolution + 2 * tempExtent);
			const Uint32 tempSpheresAreaHeight	= tempSphereResolution + 2 * tempExtent;

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, m_reflectionAtlas );
			rtSetup.SetViewport( tempSpheresAreaWidth, tempSpheresAreaHeight, 0, tempSphereOffsetY );

			GpuApi::SetupRenderTargets( rtSetup );

			//
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

			//
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)tempSphereResolution, (Float)tempSphereResolution, (Float)tempExtent, 0 ) );

			if ( envProbeSlot.m_probeParams.m_genParams.m_isInteriorFallback )
			{
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (isSkyBuffType ? 1.f : 0.f), 0, 0, 0 ) );

				CRenderCubeTexture *interiorFallbackCube = m_currGenRenderFrameInfo.m_envParametersDayPoint.m_interiorFallbackAmbientTexture.Get<CRenderCubeTexture>();				
				if ( interiorFallbackCube )
				{
					interiorFallbackCube->BindNoSampler( 0, RST_PixelShader );					
				}
				else
				{
					GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
				}
				GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearMip, GpuApi::PixelShader );

				GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterFillCubeFaceInterior );

				GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
			}
			else
			{
				// Bind textures
				GpuApi::TextureRef tex[] = { slotData.GetFaceTexture( ENVPROBEBUFFERTEX_Depth ), slotData.GetFaceTexture( ENVPROBEBUFFERTEX_Albedo ) };
				GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

				//
				GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( isSkyBuffType ? GetRenderer()->m_shaderEnvProbeFilterFillCubeFaceSkyFactor : GetRenderer()->m_shaderEnvProbeFilterFillCubeFaceAlbedo );

				// Unbind textures
				GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
			}
		}

		// Perform filtering
		FilterReflectionAtlas( m_reflectionAtlas, cubeRowIndex, false );

		// Copy to ambient atlas
		for ( Uint32 mip_i=0; mip_i<GetEnvProbeTypeNumMips( ENVPROBETYPE_Ambient ); ++mip_i )
		{
			const Uint32 sourceMipIndex = MLog2( GetEnvProbeResolution( ENVPROBETYPE_Reflection ) / GetEnvProbeResolution( ENVPROBETYPE_Ambient ) ) + mip_i;
			const Uint32 copyWidth = GpuApi::GetTextureLevelDesc( m_ambientAtlas, 0 ).width >> mip_i;
			const Uint32 copyHeight = (GpuApi::GetTextureLevelDesc( m_ambientAtlas, 0 ).height >> mip_i) / CUBE_ARRAY_CAPACITY;
			const Uint32 targetOffsetX = 0;
			const Uint32 targetOffsetY = copyHeight * dynData.m_arraySlotIndex;

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, isSkyBuffType ? m_cachedUnpackedAmbientSkyFactor : m_cachedUnpackedAmbientScene, (Uint16)mip_i );
			rtSetup.SetViewport( copyWidth, copyHeight, targetOffsetX, targetOffsetY );
			GpuApi::SetupRenderTargets( rtSetup );

			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( 0, 0, (Float)sourceMipIndex, 0 ) );

			//
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

			// Bind textures
			GpuApi::TextureRef tex[] = { m_reflectionAtlas };
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

			//
			GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterCopy );

			// Unbind textures
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
		}
	}
}

void CRenderEnvProbeManager::InitCachedUnpackedReflectionTextures( const CRenderEnvProbe &envProbe, CRenderEnvProbeManager::SCubeSlotData &slotData )
{
	PC_SCOPE_RENDER_LVL1( InitCachedUnpackedReflectionTextures );

	const SRenderEnvProbeDynamicData &dynData = envProbe.GetDynamicData();
	const SCubeSlotData &envProbeSlot = RefCubeSlotData( dynData.m_arraySlotIndex );
	const Uint32 baseResolution = GetEnvProbeDataSourceResolution();
	ASSERT( dynData.m_arraySlotIndex >= 0 && dynData.m_arraySlotIndex < CUBE_ARRAY_CAPACITY );

	const Uint32 cubeRowIndex = dynData.m_arraySlotIndex;

	const Uint32 tempExtent				= (1 << (GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection ) - 1));
	const Uint32 tempSphereResolution	= GetEnvProbeResolution( ENVPROBETYPE_Reflection );
	const Uint32 tempSphereOffsetY		= cubeRowIndex * (tempSphereResolution + 2 * tempExtent);
	const Uint32 tempSpheresAreaWidth	= 2 * (tempSphereResolution + 2 * tempExtent);
	const Uint32 tempSpheresAreaHeight	= tempSphereResolution + 2 * tempExtent;

	// unpack albedo
	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, m_cachedUnpackedAlbedo );
		rtSetup.SetViewport( tempSpheresAreaWidth, tempSpheresAreaHeight, 0, tempSphereOffsetY );

		GpuApi::SetupRenderTargets( rtSetup );
		
		//
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

		//
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)tempSphereResolution, (Float)tempSphereResolution, (Float)tempExtent, 0 ) );

		// Bind textures
		GpuApi::TextureRef tex[] = { slotData.GetFaceTexture( ENVPROBEBUFFERTEX_Depth ), slotData.GetFaceTexture( ENVPROBEBUFFERTEX_Albedo ) };
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

		//
		GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterFillCubeFaceAlbedo );

		// Unbind textures
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
	}

	// unpack normals
	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, m_cachedUnpackedNormals );
		rtSetup.SetViewport( tempSpheresAreaWidth, tempSpheresAreaHeight, 0, tempSphereOffsetY );

		GpuApi::SetupRenderTargets( rtSetup );

		//
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

		//
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)tempSphereResolution, (Float)tempSphereResolution, (Float)tempExtent, 0 ) );

		// Bind textures
		GpuApi::TextureRef tex[] = { slotData.GetFaceTexture( ENVPROBEBUFFERTEX_Depth ), slotData.GetFaceTexture( ENVPROBEBUFFERTEX_Normals ) };
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

		//
		GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterFillCubeFaceNormals );

		// Unbind textures
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
	}

	// unpack depth and sky
	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, m_cachedUnpackedDepthAndSky );
		rtSetup.SetViewport( tempSpheresAreaWidth, tempSpheresAreaHeight, 0, tempSphereOffsetY );

		GpuApi::SetupRenderTargets( rtSetup );

		//
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

		//
		CRenderCamera faceCamera;
		BuildEnvProbeCamera( faceCamera, envProbe.GetProbeGenOrigin(), 1 );

		//
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)tempSphereResolution, (Float)tempSphereResolution, (Float)tempExtent, 0 ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, faceCamera.GetPosition() );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, faceCamera.GetScreenToWorld().V[0] );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, faceCamera.GetScreenToWorld().V[1] );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_4, faceCamera.GetScreenToWorld().V[2] );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_5, faceCamera.GetScreenToWorld().V[3] );

		// Bind textures
		GpuApi::TextureRef tex[] = { slotData.GetFaceTexture( ENVPROBEBUFFERTEX_Depth ) };
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

		//
		GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterFillCubeFaceDepthAndSky );

		// Unbind textures
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
	}
}

void CRenderEnvProbeManager::UnpackIntermediateCube( CRenderCubeTexture *interiorFallbackCube, GpuApi::TextureRef texSource, GpuApi::TextureRef texTarget, Uint32 cubeRowIndex, Bool sampleLinear )
{
	PC_SCOPE_RENDER_LVL1( UnpackIntermediateCube );
	
	const Uint32 tempMargin				= 0;
	const Uint32 tempExtent				= (1 << (GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection ) - 1));
	const Uint32 tempFullWidth			= GpuApi::GetTextureLevelDesc( texTarget, 0 ).width;
	const Uint32 tempFullHeight			= GpuApi::GetTextureLevelDesc( texTarget, 0 ).height;
	const Uint32 tempSphereResolution	= GetEnvProbeResolution( ENVPROBETYPE_Reflection );
	//const Uint32 tempSphereOffsetX		= 0;
	const Uint32 tempSphereOffsetY		= cubeRowIndex * (tempSphereResolution + 2 * tempExtent);
	const Uint32 tempSpheresAreaWidth	= 2 * (tempSphereResolution + 2 * tempExtent);
	const Uint32 tempSpheresAreaHeight	= tempSphereResolution + 2 * tempExtent;

	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, texTarget );
	rtSetup.SetViewport( tempSpheresAreaWidth, tempSpheresAreaHeight, 0, tempSphereOffsetY );
	GpuApi::SetupRenderTargets( rtSetup );

	// Setup draw context
	CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

	// Bind texture
	if ( interiorFallbackCube )
	{
		interiorFallbackCube->BindNoSampler( 0, RST_PixelShader );
	}
	else
	{
		GpuApi::BindTextures( 0, 1, &texSource, GpuApi::PixelShader );
	}
	GpuApi::SetSamplerStatePreset( 0, sampleLinear ? GpuApi::SAMPSTATEPRESET_ClampLinearNoMip : GpuApi::SAMPSTATEPRESET_ClampPointNoMip );

	// Render
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)tempFullWidth, (Float)tempFullHeight, (Float)tempSphereResolution, (Float)tempMargin ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)0, (Float)tempSphereOffsetY, (Float)tempExtent, 0 ) );
	if ( interiorFallbackCube )
	{
		const CEnvInteriorFallbackParametersAtPoint &interiorParams = m_currGenRenderFrameInfo.m_envParametersArea.m_interiorFallback;
		const SWorldRenderSettings &worldRenderSett = m_currGenRenderFrameInfo.m_worldRenderSettings;

		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, interiorParams.m_colorReflectionLow.GetColorScaledGammaToLinear(true) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, interiorParams.m_colorReflectionMiddle.GetColorScaledGammaToLinear(true) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_4, interiorParams.m_colorReflectionHigh.GetColorScaledGammaToLinear(true) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_5, Vector ( 
			worldRenderSett.m_interiorFallbackReflectionThresholdLow - 0.5f * worldRenderSett.m_interiorFallbackReflectionBlendLow, 
			worldRenderSett.m_interiorFallbackReflectionThresholdHigh - 0.5f * worldRenderSett.m_interiorFallbackReflectionBlendHigh, 
			1.f / Max( 0.00001f, worldRenderSett.m_interiorFallbackReflectionBlendLow ),
			1.f / Max( 0.00001f, worldRenderSett.m_interiorFallbackReflectionBlendHigh ) ) );
		GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterUnpackInterior );
	}
	else
	{
		GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterUnpackRegular );
	}

	// Unbind textures
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
}		

Bool CRenderEnvProbeManager::UpdateCurrEnvProbeFaceTexturesLoading( Bool allowUnpack )
{
	if ( !m_pCurrGenProbe || -1 == m_pCurrGenProbe->GetDynamicData().m_arraySlotIndex )
	{
		return false;
	}

	CRenderEnvProbe &envProbe = *m_pCurrGenProbe;
	SCubeSlotData &envProbeSlot = RefCubeSlotData( envProbe.GetDynamicData().m_arraySlotIndex );

	RED_FATAL_ASSERT( !(m_currLoadingDataSource && m_cancelledLoadingDataSource), "There shouldn't be both current and cancelled data source, since we're waiting for cancel to finish before issuing new load" );
	if ( !envProbe.GetDynamicData().m_isFaceTexturesInit )
	{	
		// Wait until cancelled data source will finish (we don't want to go overbudget with envProbes textures memory pool)
		if ( m_cancelledLoadingDataSource )
		{
			IEnvProbeDataSource::tScopedLock lock ( IEnvProbeDataSource::GetCommunicationLock() );

			switch ( m_cancelledLoadingDataSource->GetState() )
			{
			case IEnvProbeDataSource::STATE_InProgress:
				// do nothing - just wait until data source load will finish
				break;

			case IEnvProbeDataSource::STATE_AllocFailed:
			case IEnvProbeDataSource::STATE_Failed:
			case IEnvProbeDataSource::STATE_Loaded:
			case IEnvProbeDataSource::STATE_NotLoading:
				m_cancelledLoadingDataSource->Rewind();
				m_cancelledLoadingDataSource = NULL;
				break;

			default:
				ASSERT( !"invalid" );		
			}

			return true;
		}

		// Update loading data source
		if ( !m_currLoadingDataSource )
		{
			m_currLoadingDataSource = m_pCurrGenProbe->GetFacesDataSource();
			if ( !m_currLoadingDataSource )
			{
				return false;
			}
		}
	
		// Process loading
		{
			IEnvProbeDataSource::tScopedLock lock ( IEnvProbeDataSource::GetCommunicationLock() );

			switch ( m_currLoadingDataSource->GetState() )
			{
			case IEnvProbeDataSource::STATE_AllocFailed:
				{
					const Float recoveryDelay = Config::cvEnvProbeAllocFailureRecoverDelay.Get();
					const Float currUpdateTime = m_lastUpdateTime;

					// Allow to retry loading in case long enough time have passed since last loaing failure
					Float &refRecoveryTimeStamp = envProbe.RefDynamicData().m_failereRecoveryTimeStamp;
					if ( !IsDuringPrefetch() && -1 != refRecoveryTimeStamp && recoveryDelay > 0 && refRecoveryTimeStamp + recoveryDelay < currUpdateTime )
					{
						envProbe.RefDynamicData().m_failereRecoveryTimeStamp = -1;
						m_currLoadingDataSource->Rewind();
						m_currLoadingDataSource->StartLoading( &envProbeSlot.m_faceTextures );
						return true;
					}
					
					// Update last failure time
					if ( IsDuringPrefetch() )
					{
						refRecoveryTimeStamp = -1;
					}
					else if ( -1 == refRecoveryTimeStamp )
					{
						refRecoveryTimeStamp = currUpdateTime;
					}

					// Release data source
					m_currLoadingDataSource = NULL;
				}
				return false;

			case IEnvProbeDataSource::STATE_Failed:
				envProbe.RefDynamicData().m_failereRecoveryTimeStamp = -1;
				m_currLoadingDataSource = NULL;
				return false;

			case IEnvProbeDataSource::STATE_NotLoading:
				envProbe.RefDynamicData().m_failereRecoveryTimeStamp = -1;
				m_currLoadingDataSource->StartLoading( &envProbeSlot.m_faceTextures );
				return true;

			case IEnvProbeDataSource::STATE_InProgress:
				envProbe.RefDynamicData().m_failereRecoveryTimeStamp = -1;
				// waiting
				return true;

			case IEnvProbeDataSource::STATE_Loaded:
				envProbe.RefDynamicData().m_failereRecoveryTimeStamp = -1;
				envProbe.RefDynamicData().m_isFaceTexturesInit = true;
				m_currLoadingDataSource->Rewind();
				m_currLoadingDataSource = NULL;
				break;

			default:
				RED_FATAL( "Invalid state" );		
			}
		}
	}

	// Init probe cached textures
	if ( allowUnpack && envProbe.GetDynamicData().m_isFaceTexturesInit && !envProbe.GetDynamicData().m_isFaceTexturesUnpacked )
	{
		InitCachedUnpackedAmbientTextures( envProbe.GetDynamicData(), envProbeSlot );
		InitCachedUnpackedReflectionTextures( envProbe, envProbeSlot );
		envProbe.RefDynamicData().m_isFaceTexturesUnpacked = true;		
	}

	//
	return true;
}

void CRenderEnvProbeManager::CopyToTempParaboloid( const GpuApi::TextureRef &texReflectionAtlas )
{
	PC_SCOPE_RENDER_LVL1( CopyToTempParaboloid );

	const Uint32 typeResolution = GetEnvProbeResolution( ENVPROBETYPE_Reflection );
	const Uint32 numTypeMips = GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection );

	const Uint32 mipResolution = typeResolution;

	const Uint32 tempMargin				= ENVPROBE_FILTER_TEMP_MARGIN;
	const Uint32 tempFullWidth			= GpuApi::GetTextureLevelDesc( m_tempParaboloid[0], 0 ).width;
	const Uint32 tempFullHeight			= GpuApi::GetTextureLevelDesc( m_tempParaboloid[0], 0 ).height;
	const Uint32 tempSphereResolution	= mipResolution;
	const Uint32 tempSpheresAreaWidth	= 2 * (tempSphereResolution + 2 * tempMargin);
	const Uint32 tempSpheresAreaHeight	= CUBE_ARRAY_CAPACITY * (tempSphereResolution + 2 * tempMargin);
	const Uint32 tempExtent				= (1 << (GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection ) - 1));

	{
		const Uint32 viewportWidth = tempSpheresAreaWidth;
		const Uint32 viewportHeight = tempSpheresAreaHeight;
		const Uint32 offsetY = 0;

		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, m_tempParaboloid[0] );
		rtSetup.SetViewport( viewportWidth, viewportHeight, 0, offsetY );
		GpuApi::SetupRenderTargets( rtSetup );

		// Setup draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

		//
		GpuApi::TextureRef tex[] = { texReflectionAtlas };
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearMip );
		const GpuApi::TextureLevelDesc &texLevelDesc = GpuApi::GetTextureLevelDesc( texReflectionAtlas, 0 );

		// Render
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)tempFullWidth, (Float)tempFullHeight, (Float)tempSphereResolution, (Float)tempMargin ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)0, (Float)tempExtent, (Float)tempExtent, 0 ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( 1.f / texLevelDesc.width, 1.f / texLevelDesc.height, 0, 0 ) );
		GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterCopyToTemp );

		// Unbind textures
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
	}
}

void CRenderEnvProbeManager::DownscaleTempParaboloid( const GpuApi::TextureRef &texReflectionAtlas, Int32 segmentIndex, Uint32 mipIndex, Bool usePrepareMerge )
{
	PC_SCOPE_RENDER_LVL1( DownscaleTempParaboloid );

	ASSERT( mipIndex > 0 );
	const Uint32 typeResolution = GetEnvProbeResolution( ENVPROBETYPE_Reflection );
	const Uint32 mipResolution = typeResolution >> mipIndex;

	const Uint32 tempMargin				= ENVPROBE_FILTER_TEMP_MARGIN;
	const Uint32 tempFullWidth			= GpuApi::GetTextureLevelDesc( m_tempParaboloid[0], 0 ).width;
	const Uint32 tempFullHeight			= GpuApi::GetTextureLevelDesc( m_tempParaboloid[0], 0 ).height;
	const Uint32 tempSphereResolution	= mipResolution;
	const Uint32 tempSpheresAreaWidth	= 2 * (tempSphereResolution + 2 * tempMargin);
	const Uint32 tempSpheresAreaHeight	= CUBE_ARRAY_CAPACITY * (tempSphereResolution + 2 * tempMargin);
	const Uint32 tempExtent				= (1 << (GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection ) - 1)) >> mipIndex;

	// Downscale previous mipmap
	{
		const Uint32 viewportWidth = tempSpheresAreaWidth;
		const Uint32 viewportHeight = tempSpheresAreaHeight / (-1 != segmentIndex ? CUBE_ARRAY_CAPACITY : 1);
		const Uint32 offsetY = -1 != segmentIndex ? segmentIndex * viewportHeight : 0;

		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, m_tempParaboloid[0] );
		rtSetup.SetViewport( viewportWidth, viewportHeight, 0, offsetY );
		GpuApi::SetupRenderTargets( rtSetup );

		// Setup draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

		//
		GpuApi::TextureRef tex[] = { texReflectionAtlas };
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearMip );
		const GpuApi::TextureLevelDesc &texLevelDesc = GpuApi::GetTextureLevelDesc( texReflectionAtlas, (Uint16)mipIndex - 1 );

		// Render
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)tempFullWidth, (Float)tempFullHeight, (Float)tempSphereResolution, (Float)tempMargin ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)(mipIndex - 1), (Float)tempExtent, (Float)(tempExtent << 1), 0 ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( 1.f / texLevelDesc.width, 1.f / texLevelDesc.height, 0, 0 ) );
		GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( usePrepareMerge ? GetRenderer()->m_shaderEnvProbeFilterPrepareDownscale : GetRenderer()->m_shaderEnvProbeFilterDownscale );

		// Unbind textures
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
	}
}

void CRenderEnvProbeManager::BlurTempParaboloid( const GpuApi::TextureRef &texReflectionAtlas, Int32 segmentIndex, Uint32 mipIndex, Bool usePrepareMerge )
{
	PC_SCOPE_RENDER_LVL1( BlurTempParaboloid );

	const Bool isSmallBlur = 0 == mipIndex;
	const Uint32 typeResolution = GetEnvProbeResolution( ENVPROBETYPE_Reflection );
	const Uint32 mipResolution = typeResolution >> mipIndex;

	const Uint32 tempMargin				= ENVPROBE_FILTER_TEMP_MARGIN;
	const Uint32 tempFullWidth			= GpuApi::GetTextureLevelDesc( m_tempParaboloid[0], 0 ).width;
	const Uint32 tempFullHeight			= GpuApi::GetTextureLevelDesc( m_tempParaboloid[0], 0 ).height;
	const Uint32 tempSphereResolution	= mipResolution;
	const Uint32 tempSpheresAreaWidth	= 2 * (tempSphereResolution + 2 * tempMargin);
	const Uint32 tempSpheresAreaHeight	= CUBE_ARRAY_CAPACITY * (tempSphereResolution + 2 * tempMargin);
	const Uint32 tempExtent				= (1 << (GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection ) - 1)) >> mipIndex;

	// Blur
	{
		const Uint32 blurRange = 5; // MUST MATCH SHADER!!
		const Uint32 blurNumWeights = blurRange + 1;
		Float blurWeights[blurNumWeights];
		{
			struct Local
			{
				static Float CalcGauss( Float normFactor )
				{
					Float a = 1;
					Float b = 0;
					Float c = 2.1f;
					Float d = 6.4f;
					Float e = 1.5f;

					return pow( a * exp( - pow((normFactor - b) * d, 2) / (2 * c * c) ), e );
				}
			};

			Float weightsSum = 0;
			for ( Uint32 i=0; i<blurNumWeights; ++i )
			{
				const Float currWeight = Local::CalcGauss( i / (Float)blurNumWeights );
				blurWeights[i] = currWeight;
				weightsSum += currWeight * (i > 0 ? 2 : 1);
			}

			const Float invWeightsSum = 1.f / weightsSum;
			for ( Uint32 i=0; i<blurNumWeights; ++i )
			{
				blurWeights[i] *= invWeightsSum;
			}
		}

		for ( Uint32 blur_dir_i=0; blur_dir_i<2; ++blur_dir_i )
		{
			const Uint32 viewportWidth = tempSpheresAreaWidth;
			const Uint32 viewportHeight = tempSpheresAreaHeight / (-1 != segmentIndex ? CUBE_ARRAY_CAPACITY : 1);
			const Uint32 offsetY = -1 != segmentIndex ? segmentIndex * viewportHeight : 0;

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, m_tempParaboloid[1] );
			rtSetup.SetViewport( viewportWidth, viewportHeight, 0, offsetY );
			GpuApi::SetupRenderTargets( rtSetup );

			// Setup draw context
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

			//
			GpuApi::TextureRef tex[] = { m_tempParaboloid[0] };
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );

			// Render
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)tempFullWidth, (Float)tempFullHeight, (Float)tempSphereResolution, (Float)tempMargin ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)(blur_dir_i ? 1 : 0), (Float)(blur_dir_i ? 0 : 1), (Float)mipIndex, 0 ) );

			if ( isSmallBlur )
			{
				GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterBlurSmall );
			}
			else
			{
				COMPILE_ASSERT( 6 == blurNumWeights );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( blurWeights[0], blurWeights[1], blurWeights[2], blurWeights[3] ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( blurWeights[4], blurWeights[5], 0, 0 ) );

				GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterBlur );
			}			

			// Unbind textures
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
			Swap( m_tempParaboloid[0], m_tempParaboloid[1] );
		}
	}

	// Merge
	{
		const Uint32 viewportWidth = 2 * (tempSphereResolution + 2 * tempExtent);
		const Uint32 viewportHeight = CUBE_ARRAY_CAPACITY * (tempSphereResolution + 2 * tempExtent) / (-1 != segmentIndex ? CUBE_ARRAY_CAPACITY : 1);
		const Uint32 offsetY = -1 != segmentIndex ? segmentIndex * viewportHeight : 0;

		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, texReflectionAtlas, (Uint16)mipIndex );
		rtSetup.SetViewport( viewportWidth, viewportHeight, 0, offsetY );
		GpuApi::SetupRenderTargets( rtSetup );

		// Setup draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

		//
		GpuApi::TextureRef tex[] = { m_tempParaboloid[0], usePrepareMerge ? GpuApi::TextureRef::Null() : m_mergeSumTexture };
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );

		// Render
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)tempFullWidth, (Float)tempFullHeight, (Float)tempSphereResolution, (Float)tempMargin ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)tempExtent, 0, 0, 0 ) );
		if ( usePrepareMerge )
		{
			GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterPrepareMerge );
		}
		else
		{
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( (Float)mipIndex, 0, 0, 0 ) );
			GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderEnvProbeFilterMerge );
		}

		// Unbind textures
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
	}
}

void CRenderEnvProbeManager::FilterReflectionAtlas( const GpuApi::TextureRef &texReflectionAtlas, Int32 segmentIndex, Bool usePrepareMerge )
{	
	PC_SCOPE_RENDER_LVL1( FilterReflectionAtlas );
	
	const Uint32 typeResolution = GetEnvProbeResolution( ENVPROBETYPE_Reflection );
	const Uint32 numTypeMips = GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection );

	// Filter
	for ( Uint16 mip_i=1; mip_i<numTypeMips; ++mip_i )
	{
		DownscaleTempParaboloid( texReflectionAtlas, segmentIndex, mip_i, usePrepareMerge );
		BlurTempParaboloid( texReflectionAtlas, segmentIndex, mip_i, usePrepareMerge );
	}
}

Bool CRenderEnvProbeManager::UpdateEnvProbe( const CRenderCollector &mainRenderCollector, CRenderInterface &renderer, Float updateTimeDelta )
{
	PC_SCOPE_RENDER_LVL1( UpdateEnvProbe );

	CRenderSceneEx *scene = mainRenderCollector.m_scene;
	ASSERT( scene );

	if ( !m_pCurrGenProbe )
	{
		return false;
	}

	CRenderEnvProbe &envProbe = *m_pCurrGenProbe;
	
	if ( -1 == envProbe.GetDynamicData().m_arraySlotIndex )
	{
		return false;
	}

	SRenderEnvProbeDynamicData &envProbeData = envProbe.RefDynamicData();
	SCubeSlotData &envProbeSlot = RefCubeSlotData( envProbe.GetDynamicData().m_arraySlotIndex );
	const Uint32 baseResolution = GetEnvProbeDataSourceResolution();

	Int32 progressRefIndex = 0;
	Bool progressIncAllowed = true;

	// Create tiled deferred constants snaphots
	// This guarantees that dimmers/lights shared between faces will have the same parameters.
	{
		if ( progressRefIndex == m_currGenProgress )
		{
			PC_SCOPE_RENDER_LVL1( UpdateEnvProbe_BuildSnapshots );

			const Vector envProbeOrigin = envProbe.GetProbeGenOrigin();

			// Collect lights
			{
				{
					PC_SCOPE_RENDER_LVL1( UpdateEnvProbe_CollectLights );

					m_tempCollectedLightsArray.ClearFast();
					if ( m_currGenRenderFrameInfo.m_worldRenderSettings.m_enableEnvProbeLights )
					{
						scene->CollectEnvProbeLights( envProbeOrigin, Config::cvEnvProbeLightsMaxDistance.Get(), m_tempCollectedLightsArray );
					}
				}

				// Sort lights
				{
					PC_SCOPE_RENDER_LVL1( UpdateEnvProbe_SortLight );

					struct SortEnvProbeLights
					{
						RED_INLINE Bool operator()( const SCollectedEnvProbeLight& v1, const SCollectedEnvProbeLight& v2 )  const 
						{ 
							return v1.m_sortKey < v2.m_sortKey; 
						}
					};

					Sort( m_tempCollectedLightsArray.Begin(), m_tempCollectedLightsArray.End(), SortEnvProbeLights() );
				}

				// Copy to lights array
				{
					PC_SCOPE_RENDER_LVL1( UpdateEnvProbe_CopyLights );

					const Uint32 capacityMultiplier = 2; //< Get some more lights in case some of them would are faded out or for some other reason will be rejected bo ty constant buffer compiler
					const Uint32 numToCopy = Min<Uint32>( m_tempCollectedLightsArray.Size(), CRenderInterface::TILED_DEFERRED_LIGHTS_CAPACITY * capacityMultiplier );
					m_tempLightsArray.ResizeFast( numToCopy );
					for ( Uint32 i=0; i<numToCopy; ++i )
					{
						m_tempLightsArray[i] = m_tempCollectedLightsArray[i].m_light;
					}
				}
			}

			// Setup lights into the constant buffer
			{
				PC_SCOPE_RENDER_LVL1( UpdateEnvProbe_LightsConstants );

				GetRenderer()->CalculateTiledDeferredConstants_Lights( nullptr, envProbeOrigin, m_currGenRenderFrameInfo.m_worldRenderSettings, m_currGenRenderFrameInfo.m_envParametersArea.m_colorGroups, m_tempLightsArray.Size(), m_tempLightsArray.TypedData(), false, false, 0 );
			}

			//
			for ( Uint32 face_i=0; face_i<6; ++face_i )
			{
				// Prepare frameInfo
				CRenderCamera faceCamera;
				BuildEnvProbeCamera( faceCamera, envProbeOrigin, face_i );
				m_currGenRenderFrameInfo.AdaptEnvProbeFaceRenderInfo( faceCamera, baseResolution, baseResolution );
				
				// Setup constants. No flushing. Note that not all are set here. Some parameters are deferred for a better moment (before actual using this constants), if they are resource dependent (shadows etc).
				GetRenderer()->CalculateTiledDeferredConstants_Various( m_currGenRenderFrameInfo );
				GetRenderer()->CalculateTiledDeferredConstants_Dimmers( m_currGenRenderFrameInfo, 0, nullptr );

				// Grab the constants
				SGenPerFaceData &faceData = m_currGenPerFaceData[face_i];
				faceData.tiledConstantsSnaphot.ResizeFast( GetRenderer()->GetTileDeferredConstantsDataSize() );
				GetRenderer()->ExportTiledDeferredConstants( faceData.tiledConstantsSnaphot.Data() );
			}

			// Remove collected lights
			m_tempLightsArray.ClearFast();
		}

		// Don't inc here because snapshots creation is quite cheap, so it's 
		// not worth a whole frame to wait for following steps.
		//
		// ++progressRefIndex;
	}

	// Update faces loading
	{
		if ( progressRefIndex == m_currGenProgress )
		{
			PC_SCOPE_RENDER_LVL1( UpdateEnvProbe_LoadFaces );

			// Update texture load, cancel probe if failed
			if ( !UpdateCurrEnvProbeFaceTexturesLoading( true ) )
			{
				CancelCurrProbeUpdate();
				return false;
			}

			// Need to wait a little for the result
			if ( !envProbeData.m_isFaceTexturesInit )
			{
				return false; // needed to prevent stalls
			}

			// Don't inc here for purpose. so that we wouldn't wait a whole frame in case data is already in place.
			// with this commented out, algorithm will just smootly move forward to the next stuff.
			//
			// ++progressRefIndex;
			ASSERT( envProbeData.m_isFaceTexturesInit );
			ASSERT( envProbeData.m_isFaceTexturesUnpacked );
		}
	}

	// Render base atlas entry (basic scene)
	{
		const EEnvProbeType type_pass = ENVPROBETYPE_Reflection;

		// Render faces
		for ( Uint32 face_i=0; face_i<6; ++face_i )
		{
			if ( progressRefIndex == m_currGenProgress )
			{	
				PC_SCOPE_RENDER_LVL1( UpdateEnvProbe_RenderFace );

				// Render face color
				{
					CRenderCamera faceCamera;
					BuildEnvProbeCamera( faceCamera, envProbe.GetProbeGenOrigin(), face_i );
					m_currGenRenderFrameInfo.AdaptEnvProbeFaceRenderInfo( faceCamera, baseResolution, baseResolution );	// ace_ibl_fix: adapt process shouldn't be required here after I reorganize to the order of updates (faceByFace). I need to to this anyway because this way I'll reduce the number of temporary buffers.

					// Render face
					MeshDrawingStats meshDrawingStats;
					const void *tiledConstantsSnapshot = m_currGenPerFaceData[face_i].tiledConstantsSnaphot.Data();
					RenderEnvProbeFace( mainRenderCollector, meshDrawingStats, m_currGenRenderFrameInfo, scene, renderer, envProbe, envProbeSlot, face_i, m_tempFaceTexture, m_surfaceShadowMask, baseResolution, tiledConstantsSnapshot );

					// Render to intermediate cube
					{
						GpuApi::RenderTargetSetup rtSetup;
						rtSetup.SetColorTarget( 0, m_intermediateReflectionCube, GpuApi::CalculateCubemapSliceIndex( GpuApi::GetTextureDesc( m_intermediateReflectionCube ), 0, (Uint16)facesMapping[face_i], 0 ) );
						rtSetup.SetViewport( baseResolution, baseResolution );
						GpuApi::SetupRenderTargets( rtSetup );

						CopyTextureOneToOne( m_tempFaceTexture, 0, 0, facesFlipHorizontal[face_i], facesFlipVertical[face_i], facesRotation[face_i] );
					}
				}
			}
			++progressRefIndex;
		}

		// Unpack to paraboloid
		if ( progressRefIndex == m_currGenProgress )
		{
			PC_SCOPE_RENDER_LVL1( UpdateEnvProbe_UnpackParaboloid );

			const Uint32 cubeRowIndex = envProbeData.m_arraySlotIndex;
			ASSERT( cubeRowIndex < CUBE_ARRAY_CAPACITY );

			// Unpack first reflection mip
			CRenderCubeTexture *interiorFallbackCube = nullptr;
			if ( envProbe.GetProbeParams().m_genParams.m_isInteriorFallback )
			{
				interiorFallbackCube = m_currGenRenderFrameInfo.m_envParametersDayPoint.m_interiorFallbackReflectionTexture.Get<CRenderCubeTexture>();
			}
			UnpackIntermediateCube( interiorFallbackCube, m_intermediateReflectionCube, m_reflectionAtlasBaseCurrent, cubeRowIndex, false );

			// Copy to blend textured because this cubemap will start to blend in right now (don't want any delays for newly loaded cubes)
			if ( !envProbeData.m_isArraySlotInit )
			{
				CopyReflectionAtlas( m_reflectionAtlasBaseCurrent, m_reflectionAtlasBaseBlendSrc, cubeRowIndex, 1 );
				CopyReflectionAtlas( m_reflectionAtlasBaseCurrent, m_reflectionAtlasBaseBlendDest, cubeRowIndex, 1 );
				m_finalCompositionCounter = 0;
			}
		}
		++progressRefIndex;
	}

	// Proceed with cube gen progress counter
	if ( progressIncAllowed )
	{
		++m_currGenProgress;
	}

	// Mark as init if generation have finished
	if ( progressRefIndex == m_currGenProgress )
	{
		PC_SCOPE_RENDER_LVL1( UpdateEnvProbe_Finalize );

		ASSERT( m_lastUpdateTime >= 0 );
		ASSERT( m_lastUpdateTime >= envProbeData.m_lastUpdateTime );

		envProbeData.m_isArraySlotInit = true;
		envProbeData.m_lastFullUpdateStartUpdateTime = m_currGenStartUpdateTime;
		envProbeData.m_lastFullUpdateFinishUpdateTime = m_lastUpdateTime;

		// Update info in the slot
		//ASSERT( 0 == envProbeSlot.m_reflectionBaseUpdateCounter ); disabled because it may happen (and it's valid) in case of fastUpdate
		++envProbeSlot.m_reflectionBaseUpdateCounter;

		CancelCurrProbeUpdate();
	}

	return true;
}

void CRenderEnvProbeManager::SetupPrefetch( const Vector &position )
{
	if ( IsDuringPrefetch() )
	{
		CancelPrefetch();
	}
	else
	{
		CancelCurrProbeUpdate();
	}

	m_isDuringPrefetch			= true;
	//m_isPostPrefetch			= false;
	//m_needsPostPrefetchUpdate	= false;
	m_prefetchPosition			= position;

	RED_ASSERT( !m_pCurrGenProbe );
	RED_ASSERT( !m_currLoadingDataSource );
}

Bool CRenderEnvProbeManager::IsDuringPrefetch() const
{
	return m_isDuringPrefetch;
}

void CRenderEnvProbeManager::UpdatePrefetch()
{
	if ( !IsDuringPrefetch() )
	{
		return;
	}

	// Update slots (with fast selection option)
	UpdateSlots( m_prefetchPosition, 999.f, true );
	
	// Cancel probe update if not suitable
	if ( m_pCurrGenProbe && !ProbeQualifiesForPrefetch( *m_pCurrGenProbe ) )
	{
		CancelCurrProbeUpdate();
		RED_ASSERT( !m_pCurrGenProbe );
	}

	// Update probe loading
	if ( m_pCurrGenProbe )
	{
		if ( !UpdateCurrEnvProbeFaceTexturesLoading( false ) )
		{
			m_pCurrGenProbe->RefDynamicData().m_prefetchFailed = true;
		}

		RED_ASSERT( m_pCurrGenProbe );
		if ( m_pCurrGenProbe && m_pCurrGenProbe->GetDynamicData().m_isFaceTexturesInit && !m_pCurrGenProbe->GetDynamicData().m_isFaceTexturesUnpacked )
		{
			m_needsPostPrefetchUpdate = true;
		}
	}

	// Cancel probe update if faces init
	if ( m_pCurrGenProbe && m_pCurrGenProbe->GetDynamicData().m_isFaceTexturesInit )
	{
		CancelCurrProbeUpdate();
		RED_ASSERT( !m_pCurrGenProbe );
	}

	// Envprobes table
	Bool isProbesTableBuilt = false;
	SScopedEnvProbesTable probesTable ( false );

	// Choose probe to update
	if ( !m_pCurrGenProbe && !m_currLoadingDataSource )
	{
		if ( !isProbesTableBuilt )
		{
			isProbesTableBuilt = true;
			probesTable.Build();
		}

		for ( Uint32 i=0; i<CUBE_ARRAY_CAPACITY; ++i )
		{
			CRenderEnvProbe *probe = probesTable[i];
			if ( !probe || probe->GetDynamicData().m_isFaceTexturesInit || !ProbeQualifiesForPrefetch(*probe) )
			{
				continue;
			}

			m_pCurrGenProbe = probe;
			m_pCurrGenProbe->AddRef();
			break;
		}
	}

	// Finish prefetch if suitable
	if ( !m_pCurrGenProbe && !m_currLoadingDataSource )
	{
		if ( !isProbesTableBuilt )
		{
			isProbesTableBuilt = true;
			probesTable.Build();
		}

		Bool isPrefetchFinished = true;
		for ( Uint32 i=0; i<CUBE_ARRAY_CAPACITY; ++i )
		{
			CRenderEnvProbe *probe = probesTable[i];
			if ( probe && !(probe->RefDynamicData().m_isFaceTexturesInit || probe->RefDynamicData().m_prefetchFailed) )
			{
				isPrefetchFinished = false;
			}
		}

		if ( isPrefetchFinished )
		{
			m_isDuringPrefetch = false;
			m_isPostPrefetch = true;
		}
	}
}

void CRenderEnvProbeManager::CancelPrefetch()
{
	if ( !IsDuringPrefetch() )
	{
		return;
	}

	CancelCurrProbeUpdate();

	m_isDuringPrefetch = false;
}

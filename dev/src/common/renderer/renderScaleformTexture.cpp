/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#include "renderScaleformTexture.h"
#include "Render\Render_TextureUtil.h"
#include "../../common/engine/scaleformTextureCacheImage.h"
#include "../../common/engine/renderScaleformCommands.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"

#include "../engine/scaleformTextureCacheQuee.h"
#include "../engine/renderSettings.h"

extern CRenderScaleformTextureFormat::Mapping TextureFormatMapping[];

// For SF_AMP_SCOPE_TIMER
using SF::AmpFunctionTimer;
using SF::AmpServer;

// For SF_HEAP_AUTO_ALLOC
using SF::Stat_Default_Mem;

CRenderScaleformTexture::CRenderScaleformTexture(SF::Render::TextureManagerLocks* pmanagerLocks, const CRenderScaleformTextureFormat* pformat,	unsigned mipLevels, const SF::Render::ImageSize& size, unsigned use, SF::Render::ImageBase* pimage) 
	: SF::Render::Texture(pmanagerLocks, size, (SF::UByte)mipLevels, (SF::UInt16)use, pimage, pformat)
	, m_job( nullptr )
	, m_casheImage( nullptr )
	, m_streamLoaded( false )
	, m_bindTick( 0 )
{
	TextureCount = (SF::UByte) pformat->GetPlaneCount();
	if (TextureCount > 1)
	{
		m_textureArray = (HWTextureDesc*) SF_HEAP_AUTO_ALLOC(this, sizeof(HWTextureDesc) * TextureCount);
	}
	else
	{
		m_textureArray = &m_texture;
	}
	Red::System::MemorySet(m_textureArray, 0, sizeof(HWTextureDesc) * TextureCount);
}

CRenderScaleformTexture::CRenderScaleformTexture(SF::Render::TextureManagerLocks* pmanagerLocks, const GpuApi::TextureRef& texture, SF::Render::ImageSize size, SF::Render::ImageBase* image) 
	: SF::Render::Texture(pmanagerLocks, size, 0, 0, image, 0 )
	, m_job( nullptr )
	, m_casheImage( nullptr )
	, m_streamLoaded( false )
	, m_bindTick( 0 )
{
	TextureFlags |= TF_UserAlloc;
	ASSERT( texture );
	GpuApi::AddRef( texture );
	m_textureArray = &m_texture;
	m_textureArray[0].m_texture = texture;
	m_textureArray[0].m_size    = size;
}

CRenderScaleformTexture::~CRenderScaleformTexture()
{
	//  pImage must be null, since ImageLost had to be called externally.
	ASSERT(pImage == 0);

	SF::Mutex::Locker  lock(&pManagerLocks->TextureMutex);

	// Remove texture from streaming list in manager
	if( GetManager() )
	{
		GetManager()->RemoveStreamedTexture( this );
	}

	if( m_job )
	{
		m_job->Cancel();
		m_job = nullptr;
	}

	if ((State == State_Valid) || (State == State_Lost))
	{
		// pManagerLocks->pManager should still be valid for these states.
		ASSERT(pManagerLocks->pManager);
		RemoveNode();
		pNext = pPrev = 0;
		// If not on Render thread, add HW textures to queue.
		ReleaseHWTextures();
	}

	if ((m_textureArray != &m_texture) && m_textureArray)
	{
		SF_FREE(m_textureArray);
	}

	SAFE_RELEASE(m_casheImage);

}

void CRenderScaleformTexture::GetUVGenMatrix(SF::Render::Matrix2F* mat) const
{
	// UV Scaling rules are as follows:
	//  - If image got scaled, divide by original size.
	//  - If image is padded, divide by new texture size.
	const SF::Render::ImageSize& sz = (TextureFlags & TF_Rescale) ? ImgSize : m_textureArray[0].m_size;
	*mat = SF::Render::Matrix2F::Scaling(1.0f / (Float)sz.Width, 1.0f / (Float)sz.Height);
}

SFBool IsApiFormatRescaleCompatible( GpuApi::eTextureFormat format,	SF::Render::ImageFormat *ptargetImageFormat, SF::Render::ResizeImageType* presizeType)
{
	switch(format)
	{
	case GpuApi::TEXFMT_R8G8B8A8 /*DXGI_FORMAT_R8G8B8A8_UNORM*/:
		*ptargetImageFormat = SF::Render::Image_R8G8B8A8;
		*presizeType        = SF::Render::ResizeRgbaToRgba;
		return true;
// FIXME:/TODO
// 	case DXGI_FORMAT_B8G8R8A8_UNORM:
// 		*ptargetImageFormat = Image_R8G8B8A8;
// 		*presizeType        = ResizeRgbaToRgba;
// 		return true;
	case GpuApi::TEXFMT_A8 /*DXGI_FORMAT_A8_UNORM*/:
		*ptargetImageFormat = SF::Render::Image_A8;
		*presizeType        = SF::Render::ResizeGray;
		return true;
	default:
		break;
	}
	return false;
}

// FIXME:/TODO: Commented out formats have no mapping in the GpuApi, could just add 'em...
SFBool IsApiFormatMipGenCompatible( GpuApi::eTextureFormat format )
{
	switch(format)
	{
	case GpuApi::TEXFMT_A8 /*DXGI_FORMAT_A8_UNORM*/:
	case GpuApi::TEXFMT_Float_R10G10B10A2 /*DXGI_FORMAT_R10G10B10A2_UNORM*/:
//	case DXGI_FORMAT_R11G11B10_FLOAT:
	case GpuApi::TEXFMT_Float_R16 /*DXGI_FORMAT_R16_FLOAT*/:
//	case DXGI_FORMAT_R16_SNORM:
	case GpuApi::TEXFMT_Uint_16_norm /*DXGI_FORMAT_R16_UNORM*/:
	case GpuApi::TEXFMT_Float_R16G16 /*DXGI_FORMAT_R16G16_FLOAT*/:
//	case DXGI_FORMAT_R16G16_SNORM:
//	case DXGI_FORMAT_R16G16_UNORM:
	case GpuApi::TEXFMT_Float_R16G16B16A16 /*DXGI_FORMAT_R16G16B16A16_FLOAT*/:
	case GpuApi::TEXFMT_Float_R11G11B10 /*DXGI_FORMAT_R11G11B10_FLOAT*/:
//	case DXGI_FORMAT_R16G16B16A16_SNORM:
//	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case GpuApi::TEXFMT_Float_R32 /*DXGI_FORMAT_R32_FLOAT*/:
	case GpuApi::TEXFMT_Float_R32G32 /*DXGI_FORMAT_R32G32_FLOAT*/:
	case GpuApi::TEXFMT_Float_R32G32B32A32 /*DXGI_FORMAT_R32G32B32A32_FLOAT*/:
//	case DXGI_FORMAT_R8_SNORM:
	case GpuApi::TEXFMT_L8 /*DXGI_FORMAT_R8_UNORM*/:
	case GpuApi::TEXFMT_A8_Scaleform:
//	case DXGI_FORMAT_R8G8_SNORM:
	case GpuApi::TEXFMT_A8L8 /*DXGI_FORMAT_R8G8_UNORM*/:
//	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case GpuApi::TEXFMT_R8G8B8A8 /*DXGI_FORMAT_R8G8B8A8_UNORM*/:
//	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return true;
	default:
		return false;
	}
}

#if WITH_SCALEFORM_VIDEO
namespace Scaleform { namespace GFx { namespace Video {
	extern unsigned char GClearYUV[3];
} } }
static Uint8* GVideoClearYUV = GFx::Video::GClearYUV;
#else
static Uint8 GVideoClearYUV[3] = { 0x10, 0x80, 0x80 }; // black
#endif // WITH_SCALEFORM_VIDEO

static Color GVideoClearRGB = Color::BLACK;

namespace ScaleformVideoHelpers
{
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb530104%28v=vs.85%29.aspx
	void SetVideoClearRGB( const Color& rgb )
	{
		const Float Rf = (Float)rgb.R/255;
		const Float Gf = (Float)rgb.G/255;
		const Float Bf = (Float)rgb.B/255;

		const Float Yp = 0.299f * Rf + 0.587f * Gf + 0.114f * Bf;
		const Float Pb = (0.5f / (1.f - 0.114f)) * ( Bf - Yp );
		const Float Pr = (0.5f / (1.f - 0.299f)) * (Rf - Yp );

		const Uint8 Yp8 = (Uint8)(16 + 219 * Yp);
		const Uint8 Cb8 = (Uint8)(128 + 224 * Pb);
		const Uint8 Cr8 = (Uint8)(128 + 224 * Pr);

		GVideoClearYUV[0] = Clamp<Uint8>( Yp8, 0x10, 0xEB );
		GVideoClearYUV[1] = Clamp<Uint8>( Cb8, 0x10, 0xF0 );
		GVideoClearYUV[2] = Clamp<Uint8>( Cr8, 0x10, 0xF0 );
		GVideoClearRGB = rgb;
	}

	const Uint8* GetVideoClearYUV()
	{
		return GVideoClearYUV;
	}

	const Color& GetVideoClearRGB()
	{
		return GVideoClearRGB;
	}
}

// These are helper functions for clearing scaleform video textures (which is a hack)
namespace ScaleformVideoHelpers
{
	RED_INLINE GpuApi::TextureLevelInitData* CreateLevelInitData( Uint32 dataSize )
	{
		ASSERT( dataSize > 0 );
		GpuApi::TextureLevelInitData* levelInitData = new GpuApi::TextureLevelInitData();		// only one mip level
		Uint8* clearedMemory = (Uint8*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, dataSize );	// The pixel format is A8, so dataSize (width*height) is ok here

		levelInitData->m_data = clearedMemory;
		levelInitData->m_isCooked = false;

		return levelInitData;
	}

	RED_INLINE void ClearDataWith( Uint8* data, Uint32 dataSize, Uint8 value )
	{
		ASSERT( dataSize > 0 && data != nullptr );

		Red::System::MemorySet( (void*)data, (Int32)value, (size_t)dataSize );
	}

	RED_INLINE GpuApi::TextureInitData CreateInitData( Uint32 width, Uint32 height, Uint8 clearValue )
	{
		ASSERT( width > 0 && height > 0 );

		Uint32 size = width * height;
		GpuApi::TextureLevelInitData* levelInitData = CreateLevelInitData( size );
		ClearDataWith( (Uint8*)levelInitData->m_data, size, clearValue );

		GpuApi::TextureInitData initData;
		initData.m_isCooked = false;
		initData.m_mipsInitData = levelInitData;

		return initData;
	}

	RED_INLINE void ReleaseInitDataComponents( GpuApi::TextureInitData& initData )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, initData.m_mipsInitData->m_data );
		delete initData.m_mipsInitData;
	}
}

SFBool CRenderScaleformTexture::Initialize()
{    
#ifndef RED_FINAL_BUILD
	SF_AMP_SCOPE_TIMER(GetManager()->RenderThreadId == SF::GetCurrentThreadId() ? SF::AmpServer::GetInstance().GetDisplayStats() : NULL, __FUNCTION__, SF::Amp_Profile_Level_Medium);
#endif

	if ( TextureFlags & TF_UserAlloc )
	{
		return Initialize( m_textureArray[0].m_texture );
	}

	Bool resize  = false;
	SF::Render::ImageFormat format = GetImageFormat();

	Uint32 itex;

	// Determine sizes of textures.
	if (State != State_Lost)
	{        
		for (itex = 0; itex < TextureCount; itex++)
		{
			HWTextureDesc& tdesc = m_textureArray[itex];
			tdesc.m_size = SF::Render::ImageData::GetFormatPlaneSize(format, ImgSize, itex);
		}

		if (resize && (Use & SF::Render::ImageUse_Wrap))
		{
			if (SF::Render::ImageData::IsFormatCompressed(format))
			{
				HALT( "CreateTexture failed - Can't rescale compressed Wrappable image to Pow2");
				State = State_InitFailed;
				return false;
			}
			TextureFlags |= TF_Rescale;
		}
	}

	// Determine how many mipLevels we should have and whether we can
	// auto-generate them or not.
	unsigned allocMipLevels = MipLevels;

	if (Use & SF::Render::ImageUse_GenMipmaps)
	{
		// FIXME: IsD3DFormatMipGenCompatible() for GpuApi...
		HALT(  "Currently disabling prerasterized font mipmapping gen" );
#if 0
		ASSERT(MipLevels == 1);
		if (IsD3DFormatMipGenCompatible(GetTextureFormat()->GetD3DFormat()))
		{            
			TextureFlags |= TF_SWMipGen;
			// If using SW MipGen, determine how many mip-levels we should have.
			allocMipLevels = 31;
			for (itex = 0; itex < TextureCount; itex++)
				allocMipLevels = Alg::Min(allocMipLevels,
				ImageSize_MipLevelCount(pTextures[itex].Size));
			MipLevels = (UByte)allocMipLevels;
		}
		else
#endif // #if 0
		{
			allocMipLevels = 0;
		}
	}

	// Only use Dynamic textures for updatable/mappable textures.
	// Also, since Dynamic textures can be lost, don't allow them if ImageUse_InitOnly
	// is not specified.
	Bool allowDynamicTexture = ((Use & SF::Render::ImageUse_InitOnly) != 0) && ((Use & (SF::Render::ImageUse_PartialUpdate | SF::Render::ImageUse_Map_Mask)) != 0);
	Bool renderTarget = (Use & SF::Render::ImageUse_RenderTarget) != 0;

	Uint32 usage  = 0; //D3D1x(USAGE_DEFAULT);

	usage |= GpuApi::TEXUSAGE_Samplable; //UINT cpu            = 0; UINT bindFlags      = D3D1x(BIND_SHADER_RESOURCE);

	if (allowDynamicTexture)
	{
		usage |= GpuApi::TEXUSAGE_Dynamic; //D3D1x(USAGE_DYNAMIC); cpu   |= D3D1x(CPU_ACCESS_WRITE);
	}
	if (renderTarget) // NOTE: GpuApi will also bind with D3D11_BIND_UNORDERED_ACCESS because of msaaLevel
	{
		usage |= GpuApi::TEXUSAGE_RenderTarget; // bindFlags |= D3D1x(BIND_RENDER_TARGET);
	}
		
	GpuApi::eTextureFormat textureFormat = GetTextureFormat()->GetApiTextureFormat();

	Bool isVideoTexture = ((textureFormat == GpuApi::TEXFMT_A8_Scaleform) && (TextureCount == 3));

	const Uint8* const yuv = ScaleformVideoHelpers::GetVideoClearYUV();

	// Create textures
	for (itex = 0; itex < TextureCount; itex++)
	{
		HWTextureDesc& tdesc = m_textureArray[itex];

		GpuApi::TextureDesc desc;
		desc.type = GpuApi::TEXTYPE_2D;
		desc.format = textureFormat;
		desc.width = tdesc.m_size.Width;
		desc.height = tdesc.m_size.Height;
		desc.usage = usage;
		desc.initLevels = MipLevels;

#ifndef RED_FINAL_BUILD
		static_cast<CRenderScaleformTextureManager*>( GetManager() )->IncreaseMemoryCount( GpuApi::CalcTextureSize( desc ) );
#endif

		// HACK - if textures are video textures (we detect that based on format == TEXFMT_A8_Scaleform and TextureCount == 3)
		// then we have to clear them with black color in YUV color space, otherwise first video's frame might appear corrupted

		if( isVideoTexture == false )		// If not video texture format, then create without initializing with init data
		{
			tdesc.m_texture = GpuApi::CreateTexture( desc, GpuApi::TEXG_UI );
			ASSERT( tdesc.m_texture );
			if ( ! tdesc.m_texture )
			{
				// Texture creation failed, release all textures and fail.
				ReleaseHWTextures();
				if (State != State_Lost)
				{
					State = State_InitFailed;
				}
				return false;
			}
		}
		else			// If has video format, initialize with black color in YUV color space
		{
			// Clear scaleform video textures (luma and both chromas), TEXFMT_A8_Scaleform is used for them
			GpuApi::TextureInitData initData;

			// First texture in array is luma texture, second and third are chroma textures. Luma and chromas can have different sizes - it depends on video format.
			RED_FATAL_ASSERT( itex < 3, "Itex %u>= 3", itex );
			const Uint8 clearValue = yuv[itex];
			initData = ScaleformVideoHelpers::CreateInitData( desc.width, desc.height, clearValue );

			tdesc.m_texture = GpuApi::CreateTexture( desc, GpuApi::TEXG_UI, &initData );
			ASSERT( tdesc.m_texture );

			ScaleformVideoHelpers::ReleaseInitDataComponents( initData );

			if ( ! tdesc.m_texture )
			{
				// Texture creation failed, release all textures and fail.
				ReleaseHWTextures();
				if (State != State_Lost)
				{
					State = State_InitFailed;
				}
				return false;
			}
		}

		GpuApi::SetTextureDebugPath( tdesc.m_texture, "SF texture" );

		// NOTE: SF ref just blindly creates creates a CreateShaderResourceView in the hope that the right bind setting was there.
	}

	// Upload image content to texture, if any.
	if (pImage && !SF::Render::Texture::Update())
	{
		HALT( "CreateTexture failed - couldn't initialize texture" );
		ReleaseHWTextures();
		if (State != State_Lost)
		{
			State = State_InitFailed;
		}
		return false;
	}

	State = State_Valid;
	return SF::Render::Texture::Initialize();
}

SFBool CRenderScaleformTexture::Initialize( GpuApi::TextureRef& texture)
{
	if ( texture.isNull() )
		return false;

	CRenderScaleformTextureManager* pmanager = GetManager();

	// If this is called explicitly (ie. not after the constructor), and the texture is being replaced,
	// A reference needs to be added to the new texture.
	if ( m_textureArray[0].m_texture != texture )
	{
		ReleaseHWTextures();

		m_textureArray[0].m_texture = texture;
		GpuApi::AddRef( m_textureArray[0].m_texture );
	}

	GpuApi::TextureDesc texDesc;
	GpuApi::GetTextureDesc( texture, texDesc );

	MipLevels = (SF::UByte)texDesc.initLevels;

	pFormat = 0;

	// If an image is provided, try to obtain the texture format from the image.
	if ( pImage )
	{
		pFormat = (CRenderScaleformTextureFormat*)pmanager->getTextureFormat(pImage->GetFormatNoConv());
	}

	// Otherwise, figure out the texture format, based on the mapping table.    
	if ( pFormat == 0 )
	{
		CRenderScaleformTextureFormat::Mapping* pmapping;
		for (pmapping = TextureFormatMapping; pmapping->m_format != SF::Render::Image_None; pmapping++)
		{
			if ( pmapping->m_textureFormat == texDesc.format )
			{
				pFormat = (CRenderScaleformTextureFormat*)pmanager->getTextureFormat(pmapping->m_format);
				break;
			}
		}
	}

	// Could not determine the format.
	if ( !pFormat)
	{
		HALT( "Texture::Initialize failed - couldn't determine ImageFormat of user supplied texture." );
		State = State_InitFailed;
		return false;
	}
	
	// Fill out the HW description.
	if( m_textureArray[0].m_size == SF::Render::ImageSize(0) )
	{
		m_textureArray[0].m_size.SetSize(texDesc.width, texDesc.height);
	}

	// Override the image size if it was not provided.
	if ( ImgSize == SF::Render::ImageSize(0) )
	{
		ImgSize = m_textureArray[0].m_size;
	}

	State = State_Valid;
	return SF::Render::Texture::Initialize();
}

void CRenderScaleformTexture::computeUpdateConvertRescaleFlags( SFBool rescale, SFBool swMipGen, SF::Render::ImageFormat inputFormat, SF::Render::ImageRescaleType &rescaleType, SF::Render::ImageFormat &rescaleBuffFromat, SFBool &convert )
{
	SF_UNUSED(inputFormat);
	if (rescale && !IsApiFormatRescaleCompatible(GetTextureFormat()->GetApiTextureFormat(),&rescaleBuffFromat, &rescaleType))
	{
		convert = true;
	}
	
	if (swMipGen && !IsApiFormatMipGenCompatible(GetTextureFormat()->GetApiTextureFormat()))
	{
		convert = true;
	}
}


void CRenderScaleformTexture::ReleaseHWTextures(SFBool staging)
{
	SF::Render::Texture::ReleaseHWTextures(staging);

	CRenderScaleformTextureManager* pmanager = GetManager();
	
	if( pmanager->GetStreamingQueue() && m_job )
	{
		pmanager->GetStreamingQueue()->FinishJob( m_job );
		m_job = nullptr;
	}

	Bool useKillList = (pmanager->RenderThreadId == 0 || SF::GetCurrentThreadId() != pmanager->RenderThreadId);

	for ( Int32 itex = TextureCount-1 ; itex >= 0 ; --itex )
	{
		GpuApi::TextureRef& texture = m_textureArray[itex].m_texture;
		GpuApi::TextureRef stagingTexture = staging ? m_textureArray[itex].m_stagingTexture : GpuApi::TextureRef::Null();

		if ( texture )
		{

#ifndef RED_FINAL_BUILD
			// If its streamed, decrease streaming memory
			if( m_streamLoaded )
			{
				pmanager->GetStreamingQueue()->DecreaseMemoryCount( GpuApi::CalcTextureSize( GpuApi::GetTextureDesc(texture) ) );
			}
			// If its unstreamed and it was nothing to do with streaming, decrease manager memory
			else if( !pmanager->GetStreamingQueue()->IsTextureOwned( texture ) )
			{
				pmanager->DecreaseMemoryCount( GpuApi::CalcTextureSize( GpuApi::GetTextureDesc(texture) ) );
			}
			if( stagingTexture )
			{
				pmanager->DecreaseMemoryCount( GpuApi::CalcTextureSize( GpuApi::GetTextureDesc(stagingTexture) ) );
			}
#endif

			if (useKillList)
			{
				pmanager->m_D3DTextureKillList.PushBack(texture);

				if ( stagingTexture )
				{
					pmanager->m_D3DTextureKillList.PushBack( stagingTexture );
				}
			}
			else
			{
				GpuApi::Release( texture );

				GpuApi::SafeRelease( stagingTexture );
			}
		}

		m_textureArray[itex].m_texture = GpuApi::TextureRef::Null();

		if ( staging )
		{
			m_textureArray[itex].m_stagingTexture = GpuApi::TextureRef::Null();
		}
	}

	m_streamLoaded = false;
}

void	CRenderScaleformTexture::Unstream()
{
	// Remove texture from streaming list in manager, bcoz it might be readded again somewhere in the future
	// Use no lock version - we are under mutex arleady
	GetManager()->RemoveStreamedTextureNoLock( this );

	if( !m_streamLoaded )
	{
		return;
	}

	ReleaseHWTextures();

	CRenderScaleformTextureManager* pmanager = GetManager();
	CScaleformTextureCacheQueue* queue = pmanager->GetStreamingQueue();

	// Mark texture as unstreamed
	m_texture.m_texture = queue->GetUndefinedTexture();
	GpuApi::AddRef( m_texture.m_texture );

#ifndef RED_FINAL_BUILD
	queue->LogCurrentMemory();
#endif

	m_streamLoaded = false;
	m_bindTick = 0;
}


void	CRenderScaleformTexture::ProcessStreamingFinish()
{

	if( m_streamLoaded )
	{
		return;
	}

	CRenderScaleformTextureManager* pmanager = GetManager();
	CScaleformTextureCacheQueue* queue = pmanager->GetStreamingQueue();

	if( !queue )
	{
		return;
	}

	// If we have a job and it isn't started yet, try starting it now. This is just in case a texture couldn't start when the
	// job was created, is not used for rendering after that, but isn't destroyed either, to prevent the job from sticking
	// around forever.
	if ( m_job && m_job->GetStatus() == ESJS_Uninitialized )
	{
		ProcessStreaming();
	}


	for ( Int32 i = TextureCount-1; i >= 0 ; --i )
	{
		GpuApi::TextureRef& texture = m_textureArray[i].m_texture;

		if( queue->IsTexturePending( texture ) && m_job )
		{
			if( m_job->Check() )
			{
				// Release pending texture
				GpuApi::Release( texture );

				GpuApi::TextureRef tex = m_job->CreateTexture();

				queue->FinishJob( m_job );
				m_job = nullptr;

				if( tex )
				{
					// Initialize( tex );
					texture = tex;
					m_streamLoaded = true;
				}
				else
				{
					texture = GpuApi::TextureRef::Null();
				}

			}

		}

	}

}

void	CRenderScaleformTexture::ProcessStreaming()
{
	// If the texture is fully loaded, or has a job that is in progress, nothing to do.
	if( m_streamLoaded || ( m_job && m_job->GetStatus() != ESJS_Uninitialized ) )
	{
		return;
	}

	CRenderScaleformTextureManager* pmanager = GetManager();
	CScaleformTextureCacheQueue* queue = pmanager->GetStreamingQueue();
	
	if( !queue )
	{
		return;
	}

	for ( Int32 i = TextureCount-1; i >= 0 ; --i )
	{
		GpuApi::TextureRef& texture = m_textureArray[i].m_texture;

		if( queue->IsTextureUndefined( texture ) )
		{
			RED_ASSERT( m_casheImage )
			CScaleformTextureCacheJob* job = m_job ? m_job : queue->GetJobForTexture( m_casheImage );

			if( job == nullptr )
			{
				RED_HALT( "Failed to get/create a streaming job" );
				return;
			}

			// If there is a job and its yet not initialized (it her first time darling)
			if( job->GetStatus() == ESJS_Uninitialized )
			{
				if( job->Start() == ESJS_Processing )
				{
					// Switch to the "pending" placeholder
					GpuApi::SafeRefCountAssign( texture, queue->GetPendingTexture() );
				}
			}

			// If this is a new job for us, register with the texture manager.
			if( m_job == nullptr )
			{
				pmanager->AddStreamedTexture( this );
			}

			// Store the job whether it started or not. We'll keep trying here until the state changes from Uninitialized.
			// If we end up destroyed, before even starting the job, it'll still get cleaned up.
			m_job = job;
		}

	}

}

// Applies a texture to device starting at pstageIndex, advances index
void    CRenderScaleformTexture::ApplyTexture( SFUInt stageIndex, const SF::Render::ImageFillMode& fm )
{
	SF::Render::Texture::ApplyTexture(stageIndex, fm);

	m_bindTick = GetManager()->GetCurrentTime();

	// If there is job attached it means it is not fully loaded
	ProcessStreaming();

	GpuApi::TextureRef textures[ MaxTextureCount ];

	for ( Int32 i = TextureCount-1; i >= 0 ; --i )
	{
		textures[i] = m_textureArray[i].m_texture;
	}

	{
		CRenderScaleformTextureManager* pmanager = GetManager();

		CScaleformTextureCacheQueue* queue = pmanager->GetStreamingQueue();

		GpuApi::eSamplerStatePreset sampler = pmanager->m_samplerPresets[fm.Fill];

		// Assumption: all textures in a stage use the same sampler.
		pmanager->SetSamplerState(stageIndex, TextureCount, textures, sampler);
	}
}

SFBool    CRenderScaleformTexture::Update(const UpdateDesc* updates, SFUInt count, SFUInt mipLevel)
{
	SF_AMP_SCOPE_RENDER_TIMER(__FUNCTION__, SF::Amp_Profile_Level_Medium);
	if (!GetManager()->mapTexture(this, mipLevel, 1))
	{
		HALT( "Texture::Update failed - couldn't map texture" );
		return false;
	}

	SF::Render::ImageFormat format = GetImageFormat(); 
	SF::Render::ImagePlane  dplane;

	for (Uint32 i = 0; i < count; i++)
	{
		const UpdateDesc &desc = updates[i];
		SF::Render::ImagePlane   splane(desc.SourcePlane);

		pMap->Data.GetPlane(desc.PlaneIndex, &dplane);
		dplane.pData += desc.DestRect.y1 * dplane.Pitch + desc.DestRect.x1 * GetTextureFormat()->m_mapping->m_bytesPerPixel;

		splane.SetSize(desc.DestRect.GetSize());
		dplane.SetSize(desc.DestRect.GetSize());
		ConvertImagePlane(dplane, splane, format, desc.PlaneIndex, pFormat->GetScanlineCopyFn(), 0);
	}

	GetManager()->unmapTexture(this);
	return true;
}

void CRenderScaleformTexture::ForceUpdateBindTime()
{
	m_bindTick = GetManager()->GetCurrentTime();
}

// ***** DepthStencilSurface

CRenderScaleformDepthStencilSurface::CRenderScaleformDepthStencilSurface(SF::Render::TextureManagerLocks* pmanagerLocks, const SF::Render::ImageSize& size) 
	: SF::Render::DepthStencilSurface(pmanagerLocks, size)
{
}

CRenderScaleformDepthStencilSurface::~CRenderScaleformDepthStencilSurface()
{
	if ( m_depthStencilTexture )
	{
#ifndef RED_FINAL_BUILD
		static_cast<CRenderScaleformTextureManager*>( GetTextureManager() )->DecreaseMemoryCount( GpuApi::CalcTextureSize( GpuApi::GetTextureDesc( m_depthStencilTexture ) ) );
#endif

		GpuApi::Release( m_depthStencilTexture );
	}
}

SFBool CRenderScaleformDepthStencilSurface::Initialize()
{
	ASSERT ( m_depthStencilTexture.isNull() );

	GpuApi::TextureDesc desc;
	desc.type = GpuApi::TEXTYPE_2D;
	desc.format = GpuApi::TEXFMT_D24S8;
	desc.width = Size.Width;
	desc.height = Size.Height;
	desc.usage = GpuApi::TEXUSAGE_DepthStencil;
	desc.initLevels = 1;
	
#ifndef RED_FINAL_BUILD
	static_cast<CRenderScaleformTextureManager*>( GetTextureManager() )->IncreaseMemoryCount( GpuApi::CalcTextureSize( desc ) );
#endif

	m_depthStencilTexture = GpuApi::CreateTexture( desc, GpuApi::TEXG_UI );
	if ( ! m_depthStencilTexture )
	{
		State = CRenderScaleformTexture::State_InitFailed;
	}
	else
	{
		State = CRenderScaleformTexture::State_Valid;
		GpuApi::SetTextureDebugPath( m_depthStencilTexture, "SF depthstencil" );
	}
		
	return State == CRenderScaleformTexture::State_Valid;
}

// ***** MappedTexture

SFBool CRenderScaleformMappedTexture::Map(SF::Render::Texture* ptexture, SFUInt mipLevel, SFUInt levelCount)
{
	ASSERT(!IsMapped());
	ASSERT((mipLevel + levelCount) <= ptexture->MipLevels);

	// Initialize Data as efficiently as possible.
	if (levelCount <= PlaneReserveSize)
	{
		Data.Initialize(ptexture->GetImageFormat(), levelCount, Planes, ptexture->GetPlaneCount(), true);
	}
	else if (!Data.Initialize(ptexture->GetImageFormat(), levelCount, true))
	{
		return false;
	}

	CRenderScaleformTexture* apiTexture	= reinterpret_cast<CRenderScaleformTexture*>(ptexture);

	pTexture                            = ptexture;
	StartMipLevel                       = mipLevel;
	LevelCount                          = levelCount;

	Uint32 textureCount = ptexture->TextureCount;

	for (Uint32 itex = 0; itex < textureCount; itex++)
	{
		CRenderScaleformTexture::HWTextureDesc &tdesc = apiTexture->m_textureArray[itex];
		SF::Render::ImagePlane              plane(tdesc.m_size, 0);

		for(Uint32 i = 0; i < StartMipLevel; i++)
		{
			plane.SetNextMipSize();
		}

		const Uint32 NUM_VIDEO_PLANES = 3;
		const Bool isStagingTexture = textureCount != NUM_VIDEO_PLANES;

		if (! tdesc.m_stagingTexture )
		{
			GpuApi::TextureDesc inputDesc;
			GpuApi::GetTextureDesc( tdesc.m_texture, inputDesc );
			GpuApi::TextureDesc stagingDesc( inputDesc );
			stagingDesc.usage = isStagingTexture ? GpuApi::TEXUSAGE_StagingWrite : GpuApi::TEXUSAGE_Dynamic|GpuApi::TEXUSAGE_Samplable;
			tdesc.m_stagingTexture = GpuApi::CreateTexture( stagingDesc, GpuApi::TEXG_UI );
			if ( tdesc.m_stagingTexture )
			{
				GpuApi::SetTextureDebugPath( tdesc.m_stagingTexture, "SF staging/dynamic" );
			}
			else
			{
				return false;
			}

#ifndef RED_FINAL_BUILD
			static_cast<CRenderScaleformTextureManager*>( ptexture->GetTextureManager() )->IncreaseMemoryCount( GpuApi::CalcTextureSize( stagingDesc ) );
#endif
		}

		// The only case where we need to copy the GPU resource to the CPU is in the case of RenderTargets.
		// RenderTargets (presumably) will be updated the by the GPU, and therefore the CPU needs the updated
		// contents. Things that will only be updated by the CPU (eg. GlyphCache) will always have the most
		// updated results in the staging texture, and therefore do not need the copyback. Note that on feature
		// levels lower than 10_0, CopyResource GPU->CPU is not supported, so give a warning in this case and skip it.
		if ((ptexture->Use & (SF::Render::ImageUse_MapRenderThread|SF::Render::ImageUse_RenderTarget)) == (SF::Render::ImageUse_MapRenderThread|SF::Render::ImageUse_RenderTarget))
		{
			//static bool checkHandleCopyResource = false;
			static Bool canHandleCopyResource = true;

			if(!canHandleCopyResource)
			{
				WARN_RENDERER( TXT( "Attempt to Map a texture with ImageUse_RenderTarget, but device feature level ") 
								TXT( "does not support CopyResource, skipping copy (GPU operations may be invalidated).") );
			}

			if (canHandleCopyResource)
			{
				GetRenderer()->CopyTextureData( tdesc.m_stagingTexture, 0, 0, tdesc.m_texture, 0, 0 );
			}
		}

		for (Uint32 level = 0; level < levelCount; level++)
		{
			//D3D1x(MAPPED_TEXTURE2D) mappedResource; = D3D11_MAPPED_SUBRESOURCE

			Uint32 outPitch = 0;
			const Uint32 lockFlags = isStagingTexture ? GpuApi::BLF_Write : GpuApi::BLF_Discard;
			void* pMappedData = GpuApi::LockLevel( tdesc.m_stagingTexture, level, 0, lockFlags, outPitch );

			if ( pMappedData )
			{
				plane.Pitch    = outPitch;
				plane.pData    = static_cast< SF::UByte* >( pMappedData );
				plane.DataSize = SF::Render::ImageData::GetMipLevelSize(Data.GetFormat(), plane.GetSize(), itex); 
			}
			else
			{
				plane.Pitch    = 0;
				plane.pData    = 0;
				plane.DataSize = 0;
				return false;
			}

			Data.SetPlane(level * textureCount + itex, plane);
			// Prepare for next level.
			plane.SetNextMipSize();
		}
	}

	pTexture->pMap = this;
	return true;
}

void CRenderScaleformMappedTexture::Unmap(SFBool applyUpdate)
{
	CRenderScaleformTexture* apiTexture                         = reinterpret_cast<CRenderScaleformTexture*>(pTexture);

	Uint32 textureCount = pTexture->TextureCount;
	for (Uint32 itex = 0; itex < textureCount; itex++)
	{
		CRenderScaleformTexture::HWTextureDesc &tdesc = apiTexture->m_textureArray[itex];
		SF::Render::ImagePlane plane;

		// Unmap the entire resource first.
		for (Int32 level = 0; level < LevelCount; level++)
		{
			GpuApi::UnlockLevel( tdesc.m_stagingTexture, level, 0 );
		}

		if (applyUpdate)
		{
			for (Int32 level = 0; level < LevelCount; level++)
			{
				Data.GetPlane(level * textureCount + itex, &plane);
				if (plane.pData)
				{
					GetRenderer()->CopyTextureData( tdesc.m_texture, level + StartMipLevel, 0, tdesc.m_stagingTexture, level, 0 );
					plane.pData = 0;
				}
			}
		}
	}

	pTexture->pMap = 0;
	pTexture       = 0;
	StartMipLevel  = 0;
	LevelCount     = 0;
}

namespace // anonymous
{
	// SF::Render::ImageFillMode(WrapMode wrap, SampleMode sample) : Fill ((UByte)(wrap|sample)) { }
	SF::UByte GetImageFillMode( SF::Render::WrapMode wrap, SF::Render::SampleMode sample )
	{
		ASSERT( (SF::UByte)(wrap|sample) < 4 /*=CRenderScaleformTextureManager::SamplerTypeCount*/ );
		return (SF::UByte)(wrap|sample);
	}
}

// ***** TextureManager
CRenderScaleformTextureManager::CRenderScaleformTextureManager(	SF::ThreadId renderThreadId, SF::Render::ThreadCommandQueue* commandQueue, SF::Render::TextureCache* texCache) 
	: SF::Render::TextureManager(renderThreadId, commandQueue, texCache)
	, m_textureCacheQuee( nullptr )
	, m_currentTime( 0.0f )
	, m_textureUnloadDelay( Config::cvScaleformTextureUnstreamDelay.Get() )
	, m_pinTextures( false )
{
	initTextureFormats();

	ASSERT( SF::Render::Wrap_Count == 2 );
	ASSERT( SF::Render::Sample_Count == 2 );

	// FIXME:/TODO: Or just fill in directly without the stupid SF computation (init from an in-order array)
	m_samplerPresets[ GetImageFillMode( SF::Render::Wrap_Repeat, SF::Render::Sample_Point ) ] = GpuApi::SAMPSTATEPRESET_Scaleform_WrapPointMip;
	m_samplerPresets[ GetImageFillMode( SF::Render::Wrap_Repeat, SF::Render::Sample_Linear ) ] = GpuApi::SAMPSTATEPRESET_Scaleform_WrapLinearMip;
	m_samplerPresets[ GetImageFillMode( SF::Render::Wrap_Clamp, SF::Render::Sample_Point ) ] = GpuApi::SAMPSTATEPRESET_Scaleform_ClampPointMip;
	m_samplerPresets[ GetImageFillMode( SF::Render::Wrap_Clamp, SF::Render::Sample_Linear ) ] = GpuApi::SAMPSTATEPRESET_Scaleform_ClampLinearMip;

	m_textureCacheQuee = new CScaleformTextureCacheQueue();

	// If we are in render thread, create textures now, if not, push it to command
	if( renderThreadId == SF::GetCurrentThreadId() )
	{
		m_textureCacheQuee->CreatePlaceholderTexture();
	}
	else
	{
		CGuiRenderCommand<CGuiRenderCommand_CreateStreamingPlaceholderTextures>::Send( m_textureCacheQuee );
	}

}

CRenderScaleformTextureManager::~CRenderScaleformTextureManager()
{   
	SF::Mutex::Locker lock(&pLocks->TextureMutex);
	Reset();
	pLocks->pManager = 0;

	SAFE_RELEASE( m_textureCacheQuee );
	
	// ctremblay HACK It this point, for some unknown reason the heap page associated with the Image in this Queue has already been freed! 
	// For just bypass the queue entry destruction. Game is existing, so this issue do not have in-game impact.
	ImageUpdates.Queue.Clear(); 
}

void CRenderScaleformTextureManager::Reset()
{
	SF::Mutex::Locker lock(&pLocks->TextureMutex);

	m_streamedTextures.ClearFast();
	m_texturesToStream.ClearFast();
	
	// InitTextureQueue MUST be empty, or there was a thread
	// service problem.
	ASSERT(TextureInitQueue.IsEmpty());

	// Notify all textures
	while (!Textures.IsEmpty())
	{
		Textures.GetFirst()->LoseManager();
	}

	processTextureKillList();
}

void CRenderScaleformTextureManager::SetSamplerState( SFUInt stage, SFUInt textureCount, GpuApi::TextureRef* textures, GpuApi::eSamplerStatePreset state /*=GpuApi::SAMPSTATEPRESET_Max*/ )
{
	Bool loadSamplers = false;
	Bool loadTextures = false;
	for ( Uint32 i = 0; i < textureCount; ++i)
	{
		if ( m_currentSamplers[i+stage] != state )
		{
			loadSamplers = true;
		}

		if ( m_currentTextureRefIds[i+stage] != textures[i] )
		{
			loadTextures = true;
		}
	}

	// GpuApi::SAMPSTATEPRESET_Max is used as "don't care". In the SF impl, they clear the sampler states with null instead,
	// but we'll leave that up the the render state manager after scaleform is done.
	
	if ( loadSamplers ) // GpuApi has its own cache, but just set like this anyway for now. We cleared the sampler states at the start of the scene.
	{
		if ( state != GpuApi::SAMPSTATEPRESET_Max )
		{
			GpuApi::SetSamplerStateCommon( stage, textureCount, state, GpuApi::PixelShader );
		}
		else
		{
			// Scaleform reference impl would clear the states here
			//GpuApi::InvalidateSamplerStatesCommon( stage, textureCount, GpuApi::PixelShader );
		}
		Red::System::MemorySet( &m_currentSamplers[ stage ], state, sizeof(state) * textureCount );
	}	

	if ( loadTextures )
	{
		// NOTE: Scaleform passes in null textures to clear them, but out GpuApi would instead
		// bind a "default" texture to show its missing.
		//GpuApi::BindSamplers( stage, textureCount, textures, GpuApi::PixelShader );
		for ( Uint32 i = 0; i < textureCount; ++i )
		{
			if ( textures[i] )
			{
				GpuApi::BindTextures( stage + i, 1, &(textures[i]), GpuApi::PixelShader );
			}
			else
			{
				GpuApi::BindTextures( stage + i, 1, nullptr, GpuApi::PixelShader );
			}
		}

		for ( Uint32 i = 0; i < textureCount; ++i )
		{
			// Shouldn't be released meanwhile. The SF reference implementation
			// doesn't add a ref and does a straight Red::System::MemoryCopy of pointers
			m_currentTextureRefIds[ stage + i ] = textures[ i ];
		}
	}
}

void CRenderScaleformTextureManager::BeginScene()
{
	Red::System::MemorySet( m_currentSamplers, GpuApi::SAMPSTATEPRESET_Max, sizeof(m_currentSamplers) );
	
	// SF reference implementation would clear the states here
	//GpuApi::InvalidateSamplerStatesCommon( 0, MaximumStages, GpuApi::PixelShader );	
	
	Red::System::MemorySet( m_currentTextureRefIds, 0, sizeof(m_currentTextureRefIds) );
	GpuApi::BindTextures( 0, MaximumStages, nullptr, GpuApi::PixelShader );
}


// ***** D3D1x Format mapping and conversion functions

// void SF_STDCALL D3D1x(CopyScanline8_Extend_A8_A8L8)(SF::UByte* pd, const SF::UByte* ps, SF::UPInt size,
// 	SF::Render::Palette*, void*)
// {
// 	for (SF::UPInt i = 0; i< size; i++, pd+=2, ps++)
// 	{        
// 		pd[0] = 255;
// 		pd[1] = ps[0];
// 	}
// }
// 
// void SF_STDCALL D3D1x(CopyScanline8_Extend_A8_A4R4G4B4)(SF::UByte* pd, const SF::UByte* ps, SF::UPInt size,
// 	SF::Render::Palette*, void*)
// {
// 	for (SF::UPInt i = 0; i< size; i++, pd+=2, ps++)
// 	{        
// 		pd[0] = 255;
// 		pd[1] = ps[0] | 0x0f; // Copy high 4 bits; low bits set to F.
// 	}
// }
// 
// void SF_STDCALL D3D1x(CopyScanline16_Retract_A8L8_A8)(SF::UByte* pd, const SF::UByte* ps, SF::UPInt size,
// 	SF::Render::Palette*, void*)
// {
// 	for (SF::UPInt i = 0; i< size; i++, pd++, ps+=2)
// 	{        
// 		pd[0] = ps[1];
// 	}
// }
// 
// void SF_STDCALL D3D1x(CopyScanline16_Retract_A4R4G4B4_A8)(SF::UByte* pd, const SF::UByte* ps, SF::UPInt size,
// 	SF::Render::Palette*, void*)
// {
// 	for (SF::UPInt i = 0; i< size; i++, pd++, ps+=2)
// 	{        
// 		pd[0] = ps[1] & ~0x0f; // Copy high 4 bits; zero low bits.
// 	}
// }


// Image to Texture format conversion and mapping table,
// organized by the order of preferred image conversions.

CRenderScaleformTextureFormat::Mapping TextureFormatMapping[] = 
{
	// FIXME:/NOTE: TEXFMT_NULL instead of adding mapping to GpuApi. See if really needed first.

	// Warning: Different versions of the same ImageFormat must go right after each-other,
	// as initTextureFormats relies on that fact to skip them during detection.
	{ SF::Render::Image_R8G8B8A8,   GpuApi::TEXFMT_R8G8B8A8 /*DXGI_FORMAT_R8G8B8A8_UNORM*/, 4, &SF::Render::Image::CopyScanlineDefault,             &SF::Render::Image::CopyScanlineDefault },
	{ SF::Render::Image_B8G8R8A8,   GpuApi::TEXFMT_NULL /*DXGI_FORMAT_B8G8R8A8_UNORM*/, 4, &SF::Render::Image::CopyScanlineDefault,             &SF::Render::Image::CopyScanlineDefault },
	{ SF::Render::Image_R8G8B8,     GpuApi::TEXFMT_R8G8B8A8 /*DXGI_FORMAT_R8G8B8A8_UNORM*/, 4, &SF::Render::Image_CopyScanline24_Extend_RGB_RGBA,   &SF::Render::Image_CopyScanline32_Retract_RGBA_RGB },
	{ SF::Render::Image_B8G8R8,     GpuApi::TEXFMT_NULL /*DXGI_FORMAT_B8G8R8A8_UNORM*/, 4, &SF::Render::Image_CopyScanline24_Extend_RGB_RGBA,   &SF::Render::Image_CopyScanline32_Retract_RGBA_RGB },
	{ SF::Render::Image_A8,         GpuApi::TEXFMT_A8_Scaleform /*DXGI_FORMAT_R8_UNORM*/,   1, &SF::Render::Image::CopyScanlineDefault,             &SF::Render::Image::CopyScanlineDefault },

	// Compressed formats.
	{ SF::Render::Image_DXT1,       GpuApi::TEXFMT_BC1 /*DXGI_FORMAT_BC1_UNORM*/,      0, &SF::Render::Image::CopyScanlineDefault,             &SF::Render::Image::CopyScanlineDefault},
	{ SF::Render::Image_DXT3,       GpuApi::TEXFMT_BC2 /*DXGI_FORMAT_BC2_UNORM*/,      0, &SF::Render::Image::CopyScanlineDefault,             &SF::Render::Image::CopyScanlineDefault},
	{ SF::Render::Image_DXT5,       GpuApi::TEXFMT_BC3 /*DXGI_FORMAT_BC3_UNORM*/,      0, &SF::Render::Image::CopyScanlineDefault,             &SF::Render::Image::CopyScanlineDefault},
	{ SF::Render::Image_BC7,		GpuApi::TEXFMT_BC7 /*DXGI_FORMAT_BC7_UNORM*/,	   0, &SF::Render::Image::CopyScanlineDefault,			   &SF::Render::Image::CopyScanlineDefault},
 
	// Video formats.
	{ SF::Render::Image_Y8_U2_V2,   GpuApi::TEXFMT_A8_Scaleform /*DXGI_FORMAT_R8_UNORM*/,   1, &SF::Render::Image::CopyScanlineDefault,             &SF::Render::Image::CopyScanlineDefault },
	{ SF::Render::Image_Y8_U2_V2_A8,GpuApi::TEXFMT_A8_Scaleform /*DXGI_FORMAT_R8_UNORM*/,   1, &SF::Render::Image::CopyScanlineDefault,             &SF::Render::Image::CopyScanlineDefault },

	{ SF::Render::Image_None,       GpuApi::TEXFMT_NULL /*DXGI_FORMAT_UNKNOWN*/,        0, 0,                                       0 }
};


void        CRenderScaleformTextureManager::initTextureFormats()
{
// 	// Obtain the feature level, so we can check if the TextureFormat is unsupported by the current feature level.
// 	D3D_FEATURE_LEVEL currentFeatureLevel = D3D_FEATURE_LEVEL_10_0;
// #if (SF_D3D_VERSION == 11)
// 	currentFeatureLevel = pDevice->GetFeatureLevel();
// #elif (SF_D3D_VERSION == 10 )
// 	Ptr<ID3D1x(Device1)> d3d10Device1;
// 	if ( SUCCEEDED(pDevice->QueryInterface(IID_ID3D10Device1, (void**)&d3d10Device1.GetRawRef())) && d3d10Device1)
// 	{
// 		currentFeatureLevel = (D3D_FEATURE_LEVEL)d3d10Device1->GetFeatureLevel();
// 	}
// #endif

	CRenderScaleformTextureFormat::Mapping* pmapping = 0;
	for (pmapping = TextureFormatMapping; pmapping->m_format != SF::Render::Image_None; pmapping++)
	{
// 		// Must exclude non-supported image formats before checking the support, because they will
// 		// cause the D3D11 debug runtime to assert.
// 		if (currentFeatureLevel < pmapping->MinFeatureLevel)
// 			continue;

//		UINT formatSupport;
		// See if format is supported.        
// 		if (SUCCEEDED( pDevice->CheckFormatSupport(pmapping->D3DFormat, &formatSupport) ) &&
// 			formatSupport & D3D1x(FORMAT_SUPPORT_TEXTURE2D) )
// 		{

		//FIXME: Bother with GPU API here, but either give it a BS textureDesc or have the buffer already...
			//pmapping->BytesPerPixel = GpuApi::CalcTextureSize( desc with pmapping->m_textureFormat );
		
		CRenderScaleformTextureFormat* tf = SF_HEAP_AUTO_NEW(this) CRenderScaleformTextureFormat(pmapping, 0);
			// And now check its capabilities to assign extra Usage.
// 			if (formatSupport & D3D1x(FORMAT_SUPPORT_MIP_AUTOGEN) )
// 			{
// 				// tf.D3DUsage |= D3DUSAGE_AUTOGENMIPMAP;
// 			}
			TextureFormats.PushBack(tf);

			// If format added, skip additional mappings for it.
			while ((pmapping+1)->m_format == pmapping->m_format)
			{
				pmapping++;
			}
//		}
	}
}


void    CRenderScaleformTextureManager::processTextureKillList()
{
	for ( Int32 i = (Int32)m_D3DTextureKillList.GetSize()-1 ; i >= 0 ; --i )
	{
		GpuApi::Release( m_D3DTextureKillList[i] );
	}
	m_D3DTextureKillList.Clear();
}

void    CRenderScaleformTextureManager::processInitTextures()
{
	// TextureMutex lock expected externally.
	//Mutex::Locker lock(&TextureMutex);

	if (!TextureInitQueue.IsEmpty() || !DepthStencilInitQueue.IsEmpty())
	{
		while (!TextureInitQueue.IsEmpty())
		{
			SF::Render::Texture* ptexture = TextureInitQueue.GetFirst();
			ptexture->RemoveNode();
			ptexture->pPrev = ptexture->pNext = 0;
			if (ptexture->Initialize())
			{
				Textures.PushBack(ptexture);
			}
		} 
		while (!DepthStencilInitQueue.IsEmpty())
		{
			SF::Render::DepthStencilSurface* pdss = DepthStencilInitQueue.GetFirst();
			pdss->RemoveNode();
			pdss->pPrev = pdss->pNext = 0;
			pdss->Initialize();
		}
		pLocks->TextureInitWC.NotifyAll();
	}
}

SF::Render::Texture* CRenderScaleformTextureManager::CreateTexture(SF::Render::ImageFormat format, SFUInt mipLevels, const SF::Render::ImageSize& size, SFUInt use, SF::Render::ImageBase* pimage, SF::Render::MemoryManager* allocManager)
{
	SF_UNUSED(allocManager);
#ifndef RED_FINAL_BUILD
	SF_AMP_SCOPE_TIMER(RenderThreadId == SF::GetCurrentThreadId() ? AmpServer::GetInstance().GetDisplayStats() : NULL, __FUNCTION__, SF::Amp_Profile_Level_Medium);
#endif
	if ( ! GpuApi::IsInit() )
	{
		HALT( "CreateTexture failed - TextueManager has been Reset" );
		return nullptr;
	}

	CRenderScaleformTexture* ptexture = nullptr;

	Bool streamTexture = false;
	if ( pimage != nullptr && pimage->GetImageType() == SF::Render::ImageBase::Type_Other )
	{
		RED_FATAL_ASSERT( m_textureCacheQuee != nullptr , "Scaleform queue manager is not created" );
		RED_FATAL_ASSERT( !m_textureCacheQuee->GetUndefinedTexture().isNull() , "Placeholder texture not ready yet" );

		CScaleformTextureCacheImage* texCacheImage = static_cast< CScaleformTextureCacheImage* >( pimage );

		ptexture = SF_HEAP_AUTO_NEW(this) CRenderScaleformTexture( pLocks, m_textureCacheQuee->GetUndefinedTexture() , texCacheImage->GetSize(), pimage );

		ptexture->SetCacheImage( new CScaleformTextureCacheImage( *texCacheImage ) );
		streamTexture = true;
	}

	if ( ptexture == nullptr )
	{
		CRenderScaleformTextureFormat* ptformat = (CRenderScaleformTextureFormat*)precreateTexture(format, use, pimage);
		if ( !ptformat )
		{
			return nullptr;
		}

		ptexture = SF_HEAP_AUTO_NEW(this) CRenderScaleformTexture(pLocks, ptformat, mipLevels, size, use, pimage);
	}

	SF::Render::Texture* postInitTexture = postCreateTexture(ptexture, use);

	if ( streamTexture && postInitTexture )
	{
		SF::Mutex::Locker  lock(&m_streamedTextureMutex);
		m_texturesToStream.PushBack( ptexture );
	}

	return postInitTexture;
}

SF::Render::Texture* CRenderScaleformTextureManager::CreateTexture( const GpuApi::TextureRef& apiTexture, SF::Render::ImageSize imgSize, SF::Render::ImageBase* image )
{
	if ( apiTexture.isNull() )
	{
		return 0;
	}

#ifndef RED_FINAL_BUILD
	IncreaseMemoryCount( GpuApi::CalcTextureSize( GpuApi::GetTextureDesc(apiTexture) ) );
#endif

	CRenderScaleformTexture* ptexture = SF_HEAP_AUTO_NEW(this) CRenderScaleformTexture(pLocks, apiTexture, imgSize, image);

	return postCreateTexture(ptexture, 0);
}

SFUInt CRenderScaleformTextureManager::GetTextureUseCaps(SF::Render::ImageFormat format)
{
	// ImageUse_InitOnly (ImageUse_NoDataLoss alias) ok while textures are Managed    
	Uint32 use = SF::Render::ImageUse_InitOnly | SF::Render::ImageUse_Update;
	if (!SF::Render::ImageData::IsFormatCompressed(format))
	{
		use |= SF::Render::ImageUse_PartialUpdate | SF::Render::ImageUse_GenMipmaps;
	}

	const SF::Render::TextureFormat* ptformat = getTextureFormat(format);
	if (!ptformat)
	{
		return 0;
	}

	if (isScanlineCompatible(ptformat))
	{
		use |= SF::Render::ImageUse_MapRenderThread;
	}

	return use;   
}

SF::Render::DepthStencilSurface* CRenderScaleformTextureManager::CreateDepthStencilSurface(const SF::Render::ImageSize& size, SF::Render::MemoryManager* manager)
{
	SF_UNUSED(manager);
	if ( ! GpuApi::IsInit() )
	{
		HALT( "CreateDepthStencilSurface failed - TextueManager has been Reset" );
		return 0;
	}

	CRenderScaleformDepthStencilSurface* pdss = SF_HEAP_AUTO_NEW(this) CRenderScaleformDepthStencilSurface(pLocks, size);
	return postCreateDepthStencilSurface(pdss);
}

SF::Render::DepthStencilSurface* CRenderScaleformTextureManager::CreateDepthStencilSurface( const GpuApi::TextureRef& texture )
{
	if ( ! texture )
	{
		return 0;
	}

	GpuApi::AddRef( texture );

	GpuApi::TextureDesc desc;
	GpuApi::GetTextureDesc( texture, desc );

#ifndef RED_FINAL_BUILD
	IncreaseMemoryCount( GpuApi::CalcTextureSize( GpuApi::GetTextureDesc(texture) ) );
#endif

	CRenderScaleformDepthStencilSurface* pdss = SF_HEAP_AUTO_NEW(this) CRenderScaleformDepthStencilSurface(pLocks, SF::Render::ImageSize(desc.width, desc.height) );
	pdss->m_depthStencilTexture = texture;
	pdss->State = CRenderScaleformTexture::State_Valid;
	return pdss;
}

void CRenderScaleformTextureManager::ForceTextureStreaming( Bool pin )
{
	SF::Mutex::Locker  lock(&m_streamedTextureMutex);

	// Ensure to keep them streamed while processing now and in the future. Set the actual pin value at the end.
	m_pinTextures = true;

	for ( CRenderScaleformTexture* texture : m_texturesToStream )
	{
		m_streamedTextures.PushBackUnique( texture );
	}

	for ( CRenderScaleformTexture* texture : m_texturesToStream )
	{
		texture->ProcessStreaming();
	}

	m_texturesToStream.ClearFast();

	// Keep from getting unstreamed
	for ( CRenderScaleformTexture* texture : m_streamedTextures )
	{
		texture->ForceUpdateBindTime();
	}

	m_pinTextures = pin;
}

void CRenderScaleformTextureManager::AddStreamedTexture( CRenderScaleformTexture* texture ) 
{ 
	RED_ASSERT( texture , TXT("Texture shoudn't be null") );
	SF::Mutex::Locker  lock(&m_streamedTextureMutex);

	m_texturesToStream.RemoveFast( texture );
	// We definitly don't want double entries of the texture on the list, bcoz its going to be removed just once
	m_streamedTextures.PushBackUnique( texture ); 
};

Bool CRenderScaleformTextureManager::RemoveStreamedTexture( CRenderScaleformTexture* texture ) 
{ 
	RED_ASSERT( texture , TXT("Texture shoudn't be null") );
	SF::Mutex::Locker  lock(&m_streamedTextureMutex);

	return RemoveStreamedTextureNoLock( texture );
};

Bool CRenderScaleformTextureManager::RemoveStreamedTextureNoLock( CRenderScaleformTexture* texture )
{ 
	RED_ASSERT( texture , TXT("Texture shoudn't be null") );
	m_texturesToStream.RemoveFast( texture );
	return m_streamedTextures.RemoveFast( texture ); 
};

Bool CRenderScaleformTextureManager::IsAnyTexturePending()
{ 
	if( m_textureCacheQuee && m_textureCacheQuee->GetNumPendingJobs() != 0 )
	{
		return true;
	}

	/*
	SF::Mutex::Locker  lock(&m_streamedTextureMutex);

	for( Int32 i = 0, end = m_streamedTextures.Size(); i < end; ++i )
	{
		if( CRenderScaleformTexture* it = m_streamedTextures[i] )
		{ 
			if( it->IsStreamable() == false )
			{
				return true;
			}
		}
	}
	*/

	return false;
}

void CRenderScaleformTextureManager::ProcessTextures()
{
	if( m_textureCacheQuee && m_textureCacheQuee->IsSuspended() )
	{
		return;
	}

	SF::Mutex::Locker  lock(&m_streamedTextureMutex);

	const Float unstreamTime = m_currentTime - m_textureUnloadDelay;

	for( Int32 i = m_streamedTextures.SizeInt()-1; i >= 0; --i )
	{
		CRenderScaleformTexture* it = m_streamedTextures[i];
		if( it )
		{

			if( it->IsStreamable() )
			{
				Float bindTick = it->GetBindTime();
				if( bindTick > 0.0f && bindTick < unstreamTime  && !m_pinTextures )
				{
					it->Unstream();
				}
			}
			else
			{
				it->ProcessStreamingFinish();
			}
		}
	}

	RED_ASSERT( m_textureCacheQuee->GetNumPendingJobs() <= m_streamedTextures.Size(), TXT("More jobs in the scaleform texture queue than registered streamed textures! Probably leaking something!") );
}


//////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

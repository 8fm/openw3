/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

//#include "Render/D3D1x/D3D1x_Config.h"
#include "Kernel/SF_List.h"
#include "Kernel/SF_Threads.h"
#include "Render/Render_Image.h"
#include "Render/Render_ThreadCommandQueue.h"
#include "Kernel/SF_HeapNew.h"

#include "../engine/scaleformTextureCacheQuee.h"

// Forward declarations
class CRenderScaleformMappedTexture;
class CRenderScaleformTextureManager;

////////////////////////////////////////////////////////////////////////////////////////////
//	CRenderScaleformTextureFormat
//
// TextureFormat describes format of the texture and its caps.
// Format includes allowed usage capabilities and ImageFormat
// from which texture is supposed to be initialized.
//
////////////////////////////////////////////////////////////////////////////////////////////

struct CRenderScaleformTextureFormat : public SF::Render::TextureFormat
{
	struct Mapping
	{
		SF::Render::ImageFormat  m_format;
		GpuApi::eTextureFormat   m_textureFormat;
		SF::UByte                m_bytesPerPixel; // Cached from GpuApi call

		//GFx 4.3:  D3D_FEATURE_LEVEL        MinFeatureLevel;

		SF::Render::Image::CopyScanlineFunc  m_copyFunc;
		SF::Render::Image::CopyScanlineFunc  m_uncopyFunc;
	};

	CRenderScaleformTextureFormat(const Mapping* mapping, Uint32 d3dusage) 
		: m_mapping(mapping)
		, m_D3DUsage(d3dusage) 
	{ }

	const Mapping*	m_mapping;
	Uint32			m_D3DUsage; // FIXME: Get rid of this

	virtual SF::Render::ImageFormat             GetImageFormat() const      { return m_mapping->m_format; }
	virtual SF::Render::Image::CopyScanlineFunc GetScanlineCopyFn() const   { return m_mapping->m_copyFunc; }
	virtual SF::Render::Image::CopyScanlineFunc GetScanlineUncopyFn() const { return m_mapping->m_uncopyFunc; }

	// GpuApi Specific.
	GpuApi::eTextureFormat           GetApiTextureFormat() const   { return m_mapping->m_textureFormat; }
};

////////////////////////////////////////////////////////////////////////////////////////////
//	CRenderScaleformTexture
//
// D3D1x Texture class implementation; it many actually include several HW 
// textures (one for each ImageFormat plane).
//
////////////////////////////////////////////////////////////////////////////////////////////

class CRenderScaleformTexture : public SF::Render::Texture
{
public:
	static const SF::UByte      MaxTextureCount = 4;

	struct HWTextureDesc
	{        
		SF::Render::ImageSize       m_size;
		GpuApi::TextureRef			m_texture;
		GpuApi::TextureRef			m_stagingTexture;

		HWTextureDesc()
			: m_size( 0 ,0 )
			, m_texture( NULL )
			, m_stagingTexture( NULL )
		{}

	};
	
	// TextureDesc array is allocated if more then one is needed.
	HWTextureDesc*          m_textureArray;
	HWTextureDesc           m_texture;

	CScaleformTextureCacheJob* m_job;

	CScaleformTextureCacheImage* m_casheImage;
	Bool					m_streamLoaded;
	Float					m_bindTick;

public:

	CRenderScaleformTexture(SF::Render::TextureManagerLocks* pmanagerLocks, const CRenderScaleformTextureFormat* pformat, SFUInt mipLevels, const SF::Render::ImageSize& size, SFUInt use, SF::Render::ImageBase* pimage);
	
	CRenderScaleformTexture(SF::Render::TextureManagerLocks* pmanagerLocks, const GpuApi::TextureRef& texture, SF::Render::ImageSize imgSize, SF::Render::ImageBase* pimage);
	
	~CRenderScaleformTexture();

public:

	// *** Interface implementation
	virtual SF::Render::ImageSize       GetTextureSize(SFUInt plane =0) const { return m_textureArray[plane].m_size; }

	virtual SF::Render::Image*          GetImage() const			{ ASSERT(!pImage || (pImage->GetImageType() != SF::Render::Image::Type_ImageBase)); return (SF::Render::Image*)pImage; }

	virtual SF::Render::ImageFormat     GetFormat() const			{ return GetImageFormat(); }

	virtual void						GetUVGenMatrix(SF::Render::Matrix2F* mat) const;

	virtual SFBool						Update(const UpdateDesc* updates, SFUInt count = 1, SFUInt mipLevel = 0);    

public:

	CRenderScaleformTextureManager*		GetManager() const			{ return (CRenderScaleformTextureManager*)pManagerLocks->pManager; }
	
	virtual SFBool						IsValid() const				{ return m_textureArray != 0; }

	virtual SFBool						Initialize();
	
	SFBool								Initialize( GpuApi::TextureRef& texture);
	
	virtual void						ReleaseHWTextures(SFBool staging = true);

	void								Unstream();

	RED_INLINE Float					GetBindTime() const { return m_bindTick; };
	void								ForceUpdateBindTime();

	RED_INLINE Bool						IsStreamable() const { return m_streamLoaded; }

	// Applies a texture to device starting at pstageIndex, advances index
	// TBD: Texture matrix may need to be adjusted if image scaling is done.
	virtual void						ApplyTexture(SFUInt stageIndex, const SF::Render::ImageFillMode& fm);

	const CRenderScaleformTextureFormat*GetTextureFormat() const	{ return reinterpret_cast<const CRenderScaleformTextureFormat*>(pFormat); }
	
	const CRenderScaleformTextureFormat::Mapping*   GetTextureFormatMapping() const { return pFormat ? reinterpret_cast<const CRenderScaleformTextureFormat*>(pFormat)->m_mapping : 0; }

	void								ProcessStreaming();

	void								ProcessStreamingFinish();

	RED_INLINE void						SetStreamingJob( CScaleformTextureCacheJob* job ) { m_job = job; }

	RED_INLINE void						SetCacheImage( CScaleformTextureCacheImage* casheImage ) { m_casheImage = casheImage; }

protected:
	virtual void						computeUpdateConvertRescaleFlags( SFBool rescale, SFBool swMipGen, SF::Render::ImageFormat inputFormat, SF::Render::ImageRescaleType &rescaleType, SF::Render::ImageFormat &rescaleBuffFromat, SFBool &convert );
};

////////////////////////////////////////////////////////////////////////////////////////////
//	CRenderScaleformDepthStencilSurface
//
// GpuApi DepthStencilSurface implementation. 
//
////////////////////////////////////////////////////////////////////////////////////////////

class CRenderScaleformDepthStencilSurface : public SF::Render::DepthStencilSurface
{
public:

	GpuApi::TextureRef				m_depthStencilTexture;

public:

	CRenderScaleformDepthStencilSurface(SF::Render::TextureManagerLocks* pmanagerLocks, const SF::Render::ImageSize& size);
	
	~CRenderScaleformDepthStencilSurface();

public:

	SFBool                          Initialize();
	
};

////////////////////////////////////////////////////////////////////////////////////////////
//	CRenderScaleformMappedTexture
//
// *** MappedTexture
//
////////////////////////////////////////////////////////////////////////////////////////////

class CRenderScaleformMappedTexture : public SF::Render::MappedTextureBase
{
	friend class CRenderScaleformTexture;

public:

	CRenderScaleformMappedTexture() : SF::Render::MappedTextureBase() { }

public:

	virtual SFBool Map(SF::Render::Texture* ptexture, SFUInt mipLevel, SFUInt levelCount);
	virtual void Unmap(SFBool applyUpdate = true);
};

////////////////////////////////////////////////////////////////////////////////////////////
//	CRenderScaleformTextureManager
//
// GpuApi Texture Manger.
// This class is responsible for creating textures and keeping track of them
// in the list.
//
////////////////////////////////////////////////////////////////////////////////////////////

class CRenderScaleformTextureManager : public SF::Render::TextureManager
{
	friend class CRenderScaleformTexture;
	friend class CRenderScaleformDepthStencilSurface;

	typedef SF::ArrayConstPolicy<8, 8, false>   KillListArrayPolicy;

	typedef SF::ArrayLH< GpuApi::TextureRef, SF::StatRender_TextureManager_Mem, KillListArrayPolicy>    D3DResourceArray;
// 	typedef ArrayLH<ID3D1x(View)*,
// 		StatRender_TextureManager_Mem,
// 		KillListArrayPolicy>    D3DViewArray;

	CScaleformTextureCacheQueue*	m_textureCacheQuee;

	CRenderScaleformMappedTexture	m_mappedTexture;    

	// Lists protected by TextureManagerLocks::TextureMutex.
	D3DResourceArray				m_D3DTextureKillList;

	static const SFUInt				SamplerTypeCount = (SF::Render::Sample_Count * SF::Render::Wrap_Count);
	GpuApi::eSamplerStatePreset		m_samplerPresets[SamplerTypeCount];

	// Detecting redundant sampler/address setting.
	static const SFInt				MaximumStages = 4;
	GpuApi::eSamplerStatePreset		m_currentSamplers[MaximumStages];
	Uint32							m_currentTextureRefIds[MaximumStages];

	TDynArray<CRenderScaleformTexture*>		m_streamedTextures;
	TDynArray<CRenderScaleformTexture* >	m_texturesToStream;
	SF::Mutex						m_streamedTextureMutex;

	Float							m_currentTime;
	Float							m_textureUnloadDelay;
	Bool							m_pinTextures;

#ifndef RED_FINAL_BUILD
	Uint32							m_currentMemory;
	Uint32							m_peakMemory;
#endif 

	// Detects supported D3DFormats and capabilities.
	void									initTextureFormats();
	virtual SF::Render::MappedTextureBase&	getDefaultMappedTexture() { return m_mappedTexture; }
	virtual SF::Render::MappedTextureBase*	createMappedTexture()     { return SF_HEAP_AUTO_NEW(this) CRenderScaleformMappedTexture; }

	virtual void							processTextureKillList();
	virtual void							processInitTextures();    

public:

	CRenderScaleformTextureManager(	SF::ThreadId renderThreadId, SF::Render::ThreadCommandQueue* commandQueue, SF::Render::TextureCache* texCache = 0);

	~CRenderScaleformTextureManager();
	
	// Force textures to start streaming, even if not visible and rendered yet. E.g., we're behind a loading screen and want the textures there before we drop it. Like the minimap.
	// Normally ProcessStreaming() is called only on visible textures
	void									ForceTextureStreaming( Bool pinTextures );

	// Used once texture manager is no longer necessary.
	void									Reset();

	// Does rendundancy checking on state setting.
	void									SetSamplerState( SFUInt stage, SFUInt textureCount, GpuApi::TextureRef* textures, GpuApi::eSamplerStatePreset state = GpuApi::SAMPSTATEPRESET_Max );

	virtual void							BeginScene();

	// *** TextureManager
	virtual SF::Render::Texture*			CreateTexture(SF::Render::ImageFormat format, unsigned mipLevels,	const SF::Render::ImageSize& size,	unsigned use, SF::Render::ImageBase* pimage, SF::Render::MemoryManager* manager = 0 );

	virtual SF::Render::Texture*			CreateTexture( const GpuApi::TextureRef& apiTexture, SF::Render::ImageSize imgSize = SF::Render::ImageSize(0), SF::Render::ImageBase* image = 0);

	virtual SFUInt							GetTextureUseCaps(SF::Render::ImageFormat format);
	
	SFBool									IsMultiThreaded() const    { return RenderThreadId != 0; }

	// Returns quee that manages the loading of the streamed textures
	CScaleformTextureCacheQueue*			GetStreamingQueue() { return m_textureCacheQuee; }

	Bool									IsAnyTexturePending();

	virtual SF::Render::DepthStencilSurface* CreateDepthStencilSurface(const SF::Render::ImageSize& size, SF::Render::MemoryManager* manager = 0);

	virtual SF::Render::DepthStencilSurface* CreateDepthStencilSurface( const GpuApi::TextureRef& texture );

	virtual SFBool							IsDrawableImageFormat(SF::Render::ImageFormat format) const { return (format == SF::Render::Image_B8G8R8A8) || (format == SF::Render::Image_R8G8B8A8); }

	RED_INLINE Float						GetCurrentTime() const { return m_currentTime; }

	RED_INLINE void							SetCurrentTime( Float currentTime ) { m_currentTime = currentTime; }

public:

	void									ProcessTextures();

	void									AddStreamedTexture( CRenderScaleformTexture* texture );

	Bool									RemoveStreamedTexture( CRenderScaleformTexture* texture );

	Bool									RemoveStreamedTextureNoLock( CRenderScaleformTexture* texture );

#ifndef RED_FINAL_BUILD

	RED_INLINE void							IncreaseMemoryCount( Uint32 memSize )
	{
		m_currentMemory += memSize;
		m_peakMemory = ::Max( m_peakMemory , m_currentMemory );
	}

	RED_INLINE void							DecreaseMemoryCount( Uint32 memSize )
	{
		m_currentMemory -= memSize;
	}

	RED_INLINE Uint32 GetCurrentMemoryUsage() const { return m_currentMemory; }

	RED_INLINE Uint32 GetPeakMemoryUsage() const { return m_peakMemory; }

#endif

};

//////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////
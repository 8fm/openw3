/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderDynamicResource.h"
#include "renderResourceIterator.h"
#include "renderHelpers.h"
#include "../engine/textureGroup.h"

class IBitmapTextureStreamingSource;
class IRenderTextureStreamingTask;
struct STextureStreamResults;


// Texture
class CRenderTextureBase : public IDynamicRenderResource, public TRenderResourceListWithCache<CRenderTextureBase>
{
	DECLARE_RENDER_RESOURCE_ITERATOR;

protected:
	GpuApi::TextureRef					m_texture;					//!< Resident texture resource
	GpuApi::TextureRef					m_hiResTexture;				//!< Streamed hi res texture resource

	GpuApi::eSamplerStatePreset			m_samplerStatePreset;		//!< Sampler states preset
	CName								m_textureGroupName;			//!< Name of the source texture group
	ETextureCategory					m_textureCategory;			//!< Texture category
	Float								m_lastBindDistance;			//!< Smallest rendering distance that was calculated for this texture

	IBitmapTextureStreamingSource*		m_streamingSource;			//!< Streaming information for textures that support streaming
	IRenderTextureStreamingTask*		m_streamingTask;			//!< Texture streaming in progress
	Int32								m_streamingMipIndex:6;		//!< Loaded streamed mipmap
	Int32								m_maxStreamingMipIndex:6;	//!< Maximum streamable mipmap
	Int32								m_residentMipIndex:6;		//!< Resident mipmap
	Int32								m_pendingMipIndex:6;		//!< Mipmap we want to stream in

	Uint16								m_streamingKey;

	Uint32								m_umbraId;
	Uint32								m_approxSize;				//!< Cached approximate size of the texture, not accurate because it doesn't take padding into account

	Red::Threads::CAtomic< Int32 >		m_streamingLocks;			//!< Track number of locks on this texture's streaming. If > 0, prefer to not unload it.
	Red::Threads::CAtomic< Int32 >		m_waitingRequests;			//!< Track number of texture requests waiting for this texture.

#ifndef RED_FINAL_BUILD
	String								m_debugDepotPath;
	Uint32								m_texUsedMemory;
	Uint32								m_hiResTexUsedMemory;
#endif

public:
	//! Set the texture's depot path. This is used to generate an Umbra ID, as well as to set the debug path on the gpu texture.
	void SetDepotPath( const String& path );

#ifndef RED_FINAL_BUILD
	//! Get the texture's depot path
	const String& GetDepotPath() const { return m_debugDepotPath; }
#else
	const String& GetDepotPath() const { return String::EMPTY; }
#endif

	//! Set texture group. Will set up a default sampler preset based on it.
	void SetTextureGroup( const CName& group );

	CRenderTextureBase( const GpuApi::TextureRef& texture );
	virtual ~CRenderTextureBase();

	//! Init
	virtual void InitWithGpuTexture( const GpuApi::TextureRef& texture );

	//! Get gpuapi texture
	RED_INLINE const GpuApi::TextureRef& GetTextureRef() const { return m_texture; }

	// TODO: Remove after texture streaming tests
	//! Get gpuapi hi-res texture
	RED_INLINE const GpuApi::TextureRef& GetHiResTextureRef() const { return m_hiResTexture; }

	//! Get name of the source texture group
	RED_INLINE const CName& GetTextureGroupName() const { return m_textureGroupName; }

	//! Get texture content categoryu
	RED_INLINE const ETextureCategory GetTextureCategory() const { return m_textureCategory; }

	//! Get Umbra ID for this texture
	RED_INLINE Uint32 GetUmbraId() const { return m_umbraId; }

	//! Get approximate size of the full resolution texture.
	//NOTE: This is only used for initializing STextureStreamingRegistration. Along with the other member overload, if this
	//were removed, then approxSize calculation could just be moved to the Registration.
	RED_FORCE_INLINE Uint32 GetApproxSize() const { return m_approxSize; }

	//! Get approximate size of the texture, starting from a mip level. Dropping the highest mip from a full chain drops the size to 1/4.
	//! Note : This does not take into account any platform-specific padding.
	static RED_INLINE Uint32 GetApproxSize( Uint32 baseSize, Uint32 mip ) { return Max<Uint32>( (baseSize >> ( mip * 2 )), 4 ); }

	//TODO: This one is only used by some debug stuff. Could just remove it and drop the m_approxSize member.
	RED_INLINE Uint32 GetApproxSize( Uint32 mip ) const { return GetApproxSize( m_approxSize, mip ); }

	//! Set sampler state info
	void SetSamplerStatePreset( GpuApi::eSamplerStatePreset preset );

	//! Bind texture but no sampler state
	void BindNoSampler( Uint32 textureIndex, ERenderShaderType shaderStage = RST_PixelShader, Float distance = 0.f );

	//! Bind texture but no sampler state
	void BindNoSamplerFast( Uint32 textureIndex, ERenderShaderType shaderStage = RST_PixelShader, Float distance = 0.f );

	//! Bind texture with texture's default sampler
	void Bind( Uint32 samplerIndex, ERenderShaderType shaderStage = RST_PixelShader, Float distance = 0.f );

	//! Bind texture with sampler state override
	void Bind( Uint32 samplerIndex, GpuApi::eSamplerStatePreset forcedSamplerStatePreset, ERenderShaderType shaderStage = RST_PixelShader, Float distance = 0.f );

	//! Device lost
	virtual void OnDeviceLost();

	//! Device lost
	virtual void OnDeviceReset();

	//! Get video memory consumption
	virtual Uint32 GetUsedVideoMemory() const;
#ifndef RED_FINAL_BUILD
	Uint32 GetCachedUsedVideoMemory();
#endif

	//! Update streaming state, so that unused textures will eventually timeout and expire.
	void UpdateStreamingExpiration();


	//! Should just be called during creation and if a streaming source becomes ready.
	void UpdateApproximateSize();


	//! Get the maximum number of mips for this texture. For a non-streamed texture, this is just the number of mips. For a streamed
	//! texture, it's the number of mips if it were fully streamed.
	Uint8 GetMaxMipCount() const;

	Uint16 GetMaxWidth() const;
	Uint16 GetMaxHeight() const;


	//////////////////////////////////////////////////////////////////////////
	// Streaming

	void OnNewTextureCache();

	//! Get the last distance this texture was bound with
	RED_INLINE Float GetLastBindDistance() const { return m_lastBindDistance; }

	//! Set the last bind distance to the given distance, if it's not already closer.
	RED_INLINE void UpdateLastBindDistance( Float distance ) { m_lastBindDistance = Clamp<Float>( distance, 0, m_lastBindDistance ); }

	//! Reset last bind distance to farthest value (texture not in use).
	RED_INLINE void ResetLastBindDistance() { m_lastBindDistance = FLT_MAX; }

	//! Get texture streaming source
	RED_INLINE IBitmapTextureStreamingSource* GetStreamingSource() const { return m_streamingSource; }

	//! Does this texture support streaming ?
	RED_INLINE Bool HasStreamingSource() const { return m_streamingSource != nullptr; }

	//! Are we streaming right now ?
	RED_INLINE Bool HasStreamingPending() const { return m_streamingTask != nullptr; }

	//! Get the mip we are currently streaming (valid if streaming is pending).
	RED_INLINE Int8 GetPendingMip() const { return m_pendingMipIndex; }

	//! Get mip index of streamed texture
	RED_INLINE Int8 GetStreamingMipIndex() const { return m_streamingMipIndex; }

	//! Get maximum streamable mipmap index
	RED_INLINE Int8 GetMaxStreamingMipIndex() const { return m_maxStreamingMipIndex; }

	//! Get mip index of the resident texture
	RED_INLINE Int8 GetResidentMipIndex() const { return m_residentMipIndex; }

	//! Is hi res texture loaded ?
	RED_INLINE Bool HasHiResLoaded() const { return !m_hiResTexture.isNull(); }

	//! Add a lock if this texture can be streamed.
	Bool LockStreaming();

	//! Remove a lock on the streaming. Won't unload or anything.
	void UnlockStreaming();

	RED_INLINE Bool HasStreamingLock() const { return m_streamingLocks.GetValue() > 0; }

	//! Increment "Waiting Requests" counter. Should be called by texture request when it starts waiting for this texture.
	RED_INLINE void AddWaitingRequest() { m_waitingRequests.Increment(); }
	//! Decrement "Waiting Requests" counter. Should be called when a texture request is no longer waiting for this texture, because
	//! it was streamed in, or because it was force-cancelled.
	RED_INLINE void ReleaseWaitingRequest() { m_waitingRequests.Decrement(); }

	RED_INLINE Bool HasWaitingRequest() const { return m_waitingRequests.GetValue() > 0; }



	//////////////////////////////////////////////////////////////////////////
	// Streaming -- These should only be called by the texture streaming system in controlled conditions.

	//! Start streaming the given mip. If requested is already the currently pending mip, then just return.
	//! Will set outAllowNewStreaming to false if further streaming should be blocked (e.g. IO is saturated). It should be
	//! true coming in, if it's false nothing will be started.
	Bool StartStreaming( Int8 requestedMip, Int8 priority, Bool& inoutAllowNewStreaming );

	//! Check if the streaming task is finished, and fetch results if it is.
	Bool TryFinishStreaming( STextureStreamResults& results );

	//! Check if the streaming task is finished, and fetch results if it is.
	//! allowNewStreaming signifies whether new loading should be started. It will be set to false if new IO cannot be serviced.
	void TickStreaming( Bool& allowNewStreaming );

	//! Apply results from the streaming update task. Update hi-res texture and streamed mip index.
	void ApplyStreamingResults( const STextureStreamResults& results );

	//! Unload hi-res texture.
	Bool UnstreamHiRes();

	//! Cancel in-flight streaming
	void CancelStreaming();

protected:
	//! Setup initial streaming information. residentMip is the largest mip index that exists in the resident texture. maxStreamMip is the
	//! index of the largest mip that can be streamed in. maxStreamMip <= residentMip. streamingSource can be null.
	void InitStreaming( Uint8 residentMip, Uint8 maxStreamMip, IBitmapTextureStreamingSource* streamingSource );

	//! Called when streaming is updated or released.
	virtual void OnStreamingChanged() {}

public:
	//! Figure out which mips should be resident, and how much we can stream in. inoutResidentMip will hold the index of the first (largest)
	//! resident mip. inoutMaxStreamMip will hold the index of the largest mip we can stream in. They may be unmodified, so the caller should
	//! provide reasonable defaults (the engine texture's resident mip and 0, for example).
	static void CalcMipStreaming( const CName textureGroupName, Uint32 width, Uint32 height, Uint32 mipCount, Uint8* inoutResidentMip, Uint8* inoutMaxStreamMip );

	//! Called when streaming settings / texture quality is updated
	virtual void RecalculateMipStreaming() {};

	//////////////////////////////////////////////////////////////////////////
};

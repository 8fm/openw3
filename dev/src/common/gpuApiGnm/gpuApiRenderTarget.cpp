/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../redMath/numericalutils.h"

namespace GpuApi
{

	// ----------------------------------------------------------------------
	// RenderTargetSetup

	RenderTargetSetup::RenderTargetSetup ()
	{
		Reset();
	}

	RenderTargetSetup& RenderTargetSetup::SetNullColorTarget()
	{
		numColorTargets = 0;
		for ( int i=0; i<MAX_RENDERTARGETS; ++i )
		{
			colorTargets[i] = TextureRef::Null();
			colorTargetsSlices[i] = -1;
		}
		return *this;
	}

	RenderTargetSetup& RenderTargetSetup::SetColorTarget( Uint32 index, const TextureRef &target, Int16 arraySlice/*=-1*/)
	{
		if (index >= MAX_RENDERTARGETS)
		{
			GPUAPI_HALT(  "Setting rendertarget to >MAX_RENDERTARGETS slot" );
			return *this;
		}
		// D3T - disabling this assert for now because it is firing when a texture is bound, which I currently need to copy a texture with GPU!
		//GPUAPI_ASSERT( !target || ((TEXUSAGE_RenderTarget | TEXUSAGE_BackBuffer) & GetTextureDesc(target).usage) );
		GPUAPI_ASSERT( !colorTargets[index] || colorTargets[index]==target, TXT( "Prevents unnecessary addRef/release ambiguity" ) );
		colorTargets[index] = target;
		colorTargetsSlices[index] = arraySlice;
		numColorTargets = (index >= numColorTargets ? index + 1 : numColorTargets);
		GPUAPI_ASSERT( numColorTargets <= MAX_RENDERTARGETS );
		return *this;
	}

	RenderTargetSetup& RenderTargetSetup::SetUnorderedAccessView( Uint32 index, const TextureRef &target )
	{
		if (index >= MAX_UAV)
		{
			GPUAPI_HALT("Setting UAV to >MAX_UAV slot" );
			return *this;
		}
		GPUAPI_ASSERT( !target || ((TEXUSAGE_RenderTarget | TEXUSAGE_BackBuffer) & GetTextureDesc(target).usage) );
		GPUAPI_ASSERT( !unorderedAccessViews[index] || unorderedAccessViews[index]==target, TXT( "Prevents unnecessary addRef/release ambiguity" ) );
		GPUAPI_ASSERT( !unorderedAccessViewsBuf[index], TXT( "Prevents unnecessary addRef/release ambiguity" ) );

		unorderedAccessViews[index] = target;
		numUAVs = (index >= numUAVs ? index + 1 : numUAVs);

		GPUAPI_ASSERT( numUAVs <= MAX_UAV );
		return *this;
	}

	RenderTargetSetup& RenderTargetSetup::SetUnorderedAccessView( Uint32 index, const BufferRef &target )
	{
		if (index >= MAX_UAV)
		{
			GPUAPI_HALT(  "Setting UAV to >MAX_UAV slot" );
			return *this;
		}
		GPUAPI_ASSERT( !unorderedAccessViews[index], TXT( "Prevents unnecessary addRef/release ambiguity" ) );
		GPUAPI_ASSERT( !unorderedAccessViewsBuf[index] || unorderedAccessViewsBuf[index]==target, TXT( "Prevents unnecessary addRef/release ambiguity" ) );

		unorderedAccessViewsBuf[index] = target;
		numUAVs = (index >= numUAVs ? index + 1 : numUAVs);

		GPUAPI_ASSERT( numUAVs <= MAX_UAV );
		return *this;
	}

	RenderTargetSetup& RenderTargetSetup::SetDepthStencilTarget( const TextureRef &target, Int16 sliceID/*=-1*/, Bool isReadOnly/*=false*/ )
	{
		GPUAPI_ASSERT( !depthTarget || depthTarget==target, TXT( "Prevents unnecessary addRef/release ambiguity" ) );
		GPUAPI_ASSERT( !target || (TEXUSAGE_DepthStencil & GetTextureDesc(target).usage) );
		depthTarget = target;
		depthTargetReadOnly = isReadOnly;
		depthTargetSlice = sliceID;
		return *this;
	}

	RenderTargetSetup& RenderTargetSetup::SetViewport( const ViewportDesc &viewport )
	{
		this->viewport = viewport;
		return *this;
	}

	RenderTargetSetup& RenderTargetSetup::SetViewport( Uint32 newWidth, Uint32 newHeight, Uint32 newX, Uint32 newY )
	{
		return SetViewport( ViewportDesc().Set( newWidth, newHeight, newX, newY ) );
	}

	RenderTargetSetup& RenderTargetSetup::SetViewport( Uint32 newWidth, Uint32 newHeight )
	{
		return SetViewport( newWidth, newHeight, 0, 0 );
	}

	RenderTargetSetup& RenderTargetSetup::SetViewportFromTarget( const TextureRef &target )
	{
		GPUAPI_ASSERT( target );
		//GPUAPI_ASSERT( ((TEXUSAGE_RenderTarget | TEXUSAGE_BackBuffer) & GetTextureDesc(target).usage) );
		TextureDesc desc = GetTextureDesc( target );
		return SetViewport( desc.width, desc.height, 0, 0 );
	}

	RenderTargetSetup& RenderTargetSetup::ChangeAllRefCounts( bool inc )
	{
		GPUAPI_ASSERT( numColorTargets <= MAX_RENDERTARGETS );

		for ( Uint32 i=0; i<numColorTargets; ++i )
		{
			SafeChangeRefCount( colorTargets[i], inc );
		}

		GPUAPI_ASSERT( numUAVs <= MAX_UAV );
		for ( Uint32 i=0; i<numUAVs; ++i )
		{
			SafeChangeRefCount( unorderedAccessViews[i], inc );
		}

		for ( Uint32 i=0; i<numUAVs; ++i )
		{
			SafeChangeRefCount( unorderedAccessViewsBuf[i], inc );
		}

		SafeChangeRefCount( depthTarget, inc );

#if GPUAPI_DEBUG
		for ( Uint32 i=numColorTargets; i<MAX_RENDERTARGETS; ++i )
		{
			GPUAPI_ASSERT( !colorTargets[i] );
		}
#endif

		return *this;
	}

	RenderTargetSetup& RenderTargetSetup::Reset()
	{
		numColorTargets = 0;

		for ( Uint32 i=0; i<MAX_RENDERTARGETS; ++i )
		{
			colorTargets[i] = TextureRef::Null();
			//dex++
			colorTargetsSlices[i] = -1;
			//dex--
		}

		numUAVs = 0;

		for ( Uint32 i=0; i<MAX_UAV; ++i )
		{
			unorderedAccessViews[i] = TextureRef::Null();
		}

		for ( Uint32 i=0; i<MAX_UAV; ++i )
		{
			unorderedAccessViewsBuf[i] = BufferRef::Null();
		}

		depthTarget = TextureRef::Null();
		depthTargetReadOnly = false;

		//dex++
		depthTargetSlice = -1;
		//dex--

		viewport = ViewportDesc ();
		// GNM does not like 0 sized viewports!
		viewport.width = 1;
		viewport.height = 1;

		return *this;
	}

	// ----------------------------------------------------------------------

	void SetupBlankRenderTargets()
	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetViewport( 1, 1, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );
	}

	void WaitForDepthTargetWrites( Bool flushHTile )
	{
		SDeviceData &dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		// wait for depth target writes
		if ( dd.m_StateRenderTargetSetup.depthTarget)
		{
			GpuApi::STextureData& prevData = GetDeviceData().m_Textures.Data (dd.m_StateRenderTargetSetup.depthTarget);
			const TextureDesc& desc = GetTextureDesc(dd.m_StateRenderTargetSetup.depthTarget);

			Int16 slice = dd.m_StateRenderTargetSetup.depthTargetSlice;
			Uint16 mipIndex = (slice < 0) ? 0 : CalculateMipFromSliceIndex(desc, (Uint16)slice);
			if (mipIndex < prevData.m_aliasedAsDepthStencilsSize)
			{
				const sce::Gnm::DepthRenderTarget* prevDS = &prevData.m_aliasedAsDepthStencils[mipIndex];
				gfxc.waitForGraphicsWrites( prevDS->getZWriteAddress256ByteBlocks(), 
					prevDS->getZSliceSizeInBytes() / 256, 
					sce::Gnm::kWaitTargetSlotDb, 
					sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
					sce::Gnm::kExtendedCacheActionFlushAndInvalidateDbCache,
					sce::Gnm::kStallCommandBufferParserEnable);
			}
			else
			{
				GPUAPI_HALT("CalculateMipFromSliceIndex OUT OF RANGE!");
			}

			if ( flushHTile )
			{
				gfxc.triggerEvent(sce::Gnm::kEventTypeFlushAndInvalidateDbMeta);
			}
		}
	}
	
	Bool g_deferWaitOnGraphicsWrites = true;

	void SetupRenderTargets( const RenderTargetSetup &setup )
	{
		// D3T - disabling this assert for now as a temporary rendertarget is created to copy textures on GPU
		//GPUAPI_ASSERT( IsSetupSupported(setup) );

		SDeviceData &dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		Uint32 numRts = setup.numColorTargets;
		Uint32 numUavs = setup.numUAVs;

		if ( numRts > MAX_RENDERTARGETS )
		{
			GPUAPI_HALT( "Color rendertarget count > MAX_RENDERTARGETS" );
			numRts = MAX_RENDERTARGETS;
		}

		if ( numUavs > MAX_UAV )
		{
			GPUAPI_HALT( "Unordered access view count > MAX_UAV" );
			numUavs = MAX_UAV;
		}

		// wait for depth target writes
		if ( !g_deferWaitOnGraphicsWrites )
		{
			WaitForDepthTargetWrites( false );
		}

		SetCBControlInternal( dd, gfxc, numRts > 0 ? sce::Gnm::kCbModeNormal : sce::Gnm::kCbModeDisable );

		// Color render targets
		if ( numRts > 0 || dd.m_StateRenderTargetSetup.numColorTargets > 0 )
		{
			Uint32 maxNumRTs = Red::Math::NumericalUtils::Max( numRts, dd.m_StateRenderTargetSetup.numColorTargets );

			Uint32 renderTargetMask = 0;

			for ( Uint32 rt_i=0; rt_i<maxNumRTs; ++rt_i )
			{
				if ( !g_deferWaitOnGraphicsWrites && rt_i < dd.m_StateRenderTargetSetup.numColorTargets )
				{
					const GpuApi::STextureData& prevData = dd.m_Textures.Data( dd.m_StateRenderTargetSetup.colorTargets[ rt_i ] );
					const TextureDesc& desc = GetTextureDesc(dd.m_StateRenderTargetSetup.colorTargets[ rt_i ]);

					Int16 slice = dd.m_StateRenderTargetSetup.colorTargetsSlices[rt_i];
					Uint16 mipIndex = (slice < 0) ? 0 : CalculateMipFromSliceIndex(desc, (Uint16)slice);

					if (mipIndex < prevData.m_aliasedAsRenderTargetsSize)
					{
						// D3T - this is waiting for writes for the whole texture array range (i.e. 6 slices for cubemaps) - this could be optimised
						const sce::Gnm::RenderTarget& prevRT = prevData.m_aliasedAsRenderTargets[mipIndex];
						gfxc.waitForGraphicsWrites( prevRT.getBaseAddress256ByteBlocks(), 
							prevRT.getSliceSizeInBytes() / 256, 
							sce::Gnm::kWaitTargetSlotAll, 
							sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
							sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
							sce::Gnm::kStallCommandBufferParserEnable);
					}
					else if (prevData.m_aliasedAsRenderTargetsSize == 0)
					{
						// probably created a tempRT for calling Clear
						uint64_t surfaceOffset, surfaceSize;
						if (sce::GpuAddress::kStatusSuccess == sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&surfaceOffset, &surfaceSize, &prevData.m_texture, mipIndex, (slice < 0) ? 0 : slice))
						{
							uint32_t baseAddress = ((uint64_t)prevData.m_texture.getBaseAddress() + surfaceOffset) >> 8;
							uint32_t baseSize = (uint32_t)surfaceSize;
							gfxc.waitForGraphicsWrites( 
								baseAddress, 
								baseSize,
								sce::Gnm::kWaitTargetSlotAll, 
								sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
								sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
								sce::Gnm::kStallCommandBufferParserEnable);
						}
					} 
					else 
					{
						GPUAPI_HALT("CalculateMipFromSliceIndex OUT OF RANGE!");
					}
				}

				if ( rt_i >= numRts )
				{
					gfxc.setRenderTarget( rt_i, nullptr );
				}
				else
				{
					const sce::Gnm::RenderTarget* gnmRT = nullptr;
					sce::Gnm::RenderTarget tempRT;
					TextureRef ref = setup.colorTargets[rt_i];
					if ( ref )
					{
						const TextureDesc& desc = GetTextureDesc(ref);
						GpuApi::STextureData& texData = dd.m_Textures.Data(ref);
						GPUAPI_ASSERT( texData.m_debugPath != nullptr );

						Uint32 baseSlice = 0;
						Uint32 lastSlice = texData.m_Desc.sliceNum-1;
						if (setup.colorTargetsSlices[rt_i] != -1)
							baseSlice = lastSlice = setup.colorTargetsSlices[rt_i];

						Uint16 mipIndex = CalculateMipFromSliceIndex(desc, baseSlice);
						Uint16 arraySlice = CalculateArrayIndexFromSlice(desc, baseSlice);
						Uint16 lastArraySlice = CalculateArrayIndexFromSlice(desc, lastSlice);
						if (texData.m_aliasedAsRenderTargets == nullptr)
						{
							// we need this case to call StretchRect on non RenderTarget resource
							Int32 res = tempRT.initFromTexture(&texData.m_texture, mipIndex);
							if (res == 0)
							{
								tempRT.setArrayView(arraySlice, lastArraySlice);
								gnmRT = &tempRT;
								texData.EncodeBoundData(FrameIndex(), mipIndex);
							}
							else
							{
								GPUAPI_HALT("Failed to create renderTarget from Texture!: 0x%08x", res);
							}
						}
						else if (mipIndex < texData.m_aliasedAsRenderTargetsSize)
						{
							GPUAPI_ASSERT ((desc.type == TEXTYPE_CUBE) || (arraySlice < desc.sliceNum && lastArraySlice < desc.sliceNum), TXT("Array Slice is out of range!"));
							GPUAPI_ASSERT ((desc.type != TEXTYPE_CUBE) || (arraySlice < (desc.sliceNum * 6) && lastArraySlice < (desc.sliceNum * 6)), TXT("Array Slice is out of range!"));
							gnmRT = &texData.m_aliasedAsRenderTargets[mipIndex];
							texData.m_aliasedAsRenderTargets[mipIndex].setArrayView(arraySlice, lastArraySlice);
							texData.EncodeBoundData(FrameIndex(), mipIndex);

							if (texData.m_aliasedAsRenderTargets[mipIndex].getCmaskFastClearEnable() && texData.m_needsDecompress)
							{
								gfxc.setCmaskClearColor(rt_i, (Uint32*)&texData.m_clearColor);
							}

							if ( desc.usage & TEXUSAGE_BackBuffer )
							{
								WaitUntilBackBufferReady();
							}
						}
						else
						{
							GPUAPI_HALT ("CalculateMipFromSliceIndex OUT OF RANGE!");
						}
					}

					gfxc.setRenderTarget( rt_i, gnmRT );

					renderTargetMask |= ( 15 << ( rt_i * 4 ) );
				}
			}

            // D3T - should this be done in gpuApiRenderState?
			//gfxc.setRenderTargetMask( renderTargetMask );
		}

		// UAV
		{
			GPUAPI_ASSERT( numUavs <= MAX_UAV );
			for ( Uint32 rt_i=0; rt_i<numUavs; ++rt_i )
			{
				GPUAPI_HALT("UAVs NOT IMPLEMENTED");
				/*TextureRef ref = rt_i<numUavs ? setup.unorderedAccessViews[rt_i] : TextureRef::Null();
				BufferRef bRef = rt_i<numUavs ? setup.unorderedAccessViewsBuf[rt_i] : BufferRef::Null();

				uav[rt_i] = (ref ? GetD3DUnorderedAccessView(ref) : ( bRef ? (GetDeviceData().m_Buffers.Data(bRef).m_pUnorderedAccessView) : NULL ) );
				GPUAPI_ASSERT( (NULL == uav[rt_i]) == ref.isNull() && (NULL == uav[rt_i]) == bRef.isNull() );*/
			}
		}

		// Depth render targets
		{
			const sce::Gnm::DepthRenderTarget* gnmDS = nullptr;
			if ( setup.depthTarget )
			{
				GpuApi::STextureData& texData = GetDeviceData().m_Textures.Data( setup.depthTarget );
				const TextureDesc& desc = GetTextureDesc(setup.depthTarget);

				Uint32 baseSlice = 0;
				Uint32 lastSlice = texData.m_Desc.sliceNum-1;
				if (setup.depthTargetSlice != -1)
					baseSlice = lastSlice = setup.depthTargetSlice;

				Uint16 mipIndex = CalculateMipFromSliceIndex(desc, baseSlice);
				Uint16 arraySlice = CalculateArrayIndexFromSlice(desc, baseSlice);
				Uint16 lastArraySlice = CalculateArrayIndexFromSlice(desc, lastSlice);
				if (mipIndex < texData.m_aliasedAsDepthStencilsSize)
				{
					GPUAPI_ASSERT ((desc.type == TEXTYPE_CUBE) || (arraySlice < desc.sliceNum && lastArraySlice < desc.sliceNum), TXT("Array Slice is out of range!"));
					GPUAPI_ASSERT ((desc.type != TEXTYPE_CUBE) || (arraySlice < (desc.sliceNum * 6) && lastArraySlice < (desc.sliceNum * 6)), TXT("Array Slice is out of range!"));
					gnmDS = &texData.m_aliasedAsDepthStencils[mipIndex];
					texData.m_aliasedAsDepthStencils[mipIndex].setArrayView(arraySlice, lastArraySlice);
					texData.EncodeBoundData(FrameIndex(), mipIndex);

					if (gnmDS->getHtileAccelerationEnable())
					{
						gfxc.setDepthClearValue(texData.m_clearDepth);
					}
				}
				else
				{
					GPUAPI_HALT ("CalculateMipFromSliceIndex OUT OF RANGE!");
				}
			}

			gfxc.setDepthRenderTarget( gnmDS );
		}

		// Viewport
		{
			ViewportDesc viewport = setup.viewport;
			gfxc.setupScreenViewport( viewport.x, viewport.y, viewport.x + viewport.width, viewport.y + viewport.height, Red::Math::NumericalUtils::Abs( viewport.maxZ - viewport.minZ ), viewport.minZ );
		}

		// Update state shadow
		{
			GPUAPI_ASSERT( &dd.m_StateRenderTargetSetup != &setup );			
			RenderTargetSetup prevSetup = dd.m_StateRenderTargetSetup;	// ace_fix!!! to jest gejowe (nie mam shitu ktory operuje na non constach..
			dd.m_StateRenderTargetSetup = setup;
			dd.m_StateRenderTargetSetup.ChangeAllRefCounts( true );
			if ( prevSetup.depthTarget != setup.depthTarget )
			{
				// We are changing the currently bound depth target, so let's notify when this new one gets written to
				dd.m_depthWriteNotified = false;
			}
			prevSetup.ChangeAllRefCounts( false );						// ace_fix!!! to jest gejowe (nie mam shitu ktory operuje na non constach..
		}
	}

	void SetViewportRaw( const ViewportDesc& viewport )
	{
#ifndef RED_FINAL_BUILD
		{
			GpuApi::RenderTargetSetup rtTemp = GpuApi::GetRenderTargetSetup();
			rtTemp.viewport = viewport;
			GPUAPI_ASSERT( IsSetupSupported(rtTemp) );
		}
#endif
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;
		gfxc.setupScreenViewport( viewport.x, viewport.y, viewport.width, viewport.height, Red::Math::NumericalUtils::Abs( viewport.maxZ - viewport.minZ ), viewport.minZ );
	}

	Bool GenerateMipmaps( const TextureRef &texRef )
	{
		return false;
		//GetDeviceContext()->GenerateMips( texData.m_pShaderResourceView );
	}


	Bool IsSetupSupported( const RenderTargetSetup &setup )
	{
		// Handle special case for blank rendertargets setup

		if ( !setup.depthTarget && !setup.numColorTargets && !setup.numUAVs && 0 == setup.viewport.x && 0 == setup.viewport.y && 0 == setup.viewport.width && 0 == setup.viewport.height )
		{
			return true;
		}

		//

		if ( setup.viewport.width <= 0 || setup.viewport.height <= 0 || setup.numColorTargets > MAX_RENDERTARGETS )
		{
			return false;
		}

		// Test depth target

		if ( setup.depthTarget )
		{
			TextureDesc desc = GetTextureDesc( setup.depthTarget );
			if ( !(TEXUSAGE_DepthStencil & desc.usage ) )
			{
				return false;
			}

			//dex++: slices support
			if ( TEXTYPE_2D == desc.type )
			{
				if ( setup.depthTargetSlice != -1 ) return false;
			}
			else if ( TEXTYPE_ARRAY == desc.type )
			{
				if ( setup.depthTargetSlice < 0 ) return false;
				if ( setup.depthTargetSlice >= (Int32)desc.sliceNum ) return false;
			}
			else
			{
				return false;
			}
			//dex--
		}

		// Test color targets
		// (if backbuffer is bound, then it must be first, and the only color target)

		const bool isBackBuffColorConfig = 
			1 == setup.numColorTargets	&& 
			setup.colorTargets[0]		&& 
			(TEXTYPE_2D == GetTextureDesc(setup.colorTargets[0]).type)			&&
			(TEXUSAGE_BackBuffer & GetTextureDesc(setup.colorTargets[0]).usage);	

		if ( !isBackBuffColorConfig )
		{
			for ( Uint32 i=0; i<setup.numColorTargets; ++i )
			{
				if ( !setup.colorTargets[i] )
				{
					continue;
				}

				TextureDesc desc = GetTextureDesc( setup.colorTargets[i] );

				//dex++: reorganized check
				if ( 0 == ((TEXUSAGE_RenderTarget | TEXUSAGE_BackBuffer ) & desc.usage) )
				{
					return false;
				}

				// texture arrays are supported if the slice is valid (checked later)
				if ( (TEXTYPE_2D != desc.type) && (TEXTYPE_ARRAY != desc.type) && (TEXTYPE_CUBE != desc.type) )
				{
					return false;
				}
				//dex--
			}
		}

		// Test viewport (if it's not empty)

		if ( setup.viewport.width < 1 || setup.viewport.height < 1 )
		{
			return false;
		}

		// Test resolutions, bitdepths and viewport (whether its inside targets)

		{
			Bool rtPresent	= false;
			Uint32 rtWidth	= 0;
			Uint32 rtHeight	= 0;
			Uint32 rtBits		= 0;

			for ( Uint32 i=0; i<setup.numColorTargets; ++i )
			{
				const TextureRef &rt = setup.colorTargets[i];
				if ( !rt )
				{
					continue;
				}

				const TextureDesc &desc = GetDeviceData().m_Textures.Data( rt ).m_Desc;

				Uint32 mipWidth = desc.width;
				Uint32 mipHeight = desc.height;

				//dex++: check slice
				if ( desc.type == TEXTYPE_ARRAY )
				{
					GPUAPI_ASSERT( 1 == desc.initLevels && "Add support for mipmaps, as it's done for cubemaps" );

					// no valid slice selected
					if ( setup.colorTargetsSlices[i] < 0 || setup.colorTargetsSlices[i] >= (int)desc.sliceNum )
					{
						// MG: I'm disabling this, as I bind the whole texarray as single render target.
						//return false;
					}
				}
				else if ( desc.type == TEXTYPE_CUBE )
				{
					// no valid slice selected					
					if ( setup.colorTargetsSlices[i] < 0 || setup.colorTargetsSlices[i] >= (int)(desc.sliceNum*6*desc.initLevels) )
					{
						return false;
					}

					// calculate mip size
					const Uint32 mipIndex = CalculateCubemapMipIndexFromSlice( desc, setup.colorTargetsSlices[i] );
					GpuApi::TextureLevelDesc mipDesc;
					GpuApi::GetTextureLevelDesc( rt, mipIndex, mipDesc );
					mipWidth = mipDesc.width;
					mipHeight = mipDesc.height;
				}
				else
				{
					// slice selected for non array texture
					if ( setup.colorTargetsSlices[i] != -1 )
					{
						return false;
					}
				}
				//dex--

				if ( rtPresent )
				{
					// All rendertarget must have the same resolution and bit depth
					if ( rtWidth != mipWidth || rtHeight != mipHeight
						// ace_fix!!!!! przywrocic to i przy renderowaniu refrakcji dostosowac sie do tego shitu
							// || rtBits != Utils::GetTextureFormatPixelSize( desc.format ) 
								)
					{
						return false;
					}
				}
				else
				{
					rtPresent	= true;
					rtWidth		= mipWidth;
					rtHeight	= mipHeight;
					rtBits		= Utils::GetTextureFormatPixelSize( desc.format );
				}
			}

			Bool dtPresent  = false;
			Uint32 dtWidth	= 0;
			Uint32 dtHeight	= 0;
			if ( setup.depthTarget )
			{
				const TextureDesc &dt = GetDeviceData().m_Textures.Data( setup.depthTarget ).m_Desc;

				dtPresent	= true;
				dtWidth		= dt.width;
				dtHeight	= dt.height;

				if ( rtPresent )
				{
					// Rendertarget must have less or equal size than depth buffer
					if ( rtWidth > dtWidth || rtHeight > dtHeight )
					{
						return false;
					}
				}
			}

			if ( !rtPresent && !dtPresent )
			{
				return false;
			}

			// Viewport must fit into 
			const Uint32 viewportAreaWidth  = rtPresent ? rtWidth  : dtWidth;
			const Uint32 viewportAreaHeight = rtPresent ? rtHeight : dtHeight;
			if ( setup.viewport.x + setup.viewport.width  > viewportAreaWidth || 
				setup.viewport.y + setup.viewport.height > viewportAreaHeight )
			{
				return false;
			}
		}

		// We got here, so it's supported (at least on this level of abstraction:)
		return true;
	}

	const RenderTargetSetup& GetRenderTargetSetup()
	{
		return GetDeviceData().m_StateRenderTargetSetup;
	}

	void SetViewport( const ViewportDesc &viewport )
	{
		RenderTargetSetup rtSetup = GetRenderTargetSetup();
		rtSetup.viewport = viewport;
		SetupRenderTargets( rtSetup );
	}

	void GetViewport( ViewportDesc &outViewport )
	{
		outViewport = GetDeviceData().m_StateRenderTargetSetup.viewport;
	}

	const ViewportDesc& GetViewport()
	{
		return GetDeviceData().m_StateRenderTargetSetup.viewport;
	}

	TextureRef GetBackBufferTexture()
	{
		return GetSwapChainData().backBuffer->backBufferRef;
	}

	TextureRef GetDepthStencilTexture()
	{
		return GetSwapChainData().backBuffer->depthStencilRef;
	}
}

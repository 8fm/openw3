/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"

namespace GpuApi
{

	// ----------------------------------------------------------------------
	// RenderTargetSetup

	RenderTargetSetup::RenderTargetSetup ()
	{
		Reset();
	}

	//dex++: supported by DX10/DX11
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
	//dex--

	RenderTargetSetup& RenderTargetSetup::SetColorTarget( Uint32 index, const TextureRef &target, Int16 arraySlice/*=-1*/)
	{
		if (index >= MAX_RENDERTARGETS)
		{
			GPUAPI_HALT( "Setting rendertarget to >MAX_RENDERTARGETS slot" );
			return *this;
		}
		GPUAPI_ASSERT( !target || ((TEXUSAGE_RenderTarget | TEXUSAGE_BackBuffer) & GetTextureDesc(target).usage) );
		GPUAPI_ASSERT( !colorTargets[index] || colorTargets[index]==target, TXT( "Prevents unnecessary addRef/release ambiguity" ) );
		colorTargets[index] = target;
		//dex++
		colorTargetsSlices[index] = arraySlice;
		//dex--
		numColorTargets = (index >= numColorTargets ? index + 1 : numColorTargets);
		GPUAPI_ASSERT( numColorTargets <= MAX_RENDERTARGETS );
		return *this;
	}

	RenderTargetSetup& RenderTargetSetup::SetUnorderedAccessView( Uint32 index, const TextureRef &target )
	{
		if (index >= MAX_UAV)
		{
			GPUAPI_HALT( "Setting UAV to >MAX_UAV slot" );
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
			GPUAPI_HALT( "Setting UAV to >MAX_UAV slot" );
			return *this;
		}
		GPUAPI_ASSERT( !unorderedAccessViews[index], TXT( "Prevents unnecessary addRef/release ambiguity" ) );
		GPUAPI_ASSERT( !unorderedAccessViewsBuf[index] || unorderedAccessViewsBuf[index]==target, TXT( "Prevents unnecessary addRef/release ambiguity" ) );

		unorderedAccessViewsBuf[index] = target;
		numUAVs = (index >= numUAVs ? index + 1 : numUAVs);

		GPUAPI_ASSERT( numUAVs <= MAX_UAV );
		return *this;
	}

	//dex++: added slice support
	RenderTargetSetup& RenderTargetSetup::SetDepthStencilTarget( const TextureRef &target, Int16 sliceID/*=-1*/, Bool isReadOnly/*=false*/ )
	//dex--
	{
		GPUAPI_ASSERT( !depthTarget || depthTarget==target, TXT( "Prevents unnecessary addRef/release ambiguity" ) );
		GPUAPI_ASSERT( !target || (TEXUSAGE_DepthStencil & GetTextureDesc(target).usage) );
		depthTarget = target;
		depthTargetReadOnly = isReadOnly;
		//dex++
		depthTargetSlice = sliceID;
		//dex--
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

		return *this;
	}

	// ----------------------------------------------------------------------

	void SetupBlankRenderTargets()
	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetViewport( 0, 0, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );
	}

	void WaitForDepthTargetWrites( Bool flushHTile )
	{
		// empty
	}

#if MICROSOFT_ATG_DYNAMIC_SCALING
	Uint32 DynamicScalingGetTargetWidth(const GpuApi::TextureRef &ref)
	{
		if(ref.isNull())
			return 0;

		const GpuApi::STextureData& texData = GetDeviceData().m_Textures.Data(ref);

		return Uint32(texData.m_Desc.width);
	}
	Uint32 DynamicScalingGetTargetHeight(const GpuApi::TextureRef &ref)
	{
		if(ref.isNull())
			return 0;

		const GpuApi::STextureData& texData = GetDeviceData().m_Textures.Data(ref);

		return Uint32(texData.m_Desc.height);
	}
#endif

	//dex++: refactored to support slices
	void SetupRenderTargets( const RenderTargetSetup &setup )
	{
		GPUAPI_ASSERT( IsSetupSupported(setup) );

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

		ID3D11RenderTargetView* rts[MAX_RENDERTARGETS];
		ID3D11UnorderedAccessView* uav[MAX_UAV];
		ID3D11DepthStencilView *depth;

		// Color render targets
		if ( numRts > 0 )
		{
			for ( Uint32 rt_i=0; rt_i<numRts; ++rt_i )
			{
				TextureRef ref = rt_i<setup.numColorTargets ? setup.colorTargets[rt_i] : TextureRef::Null();
				//dex++: added array slice support, also added cube sub-frace support
				rts[rt_i] = (ref ? GetD3DRenderTargetView(ref, setup.colorTargetsSlices[rt_i]) : NULL);
				//dex--
				GPUAPI_ASSERT( (NULL == rts[rt_i]) == ref.isNull() );
			}
		}

		// UAV
		{
			GPUAPI_ASSERT( numUavs <= MAX_UAV );
			for ( Uint32 rt_i=0; rt_i<numUavs; ++rt_i )
			{
				TextureRef ref = rt_i<numUavs ? setup.unorderedAccessViews[rt_i] : TextureRef::Null();
				BufferRef bRef = rt_i<numUavs ? setup.unorderedAccessViewsBuf[rt_i] : BufferRef::Null();

				uav[rt_i] = (ref ? GetD3DUnorderedAccessView(ref) : ( bRef ? (GetDeviceData().m_Buffers.Data(bRef).m_pUnorderedAccessView) : NULL ) );
				GPUAPI_ASSERT( (NULL == uav[rt_i]) == ref.isNull() && (NULL == uav[rt_i]) == bRef.isNull() );
			}
		}

		// Depth render targets
		{
			//dex++: added slice support
			depth = (setup.depthTarget ? GetD3DDepthStencilView(setup.depthTarget, setup.depthTargetSlice, setup.depthTargetReadOnly) : NULL);
			//dex--
		}

		// Set viewport
		ViewportDesc viewport = setup.viewport;
		D3D11_VIEWPORT d3dViewport = Map( viewport );

		if ( setup.numUAVs > 0 )
		{
			GetDeviceContext()->OMSetRenderTargetsAndUnorderedAccessViews( numRts, rts, depth, 0, numUavs, uav, 0 );
		}
		else
		{
			GetDeviceContext()->OMSetRenderTargets( numRts, rts, depth );
		}

		GetDeviceContext()->RSSetViewports( 1, &d3dViewport );

		// Update state shadow
		{
			SDeviceData &dd = GetDeviceData();
			GPUAPI_ASSERT( &dd.m_StateRenderTargetSetup != &setup );			
			RenderTargetSetup prevSetup = dd.m_StateRenderTargetSetup;	// ace_fix!!! to jest gejowe (nie mam shitu ktory operuje na non constach..
			dd.m_StateRenderTargetSetup = setup;
			dd.m_StateRenderTargetSetup.ChangeAllRefCounts( true );
			prevSetup.ChangeAllRefCounts( false );						// ace_fix!!! to jest gejowe (nie mam shitu ktory operuje na non constach..
		}
	}
	//dex--

	void SetViewportRaw( const ViewportDesc& viewport )
	{
#ifndef RED_FINAL_BUILD
		{
			GpuApi::RenderTargetSetup rtTemp = GpuApi::GetRenderTargetSetup();
			rtTemp.viewport = viewport;
			RED_WARNING( IsSetupSupported(rtTemp), "RenderTargetSetup not supported." );
		}
#endif
		D3D11_VIEWPORT d3dViewport;
		d3dViewport.TopLeftX = (Float)viewport.x;
		d3dViewport.TopLeftY = (Float)viewport.y;
		d3dViewport.Width = (Float)viewport.width;
		d3dViewport.Height = (Float)viewport.height;
		d3dViewport.MinDepth = viewport.minZ;
		d3dViewport.MaxDepth = viewport.maxZ;

		GetDeviceContext()->RSSetViewports( 1, &d3dViewport );
	}

	//dex++
	Bool GenerateMipmaps( const TextureRef &texRef )
	{
		SDeviceData &dd = GetDeviceData();
		STextureData& texData = dd.m_Textures.Data(texRef);
		GetDeviceContext()->GenerateMips( texData.m_pShaderResourceView );
		return true;
	}
	//dex--

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
			GPUAPI_HALT("empty viewport or more rendertargets than allowed");
			return false;
		}

		// Test depth target

		if ( setup.depthTarget )
		{
			TextureDesc desc = GetTextureDesc( setup.depthTarget );
			if ( !(TEXUSAGE_DepthStencil & desc.usage ) )
			{
				GPUAPI_HALT("depth has no depth usage");
				return false;
			}

			//dex++: slices support
			if ( TEXTYPE_2D == desc.type )
			{
				if ( setup.depthTargetSlice != -1 ) 
				{
					GPUAPI_HALT("slice selected for 2D depth");
					return false;
				}
			}
			else if ( TEXTYPE_ARRAY == desc.type )
			{
				if ( setup.depthTargetSlice < 0 ) 
				{
					GPUAPI_HALT("invalid slice selected < 0");
					return false;
				}

				if ( setup.depthTargetSlice >= (Int32)desc.sliceNum ) 
				{
					GPUAPI_HALT("invalid slice selected > max");
					return false;
				}
			}
			else
			{
				GPUAPI_HALT("unsupported depth format");
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
					GPUAPI_HALT("rendertarget has no rendertarget or backbuffer usage");
					return false;
				}

				// texture arrays are supported if the slice is valid (checked later)
				if ( (TEXTYPE_2D != desc.type) && (TEXTYPE_ARRAY != desc.type) && (TEXTYPE_CUBE != desc.type) )
				{
					GPUAPI_HALT("unsupported rendertarget type");
					return false;
				}
				//dex--
			}
		}

		// Test viewport (if it's not empty)

		if ( setup.viewport.width < 1 || setup.viewport.height < 1 )
		{
			GPUAPI_HALT("empty viewport");
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
				if ( desc.type == TEXTYPE_2D )
				{
					const Int32 mipIndex = setup.colorTargetsSlices[i];
					if ( mipIndex > 0 )
					{
						if ( mipIndex >= (Int32)desc.initLevels )
						{
							GPUAPI_HALT("mipIndex beyond mip count");
							return false;
						}

						GpuApi::TextureLevelDesc mipDesc;
						GpuApi::GetTextureLevelDesc( rt, (Uint16)mipIndex, mipDesc );
						mipWidth = mipDesc.width;
						mipHeight = mipDesc.height;
					}
				}
				else if ( desc.type == TEXTYPE_ARRAY )
				{
					GPUAPI_ASSERT( 1 == desc.initLevels && "Add support for mipmaps, as it's done for cubemaps" );

					// no valid slice selected
					if ( setup.colorTargetsSlices[i] < 0 || (Uint16)setup.colorTargetsSlices[i] >= (int)desc.sliceNum )
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
						GPUAPI_HALT("invalid slice selection for cubes");
						return false;
					}

					// calculate mip size
					const Uint16 mipIndex = CalculateCubemapMipIndexFromSlice( desc, (Uint16)setup.colorTargetsSlices[i] );
					GpuApi::TextureLevelDesc mipDesc;
					GpuApi::GetTextureLevelDesc( rt, mipIndex, mipDesc );
					mipWidth = mipDesc.width;
					mipHeight = mipDesc.height;
				}
				else
				{
					//GPUAPI_ASSERT( 1 == desc.initLevels && "Add support for mipmaps (mipWidth, mipHeight)" );

					// slice selected for non array texture
					if ( setup.colorTargetsSlices[i] != -1 )
					{
						GPUAPI_HALT("slice selected for non array texture");
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
						GPUAPI_HALT("slice selected for non array texture");
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
						GPUAPI_HALT("rendertarget bigger than depth buffer");
						return false;
					}
				}
			}

			if ( !rtPresent && !dtPresent )
			{
				GPUAPI_HALT("no rendertarget and no depthstencil");
				return false;
			}

			// Viewport must fit into 
			const Uint32 viewportAreaWidth  = rtPresent ? rtWidth  : dtWidth;
			const Uint32 viewportAreaHeight = rtPresent ? rtHeight : dtHeight;
			if ( setup.viewport.x + setup.viewport.width  > viewportAreaWidth || 
				 setup.viewport.y + setup.viewport.height > viewportAreaHeight )
			{
				GPUAPI_HALT("viewport doesn't fit into rendertarget");
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
		return GetDeviceData().m_BackBuffer;
	}

	TextureRef GetDepthStencilTexture()
	{
		return GetDeviceData().m_DepthStencil;
	}

}

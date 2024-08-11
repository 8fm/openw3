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
			GPUAPI_HALT( "Setting rendertarget to >MAX_RENDERTARGETS slot" );
			return *this;
		}
		GPUAPI_ASSERT( !target || ((TEXUSAGE_RenderTarget | TEXUSAGE_BackBuffer) & GetTextureDesc(target).usage) );
		GPUAPI_ASSERT( !colorTargets[index], TXT( "Prevents unnecessary addRef/release ambiguity" ) );
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
			GPUAPI_HALT( "Setting UAV to >MAX_UAV slot" );
			return *this;
		}
		GPUAPI_ASSERT( !target || ((TEXUSAGE_RenderTarget | TEXUSAGE_BackBuffer) & GetTextureDesc(target).usage) );
		GPUAPI_ASSERT( !unorderedAccessViews[index], TXT( "Prevents unnecessary addRef/release ambiguity" ) );
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
		GPUAPI_ASSERT( !unorderedAccessViewsBuf[index], TXT( "Prevents unnecessary addRef/release ambiguity" ) );

		unorderedAccessViewsBuf[index] = target;
		numUAVs = (index >= numUAVs ? index + 1 : numUAVs);

		GPUAPI_ASSERT( numUAVs <= MAX_UAV );
		return *this;
	}

	RenderTargetSetup& RenderTargetSetup::SetDepthStencilTarget( const TextureRef &target, Int16 sliceID/*=-1*/, Bool isReadOnly/*=false*/ )
	{
		GPUAPI_ASSERT( !depthTarget, TXT( "Prevents unnecessary addRef/release ambiguity" ) );
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

		return *this;
	}

	// ----------------------------------------------------------------------

	void SetupBlankRenderTargets()
	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetViewport( 0, 0, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );
	}

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
		
		SDeviceData &dd = GetDeviceData();

		if ( dd.m_currentFrameBuffer != 0 )
		{
			OGL_CHK( glDeleteFramebuffers( 1, &dd.m_currentFrameBuffer ) );
			dd.m_currentFrameBuffer = 0;
		}

		// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
		GLuint frameBuffer = 0;

		if (numRts == 1)
		{
			STextureData& td = dd.m_Textures.Data( setup.colorTargets[0] );
			if ( td.m_Desc.usage & TEXUSAGE_BackBuffer )
			{
				// set backbuffer
				OGL_CHK( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
				OGL_CHK( glDrawBuffer( GL_BACK ) );		// Seems that this need to be set up at least once
				OGL_CHK( glViewport( setup.viewport.x, setup.viewport.y, setup.viewport.width, setup.viewport.height ) );
				OGL_CHK( glDepthRange( setup.viewport.minZ, setup.viewport.maxZ ) );
			}
			else
			{
				OGL_CHK( glGenFramebuffers( 1, &frameBuffer ) );
				OGL_CHK( glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer ) );

				// Set the texture as our colour attachement #0
				OGL_CHK( glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, td.m_texture, 0 ) );

				// Set the list of draw buffers.
				const GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
				OGL_CHK( glDrawBuffers( 1, DrawBuffers ) ); // "1" is the size of DrawBuffers
				OGL_CHK( glReadBuffer( GL_COLOR_ATTACHMENT0 ) );
			}
		}
		else
		{
			GPUAPI_HALT("MRT not supported yet");
		}

		// Depth render targets
		{
			if( setup.depthTarget )
			{
				STextureData& ds = dd.m_Textures.Data( setup.depthTarget );

				if ( ds.m_Desc.usage & TEXUSAGE_BackBufferDepth )
				{
					GPUAPI_ASSERT( setup.numColorTargets == 1 && dd.m_Textures.Data( setup.colorTargets[0] ).m_Desc.usage & TEXUSAGE_BackBuffer );
				}
				else
				if ( ds.m_Desc.usage & TEXUSAGE_DepthStencil )
				{
					if ( frameBuffer == 0 )
					{
						OGL_CHK( glGenFramebuffers( 1, &frameBuffer ) );
						OGL_CHK( glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer ) );
					}

					// Set the texture as our depth attachment
					OGL_CHK( glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ds.m_texture, 0 ) );
					
					// If its one of stencil buffers, attach stencil too
					if( ds.m_Desc.format == TEXFMT_D24S8 || ds.m_Desc.format == TEXFMT_D24FS8 )
					{
						OGL_CHK( glFramebufferTexture( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, ds.m_texture, 0 ) );
					}

				}
				else
				{
					GPUAPI_HALT("cannot be bound as depth-stencil");
				}
			}
		}

		// Always check that our framebuffer is ok
		if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
		{
			GPUAPI_HALT( "framebuffer setup error" );
		}

		glViewport( setup.viewport.x, setup.viewport.y, setup.viewport.width, setup.viewport.height );
		glDepthRange( setup.viewport.minZ, setup.viewport.maxZ );

		dd.m_currentFrameBuffer = frameBuffer;

		//GLuint rendertargets[MAX_RENDERTARGETS];

		//ID3D11RenderTargetView* rts[MAX_RENDERTARGETS];
		//ID3D11UnorderedAccessView* uav[MAX_UAV];
		//ID3D11DepthStencilView *depth;

		//// Color render targets
		//if ( numRts > 0 )
		//{
		//	for ( Uint32 rt_i=0; rt_i<numRts; ++rt_i )
		//	{
		//		TextureRef ref = rt_i<setup.numColorTargets ? setup.colorTargets[rt_i] : TextureRef::Null();
		//		//dex++: added array slice support, also added cube sub-frace support
		//		rts[rt_i] = (ref ? GetD3DRenderTargetView(ref, setup.colorTargetsSlices[rt_i]) : NULL);
		//		//dex--
		//		GPUAPI_ASSERT( (NULL == rts[rt_i]) == ref.isNull() );
		//	}
		//}

		//// UAV
		//{
		//	GPUAPI_ASSERT( numUavs <= MAX_UAV );
		//	for ( Uint32 rt_i=0; rt_i<numUavs; ++rt_i )
		//	{
		//		TextureRef ref = rt_i<numUavs ? setup.unorderedAccessViews[rt_i] : TextureRef::Null();
		//		BufferRef bRef = rt_i<numUavs ? setup.unorderedAccessViewsBuf[rt_i] : BufferRef::Null();

		//		uav[rt_i] = (ref ? GetD3DUnorderedAccessView(ref) : ( bRef ? (GetDeviceData().m_Buffers.Data(bRef).m_pUnorderedAccessView) : NULL ) );
		//		GPUAPI_ASSERT( (NULL == uav[rt_i]) == ref.isNull() && (NULL == uav[rt_i]) == bRef.isNull() );
		//	}
		//}

		//// Depth render targets
		//{
		//	//dex++: added slice support
		//	depth = (setup.depthTarget ? GetD3DDepthStencilView(setup.depthTarget, setup.depthTargetSlice, setup.depthTargetReadOnly) : NULL);
		//	//dex--
		//}

		//// Set viewport
		//ViewportDesc viewport = setup.viewport;
		//D3D11_VIEWPORT d3dViewport = Map( viewport );

		//if ( setup.numUAVs > 0 )
		//{
		//	GetDeviceContext()->OMSetRenderTargetsAndUnorderedAccessViews( numRts, rts, depth, 0, numUavs, uav, 0 );
		//}
		//else
		//{
		//	GetDeviceContext()->OMSetRenderTargets( numRts, rts, depth );
		//}

		//GetDeviceContext()->RSSetViewports( 1, &d3dViewport );

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

	void SetViewportRaw( const ViewportDesc& viewport )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

//#ifndef RED_FINAL_BUILD
//		{
//			GpuApi::RenderTargetSetup rtTemp = GpuApi::GetRenderTargetSetup();
//			rtTemp.viewport = viewport;
//			GPUAPI_ASSERT( IsSetupSupported(rtTemp) );
//		}
//#endif
//		D3D11_VIEWPORT d3dViewport;
//		d3dViewport.TopLeftX = (Float)viewport.x;
//		d3dViewport.TopLeftY = (Float)viewport.y;
//		d3dViewport.Width = (Float)viewport.width;
//		d3dViewport.Height = (Float)viewport.height;
//		d3dViewport.MinDepth = viewport.minZ;
//		d3dViewport.MaxDepth = viewport.maxZ;
//
//		GetDeviceContext()->RSSetViewports( 1, &d3dViewport );
	}

	Bool GenerateMipmaps( const TextureRef &texRef )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//SDeviceData &dd = GetDeviceData();
		//STextureData& texData = dd.m_Textures.Data(texRef);
		//GetDeviceContext()->GenerateMips( texData.m_pShaderResourceView );

		return false;
	}

	void StretchRect( const TextureRef &sourceRef, const Rect& sourceRect, const TextureRef &destRef, const Rect& destRect )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//SDeviceData &dd = GetDeviceData();

		//D3D11_BOX sourceBox;
		//sourceBox.left = sourceRect.left;
		//sourceBox.top = sourceRect.top;
		//sourceBox.back = 1;
		//sourceBox.right = sourceRect.right;
		//sourceBox.bottom = sourceRect.bottom;
		//sourceBox.front = 0;

		//STextureData& destTex = dd.m_Textures.Data(destRef);

		////this is a failsafe, it shouldn't happen but something is wrong with resizing (the whole viewport system is a fuckup) and it happens
		//if ( sourceRect.right - sourceRect.left > static_cast<Int64>(destTex.m_Desc.width) )
		//{
		//	sourceBox.right = sourceBox.left + destTex.m_Desc.width;
		//}
		//if ( sourceRect.bottom - sourceRect.top > static_cast<Int64>(destTex.m_Desc.height) )
		//{
		//	sourceBox.bottom = sourceBox.top + destTex.m_Desc.height;
		//}

		//GetDeviceContext()->CopySubresourceRegion(dd.m_Textures.Data(destRef).m_pTexture, 0, destRect.left, destRect.top, 0, dd.m_Textures.Data(sourceRef).m_pTexture, 0, &sourceBox);
	}

	void CopyTextureArraySubresource( const TextureRef &sourceRef, const TextureRef &destRef, const Rect& sourceRect, Uint32 sourceSlice, Uint32 dstSlice, Uint32 mipLevel )
	{
		//SDeviceData &dd = GetDeviceData();

		//D3D11_BOX sourceBox;
		//sourceBox.left = sourceRect.left;
		//sourceBox.top = sourceRect.top;
		//sourceBox.back = 1;
		//sourceBox.right = sourceRect.right;
		//sourceBox.bottom = sourceRect.bottom;
		//sourceBox.front = 0;

		//Uint32 sourceSubResource = D3D11CalcSubresource( mipLevel, sourceSlice, dd.m_Textures.Data(sourceRef).m_Desc.initLevels );
		//Uint32 destSubResource = D3D11CalcSubresource( mipLevel, dstSlice, dd.m_Textures.Data(destRef).m_Desc.initLevels );

		//GetDeviceContext()->CopySubresourceRegion(dd.m_Textures.Data(destRef).m_pTexture, destSubResource, sourceRect.left, sourceRect.top, 0, dd.m_Textures.Data(sourceRef).m_pTexture, sourceSubResource, &sourceBox);
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
					GPUAPI_ASSERT( 1 == desc.initLevels && "Add support for mipmaps (mipWidth, mipHeight)" );

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
		return GetDeviceData().m_BackBuffer;
	}

	TextureRef GetDepthStencilTexture()
	{
		return GetDeviceData().m_DepthStencil;
	}

}

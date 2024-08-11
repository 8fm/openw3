/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../redSystem/crt.h"

//HACK DX10 global constant files should be included
#define VSC_Global_First 0
#define VSC_Global_Last 7

#define VSC_Custom_First 30
#define VSC_Custom_Last 48

#define PSC_Custom_First 58
#define PSC_Custom_Last 100

namespace GpuApi
{
	// ----------------------------------------------------------------------

	Bool		g_IsInsideRenderBlock = false;
	static const Int32 g_drawPrimitiveUPBufferSize = 3 * 1024 * 1024;

	// ----------------------------------------------------------------------

	void BeginRender()
	{
		GPUAPI_ASSERT( !g_IsInsideRenderBlock );
		g_IsInsideRenderBlock = true;

		SDeviceData &dd = GetDeviceData();

		// Maintain one big vertex and index buffer to mimic Draw[Indexed][Instance]PrimitiveUP functionalities of DX9
		{
			if ( dd.m_drawPrimitiveUPVertexBuffer.isNull() )
			{
				// Create for the first time
				dd.m_drawPrimitiveUPVertexBuffer = GpuApi::CreateBuffer( g_drawPrimitiveUPBufferSize, BCC_Vertex, BUT_Dynamic, BAF_CPUWrite );
				GpuApi::SetBufferDebugPath(dd.m_drawPrimitiveUPVertexBuffer, "drawPrimitiveUP_VB");
			}

			if ( dd.m_drawPrimitiveUPIndexBuffer.isNull() )
			{
				// Create for the first time
				dd.m_drawPrimitiveUPIndexBuffer = GpuApi::CreateBuffer( g_drawPrimitiveUPBufferSize, BCC_Index16Bit, BUT_Dynamic, BAF_CPUWrite );
				GpuApi::SetBufferDebugPath(dd.m_drawPrimitiveUPIndexBuffer, "drawPrimitiveUP_IB");
			}
		}
		
		dd.m_NumConstantBufferUpdates = 0;
	}

	void EndRender()
	{
		GPUAPI_ASSERT( g_IsInsideRenderBlock );
		g_IsInsideRenderBlock = false;

		GpuApi::SetupBlankRenderTargets();

		SDeviceData &dd = GetDeviceData();
		dd.m_DeviceUsageStats.m_NumConstantBufferUpdates = dd.m_NumConstantBufferUpdates;
	}

	static Bool printHistogram = false;

	void Present( const Rect* sourceRect, const Rect* destRect, const SwapChainRef& swapChain, Bool useVsync, Uint32 vsyncThreshold )
	{
		// the rects are not used since DX10, no support
		RED_UNUSED( sourceRect );
		RED_UNUSED( destRect );

		//GetDevice()->Present( sourceRect, destRect, window, dirtyRegion );

		SDeviceData &dd = GetDeviceData();

#ifdef _DEBUG
		if ( printHistogram )
		{
			GPUAPI_LOG( TXT( "VS histogram:" ) );
			for (Uint32 i = 0; i<256; i++)
			{
				GPUAPI_LOG( TXT( "\t%u: %u" ), i, dd.m_VSConstantsDebugHistogram[i] );
			}
			GPUAPI_LOG( TXT( "PS histogram:" ) );
			for (Uint32 i = 0; i<256; i++)
			{
				GPUAPI_LOG( TXT( "\t%u: %u" ), i, dd.m_PSConstantsDebugHistogram[i] );
			}
		}

		Red::System::MemorySet( &dd.m_VSConstantsDebugHistogram[0], 0, sizeof(Uint32) * 256 );
		Red::System::MemorySet( &dd.m_PSConstantsDebugHistogram[0], 0, sizeof(Uint32) * 256 );
#endif

		dd.m_GlobalVSConstantBufferChanged = true;
		dd.m_CustomVSConstantBufferChanged = true;
		dd.m_GlobalPSConstantBufferChanged = true;
		dd.m_CustomPSConstantBufferChanged = true;

		SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );
		wglMakeCurrent( scd.m_deviceContext, scd.m_renderContext );		// Kamil: This isn't needed to be called every frame - will be slow

		glFlush();
		SwapBuffers( scd.m_deviceContext );
	}

	void PresentMultiplane( const Rect* sourceRect, const Rect* destRect, const SwapChainRef& swapChain, const SwapChainRef& swapChainOverlay, Bool useVsync )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");
	}

	// clear for float render targets
	void Clear( Uint32 targetMask, const Float* colorValue, Float depthValue, Uint8 stencilValue, bool ignoreViewportAndScissor )
	{
		RED_FATAL_ASSERT( colorValue != nullptr , "There must be color pointer" );

		//always ignores viewport and scissor

		OGL_CHK( glClearColor( colorValue[0], colorValue[1], colorValue[2], colorValue[3] ) );
		OGL_CHK( glClearDepth( depthValue ) );
		OGL_CHK( glClearStencil( stencilValue ) );
		OGL_CHK( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT ) );
	}

	// Clear for byte rendertargets
	void Clear( Uint32 targetMask, const Color &colorValue, Float depthValue, Uint8 stencilValue, Bool ignoreViewportAndScissor )
	{
		//always ignores viewport and scissor

		OGL_CHK( glClearColor( colorValue.r / 255.f, colorValue.g / 255.f, colorValue.b / 255.f, colorValue.a / 255.f ) );
		OGL_CHK( glClearDepth( depthValue ) );
		OGL_CHK( glClearStencil( stencilValue ) );
		OGL_CHK( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT ) );
	}

	Bool ClearColorTarget( const TextureRef &target, const Float* colorValue )
	{
		if ( !target )
		{
			return false;
		}

		// Clear target
		STextureData& textureData = GetDeviceData().m_Textures.Data( target );
		if (textureData.m_Desc.usage & TEXUSAGE_BackBuffer)
		{
			OGL_CHK( glClearColor( colorValue[0], colorValue[1], colorValue[2], colorValue[3] ) );
			OGL_CHK( glClear( GL_COLOR_BUFFER_BIT ) );
		}
		else
		{
			OGL_CHK( glClearTexImage( textureData.m_texture, 0, GL_RGBA, GL_FLOAT, colorValue ) );
		}

		return true;
	}

	Bool ClearDepthTarget( const TextureRef &target, Float depthValue, Int32 slice )
	{
		if ( !target )
		{
			return false;
		}

		// Clear target
		STextureData& textureData = GetDeviceData().m_Textures.Data( target );

		if ( textureData.m_Desc.usage & TEXUSAGE_BackBufferDepth )
		{
			OGL_CHK( glClearDepth( depthValue ) );
			OGL_CHK( glClear( GL_DEPTH_BUFFER_BIT ) );
		}
		else
		{
			slice = (slice == -1) ? 0 : slice;

			OGL_CHK( glClearTexImage( textureData.m_texture, slice, GL_DEPTH_COMPONENT, GL_FLOAT, &depthValue ) );
		}

		return true;
	}

	void ClearStencilTarget( const TextureRef &target, Uint8 stencilValue )
	{
		if ( !target )
		{
			return;
		}

		// Clear target
		STextureData& textureData = GetDeviceData().m_Textures.Data( target );
		OGL_CHK( glClearTexImage( textureData.m_texture, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencilValue ) );
	}

	void CreateConstantBuffers()
	{
		SDeviceData &dd = GetDeviceData();
		
		if ( dd.m_GlobalVSConstantBuffer == 0 )
		{
			OGL_CHK( glGenBuffers( 1, &dd.m_GlobalVSConstantBuffer ) );
			OGL_CHK( glBindBuffer( GL_UNIFORM_BUFFER, dd.m_GlobalVSConstantBuffer ) );
			OGL_CHK( glBufferData( GL_UNIFORM_BUFFER, ( VSC_Global_Last - VSC_Global_First + 1 )  * 4 * sizeof(Float), nullptr, GL_STREAM_DRAW ) );
			OGL_CHK( glBindBufferRange( GL_UNIFORM_BUFFER, 0, dd.m_GlobalVSConstantBuffer, 0, ( VSC_Global_Last - VSC_Global_First + 1 ) * 4 * sizeof(Float) ) );
		}

		if ( dd.m_CustomVSConstantBuffer == 0 )
		{
			OGL_CHK( glGenBuffers( 1, &dd.m_CustomVSConstantBuffer ) );
			OGL_CHK( glBindBuffer( GL_UNIFORM_BUFFER, dd.m_CustomVSConstantBuffer ) );
			OGL_CHK( glBufferData( GL_UNIFORM_BUFFER, ( VSC_Custom_Last - VSC_Custom_First + 1 ) * 4 * sizeof(Float), nullptr, GL_STREAM_DRAW ) );
			OGL_CHK( glBindBufferRange( GL_UNIFORM_BUFFER, 1, dd.m_CustomVSConstantBuffer, 0, ( VSC_Custom_Last - VSC_Custom_First + 1 ) * 4 * sizeof(Float) ) );
		}

		if (dd.m_GlobalPSConstantBuffer == 0)
		{
			OGL_CHK( glGenBuffers( 1, &dd.m_GlobalPSConstantBuffer ) );
			OGL_CHK( glBindBuffer( GL_UNIFORM_BUFFER, dd.m_GlobalPSConstantBuffer ) );
			OGL_CHK( glBufferData( GL_UNIFORM_BUFFER, PSC_Custom_First * 4 * sizeof(Float), nullptr, GL_STREAM_DRAW ) );
			OGL_CHK( glBindBufferRange( GL_UNIFORM_BUFFER, 4, dd.m_GlobalPSConstantBuffer, 0, PSC_Custom_First * 4 * sizeof(Float) ) );
		}

		if (dd.m_CustomPSConstantBuffer == 0)
		{
			OGL_CHK( glGenBuffers( 1, &dd.m_CustomPSConstantBuffer ) );
			OGL_CHK( glBindBuffer( GL_UNIFORM_BUFFER, dd.m_CustomPSConstantBuffer ) );
			OGL_CHK( glBufferData( GL_UNIFORM_BUFFER, (PSC_Custom_Last - PSC_Custom_First + 1) * 4 * sizeof(Float), nullptr, GL_STREAM_DRAW ) );
			OGL_CHK( glBindBufferRange( GL_UNIFORM_BUFFER, 5, dd.m_CustomPSConstantBuffer, 0, (PSC_Custom_Last - PSC_Custom_First + 1) * 4 * sizeof(Float) ) );
		}

//		if ( dd.m_CustomCSConstantBuffer == 0 )
//		{
//			D3D11_BUFFER_DESC cbDesc;
//			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
//			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//			cbDesc.ByteWidth = 32 * 4 * sizeof(Float);
//			cbDesc.MiscFlags = 0;
//			GetDevice()->CreateBuffer( &cbDesc, NULL, &dd.m_CustomCSConstantBuffer );
//#ifdef GPU_API_DEBUG_PATH
//			dd.m_CustomCSConstantBuffer->SetPrivateData( WKPDID_D3DDebugObjectName, 8, "customcs" );
//#endif
//			GetDeviceContext()->CSSetConstantBuffers( 0, 1, &dd.m_CustomCSConstantBuffer );
//		}
	}

	void BindUniformBuffersFromBindPoints()
	{
		SDeviceData &dd = GetDeviceData();
		{
			const GLuint fragmentShader = dd.m_Shaders.Data(dd.m_PixelShader).m_shader;

			const GLuint cb0index = OGL_CHK( glGetUniformBlockIndex( fragmentShader, "cb0" ) );
			const GLuint cb1index = OGL_CHK( glGetUniformBlockIndex( fragmentShader, "cb1" ) );
			const GLuint cb2index = OGL_CHK( glGetUniformBlockIndex( fragmentShader, "cb2" ) );
			const GLuint cb3index = OGL_CHK( glGetUniformBlockIndex( fragmentShader, "cb3" ) );
			const GLuint cb4index = OGL_CHK( glGetUniformBlockIndex( fragmentShader, "cb4" ) );
			const GLuint cb5index = OGL_CHK( glGetUniformBlockIndex( fragmentShader, "cb5" ) );

			if ( cb0index != GL_INVALID_INDEX )
			{
				OGL_CHK( glUniformBlockBinding( fragmentShader, cb0index, 0) );
			}

			if ( cb1index != GL_INVALID_INDEX)
			{
				OGL_CHK( glUniformBlockBinding( fragmentShader, cb1index, 1) );
			}

			if ( cb2index != GL_INVALID_INDEX)
			{
				OGL_CHK( glUniformBlockBinding( fragmentShader, cb2index, 2) );
			}

			if ( cb3index != GL_INVALID_INDEX)
			{
				OGL_CHK( glUniformBlockBinding( fragmentShader, cb3index, 3) );
			}

			if ( cb4index != GL_INVALID_INDEX)
			{
				OGL_CHK( glUniformBlockBinding( fragmentShader, cb4index, 4) );
			}

			if ( cb5index != GL_INVALID_INDEX)
			{
				OGL_CHK( glUniformBlockBinding( fragmentShader, cb5index, 5) );
			}
		}
		{
			const GLuint vertexShader = dd.m_Shaders.Data(dd.m_VertexShader).m_shader;

			const GLuint cb0index = OGL_CHK( glGetUniformBlockIndex( vertexShader, "cb0" ) );
			const GLuint cb1index = OGL_CHK( glGetUniformBlockIndex( vertexShader, "cb1" ) );
			const GLuint cb2index = OGL_CHK( glGetUniformBlockIndex( vertexShader, "cb2" ) );
			const GLuint cb3index = OGL_CHK( glGetUniformBlockIndex( vertexShader, "cb3" ) );
			const GLuint cb4index = OGL_CHK( glGetUniformBlockIndex( vertexShader, "cb4" ) );
			const GLuint cb5index = OGL_CHK( glGetUniformBlockIndex( vertexShader, "cb5" ) );

			if ( cb0index != 0xffffffff)
			{
				OGL_CHK( glUniformBlockBinding( vertexShader, cb0index, 0 + 6) );
			}

			if ( cb1index != 0xffffffff)
			{
				OGL_CHK( glUniformBlockBinding( vertexShader, cb1index, 1 + 6) );
			}

			if ( cb2index != 0xffffffff)
			{
				OGL_CHK( glUniformBlockBinding( vertexShader, cb2index, 2 + 6) );
			}

			if ( cb3index != 0xffffffff)
			{
				OGL_CHK( glUniformBlockBinding( vertexShader, cb3index, 3 + 6) );
			}

			if ( cb4index != 0xffffffff)
			{
				OGL_CHK( glUniformBlockBinding( vertexShader, cb4index, 4 + 6) );
			}

			if ( cb5index != 0xffffffff)
			{
				OGL_CHK( glUniformBlockBinding( vertexShader, cb5index, 5 + 6) );
			}
		}
	}

	void BindMainConstantBuffers()
	{
		SDeviceData &dd = GetDeviceData();

		if ( dd.m_GlobalVSConstantBuffer != 0 )
		{
			OGL_CHK( glBindBufferBase( GL_UNIFORM_BUFFER, 0 + 6, dd.m_GlobalVSConstantBuffer) );
		}

		if (dd.m_CustomVSConstantBuffer != 0)
		{
			OGL_CHK( glBindBufferBase( GL_UNIFORM_BUFFER, 1 + 6, dd.m_CustomVSConstantBuffer) );
		}

		if (dd.m_GlobalPSConstantBuffer != 0)
		{
			OGL_CHK( glBindBufferBase( GL_UNIFORM_BUFFER, 0, dd.m_GlobalPSConstantBuffer) );
		}

		if (dd.m_CustomPSConstantBuffer != 0)
		{
			OGL_CHK( glBindBufferBase( GL_UNIFORM_BUFFER, 1, dd.m_CustomPSConstantBuffer) );
		}

		// note! m_CustomCSConstantBuffer is handled in a special way, elsewhere!
	}

	void UpdateConstantBuffers(  )
	{
		SDeviceData &dd = GetDeviceData();

		if ( dd.m_GlobalVSConstantBufferChanged )
		{
			OGL_CHK( glBindBuffer( GL_UNIFORM_BUFFER, dd.m_GlobalVSConstantBuffer ) );
			OGL_CHK( glBufferSubData( GL_UNIFORM_BUFFER, 0, ( VSC_Global_Last - VSC_Global_First + 1 ) * 4 * sizeof(Float), &dd.m_VSConstants[0] ) );

			dd.m_GlobalVSConstantBufferChanged = false;
			++dd.m_NumConstantBufferUpdates;
		}

		if ( dd.m_CustomVSConstantBufferChanged )
		{
			OGL_CHK( glBindBuffer( GL_UNIFORM_BUFFER, dd.m_CustomVSConstantBuffer ) );
			OGL_CHK( glBufferSubData( GL_UNIFORM_BUFFER, 0, ( VSC_Custom_Last - VSC_Custom_First + 1 ) * 4 * sizeof(Float), &dd.m_VSConstants[VSC_Custom_First * 4] ) );

			dd.m_CustomVSConstantBufferChanged = false;
			++dd.m_NumConstantBufferUpdates;
		}

		if (dd.m_GlobalPSConstantBufferChanged)
		{
			OGL_CHK( glBindBuffer( GL_UNIFORM_BUFFER, dd.m_GlobalPSConstantBuffer ) );
			OGL_CHK( glBufferSubData( GL_UNIFORM_BUFFER, 0, PSC_Custom_First * 4 * sizeof(Float), &dd.m_PSConstants[0] ) );

			dd.m_GlobalPSConstantBufferChanged = false;
			++dd.m_NumConstantBufferUpdates;
		}

		if (dd.m_CustomPSConstantBufferChanged)
		{
			OGL_CHK( glBindBuffer( GL_UNIFORM_BUFFER, dd.m_CustomPSConstantBuffer ) );
			OGL_CHK( glBufferSubData( GL_UNIFORM_BUFFER, 0, (PSC_Custom_Last - PSC_Custom_First + 1) * 4 * sizeof(Float), &dd.m_PSConstants[PSC_Custom_First * 4] ) ); 

			dd.m_CustomPSConstantBufferChanged = false;
			++dd.m_NumConstantBufferUpdates;
		}

		OGL_CHK( glBindBuffer( GL_UNIFORM_BUFFER, 0 ) );

		//BindMainConstantBuffers();
	}

	void BindShadersInProgram()
	{
		SDeviceData &dd = GetDeviceData();

		const GLuint vertexShader = dd.m_Shaders.Data(dd.m_VertexShader).m_shader;
		const GLuint fragmentShader = dd.m_Shaders.Data(dd.m_PixelShader).m_shader;

		if (!glIsProgramPipeline(dd.m_ProgramPipeline))
		{
			OGL_CHK( glGenProgramPipelines(1, &dd.m_ProgramPipeline) );
		}

		OGL_CHK( glUseProgramStages(dd.m_ProgramPipeline, GL_VERTEX_SHADER_BIT, vertexShader) );
		OGL_CHK( glUseProgramStages(dd.m_ProgramPipeline, GL_FRAGMENT_SHADER_BIT, fragmentShader) );
		OGL_CHK( glBindProgramPipeline(dd.m_ProgramPipeline) );
	}

	void DrawPrimitive( ePrimitiveType primitive, Uint32 startVertex, Uint32 primitiveCount )
	{
		BindShadersInProgram();
		UpdateConstantBuffers();
		BindUniformBuffersFromBindPoints();

		const Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		glDrawArrays( Map(primitive), startVertex, indexNum );

		//if (dd.m_HullShader.isNull())
		//{
		//	GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );
		//}
		//else
		//{
		//	GPUAPI_ASSERT(primitive == PRIMTYPE_TriangleList || primitive == PRIMTYPE_PointList);

		//	D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
		//	if ( primitive == PRIMTYPE_PointList )
		//	{
		//		topology = D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
		//	}
		//	GetDeviceContext()->IASetPrimitiveTopology( topology );
		//}

		//GetDeviceContext()->IASetInputLayout(GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ));

		//Int32 vertCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		//GetDeviceContext()->Draw( vertCount, startVertex );
	}

	void DrawPrimitiveRaw( ePrimitiveType primitive, Uint32 startVertex, Uint32 primitiveCount )
	{
		SDeviceData &dd = GetDeviceData();

		BindShadersInProgram();
		BindUniformBuffersFromBindPoints();

		const Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		glDrawArrays( Map(primitive), startVertex, indexNum );

		//SDeviceData &dd = GetDeviceData();

		//if (dd.m_HullShader.isNull())
		//{
		//	GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );
		//}
		//else
		//{
		//	GPUAPI_ASSERT(primitive == PRIMTYPE_TriangleList || primitive == PRIMTYPE_PointList);

		//	D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
		//	if ( primitive == PRIMTYPE_PointList )
		//	{
		//		topology = D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
		//	}
		//	GetDeviceContext()->IASetPrimitiveTopology( topology );
		//}

		//Int32 vertCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		//GetDeviceContext()->Draw( vertCount, startVertex );
	}

	void DrawPrimitiveNoBuffers( ePrimitiveType primitive, Uint32 primitiveCount )
	{
		SDeviceData &dd = GetDeviceData();

		BindShadersInProgram();
		UpdateConstantBuffers();
		BindUniformBuffersFromBindPoints();

		const Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		//glVertexPointer(3, GL_FLOAT, 0, nullptr);
		//glEnableClientState(GL_VERTEX_ARRAY);

		glDrawArrays( Map(primitive), 0, indexNum );
	}

	void DrawIndexedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount )
	{
		RED_UNUSED(numVertices);
		RED_UNUSED(minIndex);

		SDeviceData &dd = GetDeviceData();

		BindShadersInProgram();
		UpdateConstantBuffers();
		BindUniformBuffersFromBindPoints();

		const Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		glDrawElements(
			Map(primitive),		// mode
			indexNum,			// count
			GL_UNSIGNED_SHORT,	// type
			(void*)0			// element array buffer offset
			);
	}

	void DrawInstancedIndexedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount, Uint32 instancesCount )
	{
		RED_UNUSED(numVertices);
		RED_UNUSED(minIndex);

		SDeviceData &dd = GetDeviceData();

		BindShadersInProgram();
		UpdateConstantBuffers();
		BindUniformBuffersFromBindPoints();

		// For "ChunkType" layouts, get an instanced version.
		// If it's not a ChunkType, then we just assume the current layout is already instanced.
		eBufferChunkType chunkType = GetChunkTypeForVertexLayout( dd.m_VertexLayout );
		if ( chunkType < BCT_Max )
		{
			chunkType = GetInstancedChunkType( chunkType );
			GpuApi::SetVertexFormatRaw( chunkType, true );
		}

		const Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		glDrawElementsInstanced(
			Map(primitive),		// mode
			indexNum,			// count
			GL_UNSIGNED_SHORT,	// type
			(void*)0,			// index buffer offset
			instancesCount		// instance count
			);
	}

	void DrawInstancedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 numVertices, Uint32 primitiveCount, Uint32 instancesCount )
	{
		SDeviceData &dd = GetDeviceData();

		BindShadersInProgram();
		UpdateConstantBuffers();
		BindUniformBuffersFromBindPoints();

		// For "ChunkType" layouts, get an instanced version.
		// If it's not a ChunkType, then we just assume the current layout is already instanced.
		eBufferChunkType chunkType = GetChunkTypeForVertexLayout( dd.m_VertexLayout );
		if ( chunkType < BCT_Max )
		{
			chunkType = GetInstancedChunkType( chunkType );
			GpuApi::SetVertexFormatRaw( chunkType, true );
		}

		const Int32 vertexCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		glDrawArraysInstanced(
			Map(primitive),		// mode
			baseVertexIndex,
			vertexCount,		// count
			instancesCount		// instance count
			);
	}

	void DrawInstancedPrimitiveNoBuffers( ePrimitiveType primitive, Uint32 vertexCount, Uint32 instancesCount )
	{
		SDeviceData &dd = GetDeviceData();

		BindShadersInProgram();
		UpdateConstantBuffers();
		BindUniformBuffersFromBindPoints();

		//Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, vertexCount );
		//glVertexPointer(3, GL_FLOAT, 0, nullptr);
		//glEnableClientState(GL_VERTEX_ARRAY);

		glDrawArraysInstanced( Map(primitive), 0, vertexCount, instancesCount );
	}

	void DrawIndexedPrimitiveRaw( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount )
	{
		RED_UNUSED(numVertices);
		RED_UNUSED(minIndex);

		SDeviceData &dd = GetDeviceData();

		BindShadersInProgram();

		const Int32 indexNum = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		glDrawElements(
			Map(primitive),		// mode
			indexNum,			// count
			GL_UNSIGNED_SHORT,	// type
			(void*)0			// element array buffer offset
			);
	}

	void DrawInstancedIndexedPrimitiveRaw( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount, Uint32 instancesCount )
	{
		SDeviceData &dd = GetDeviceData();

		BindShadersInProgram();

		// For "ChunkType" layouts, get an instanced version.
		// If it's not a ChunkType, then we just assume the current layout is already instanced.
		eBufferChunkType chunkType = GetChunkTypeForVertexLayout( dd.m_VertexLayout );
		if ( chunkType < BCT_Max )
		{
			chunkType = GetInstancedChunkType( chunkType );
			GpuApi::SetVertexFormatRaw( chunkType, true );
		}

		const Int32 vertexCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );

		glDrawArraysInstanced(
			Map(primitive),		// mode
			baseVertexIndex,
			vertexCount,		// count
			instancesCount		// instance count
			);
	}

	void DrawSystemPrimitive( ePrimitiveType primitive, Uint32 primitiveCount, eBufferChunkType vertexType, const void *vertexBuffer )
	{
		GPUAPI_ASSERT( vertexBuffer );

		SDeviceData &dd = GetDeviceData();

		BindShadersInProgram();
		UpdateConstantBuffers();
		BindUniformBuffersFromBindPoints();

		//if ( !dd.m_HullShader.isNull() && !IsControlPointPatch( primitive ) )
		//{
		//	// "Injecting" tessellation
		//	primitive = MapPrimitiveToControlPointPatch( primitive );
		//}

		const Uint32 vertexCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		const Int32 vertexDataSize = vertexCount * GetChunkTypeStride( vertexType );

		eBufferLockFlag lockType = BLF_NoOverwrite;

		if ( dd.m_currentVertexWritePosition + vertexDataSize > g_drawPrimitiveUPBufferSize )
		{
			lockType = BLF_Discard;
			dd.m_currentVertexWritePosition = 0;
		}

		// Bind vertex buffer, map it, and copy data to it
		{
			void* lockedBufferPtr = GpuApi::LockBuffer( dd.m_drawPrimitiveUPVertexBuffer, lockType, dd.m_currentVertexWritePosition, vertexDataSize );
			Red::System::MemoryCopy( lockedBufferPtr, vertexBuffer, vertexDataSize );
			GpuApi::UnlockBuffer( dd.m_drawPrimitiveUPVertexBuffer );

			const Uint32 stride = GetChunkTypeStride( vertexType );
			const Uint32 offset = dd.m_currentVertexWritePosition;
			GpuApi::BindVertexBuffers( 0, 1, &dd.m_drawPrimitiveUPVertexBuffer, &stride, &offset );

			dd.m_VertexBuffers[0] = dd.m_drawPrimitiveUPVertexBuffer;
			dd.m_VertexLayout = GetVertexLayoutForChunkType( vertexType );
		}

		//GetDeviceContext()->IASetInputLayout( GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ) );
		SetVertexFormatRaw( dd.m_VertexLayout, true );

		//GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );

		//GetDeviceContext()->Draw( vertexCount, 0 );

		glDrawArrays(
			Map(primitive),		// mode
			0,					// first
			vertexCount			// count
			);

		dd.m_currentVertexWritePosition += vertexDataSize;
		GPUAPI_ASSERT(dd.m_currentVertexWritePosition <= g_drawPrimitiveUPBufferSize);
	}

	void DrawSystemIndexedPrimitive( ePrimitiveType primitive, Uint32 minVertexIndex, Uint32 numVertices, Uint32 primitiveCount, const Uint16 *indexBuffer, eBufferChunkType vertexType, const void *vertexBuffer )
	{
		RED_UNUSED(minVertexIndex);

		GPUAPI_ASSERT( indexBuffer && vertexBuffer );

		SDeviceData &dd = GetDeviceData();

		BindShadersInProgram();
		UpdateConstantBuffers();
		BindUniformBuffersFromBindPoints();

		const Uint32 vertexCount = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		const Int32 vertexDataSize = vertexCount * GetChunkTypeStride( vertexType );

		const Int32 numIndices = MapPrimitiveCountToDrawElementCount( primitive, primitiveCount );
		const Int32 indexDataSize = numIndices * sizeof( Uint16 );

		eBufferLockFlag vertexLockType = BLF_NoOverwrite;
		eBufferLockFlag indexLockType = BLF_NoOverwrite;

		if ( dd.m_currentVertexWritePosition + vertexDataSize > g_drawPrimitiveUPBufferSize )
		{
			vertexLockType = BLF_Discard;
			dd.m_currentVertexWritePosition = 0;
		}

		if ( dd.m_currentIndexWritePosition + indexDataSize > g_drawPrimitiveUPBufferSize )
		{
			indexLockType = BLF_Discard;
			dd.m_currentIndexWritePosition = 0;
		}

		// Bind vertex buffer, map it, and copy data to it
		{
			void* lockedBufferPtr = GpuApi::LockBuffer( dd.m_drawPrimitiveUPVertexBuffer, vertexLockType, dd.m_currentVertexWritePosition, vertexDataSize );
			Red::System::MemoryCopy( lockedBufferPtr, vertexBuffer, vertexDataSize );
			GpuApi::UnlockBuffer( dd.m_drawPrimitiveUPVertexBuffer );

			const Uint32 stride = GetChunkTypeStride( vertexType );
			const Uint32 offset = dd.m_currentVertexWritePosition;
			GpuApi::BindVertexBuffers( 0, 1, &dd.m_drawPrimitiveUPVertexBuffer, &stride, &offset );

			dd.m_VertexBuffers[0] = dd.m_drawPrimitiveUPVertexBuffer;
			dd.m_VertexLayout = GetVertexLayoutForChunkType( vertexType );
		}

		// Bind index buffer, map it, and copy data to it
		{
			void* lockedBufferPtr = GpuApi::LockBuffer( dd.m_drawPrimitiveUPIndexBuffer, indexLockType, dd.m_currentIndexWritePosition, indexDataSize );
			Red::System::MemoryCopy( lockedBufferPtr, indexBuffer, indexDataSize );
			GpuApi::UnlockBuffer( dd.m_drawPrimitiveUPIndexBuffer );
			GpuApi::BindIndexBuffer( dd.m_drawPrimitiveUPIndexBuffer, dd.m_currentIndexWritePosition ); // in GL the offset has to be specified on the drawcall
			dd.m_IndexBuffer = dd.m_drawPrimitiveUPIndexBuffer;
		}

		//GetDeviceContext()->IASetInputLayout( GpuApi::GetInputLayout( dd.m_VertexLayout, dd.m_VertexShader ) );
		SetVertexFormatRaw( dd.m_VertexLayout, true );

		//GetDeviceContext()->IASetPrimitiveTopology( Map(primitive) );

		//GetDeviceContext()->Draw( vertexCount, 0 );

		glDrawElements(
			Map(primitive),		// mode
			numIndices,			// count
			GL_UNSIGNED_SHORT,	// type
			(void*)dd.m_currentIndexWritePosition			// element array buffer offset
			);

		dd.m_currentVertexWritePosition += vertexDataSize;
		dd.m_currentIndexWritePosition += indexDataSize;
		GPUAPI_ASSERT(dd.m_currentVertexWritePosition <= g_drawPrimitiveUPBufferSize);
		GPUAPI_ASSERT(dd.m_currentIndexWritePosition <= g_drawPrimitiveUPBufferSize);
	}

	void DispatchCompute( ShaderRef& computeShader, Uint32 x, Uint32 y, Uint32 z )
	{
		SDeviceData &dd = GetDeviceData();

		const GLuint computeShaderID = dd.m_Shaders.Data(computeShader).m_shader;

		if (!glIsProgramPipeline(dd.m_ProgramPipeline))
		{
			OGL_CHK( glGenProgramPipelines(1, &dd.m_ProgramPipeline) );
		}

		OGL_CHK( glUseProgramStages(dd.m_ProgramPipeline, GL_COMPUTE_SHADER_BIT, computeShaderID) );
		OGL_CHK( glBindProgramPipeline(dd.m_ProgramPipeline) );

		OGL_CHK( glDispatchCompute( x, y, z ) );

	}

	Bool ShouldDecompressBeforeSaving( const eTextureFormat format, const eTextureSaveFormat saveFormat )
	{
		//Bool shouldDecompressBecauseOfSaveFormat = saveFormat == SAVE_FORMAT_BMP || saveFormat == SAVE_FORMAT_JPG || saveFormat == SAVE_FORMAT_PNG || saveFormat == SAVE_FORMAT_TGA;

		//// list of formats that are savable to WIC in current DirectX Tex version
		//// this can be determined by looking into _DXGIToWIC function in DirectXTex lib
		//DXGI_FORMAT savableFormats[ ] = 
		//{ 
		//	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		//	DXGI_FORMAT_D32_FLOAT,
		//	DXGI_FORMAT_D16_UNORM,
		//	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		//	DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
		//	DXGI_FORMAT_R32G32B32_FLOAT,
		//	DXGI_FORMAT_R32G32B32A32_FLOAT,
		//	DXGI_FORMAT_R16G16B16A16_FLOAT,
		//	DXGI_FORMAT_R16G16B16A16_UNORM,
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	DXGI_FORMAT_B8G8R8A8_UNORM,
		//	DXGI_FORMAT_B8G8R8X8_UNORM,
		//	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
		//	DXGI_FORMAT_R10G10B10A2_UNORM,
		//	DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
		//	DXGI_FORMAT_B5G5R5A1_UNORM,
		//	DXGI_FORMAT_B5G6R5_UNORM,
		//	DXGI_FORMAT_R32_FLOAT,
		//	DXGI_FORMAT_R16_FLOAT,
		//	DXGI_FORMAT_R16_UNORM,
		//	DXGI_FORMAT_R8_UNORM,
		//	DXGI_FORMAT_A8_UNORM,
		//	DXGI_FORMAT_R1_UNORM,
		//};

		//DXGI_FORMAT dxgiFormat = Map( format );

		//Bool shouldDecompressBecauseOfTextureFormat = true;
		//for ( Uint32 i = 0; i < ARRAY_COUNT(savableFormats); ++i )
		//{
		//	if ( dxgiFormat == savableFormats[i] )
		//	{
		//		// texture format can be saved directly, no need to decompress
		//		shouldDecompressBecauseOfTextureFormat = false;
		//		break;
		//	}
		//}

		//return shouldDecompressBecauseOfSaveFormat && shouldDecompressBecauseOfTextureFormat;

		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;
	}

	Bool SaveBufferToFile( Uint32* targetBuffer, const Char* fileName, const Uint32 width, const Uint32 height, eTextureSaveFormat saveFormat, const Bool normalize /*=false*/, const Uint32 denominator /*=1*/ )
	{
		//if ( normalize )
		//{
		//	GPUAPI_ASSERT( denominator != 0 );
		//}

		//GpuApi::TextureDesc desc;
		//desc.width = width;
		//desc.height = height;
		//desc.format = TEXFMT_R8G8B8X8;
		//desc.usage = TEXUSAGE_Dynamic | TEXUSAGE_Samplable;
		//desc.initLevels = 1;
		//TextureRef texture = CreateTexture( desc, GpuApi::TEXG_System );

		//D3D11_MAPPED_SUBRESOURCE mappedTexture;
		//GPUAPI_MUST_SUCCEED( GetDeviceContext()->Map( GetDeviceData().m_Textures.Data(texture).m_pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedTexture ) );

		//// Fill texture
		//for ( Uint32 iy = 0; iy < height; ++iy )
		//{
		//	Uint8* dst = (Uint8*)mappedTexture.pData + iy * mappedTexture.RowPitch;
		//	const Uint32 *src = &targetBuffer[ iy * width * 3 ];

		//	for ( Uint32 ix = 0; ix < width; ++ix )
		//	{
		//		if ( normalize )
		//		{
		//			*(dst++) = static_cast<Uint8>( ( *(src++) ) / denominator );
		//			*(dst++) = static_cast<Uint8>( ( *(src++) ) / denominator );
		//			*(dst++) = static_cast<Uint8>( ( *(src++) ) / denominator );
		//			*(dst++) = 255;
		//		}
		//		else
		//		{
		//			*(dst++) = static_cast<Uint8>( *(src++) );
		//			*(dst++) = static_cast<Uint8>( *(src++) );
		//			*(dst++) = static_cast<Uint8>( *(src++) );
		//			*(dst++) = 255;
		//		}
		//	}
		//}

		//GetDeviceContext()->Unmap( GetDeviceData().m_Textures.Data(texture).m_pTexture, 0 );
		//
		//Bool res = SaveTextureToFile( texture, fileName, saveFormat );

		//Release( texture );

		//return res;

		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;
	}

	Bool SaveTextureToFile( const TextureRef& textureRef, const Char* fileName, const eTextureSaveFormat format )
	{
		//// save data
		//DirectX::ScratchImage* scratchImage = new DirectX::ScratchImage();
		//HRESULT captureRes = DirectX::CaptureTexture(GetDevice(), GetDeviceContext(), GetDeviceData().m_Textures.Data(textureRef).m_pTexture, *scratchImage);
		//HRESULT saveRes = S_FALSE;
		//if ( captureRes == S_OK )
		//{
		//	//! HACK - because COM component did not initialize before function SaveToFile
		//	static Bool isCoInitialized = false;
		//	if ( !isCoInitialized )
		//	{
		//		HRESULT hr = S_FALSE;
		//		switch( format )
		//		{
		//		case SAVE_FORMAT_BMP:
		//		case SAVE_FORMAT_JPG:
		//		case SAVE_FORMAT_PNG:
		//			hr = CoInitialize( NULL );
		//			break;
		//		}
		//		GPUAPI_ASSERT( hr == S_OK );
		//		isCoInitialized = true;
		//	}

		//	switch( format )
		//	{
		//	case SAVE_FORMAT_DDS:
		//		saveRes = DirectX::SaveToDDSFile(scratchImage->GetImages(), scratchImage->GetImageCount(), scratchImage->GetMetadata(), DirectX::DDS_FLAGS_NONE, fileName);
		//		break;
		//	case SAVE_FORMAT_BMP:
		//		saveRes = DirectX::SaveToWICFile(scratchImage->GetImages(), scratchImage->GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), fileName );
		//		break;
		//	case SAVE_FORMAT_JPG:
		//		saveRes = DirectX::SaveToWICFile(scratchImage->GetImages(), scratchImage->GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG), fileName );
		//		break;
		//	case SAVE_FORMAT_PNG:
		//		saveRes = DirectX::SaveToWICFile(scratchImage->GetImages(), scratchImage->GetImageCount(), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), fileName );
		//		break;
		//	}
		//}

		//if ( captureRes != S_OK || saveRes != S_OK )
		//{
		//	GPUAPI_HALT( TXT( "Texture saving failed" ) );
		//	return false;
		//}
		//scratchImage->Release();
		//return true;

		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;
	}

	Bool SaveTextureToFile( const TextureDataDesc& image, const Char* fileName, const eTextureSaveFormat saveFormat )
	{
		//Bool shouldDecompressBeforeSaving = ShouldDecompressBeforeSaving( image.format, saveFormat );
		//Uint8* decompressedData = NULL;

		//DirectX::Image img;
		//img.width = image.width;
		//img.height = image.height;

		//if ( shouldDecompressBeforeSaving )
		//{
		//	TextureDataDesc decompressedImage;
		//	decompressedImage.data = &decompressedData;
		//	decompressedImage.format = TEXFMT_R8G8B8A8;
		//	GPUAPI_ASSERT( DecompressImage( image, decompressedImage ), TXT( "Error decompressing texture before saving." ) );
		//	img.pixels = decompressedData;
		//	img.rowPitch = decompressedImage.rowPitch;
		//	img.slicePitch = decompressedImage.slicePitch;
		//	img.format = Map( decompressedImage.format );
		//}
		//else
		//{
		//	// use given data
		//	img.rowPitch = image.rowPitch;
		//	img.slicePitch = image.slicePitch;
		//	img.format = Map( image.format );
		//	img.pixels = *image.data;
		//}

		//HRESULT hr = S_FALSE;
		//switch ( saveFormat )
		//{
		//case SAVE_FORMAT_DDS:
		//	hr = DirectX::SaveToDDSFile( img, DirectX::DDS_FLAGS_NONE, fileName );
		//	break;
		//case SAVE_FORMAT_BMP:
		//	hr = DirectX::SaveToWICFile( img, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), fileName );
		//	break;
		//case SAVE_FORMAT_JPG:
		//	hr = DirectX::SaveToWICFile( img, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG), fileName );
		//	break;
		//case SAVE_FORMAT_PNG:
		//	hr = DirectX::SaveToWICFile( img, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), fileName );
		//	break;
		//case SAVE_FORMAT_TGA:
		//	hr = DirectX::SaveToTGAFile( img, fileName );
		//	break;
		//}

		//// regardless of the saving, free data buffer for decompressed texture
		//if ( shouldDecompressBeforeSaving )
		//{
		//	GPUAPI_ASSERT( decompressedData );
		//	GPU_API_FREE( GpuMemoryPool_Textures, MC_TextureData, decompressedData );
		//}

		//if ( FAILED( hr ) )
		//{
		//	if ( hr == ERROR_NOT_SUPPORTED )
		//	{
		//		GPUAPI_HALT( TXT( "Error exporting texture: format not supported." ) );
		//	}
		//	else
		//	{
		//		GPUAPI_HALT( TXT( "Error exporting texture." ) );
		//	}
		//	return false;
		//}
		//return true;

		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;
	}

	void TakeScreenshotPCImplementation( Uint32 screenshotWidth, Uint32 screenshotHeight, eTextureSaveFormat format, const Char* fileName )
	{
		//Uint32 viewportWidth = GetViewport().width;
		//Uint32 viewportHeight = GetViewport().height;

		//// get backbuffer params
		//TextureDesc desc;
		//GetTextureDesc( GetBackBufferTexture(), desc );

		//// create copy of backbuffer, we can read from it on the CPU
		//desc.usage = TEXUSAGE_Staging;
		//TextureRef backBufferCopy = CreateTexture( desc, GpuApi::TEXG_System );

		//// get appropriate d3d pointers
		//const STextureData &dstData = GetDeviceData().m_Textures.Data( backBufferCopy );
		//const STextureData &srcData = GetDeviceData().m_Textures.Data( GetBackBufferTexture() );

		//// copy data from GPU resource to resource available to CPU
		//GetDeviceContext()->CopyResource( dstData.m_pTexture, srcData.m_pTexture );

		//// create final texture, we are able to write to in on the CPU side
		//desc.width = screenshotWidth;
		//desc.height = screenshotHeight;
		//desc.usage = TEXUSAGE_Dynamic | TEXUSAGE_Samplable;
		//TextureRef finalTexture = CreateTexture( desc, GpuApi::TEXG_System );

		//// map textures
		//D3D11_MAPPED_SUBRESOURCE mappedFinalTexture;
		//GPUAPI_MUST_SUCCEED( GetDeviceContext()->Map( GetDeviceData().m_Textures.Data(finalTexture).m_pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedFinalTexture ) );
		//D3D11_MAPPED_SUBRESOURCE mappedBackBuffer;
		//GPUAPI_MUST_SUCCEED( GetDeviceContext()->Map( dstData.m_pTexture, 0, D3D11_MAP_READ, 0, &mappedBackBuffer ) );

		//Uint32 supersampledPixelsY = viewportHeight / screenshotHeight;
		//if ( supersampledPixelsY == 0 )
		//{
		//	supersampledPixelsY = 1;
		//}
		//Uint32 supersampledPixelsX = viewportWidth / screenshotWidth;
		//if ( supersampledPixelsX == 0 )
		//{
		//	supersampledPixelsX = 1;
		//}

		//Float trueSupersampledPixelsY = ((Float)screenshotHeight / (Float)viewportHeight);
		//Float trueSupersampledPixelsX = ((Float)screenshotWidth / (Float)viewportWidth );

		//// Copy data to pixel buffer
		//for ( Uint32 iy=0; iy<screenshotHeight; iy++ )
		//{
		//	Uint8 *dst = (Uint8*)mappedFinalTexture.pData + iy * mappedFinalTexture.RowPitch;

		//	for ( Uint32 ix=0; ix<screenshotWidth; ix++ )
		//	{
		//		Uint32 accumRed = 0, accumBlue = 0, accumGreen = 0;

		//		Float trueTiltX = 0.0f, trueTiltY = 0.0f;

		//		for ( Uint32 j = 0; j < supersampledPixelsY; ++j, trueTiltY += trueSupersampledPixelsY )
		//		{
		//			Uint8 *src = (Uint8*)mappedBackBuffer.pData + ((iy * viewportHeight) / screenshotHeight + (Uint32)trueTiltY) * mappedBackBuffer.RowPitch;

		//			for ( Uint32 i = 0; i < supersampledPixelsX; ++i, trueTiltX += trueSupersampledPixelsX )
		//			{
		//				Uint8 *srcX = src + 4 * ((ix * viewportWidth) / screenshotWidth + (Uint32)trueTiltX);

		//				accumRed	+= *(srcX++);
		//				accumGreen	+= *(srcX++);
		//				accumBlue	+= *(srcX++);
		//				srcX++;
		//			}
		//		}

		//		*(dst++) = (Uint8)( accumRed / (supersampledPixelsX * supersampledPixelsY ) );
		//		*(dst++) = (Uint8)( accumGreen / (supersampledPixelsX * supersampledPixelsY ) );
		//		*(dst++) = (Uint8)( accumBlue / (supersampledPixelsX * supersampledPixelsY ) );
		//		*(dst++) = 255;
		//	}
		//}

		//GetDeviceContext()->Unmap( dstData.m_pTexture, 0 );
		//GetDeviceContext()->Unmap( GetDeviceData().m_Textures.Data(finalTexture).m_pTexture, 0 );

		//SaveTextureToFile( finalTexture, fileName, format );

		//// destroy temporary textures
		//Release( backBufferCopy );
		//Release( finalTexture );

		GPUAPI_HALT("NOT IMPLEMENTED");
	}

	void TakeScreenshot( Uint32 screenshotWidth, Uint32 screenshotHeight, eTextureSaveFormat format, const Char* fileName )
	{
		TakeScreenshotPCImplementation( screenshotWidth, screenshotHeight, format, fileName );
	}

	void GrabBackBuffer( Uint32* targetBuffer, const Rect& r, const Uint32 fullHDChunks, const Uint32 chunkNumX, const Uint32 chunkNumY, const Uint32 fullHDResX )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//// Get backbuffer desc
		//GpuApi::TextureDesc desc;
		//GpuApi::GetTextureDesc( GetBackBufferTexture(), desc );

		//// create a staging copy
		//desc.usage = GpuApi::TEXUSAGE_Staging;
		//TextureRef backBufferCopy = CreateTexture( desc, GpuApi::TEXG_System );

		//// get appropriate d3d pointers
		//const STextureData &dstData = GetDeviceData().m_Textures.Data( backBufferCopy );
		//const STextureData &srcData = GetDeviceData().m_Textures.Data( GetBackBufferTexture() );

		//// copy data from GPU resource to resource available to CPU
		//GetDeviceContext()->CopyResource( dstData.m_pTexture, srcData.m_pTexture );

		//D3D11_MAPPED_SUBRESOURCE mappedBackBuffer;
		//GPUAPI_MUST_SUCCEED( GetDeviceContext()->Map( dstData.m_pTexture, 0, D3D11_MAP_READ, 0, &mappedBackBuffer ) );

		//const Uint32 height = r.bottom - r.top;
		//const Uint32 width = r.right - r.left;

		//// Copy data to pixel buffer
		//for ( Uint32 iy = 0; iy < height; ++iy )
		//{
		//	Uint8* src = (Uint8*)mappedBackBuffer.pData + iy * mappedBackBuffer.RowPitch;
		//	Uint32 pixelYPosMultiplied = ( iy * fullHDChunks + chunkNumY ) * fullHDResX;
		//	Uint32 *dst = &targetBuffer[ ( pixelYPosMultiplied + chunkNumX ) * 3 ];
		//	
		//	for ( Uint32 ix = 0; ix < width; ++ix, dst += fullHDChunks * 3 )
		//	{
		//		*(dst)		+= *(src++);
		//		*(dst + 1)	+= *(src++);
		//		*(dst + 2)	+= *(src++);
		//		src++;
		//	}
		//}
		//
		//GetDeviceContext()->Unmap( GetDeviceData().m_Textures.Data( backBufferCopy ).m_pTexture, 0 );
		//Release( backBufferCopy );
	}
		
	void ReadBackTexture2D( Float* outData, GpuApi::TextureRef dest, GpuApi::TextureRef &outputMap, Uint32 resolutionX, Uint32 resolutionY, Uint32 step )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//D3D11_MAPPED_SUBRESOURCE mappedTexture;

		//GetDeviceContext()->Map(  GetD3DTexture2D( dest ), 0, D3D11_MAP_READ, 0, &mappedTexture );

		//Float* data = (Float*)((Uint8*)mappedTexture.pData)+step;		
		//
		//if( outData && data )
		//{
		//	const Int32 numberPixels = resolutionX*resolutionY;
		//	Int32 ind;
		//	for( ind=0; ind<numberPixels; ++ind )
		//	{
		//		outData[ind] = *data;
		//		data += step+1;
		//	}
		//}
		//GetDeviceContext()->Unmap( GetD3DTexture2D(dest), 0 );
		//// and copy
		//GetDeviceContext()->CopyResource( GetD3DTexture2D(dest), GetD3DTexture2D(outputMap) );
	}

	void DumpResourceStats()
	{
#ifdef GPU_API_DEBUG_PATH
		char buffer[512];

		// Dump list of loaded textures
		Uint32 numTextures = 0;
		{
			for ( Uint32 i=0; i<(Uint32)GetDeviceData().m_Textures._MaxResCount; ++i )
			{
				if ( GetDeviceData().m_Textures.IsInUse(i) )
				{
					const STextureData& data = GetDeviceData().m_Textures.Data(i);
					Uint32 numRef = GetDeviceData().m_Textures.GetRefCount(i);

					const char* groupType = "Unknown";
					switch ( data.m_Group )
					{
					case TEXG_System: groupType = "System"; break;
					case TEXG_Generic: groupType = "Generic"; break;
					case TEXG_Streamable: groupType = "Streamable"; break;
					case TEXG_UI: groupType = "UI"; break;
					}

					sprintf_s( buffer, "Texture[%i]: ID %i, %s, %ix%i, format %i, num ref %i, source '%s'\n", numTextures, i, groupType, data.m_Desc.width, data.m_Desc.height, data.m_Desc.format, numRef, data.m_debugPath );
					OutputDebugStringA( buffer );

					++numTextures;
				}
			}
		}

		/// Dump list of loaded buffers
		Uint32 numBuffers = 0;
		{
			for ( Uint32 i=0; i<(Uint32)GetDeviceData().m_Buffers._MaxResCount; ++i )
			{
				if ( GetDeviceData().m_Buffers.IsInUse(i) )
				{
					const SBufferData& data = GetDeviceData().m_Buffers.Data(i);
					Uint32 numRef = GetDeviceData().m_Buffers.GetRefCount(i);

					const char* bufferType = "Unknown";
					switch ( data.m_Desc.category )
					{
					case BCC_Vertex: bufferType = "Vertex"; break;
					case BCC_Index16Bit: bufferType = "Index16"; break;
					case BCC_Index32Bit: bufferType = "Index32"; break;
					}

					sprintf_s( buffer, "Buffer[%i]: ID %i, %s, %i bytes, num ref %i, source '%s'\n", numBuffers, i, bufferType, data.m_Desc.size, numRef, data.m_debugPath );
					OutputDebugStringA( buffer );

					++numBuffers;
				}
			}
		}

		/// Count queries
		Uint32 numQueries = 0;
		{
			for ( Uint32 i=0; i<(Uint32)GetDeviceData().m_Queries._MaxResCount; ++i )
			{
				if ( GetDeviceData().m_Queries.IsInUse(i) )
				{
					//const SQueryData& data = GetDeviceData().m_Queries.Data(i);
					++numQueries;
				}
			}
		}

		/// Total stats
		sprintf_s( buffer, "GPU api %i textures in use\n", numTextures );
		OutputDebugStringA( buffer );
		sprintf_s( buffer, "GPU api %i buffers in use\n", numBuffers );
		OutputDebugStringA( buffer );
		sprintf_s( buffer, "GPU api %i queries in use\n", numQueries );
		OutputDebugStringA( buffer );
#endif
	}

	void SetVertexShaderConstF( Uint32 first, const Float* data, Uint32 num )
	{
		SDeviceData &dd = GetDeviceData();
		Red::System::MemoryCopy(&dd.m_VSConstants[first * 4], data, num*4*sizeof(Float));

#ifdef _DEBUG
		for (Uint32 i = 0; i<num; ++i)
		{
			dd.m_VSConstantsDebugHistogram[first+i]++;
		}
#endif

		if ( first <= VSC_Global_Last )
		{
			dd.m_GlobalVSConstantBufferChanged = true;
		}
		else
		{
			dd.m_CustomVSConstantBufferChanged = true;
		}
	}

	void SetPixelShaderConstF( Uint32 first, const Float* data, Uint32 num )
	{
		SDeviceData &dd = GetDeviceData();
		Red::System::MemoryCopy(&dd.m_PSConstants[first * 4], data, num*4*sizeof(Float));

#ifdef _DEBUG
		for (Uint32 i = 0; i<num; ++i)
		{
			dd.m_PSConstantsDebugHistogram[first+i]++;
		}
#endif

		if ( first < PSC_Custom_First )
		{
			dd.m_GlobalPSConstantBufferChanged = true;
		}
		else if ( first < PSC_Custom_Last )
		{
			dd.m_CustomPSConstantBufferChanged = true;
		}
	}

	void SetComputeShaderConstsRaw( Uint32 dataSize, const void *dataBuffer )
	{
		//SDeviceData &dd = GetDeviceData();
		//
		//if ( !dataSize || NULL == dataBuffer )
		//{
		//	return;
		//}

		//if ( NULL == dd.m_CustomCSConstantBuffer )
		//{
		//	return;
		//}

		//// Update buffer
		//{
		//#ifdef _DEBUG
		//	{
		//		D3D11_BUFFER_DESC desc;
		//		dd.m_CustomCSConstantBuffer->GetDesc( &desc );
		//		GPUAPI_ASSERT( dataSize <= desc.ByteWidth );
		//	}
		//#endif

		//	GPUAPI_ASSERT( 0 == dataSize % sizeof(Float) );

		//	D3D11_MAPPED_SUBRESOURCE mapped;
		//	GetDeviceContext()->Map(dd.m_CustomCSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
		//	Red::System::MemoryCopy(mapped.pData, dataBuffer, dataSize);
		//	GetDeviceContext()->Unmap(dd.m_CustomCSConstantBuffer, 0);
		//}

		//// Bind buffer
		//GetDeviceContext()->CSSetConstantBuffers( 0, 1, &dd.m_CustomCSConstantBuffer );
	}

	void SetShaderDebugPath( const ShaderRef& shader, const char* debugPath )
	{
#ifdef GPU_API_DEBUG_PATH
		GPUAPI_ASSERT( GetDeviceData().m_Shaders.IsInUse(shader) );
		SShaderData &data = GetDeviceData().m_Shaders.Data(shader);
		Red::System::StringCopy( data.m_debugPath, debugPath, ARRAYSIZE(data.m_debugPath) );

		Uint32 pathLen = ( Uint32 )Red::System::StringLength( data.m_debugPath );

		// Stored as IUknown, avoiding QueryInterface
		ID3D11DeviceChild* pShader = static_cast< ID3D11DeviceChild* >( data.m_pShader );

		// Destroy previous data
		pShader->SetPrivateData( WKPDID_D3DDebugObjectName, 0, NULL );

		if (pathLen > 0)
		{
			pShader->SetPrivateData( WKPDID_D3DDebugObjectName, pathLen, data.m_debugPath );
		}
#endif
	}

	//----------------------------------------------------------------------------

	const GpuApi::SDeviceUsageStats& GetDeviceUsageStats()
	{
		SDeviceData &dd = GetDeviceData();

		return dd.m_DeviceUsageStats;
	}

	void SetVsWaveLimits(Uint32 waveLimitBy16, Uint32 lateAllocWavesMinus1)
	{
	}

	void ResetVsWaveLimits()
	{
	}
}

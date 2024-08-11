/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "speedTreeRenderInterface.h"


inline CGeometryBufferGPUAPI::CGeometryBufferGPUAPI( ) 
	: m_eIndexFormat(INDEX_FORMAT_UNSIGNED_32BIT)
	, m_uiCurrentIndexBufferSize(0)
	, m_dynamic( false )
	, m_commandList( nullptr )
	, m_lockedBuffer( nullptr )
{
}


inline CGeometryBufferGPUAPI::~CGeometryBufferGPUAPI( )
{
	if ( m_commandList )
	{
		GpuApi::CancelCommandList( m_commandList );
	}
}

inline st_bool CGeometryBufferGPUAPI::CreateVertexBuffer(st_bool bDynamic, const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize)
{
	st_bool bSuccess = false;
	
	m_dynamic = bDynamic;

	if (uiNumVertices > 0 && uiVertexSize > 0)
	{
		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = pVertexData;
		m_vertexBufferRef = GpuApi::CreateBuffer( uiNumVertices * uiVertexSize, GpuApi::BCC_Vertex, m_dynamic ? GpuApi::BUT_Dynamic : GpuApi::BUT_Immutable , m_dynamic ? GpuApi::BAF_CPUWrite : 0, &bufInitData );

		if ( m_vertexBufferRef )
		{
			GpuApi::SetBufferDebugPath( m_vertexBufferRef, "SPEEDTREEVB" );
			bSuccess = true;
		}
		else
		{
			CCore::SetError("CGeometryBufferGPUAPI::CreateVertexBuffer() failed");
		}
	}

	return bSuccess;
}


inline st_bool CGeometryBufferGPUAPI::OverwriteVertices(const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize, st_uint32 uiOffset, st_uint32 uiStream)
{
	if ( uiOffset > 0 )
	{
		GPUAPI_HALT( "Unsupported usage! Actually not even supported in the SpeedTree Reference App." );
		return false;
	}

	// should have been taken care of in CGeometryBufferRI::ReplaceVertices, but be sure
	assert(pVertexData);
	assert(uiNumVertices > 0);
	assert(VertexBufferIsValid( ));

	const Uint32 byteOffset = uiOffset * uiVertexSize;
	const Uint32 bufferLen = uiNumVertices * uiVertexSize;

	const Uint32 desiredSize = ( uiOffset + uiNumVertices ) * uiVertexSize;

	// Adjust the buffer size if necessary
	{
		const GpuApi::BufferDesc& bufferDesc = GpuApi::GetBufferDesc( m_vertexBufferRef );

		if ( desiredSize > bufferDesc.size )
		{
			GpuApi::SafeRelease( m_vertexBufferRef );
			if ( !CreateVertexBuffer( m_dynamic, nullptr, uiNumVertices, uiVertexSize ) )
			{
				GPUAPI_HALT( "Failed to reallocate vertex buffer for SpeedTree!" );
				return false;
			}
		}
	}

	void* lockedBuffer;

	if(m_lockedBuffer)
	{
		lockedBuffer = m_lockedBuffer;
	}
	else
	{
		lockedBuffer = GpuApi::LockBuffer( m_vertexBufferRef, GpuApi::BLF_Discard, byteOffset, bufferLen );
	}

	if ( lockedBuffer )
	{
		Red::System::MemoryCopy( lockedBuffer, pVertexData, bufferLen );
	}
	
	if(!m_lockedBuffer)
		GpuApi::UnlockBuffer( m_vertexBufferRef );

	// Success?
	return lockedBuffer != nullptr;
}


inline void* CGeometryBufferGPUAPI::LockForWrite(st_uint32 uiBufferSize)
{
	void* pBuffer = nullptr;

	if (m_vertexBufferRef)
	{
		// Adjust the buffer size if necessary
		{
			const GpuApi::BufferDesc& bufferDesc = GpuApi::GetBufferDesc( m_vertexBufferRef );

			if (uiBufferSize > 0 && uiBufferSize > bufferDesc.size)
			{
				GpuApi::SafeRelease( m_vertexBufferRef );
				if ( !CreateVertexBuffer( m_dynamic, nullptr, uiBufferSize, 1 ) )
				{
					GPUAPI_HALT( "Failed to reallocate vertex buffer for SpeedTree!" );
					return nullptr;
				}
			}
		}

		pBuffer = GpuApi::LockBuffer( m_vertexBufferRef, GpuApi::BLF_Discard, 0, uiBufferSize );
		m_lockedBuffer = pBuffer;
	}

	return pBuffer;
}


inline st_bool CGeometryBufferGPUAPI::UnlockFromWrite(void) const
{
	st_bool bSuccess = false;

	if ( m_vertexBufferRef && m_lockedBuffer )
	{
		GpuApi::UnlockBuffer( m_vertexBufferRef );
		bSuccess = true;
		
		m_lockedBuffer = nullptr;
	}

	return bSuccess;
}


inline st_bool CGeometryBufferGPUAPI::VertexBufferIsValid() const
{
	return !m_vertexBufferRef.isNull();
}


inline st_bool CGeometryBufferGPUAPI::EnableFormat() const
{
	GpuApi::SetVertexFormatRaw( m_vertexLayout );

	return true;
}


inline st_bool CGeometryBufferGPUAPI::DisableFormat()
{
	return true;
}


inline st_bool CGeometryBufferGPUAPI::BindVertexBuffer(st_uint32 uiStream, st_uint32 uiVertexSize) const
{
	st_bool bSuccess = false;

	if ( m_commandList )
	{
		GpuApi::SubmitCommandList( m_commandList );
		m_commandList = nullptr;
	}

#ifdef _DEBUG
	if (VertexBufferIsValid( ) && IsFormatSet( ))
	{
#endif
		Uint32 offset = 0;
		GpuApi::BindVertexBuffers( uiStream, 1, &m_vertexBufferRef, &uiVertexSize, &offset );
		bSuccess = true;

#ifdef _DEBUG
	}
	else
		CCore::SetError("CGeometryBufferGPUAPI::BindVertexBuffer, vertex buffer is not valid, cannot bind");
#endif

	return bSuccess;
}


inline st_bool CGeometryBufferGPUAPI::UnBindVertexBuffer(st_uint32 /*uiStream*/)
{
	// intentionally empty

	return true;
}


inline st_bool CGeometryBufferGPUAPI::InstancingRequiresSeparateVertexStream(void)
{
	return true;
}


inline st_bool CGeometryBufferGPUAPI::SetIndexFormat(EIndexFormat eFormat)
{
	m_eIndexFormat = eFormat;

	return (m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT || m_eIndexFormat == INDEX_FORMAT_UNSIGNED_32BIT);
}


inline st_bool CGeometryBufferGPUAPI::CreateIndexBuffer(st_bool bDynamic, const void* pIndexData, st_uint32 uiNumIndices)
{
	assert(uiNumIndices > 0);
	st_uint32 c_uiNumBytes = uiNumIndices * IndexSize( );

	GpuApi::BufferInitData bufInitData;
	bufInitData.m_buffer = pIndexData;
	m_indexBufferRef = GpuApi::CreateBuffer( c_uiNumBytes, 
											( m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT ) ? GpuApi::BCC_Index16Bit : GpuApi::BCC_Index32Bit, 
											bDynamic ? GpuApi::BUT_Dynamic : GpuApi::BUT_Immutable, 
											bDynamic ? GpuApi::BAF_CPUWrite : 0, &bufInitData );
	if ( !m_indexBufferRef )
	{
		CCore::SetError("CGeometryBufferGPUAPI::CreateIndexBuffer() failed");
		return false;
	}
	else
	{
		GpuApi::SetBufferDebugPath( m_indexBufferRef, "geometryBuffer" );
	}

	m_uiCurrentIndexBufferSize = c_uiNumBytes;

	return true;
}


inline st_bool CGeometryBufferGPUAPI::OverwriteIndices(const void* pIndexData, st_uint32 uiNumIndices, st_uint32 uiOffset)
{
	st_bool bSuccess = false;

	// should have been taken care of in CGeometryBufferRI::ReplaceVertices, but be sure
	assert(pIndexData);
	assert(uiNumIndices > 0);
	assert(IndexBufferIsValid( ));

	// lock and fill the buffer
	void* pIndexBufferContents = NULL;
	const st_uint32 c_uiOffsetInBytes = uiOffset * IndexSize( );
	const st_uint32 c_uiSizeInBytes = uiNumIndices * IndexSize( );
	assert(c_uiOffsetInBytes + c_uiSizeInBytes <= m_uiCurrentIndexBufferSize);

	pIndexBufferContents = GpuApi::LockBuffer( m_indexBufferRef, GpuApi::BLF_Discard, c_uiOffsetInBytes, c_uiSizeInBytes );

	if ( pIndexBufferContents )
	{
		Red::System::MemoryCopy( pIndexBufferContents, pIndexData, c_uiSizeInBytes );
		GpuApi::UnlockBuffer( m_indexBufferRef );

		bSuccess = true;
	}

	return bSuccess;
}


inline st_bool CGeometryBufferGPUAPI::ClearIndices()
{
	GpuApi::SafeRelease( m_indexBufferRef );

	m_uiCurrentIndexBufferSize = 0;

	return true;
}


inline st_bool CGeometryBufferGPUAPI::IndexBufferIsValid() const
{
	return !m_indexBufferRef.isNull();
}


inline st_bool CGeometryBufferGPUAPI::BindIndexBuffer() const
{
	// CGeometryBufferRI class checks if the index buffer is valid before calling
	// BindIndexBuffer; no need to check twice
	GpuApi::BindIndexBuffer( m_indexBufferRef );

	return true;
}


inline st_bool CGeometryBufferGPUAPI::UnBindIndexBuffer()
{
	// It was commented by SpeedTree originally
	//DX10::Device( )->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);

	return true;
}


inline void CGeometryBufferGPUAPI::ReleaseGfxResources(void)
{
	GpuApi::SafeRelease( m_vertexBufferRef );
	GpuApi::SafeRelease( m_indexBufferRef );
	GpuApi::SafeRelease( m_vertexLayout );
}


inline st_bool CGeometryBufferGPUAPI::RenderIndexed(EPrimitiveType ePrimType, 
	st_uint32 uiStartIndex, 
	st_uint32 uiNumIndices,
	st_uint32 /*uiMinIndex*/,		// DX9
	st_uint32 /*uiNumVertices*/		// DX9
	) const
{
	st_bool bSuccess = false;

	if (uiNumIndices > 0)
	{
		GpuApi::ePrimitiveType primTypeGPUAPI = GpuApi::PRIMTYPE_Invalid;
		Uint32 numPrimitives = 0;
		switch ( ePrimType )
		{
		case PRIMITIVE_TRIANGLE_STRIP:
			primTypeGPUAPI = GpuApi::PRIMTYPE_TriangleStrip;
			numPrimitives = uiNumIndices - 2;
			break;
		case PRIMITIVE_TRIANGLES:
			primTypeGPUAPI = GpuApi::PRIMTYPE_TriangleList;
			numPrimitives = uiNumIndices / 3;
			break;
		case PRIMITIVE_POINTS:
			primTypeGPUAPI = GpuApi::PRIMTYPE_PointList;
			numPrimitives = uiNumIndices;
			break;
		case PRIMITIVE_LINE_STRIP:
			primTypeGPUAPI = GpuApi::PRIMTYPE_LineStrip;
			numPrimitives = uiNumIndices - 1;
			break;
		case PRIMITIVE_LINES:
			primTypeGPUAPI = GpuApi::PRIMTYPE_LineList;
			numPrimitives = uiNumIndices / 2;
			break;
		case PRIMITIVE_TRIANGLE_FAN:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_TRIANGLE_FAN type is not supported");
			break;
		case PRIMITIVE_QUAD_STRIP:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_QUAD_STRIP type is not supported");
			break;
		case PRIMITIVE_QUADS:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_QUAD type is not supported");
			break;
		case PRIMITIVE_LINE_LOOP:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_LINE_LOOP type is not supported");
			break;
		default:
			assert(false);
		}

		if ( primTypeGPUAPI != GpuApi::PRIMTYPE_Invalid )
		{
			GpuApi::DrawIndexedPrimitiveRaw( primTypeGPUAPI, 0, 0, 0, uiStartIndex, numPrimitives );
			bSuccess = true;
		}
	}

	return bSuccess;
}


inline st_bool CGeometryBufferGPUAPI::RenderIndexedInstanced(EPrimitiveType ePrimType,
	st_uint32 uiStartIndex,
	st_uint32 uiNumIndices,
	st_uint32 uiNumInstances,
	st_uint32 uiStartInstanceLocation) const
{
	st_bool bSuccess = false;

	if (uiNumIndices > 0)
	{
		GpuApi::ePrimitiveType primTypeGPUAPI = GpuApi::PRIMTYPE_Invalid;
		Uint32 numPrimitives = 0;
		switch ( ePrimType )
		{
		case PRIMITIVE_TRIANGLE_STRIP:
			primTypeGPUAPI = GpuApi::PRIMTYPE_TriangleStrip;
			numPrimitives = uiNumIndices - 2;
			break;
		case PRIMITIVE_TRIANGLES:
			primTypeGPUAPI = GpuApi::PRIMTYPE_TriangleList;
			numPrimitives = uiNumIndices / 3;
			break;
		case PRIMITIVE_POINTS:
			primTypeGPUAPI = GpuApi::PRIMTYPE_PointList;
			numPrimitives = uiNumIndices;
			break;
		case PRIMITIVE_LINE_STRIP:
			primTypeGPUAPI = GpuApi::PRIMTYPE_LineStrip;
			numPrimitives = uiNumIndices - 1;
			break;
		case PRIMITIVE_LINES:
			primTypeGPUAPI = GpuApi::PRIMTYPE_LineList;
			numPrimitives = uiNumIndices / 2;
			break;
		case PRIMITIVE_TRIANGLE_FAN:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_TRIANGLE_FAN type is not supported");
			break;
		case PRIMITIVE_QUAD_STRIP:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_QUAD_STRIP type is not supported");
			break;
		case PRIMITIVE_QUADS:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_QUAD type is not supported");
			break;
		case PRIMITIVE_LINE_LOOP:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_LINE_LOOP type is not supported");
			break;
		default:
			assert(false);
		}

		if ( primTypeGPUAPI != GpuApi::PRIMTYPE_Invalid )
		{
			GpuApi::DrawInstancedIndexedPrimitiveRaw( primTypeGPUAPI, 0, 0, 0, uiStartIndex, numPrimitives, uiNumInstances );
			bSuccess = true;
		}
	}

	return bSuccess;
}


inline st_bool CGeometryBufferGPUAPI::RenderArrays(EPrimitiveType ePrimType, st_uint32 uiStartVertex, st_uint32 uiNumVertices) const
{
	st_bool bSuccess = false;

#ifdef _DEBUG
	if (VertexBufferIsValid( ))
	{
#endif
		GpuApi::ePrimitiveType primTypeGPUAPI = GpuApi::PRIMTYPE_Invalid;
		Uint32 numPrimitives = 0;
		switch ( ePrimType )
		{
		case PRIMITIVE_TRIANGLE_STRIP:
			primTypeGPUAPI = GpuApi::PRIMTYPE_TriangleStrip;
			numPrimitives = uiNumVertices - 2;
			break;
		case PRIMITIVE_TRIANGLES:
			primTypeGPUAPI = GpuApi::PRIMTYPE_TriangleList;
			numPrimitives = uiNumVertices / 3;
			break;
		case PRIMITIVE_POINTS:
			primTypeGPUAPI = GpuApi::PRIMTYPE_PointList;
			numPrimitives = uiNumVertices;
			break;
		case PRIMITIVE_LINE_STRIP:
			primTypeGPUAPI = GpuApi::PRIMTYPE_LineStrip;
			numPrimitives = uiNumVertices - 1;
			break;
		case PRIMITIVE_LINES:
			primTypeGPUAPI = GpuApi::PRIMTYPE_LineList;
			numPrimitives = uiNumVertices / 2;
			break;
		case PRIMITIVE_TRIANGLE_FAN:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_TRIANGLE_FAN type is not supported");
			break;
		case PRIMITIVE_QUAD_STRIP:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_QUAD_STRIP type is not supported");
			break;
		case PRIMITIVE_QUADS:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_QUAD type is not supported");
			break;
		case PRIMITIVE_LINE_LOOP:
			CCore::SetError("CGeometryBufferGPUAPI::RenderIndexed, PRIMITIVE_LINE_LOOP type is not supported");
			break;
		default:
			assert(false);
		}

		if ( primTypeGPUAPI != GpuApi::PRIMTYPE_Invalid )
		{
			GpuApi::DrawPrimitiveRaw( primTypeGPUAPI, uiStartVertex, numPrimitives );
			bSuccess = true;
		}

#ifdef _DEBUG
	}
#endif

	return bSuccess;
}


inline st_bool CGeometryBufferGPUAPI::IsPrimitiveTypeSupported(EPrimitiveType ePrimType)
{
	return (ePrimType == PRIMITIVE_TRIANGLE_STRIP ||
		ePrimType == PRIMITIVE_TRIANGLES);
}


inline st_bool CGeometryBufferGPUAPI::IsFormatSet() const
{
	return !m_vertexLayout.isNull();
}


inline st_uint32 CGeometryBufferGPUAPI::IndexSize(void) const
{
	return (m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT) ? 2 : 4;
}


///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::CGeometryBufferDirectX11

inline CGeometryBufferDirectX11::CGeometryBufferDirectX11( ) :
	m_pVertexLayout(NULL),
	m_pVertexBuffer(NULL),
	m_pIndexBuffer(NULL),
	m_eIndexFormat(INDEX_FORMAT_UNSIGNED_32BIT),
	m_uiCurrentVertexBufferSize(0),
	m_uiCurrentIndexBufferSize(0),
	m_bDynamic(false)
{
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::~CGeometryBufferDirectX11

inline CGeometryBufferDirectX11::~CGeometryBufferDirectX11( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::CreateVertexBuffer

inline st_bool CGeometryBufferDirectX11::CreateVertexBuffer(st_bool bDynamic, const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize)
{
	st_bool bSuccess = false;

	m_bDynamic = bDynamic;

	if (uiNumVertices > 0 && uiVertexSize > 0)
	{
		assert(DX11::Device( ));

		D3D11_BUFFER_DESC sBufferDesc;
		sBufferDesc.ByteWidth = uiNumVertices * uiVertexSize;
		sBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		sBufferDesc.MiscFlags = 0;
		if (bDynamic)
		{
			sBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			sBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else
		{
			sBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			sBufferDesc.CPUAccessFlags = 0;
		}

		D3D11_SUBRESOURCE_DATA sInitData;
		sInitData.pSysMem = pVertexData;

		if (SUCCEEDED(DX11::Device( )->CreateBuffer(&sBufferDesc, pVertexData ? &sInitData : NULL, &m_pVertexBuffer)))
		{
			ST_NAME_DX11_OBJECT(m_pVertexBuffer, "speedtree vertex buffer");

			m_uiCurrentVertexBufferSize = uiNumVertices * uiVertexSize;

			bSuccess = true;
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::OverwriteVertices

inline st_bool CGeometryBufferDirectX11::OverwriteVertices(const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize, st_uint32 uiOffset, st_uint32 uiStream)
{
	ST_UNREF_PARAM(uiStream);

	st_bool bSuccess = false;

	// should have been taken care of in CGeometryBufferRI::ReplaceVertices, but be sure
	assert(pVertexData);
	assert(uiNumVertices > 0);
	assert(VertexBufferIsValid( ));

	// will this overwrite operation force us to make a new bigger vertex buffer?
	const st_uint32 c_uiOffsetInBytes = uiOffset * uiVertexSize;
	const st_uint32 c_uiSizeInBytes = uiNumVertices * uiVertexSize;
	const bool c_bResizeNeeded = (c_uiOffsetInBytes + c_uiSizeInBytes > m_uiCurrentVertexBufferSize);

	if (!c_bResizeNeeded)
	{
		D3D11_MAPPED_SUBRESOURCE sVertexBuffer;
		if (SUCCEEDED(DX11::DeviceContext( )->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sVertexBuffer)))
		{
			memcpy((st_byte*)(sVertexBuffer.pData) + c_uiOffsetInBytes, pVertexData, c_uiSizeInBytes);
			DX11::DeviceContext( )->Unmap(m_pVertexBuffer, 0);

			bSuccess = true;
		}

		bSuccess = true;
	}
	else
	{
		// if a VB resize is necessary, it's almost certainly due to an instance buffer resize; to avoid a possible reallocation
		// later of a slightly larger size, we will overestimate how much we'll need by 20%.
		ST_SAFE_RELEASE(m_pVertexBuffer);
		return CreateVertexBuffer(m_bDynamic, pVertexData, st_uint32(uiNumVertices * 1.2f), uiVertexSize);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::VertexBufferIsValid

inline st_bool CGeometryBufferDirectX11::VertexBufferIsValid(void) const
{
	return (m_pVertexBuffer != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::EnableFormat

inline st_bool CGeometryBufferDirectX11::EnableFormat(void) const
{
	assert(DX11::Device( ));

	DX11::DeviceContext( )->IASetInputLayout(m_pVertexLayout);

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::DisableFormat

inline st_bool CGeometryBufferDirectX11::DisableFormat(void)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::BindVertexBuffer

inline st_bool CGeometryBufferDirectX11::BindVertexBuffer(st_uint32 uiStream, st_uint32 uiVertexSize) const
{
	st_bool bSuccess = false;

	// unused in DX10
	(uiStream);

#ifdef _DEBUG
	if (VertexBufferIsValid( ) && IsFormatSet( ))
	{
#endif
		assert(DX11::DeviceContext( ));

		st_uint32 uiStride = uiVertexSize;
		st_uint32 uiOffset = 0;
		DX11::DeviceContext( )->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &uiStride, &uiOffset);
		bSuccess = true;

#ifdef _DEBUG
	}
	else
		CCore::SetError("CGeometryBufferDirectX11::BindVertexBuffer, vertex buffer is not valid, cannot bind");
#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::UnBindVertexBuffer

inline st_bool CGeometryBufferDirectX11::UnBindVertexBuffer(st_uint32 /*uiStream*/)
{
	// intentionally empty

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::InstancingRequiresSeparateVertexStream

inline st_bool CGeometryBufferDirectX11::InstancingRequiresSeparateVertexStream(void)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::VertexBuffer

inline ID3D11Buffer* CGeometryBufferDirectX11::VertexBuffer(void) const
{
	return m_pVertexBuffer;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::SetIndexFormat

inline st_bool CGeometryBufferDirectX11::SetIndexFormat(EIndexFormat eFormat)
{
	m_eIndexFormat = eFormat;

	return (m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT || m_eIndexFormat == INDEX_FORMAT_UNSIGNED_32BIT);
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::CreateIndexBuffer

inline st_bool CGeometryBufferDirectX11::CreateIndexBuffer(st_bool bDynamic, const void* pIndexData, st_uint32 uiNumIndices)
{
	st_bool bSuccess = false;

	assert(uiNumIndices > 0);
	assert(DX11::Device( ));
	st_uint32 c_uiNumBytes = uiNumIndices * IndexSize( );

	D3D11_BUFFER_DESC sBufferDesc;
	sBufferDesc.ByteWidth = c_uiNumBytes;
	sBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	sBufferDesc.MiscFlags = 0;
	if (bDynamic)
	{
		sBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		sBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		sBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		sBufferDesc.CPUAccessFlags = 0;
	}

	D3D11_SUBRESOURCE_DATA sInitData;
	sInitData.pSysMem = pIndexData;

	if (SUCCEEDED(DX11::Device( )->CreateBuffer(&sBufferDesc, pIndexData ? &sInitData : NULL, &m_pIndexBuffer)))
	{
		ST_NAME_DX11_OBJECT(m_pIndexBuffer, "speedtree index buffer");

		m_uiCurrentIndexBufferSize = c_uiNumBytes;

		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::OverwriteIndices

inline st_bool CGeometryBufferDirectX11::OverwriteIndices(const void* pIndexData, st_uint32 uiNumIndices, st_uint32 uiOffset)
{
	st_bool bSuccess = false;

	// should have been taken care of in CGeometryBufferRI::ReplaceVertices, but be sure
	assert(pIndexData);
	assert(uiNumIndices > 0);
	assert(IndexBufferIsValid( ));

	// will this overwrite operation force us to make a new bigger vertex buffer?
	const st_uint32 c_uiOffsetInBytes = uiOffset * IndexSize( );
	const st_uint32 c_uiSizeInBytes = uiNumIndices * IndexSize( );
	const bool c_bResizeNeeded = (c_uiOffsetInBytes + c_uiSizeInBytes > m_uiCurrentIndexBufferSize);

	if (!c_bResizeNeeded)
	{
		// lock and fill the buffer
		D3D11_MAPPED_SUBRESOURCE sIndexBuffer;
		if (SUCCEEDED(DX11::DeviceContext( )->Map(m_pIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sIndexBuffer)))
		{
			memcpy((st_byte*)(sIndexBuffer.pData) + c_uiOffsetInBytes, pIndexData, c_uiSizeInBytes);
			DX11::DeviceContext( )->Unmap(m_pIndexBuffer, 0);

			bSuccess = true;
		}
	}
	else
	{
		ST_SAFE_RELEASE(m_pIndexBuffer);
		return CreateIndexBuffer(m_bDynamic, pIndexData, uiNumIndices);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::ClearIndices

inline st_bool CGeometryBufferDirectX11::ClearIndices(void)
{
	if (m_pIndexBuffer)
	{
		m_pIndexBuffer->Release( );
		m_pIndexBuffer = NULL;
	}

	m_uiCurrentIndexBufferSize = 0;

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::IndexBufferIsValid

inline st_bool CGeometryBufferDirectX11::IndexBufferIsValid(void) const
{
	return (m_pIndexBuffer != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::BindIndexBuffer

inline st_bool CGeometryBufferDirectX11::BindIndexBuffer(void) const
{
	assert(DX11::DeviceContext( ));

	// CGeometryBufferRI class checks if the index buffer is valid before calling
	// BindIndexBuffer; no need to check twice
	DX11::DeviceContext( )->IASetIndexBuffer(m_pIndexBuffer, 
											 m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 
										     0);

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::UnBindIndexBuffer

inline st_bool CGeometryBufferDirectX11::UnBindIndexBuffer(void)
{
	assert(DX11::DeviceContext( ));

	//DX11::Device( )->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::ReleaseGfxResources

inline void CGeometryBufferDirectX11::ReleaseGfxResources(void)
{
	if (m_pVertexBuffer)
	{
		// before releasing, unbind vertex buffer if currently bound
		ID3D11Buffer* pCurrentVB = NULL;
		UINT uiUnused1 = 0, uiUnused2 = 0;
		DX11::DeviceContext( )->IAGetVertexBuffers(0, 1, &pCurrentVB, &uiUnused1, &uiUnused2);
		if (pCurrentVB == m_pVertexBuffer)
		{
			ID3D11Buffer* pNullVB = NULL;
			DX11::DeviceContext( )->IASetVertexBuffers(0, 1, &pNullVB, &uiUnused1, &uiUnused2);
		}

		// IAGetVertexBuffers adds a reference, so decrement it
		ST_SAFE_RELEASE(pCurrentVB);

		// now that it's not bound, release vertex buffer
		ST_SAFE_RELEASE(m_pVertexBuffer);
	}

	if (m_pIndexBuffer)
	{
		// before releasing, unbind index buffer if currently bound
		ID3D11Buffer* pCurrentIB = NULL;
		DXGI_FORMAT xUnused1;
		UINT uiUnused2 = 0;
		DX11::DeviceContext( )->IAGetIndexBuffer(&pCurrentIB, &xUnused1, &uiUnused2);
		if (pCurrentIB == m_pIndexBuffer)
			DX11::DeviceContext( )->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);

		// IAGetIndexBuffers adds a reference, so decrement it
		ST_SAFE_RELEASE(pCurrentIB);

		// now that it's not bound, release index buffer
		ST_SAFE_RELEASE(m_pIndexBuffer);
	}

	if (m_pVertexLayout)
	{
		// before releasing, unbind vertex layout if currently bound
		ID3D11InputLayout* pCurrentLayout = NULL;
		DX11::DeviceContext( )->IAGetInputLayout(&pCurrentLayout);
		if (pCurrentLayout == m_pVertexLayout)
			DX11::DeviceContext( )->IASetInputLayout(NULL);

		// IAGetInputLayout adds a reference, so decrement it
		ST_SAFE_RELEASE(pCurrentLayout);

		// now that it's not bound, release vertex layout
		ST_SAFE_RELEASE(m_pVertexLayout);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::RenderIndexed

inline st_bool CGeometryBufferDirectX11::RenderIndexed(EPrimitiveType ePrimType, 
													   st_uint32 uiStartIndex, 
													   st_uint32 uiNumIndices,
													   st_uint32 /*uiMinIndex*/,	// DX9
													   st_uint32 /*uiNumVertices*/	// DX9
													   ) const
{
	st_bool bSuccess = false;

	if (uiNumIndices > 0)
	{
		D3D11_PRIMITIVE_TOPOLOGY eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		switch (ePrimType)
		{
		case PRIMITIVE_TRIANGLE_STRIP:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			break;
		case PRIMITIVE_TRIANGLES:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case PRIMITIVE_POINTS:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		case PRIMITIVE_LINE_STRIP:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
		case PRIMITIVE_LINES:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case PRIMITIVE_TRIANGLE_FAN:
			CCore::SetError("CGeometryBufferDirectX11::RenderIndexed, PRIMITIVE_TRIANGLE_FAN type is not supported under the DirectX11 render interface");
			break;
		case PRIMITIVE_QUAD_STRIP:
			CCore::SetError("CGeometryBufferDirectX11::RenderIndexed, PRIMITIVE_QUAD_STRIP type is not supported under the DirectX11 render interface");
			break;
		case PRIMITIVE_QUADS:
			CCore::SetError("CGeometryBufferDirectX11::RenderIndexed, PRIMITIVE_QUADS type is not supported under the DirectX11 render interface");
			break;
		case PRIMITIVE_LINE_LOOP:
			CCore::SetError("CGeometryBufferDirectX11::RenderIndexed, PRIMITIVE_LINE_LOOP type is not supported under the DirectX11 render interface");
			break;
		default:
			assert(false);
		}

		if (eDirectX11Primitive != D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
		{
			assert(DX11::DeviceContext( ));
			DX11::DeviceContext( )->IASetPrimitiveTopology(eDirectX11Primitive);
			DX11::DeviceContext( )->DrawIndexed(uiNumIndices, uiStartIndex, 0);

			bSuccess = true;
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::RenderIndexed

inline st_bool CGeometryBufferDirectX11::RenderIndexedInstanced(EPrimitiveType ePrimType,
															    st_uint32 uiStartIndex,
																st_uint32 uiNumIndices,
															    st_uint32 uiNumInstances,
															    st_uint32 uiStartInstanceLocation) const
{
	st_bool bSuccess = false;

	if (uiNumIndices > 0)
	{
		D3D11_PRIMITIVE_TOPOLOGY eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		switch (ePrimType)
		{
		case PRIMITIVE_TRIANGLE_STRIP:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			break;
		case PRIMITIVE_TRIANGLES:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case PRIMITIVE_POINTS:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		case PRIMITIVE_LINE_STRIP:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
		case PRIMITIVE_LINES:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case PRIMITIVE_TRIANGLE_FAN:
			CCore::SetError("CGeometryBufferDirectX11::RenderIndexed, PRIMITIVE_TRIANGLE_FAN type is not supported under the DirectX11 render interface");
			break;
		case PRIMITIVE_QUAD_STRIP:
			CCore::SetError("CGeometryBufferDirectX11::RenderIndexed, PRIMITIVE_QUAD_STRIP type is not supported under the DirectX11 render interface");
			break;
		case PRIMITIVE_QUADS:
			CCore::SetError("CGeometryBufferDirectX11::RenderIndexed, PRIMITIVE_QUADS type is not supported under the DirectX11 render interface");
			break;
		case PRIMITIVE_LINE_LOOP:
			CCore::SetError("CGeometryBufferDirectX11::RenderIndexed, PRIMITIVE_LINE_LOOP type is not supported under the DirectX11 render interface");
			break;
		default:
			assert(false);
		}

		if (eDirectX11Primitive != D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
		{
			assert(DX11::DeviceContext( ));
			DX11::DeviceContext( )->IASetPrimitiveTopology(eDirectX11Primitive);
			DX11::DeviceContext( )->DrawIndexedInstanced(uiNumIndices, uiNumInstances, uiStartIndex, 0, uiStartInstanceLocation);

			bSuccess = true;
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::RenderArrays

inline st_bool CGeometryBufferDirectX11::RenderArrays(EPrimitiveType ePrimType, st_uint32 uiStartVertex, st_uint32 uiNumVertices) const
{
	st_bool bSuccess = false;

#ifdef _DEBUG
	if (VertexBufferIsValid( ))
	{
#endif
		D3D11_PRIMITIVE_TOPOLOGY eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		switch (ePrimType)
		{
		case PRIMITIVE_TRIANGLE_STRIP:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			break;
		case PRIMITIVE_TRIANGLES:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case PRIMITIVE_POINTS:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		case PRIMITIVE_LINE_STRIP:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
		case PRIMITIVE_LINES:
			eDirectX11Primitive = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case PRIMITIVE_TRIANGLE_FAN:
			CCore::SetError("CGeometryBufferDirectX11::RenderArrays, PRIMITIVE_TRIANGLE_FAN type is not supported under the DirectX11 render interface");
			break;
		case PRIMITIVE_QUAD_STRIP:
			CCore::SetError("CGeometryBufferDirectX11::RenderArrays, PRIMITIVE_QUAD_STRIP type is not supported under the DirectX11 render interface");
			break;
		case PRIMITIVE_QUADS:
			CCore::SetError("CGeometryBufferDirectX11::RenderArrays, PRIMITIVE_QUADS type is not supported under the DirectX11 render interface");
			break;
		case PRIMITIVE_LINE_LOOP:
			CCore::SetError("CGeometryBufferDirectX11::RenderArrays, PRIMITIVE_LINE_LOOP type is not supported under the DirectX11 render interface");
			break;
		default:
			assert(false);
		}

		if (eDirectX11Primitive != D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
		{
			assert(DX11::DeviceContext( ));
			DX11::DeviceContext( )->IASetPrimitiveTopology(eDirectX11Primitive);
			DX11::DeviceContext( )->Draw(uiNumVertices, uiStartVertex);
			bSuccess = true;
		}

#ifdef _DEBUG
	}
#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::IsPrimitiveTypeSupported

inline st_bool CGeometryBufferDirectX11::IsPrimitiveTypeSupported(EPrimitiveType ePrimType)
{
	return (ePrimType == PRIMITIVE_TRIANGLE_STRIP ||
			ePrimType == PRIMITIVE_TRIANGLES);
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::IsFormatSet

inline st_bool CGeometryBufferDirectX11::IsFormatSet(void) const
{
	return (m_pVertexLayout != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::IndexSize

inline st_uint32 CGeometryBufferDirectX11::IndexSize(void) const
{
	return (m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT) ? 2 : 4;
}


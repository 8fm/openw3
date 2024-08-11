///////////////////////////////////////////////////////////////////////  
//  GeometryBuffer.inl
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
//  CGeometryBufferOrbis::SAttribParams::SAttribParams

inline CGeometryBufferOrbis::SAttribParams::SAttribParams( ) :
	m_nOffset(-1)
{
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOrbis::SAttribParams::IsActive

inline st_bool CGeometryBufferOrbis::SAttribParams::IsActive(void) const
{
	return (m_nOffset > -1);
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::CGeometryBufferOrbis

inline CGeometryBufferOrbis::CGeometryBufferOrbis( ) :
	m_pVertexBuffer(NULL),
	m_pIndexBuffer(NULL),
	m_uiNumVertices(0),
	m_uiVertexSize(0),
	m_uiNumIndices(0),
	m_eIndexFormat(INDEX_FORMAT_UNSIGNED_32BIT)
{
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::~CGeometryBufferOrbis

inline CGeometryBufferOrbis::~CGeometryBufferOrbis( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::CreateVertexBuffer

inline st_bool CGeometryBufferOrbis::CreateVertexBuffer(st_bool /*bDynamic*/, const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize)
{
	st_bool bSuccess = false;
	
	if (m_pVertexBuffer != NULL)
	{
		Orbis::Release(m_pVertexBuffer);
		m_pVertexBuffer = NULL;
	}

	if (uiNumVertices > 0 && uiVertexSize > 0)
	{
		const st_uint32 c_uiSizeInBytes = uiNumVertices * uiVertexSize;

		m_pVertexBuffer = Orbis::Allocate(c_uiSizeInBytes, sce::Gnm::kAlignmentOfBufferInBytes, true);
		if (m_pVertexBuffer != NULL)
		{
			if (pVertexData != NULL)
				memcpy(m_pVertexBuffer, pVertexData, c_uiSizeInBytes);

			m_uiNumVertices = uiNumVertices;
			m_uiVertexSize = uiVertexSize;

			BuildOrbisVertexBuffer( );

			bSuccess = true;
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::OverwriteVertices

inline st_bool CGeometryBufferOrbis::OverwriteVertices(const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize, st_uint32 uiOffset, st_uint32 uiStream)
{
	// should have been taken care of in CGeometryBufferRI::OverwriteVertices, but be sure
	assert(pVertexData);
	assert(uiNumVertices > 0);
	assert(VertexBufferIsValid( ));

	st_uint32 uiWantedNumVertices = uiNumVertices + uiOffset;

	if ((uiVertexSize * uiWantedNumVertices > m_uiVertexSize * m_uiNumVertices) ||
		(uiVertexSize != m_uiVertexSize))
	{
		// give the buffer a little headroom so it doesn't have to reallocate so often
		uiWantedNumVertices *= 4;
		uiWantedNumVertices /= 3;

		void* pNewBuffer = Orbis::Allocate(uiVertexSize * uiWantedNumVertices, sce::Gnm::kAlignmentOfBufferInBytes, true);
		if (pNewBuffer != NULL)
		{
			if (uiVertexSize == m_uiVertexSize)
				memcpy(pNewBuffer, m_pVertexBuffer, m_uiVertexSize * m_uiNumVertices);

			Orbis::Release(m_pVertexBuffer);
			m_pVertexBuffer = pNewBuffer;
		
			m_uiNumVertices = uiWantedNumVertices;
			m_uiVertexSize = uiVertexSize;
			BuildOrbisVertexBuffer( );
		}
	}
	
	memcpy((st_byte*)m_pVertexBuffer + uiOffset * uiVertexSize, pVertexData, uiNumVertices * uiVertexSize);

	return true; // validity checks are performed in CGeometryBufferRI::OverwriteVertices
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::VertexBufferIsValid

inline st_bool CGeometryBufferOrbis::VertexBufferIsValid(void) const
{
	return (m_pVertexBuffer != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::EnableFormat

inline st_bool CGeometryBufferOrbis::EnableFormat(void) const
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::DisableFormat

inline st_bool CGeometryBufferOrbis::DisableFormat(void)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::BindVertexBuffer

inline st_bool CGeometryBufferOrbis::BindVertexBuffer(st_uint32 uiStream, st_uint32 uiVertexSize) const
{
	st_bool bSuccess = false;
	
#ifndef NDEBUG
	if (VertexBufferIsValid( ))
	{
#endif
		Orbis::Context( )->setVertexBuffers(sce::Gnm::kShaderStageVs, 0, m_vBuffers.size( ), &m_vBuffers[0]);
		bSuccess = true;

#ifndef NDEBUG
	}
	else
		CCore::SetError("CGeometryBufferOrbis::BindVertexBuffer, vertex buffer is not valid, cannot bind");
#endif
		
	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::InstancingRequiresSeparateVertexStream

inline st_bool CGeometryBufferOrbis::InstancingRequiresSeparateVertexStream(void)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::BindVertexBufferForInstancing

inline st_bool CGeometryBufferOrbis::BindVertexBufferForInstancing(const CGeometryBufferOrbis* pInstanceBuffer) const
{
	st_bool bSuccess = false;
	
#ifndef NDEBUG
	if (VertexBufferIsValid( ))
	{
#endif
		CFixedArray<sce::Gnm::Buffer, 16> vBuffers = pInstanceBuffer->m_vBuffers;
		vBuffers += m_vBuffers;

		Orbis::Context( )->setVertexBuffers(sce::Gnm::kShaderStageVs, 0, vBuffers.size( ), &vBuffers[0]);

		bSuccess = true;

#ifndef NDEBUG
	}
	else
		CCore::SetError("CGeometryBufferOrbis::BindVertexBuffer, vertex buffer is not valid, cannot bind");
#endif
		
	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::UnBindVertexBuffer

inline st_bool CGeometryBufferOrbis::UnBindVertexBuffer(st_uint32 uiStream)
{
	Orbis::Context( )->setVertexBuffers(sce::Gnm::kShaderStageVs, 0, st_uint32(VERTEX_ATTRIB_COUNT), NULL);
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::SetIndexFormat

inline st_bool CGeometryBufferOrbis::SetIndexFormat(EIndexFormat eFormat)
{
	m_eIndexFormat = eFormat;

	return (m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT || m_eIndexFormat == INDEX_FORMAT_UNSIGNED_32BIT);
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::CreateIndexBuffer

inline st_bool CGeometryBufferOrbis::CreateIndexBuffer(st_bool /*bDynamic*/, const void* pIndexData, st_uint32 uiNumIndices)
{
	st_bool bSuccess = false;
	
	if (m_pIndexBuffer != NULL)
	{
		Orbis::Release(m_pIndexBuffer);
		m_pIndexBuffer = NULL;
	}

	if (uiNumIndices > 0)
	{
		const st_uint32 c_uiSizeInBytes = uiNumIndices * IndexSize( );

		m_pIndexBuffer = Orbis::Allocate(c_uiSizeInBytes, sce::Gnm::kAlignmentOfBufferInBytes, true);
		if (m_pIndexBuffer != NULL)
		{
			if (pIndexData != NULL)
				memcpy(m_pIndexBuffer, pIndexData, c_uiSizeInBytes);

			m_uiNumIndices = uiNumIndices;
			bSuccess = true;
		}
	}
	
	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::OverwriteIndices

inline st_bool CGeometryBufferOrbis::OverwriteIndices(const void* pIndices, st_uint32 uiNumIndices, st_uint32 uiOffset)
{
	// should have been taken care of in CGeometryBufferRI::ReplaceIndices, but be sure
	assert(pIndices);
	assert(uiNumIndices > 0);
	assert(IndexBufferIsValid( ));

	const st_uint32 c_uiIndexSize = IndexSize( );
	const st_uint32 c_uiDesiredSize = c_uiIndexSize * (uiNumIndices + uiOffset);
	if (c_uiDesiredSize > m_uiNumIndices * c_uiIndexSize)
	{
		void* pNewBuffer = Orbis::Allocate(c_uiDesiredSize, sce::Gnm::kAlignmentOfBufferInBytes, true);
		if (pNewBuffer != NULL)
		{
			memcpy(pNewBuffer, m_pIndexBuffer, m_uiNumIndices * c_uiIndexSize);
			Orbis::Release(m_pIndexBuffer);
			m_pIndexBuffer = pNewBuffer;
		}
		
		m_uiNumIndices = uiNumIndices + uiOffset;
	}

	memcpy(static_cast<uint8_t*>(m_pIndexBuffer) + uiOffset * c_uiIndexSize, pIndices, uiNumIndices * c_uiIndexSize);
	
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::ClearIndices

inline st_bool CGeometryBufferOrbis::ClearIndices(void)
{
	Orbis::Release(m_pIndexBuffer);
	m_pIndexBuffer = NULL;
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::IndexBufferIsValid

inline st_bool CGeometryBufferOrbis::IndexBufferIsValid(void) const
{
	return (m_pIndexBuffer != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::BindIndexBuffer

inline st_bool CGeometryBufferOrbis::BindIndexBuffer(void) const
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::UnBindIndexBuffer

inline st_bool CGeometryBufferOrbis::UnBindIndexBuffer(void)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::ReleaseGfxResources

inline void CGeometryBufferOrbis::ReleaseGfxResources(void)
{
	Orbis::Release(m_pVertexBuffer);
	m_pVertexBuffer = NULL;
	Orbis::Release(m_pIndexBuffer);
	m_pIndexBuffer = NULL;
	m_vBuffers.clear( );
	m_uiNumVertices = 0;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::RenderIndexed

inline st_bool CGeometryBufferOrbis::RenderIndexed(EPrimitiveType ePrimType, 
												 st_uint32 uiStartIndex, 
												 st_uint32 uiNumIndices, 
												 st_uint32 uiMinIndex,
												 st_uint32 uiNumVertices) const
{
	st_bool bSuccess = false;
	
#ifndef NDEBUG
	if (IndexBufferIsValid( ))
	{
#endif
		if (uiNumIndices > 0)
		{
			Orbis::Context( )->setPrimitiveType(FindOrbisPrimitiveType(ePrimType));
			Orbis::Context( )->setIndexSize((m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT)	? sce::Gnm::kIndexSize16 : sce::Gnm::kIndexSize32);
			Orbis::Context( )->drawIndex(uiNumIndices, static_cast<uint8_t*>(m_pIndexBuffer) + uiStartIndex * IndexSize( ));

			bSuccess = true;		
		}
#ifndef NDEBUG
	}
#endif
	
	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOrbis::RenderIndexedInstanced

inline st_bool CGeometryBufferOrbis::RenderIndexedInstanced(EPrimitiveType ePrimType,
															 st_uint32 uiStartIndex,
															 st_uint32 uiNumIndices,
															 st_uint32 uiNumInstances,
															 st_uint32 uiStartInstanceLocation) const
{
	ST_UNREF_PARAM(uiStartInstanceLocation);

	st_bool bSuccess = false;
	
#ifndef NDEBUG
	if (IndexBufferIsValid( ))
	{
#endif
		if (uiNumIndices > 0)
		{
			Orbis::Context( )->setPrimitiveType(FindOrbisPrimitiveType(ePrimType));
			Orbis::Context( )->setIndexSize((m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT)	? sce::Gnm::kIndexSize16 : sce::Gnm::kIndexSize32);
			Orbis::Context( )->drawIndex(uiNumIndices, static_cast<uint8_t*>(m_pIndexBuffer) + uiStartIndex * IndexSize( ));

			bSuccess = true;		
		}
#ifndef NDEBUG
	}
#endif
	
	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::RenderArrays

inline st_bool CGeometryBufferOrbis::RenderArrays(EPrimitiveType ePrimType, 
												st_uint32 uiStartVertex, 
												st_uint32 uiNumVertices) const
{
	return false;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::IsPrimitiveTypeSupported

inline st_bool CGeometryBufferOrbis::IsPrimitiveTypeSupported(EPrimitiveType ePrimType)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::IndexSize

inline st_uint32 CGeometryBufferOrbis::IndexSize(void) const
{
	return (m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT) ? sizeof(uint16_t) : sizeof(uint32_t);
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::FindOrbisPrimitiveType

inline sce::Gnm::PrimitiveType CGeometryBufferOrbis::FindOrbisPrimitiveType(EPrimitiveType ePrimType)
{
	sce::Gnm::PrimitiveType uiReturn = sce::Gnm::kPrimitiveTypeNone;
	switch (ePrimType)
	{
	case PRIMITIVE_POINTS:
		uiReturn = sce::Gnm::kPrimitiveTypePointList;
		break;
	case PRIMITIVE_LINES:
		uiReturn = sce::Gnm::kPrimitiveTypeLineList;
		break;
	case PRIMITIVE_LINE_LOOP:
		uiReturn = sce::Gnm::kPrimitiveTypeLineLoop;
		break;
	case PRIMITIVE_LINE_STRIP:
		uiReturn = sce::Gnm::kPrimitiveTypeLineStrip;
		break;
	case PRIMITIVE_TRIANGLES:
		uiReturn = sce::Gnm::kPrimitiveTypeTriList;
		break;
	case PRIMITIVE_TRIANGLE_STRIP:
		uiReturn = sce::Gnm::kPrimitiveTypeTriStrip;
		break;
	case PRIMITIVE_TRIANGLE_FAN:
		uiReturn = sce::Gnm::kPrimitiveTypeTriFan;
		break;
	case PRIMITIVE_QUADS:
		uiReturn = sce::Gnm::kPrimitiveTypeQuadList;
		break;
	case PRIMITIVE_QUAD_STRIP:
		uiReturn = sce::Gnm::kPrimitiveTypeQuadStrip;
		break;
	default:
		break;
	}

	assert(uiReturn != sce::Gnm::kPrimitiveTypeNone);
	return uiReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::BuildOrbisVertexBuffer

inline void CGeometryBufferOrbis::BuildOrbisVertexBuffer(void)
{
	m_vBuffers.clear( );

	if (m_uiVertexSize > 0)
	{
		for (st_uint32 i = st_uint32(VERTEX_ATTRIB_0); i < st_uint32(VERTEX_ATTRIB_COUNT); ++i)
		{
			const SAttribParams* pParams = m_asAttribParams + i;
			if (pParams->IsActive( ))
			{
				sce::Gnm::Buffer cNewBuffer;
				cNewBuffer.initAsVertexBuffer(static_cast<uint8_t*>(m_pVertexBuffer) + pParams->m_nOffset, pParams->m_eDataType, m_uiVertexSize, m_uiNumVertices);
				cNewBuffer.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO);
				m_vBuffers.push_back(cNewBuffer);
			}
		}
	}
}


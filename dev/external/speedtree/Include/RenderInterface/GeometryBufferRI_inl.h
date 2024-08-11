///////////////////////////////////////////////////////////////////////  
//  GeometryBufferRI.inl
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
//  CGeometryBufferRI::CGeometryBufferRI

CGeometryBufferRI_TemplateList
inline CGeometryBufferRI_t::CGeometryBufferRI(st_bool bDynamicVB, st_bool bDynamicIB, const char* pResourceId)
	: m_bDynamicVB(bDynamicVB)
	, m_uiNumVertices(0)
	, m_bDynamicIB(bDynamicIB)
	, m_uiIndexSize(4)
	, m_uiNumIndices(0)
	, m_nVertexHeapHandle(-1)
	, m_nIndexHeapHandle(-1)
	, m_pResourceId(pResourceId)
	, m_pTechnique(NULL)
{
	m_aVertexData.SetHeapDescription("CGeometryBufferRI::CArray (vertices)");
	m_aIndexData.SetHeapDescription("CGeometryBufferRI::CArray (indices)");
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferRI::~CGeometryBufferRI

CGeometryBufferRI_TemplateList
inline CGeometryBufferRI_t::~CGeometryBufferRI( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferRI::SetVertexDecl

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::SetVertexDecl(const SVertexDecl& sVertexDecl, const TShaderTechniqueClass* pTechnique, const SVertexDecl& sInstanceVertexDecl)
{
	st_bool bSuccess = false;

	m_sVertexDecl = sVertexDecl;
	m_pTechnique = pTechnique;
	m_sInstanceVertexDecl = sInstanceVertexDecl;

	if (m_sVertexDecl.m_uiVertexSize > 0)
	{
		bSuccess = m_tGeometryBufferPolicy.SetVertexDecl(sVertexDecl, pTechnique, sInstanceVertexDecl);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::AppendVertices

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::AppendVertices(const void* pVertexData, st_uint32 uiNumVertices)
{
	st_bool bSuccess = false;

	#ifndef NDEBUG
		if (m_sVertexDecl.m_uiVertexSize > 0)
		{
			if (pVertexData && uiNumVertices > 0)
			{
	#endif
				size_t siStartIndex = m_aVertexData.size( );
				m_aVertexData.resize(siStartIndex + m_sVertexDecl.m_uiVertexSize * uiNumVertices);

				memcpy(&m_aVertexData[siStartIndex], pVertexData, uiNumVertices * m_sVertexDecl.m_uiVertexSize);
				m_uiNumVertices += uiNumVertices;

				bSuccess = true;
	#ifndef NDEBUG
			}
		}
		else
			CCore::SetError("CGeometryBufferRI::AppendVertices, SetFormat() must be called before AppendVertices");
	#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::EndVertices

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::EndVertices(void)
{
	st_bool bSuccess = false;

	#ifndef NDEBUG
		if (m_sVertexDecl.m_uiVertexSize > 0 && !m_aVertexData.empty( ))
		{
	#endif
			bSuccess = m_tGeometryBufferPolicy.CreateVertexBuffer(m_bDynamicVB, &m_aVertexData[0], st_uint32(m_aVertexData.size( ) / m_sVertexDecl.m_uiVertexSize), m_sVertexDecl.m_uiVertexSize);

			if (bSuccess)
				CCore::ResourceAllocated(GFX_RESOURCE_VERTEX_BUFFER, CFixedString::Format("VB_%s_%p", (m_pResourceId ? m_pResourceId : ""), &m_uiNumVertices), m_aVertexData.size( ));

			// don't need the client-side copy since the buffer's been created
			m_aVertexData.clear( );

			if (m_nVertexHeapHandle > -1)
			{
				CCore::TmpHeapBlockUnlock(m_nVertexHeapHandle);
				m_nVertexHeapHandle = -1;
			}


	#ifndef NDEBUG
		}
		else
			CCore::SetError("CGeometryBufferRI::EndVertices, SetFormat() and AppendVertices() must be called before EndVertices()");
	#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::CreateUninitializedVertexBuffer

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::CreateUninitializedVertexBuffer(st_uint32 uiNumVertices)
{
	st_bool bSuccess = false;

	#ifndef NDEBUG
		if (m_sVertexDecl.m_uiVertexSize > 0)
		{
	#endif

            // always pass true for dynamic when creating an unitialized buffer
			bSuccess = m_tGeometryBufferPolicy.CreateVertexBuffer(true, NULL, uiNumVertices, m_sVertexDecl.m_uiVertexSize);
			if (bSuccess)
			{
				m_uiNumVertices = uiNumVertices;
                CCore::ResourceAllocated(GFX_RESOURCE_VERTEX_BUFFER, CFixedString::Format("VB_%s_%p", (m_pResourceId ? m_pResourceId : ""), &m_uiNumVertices), uiNumVertices * m_sVertexDecl.m_uiVertexSize);
			}

	#ifndef NDEBUG
		}
		else
			CCore::SetError("CGeometryBufferRI::CreateUninitializedVertexBuffer, SetFormat() must be called before CreateUninitializedVertexBuffer()");
	#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::OverwriteVertices

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::OverwriteVertices(const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiOffset, st_uint32 uiStream)
{
	st_bool bSuccess = false;

	#ifndef NDEBUG
		if (pVertexData && uiNumVertices > 0)
		{
	#endif
			if (m_tGeometryBufferPolicy.VertexBufferIsValid( ))
			{
				bSuccess = m_tGeometryBufferPolicy.OverwriteVertices(pVertexData, uiNumVertices, m_sVertexDecl.m_uiVertexSize, uiOffset, uiStream);
			}
			else
			{
				CCore::SetError("CGeometryBufferRI::OverwriteVertices, cannot be called until a VB is established with AppendVertices() + EndVertices()");
			}
	#ifndef NDEBUG
		}
	#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::LockForWrite

CGeometryBufferRI_TemplateList
	inline void* CGeometryBufferRI_t::LockForWrite(st_int32 uiBufferSize)
{
	void* pBuffer = NULL;

	if (m_tGeometryBufferPolicy.VertexBufferIsValid( ))
		pBuffer = m_tGeometryBufferPolicy.LockForWrite(uiBufferSize);

	return pBuffer;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::UnlockFromWrite

CGeometryBufferRI_TemplateList
	inline st_bool CGeometryBufferRI_t::UnlockFromWrite(void) const
{
	return m_tGeometryBufferPolicy.UnlockFromWrite( );
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::NumVertices

CGeometryBufferRI_TemplateList
inline st_uint32 CGeometryBufferRI_t::NumVertices(void) const
{
	return m_uiNumVertices;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::VertexSize

CGeometryBufferRI_TemplateList
inline st_uint32 CGeometryBufferRI_t::VertexSize(void) const
{
	return m_sVertexDecl.m_uiVertexSize;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::EnableFormat

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::EnableFormat(void) const
{
	st_bool bSuccess = false;

#ifndef NDEBUG
	if (m_sVertexDecl.m_uiVertexSize > 0)
	{
#endif

		bSuccess = m_tGeometryBufferPolicy.EnableFormat( );

#ifndef NDEBUG
	}
	else
		CCore::SetError("CGeometryBufferRI::EnableFormat, the vertex buffer has not been set up yet");
#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::DisableFormat

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::DisableFormat(void)
{
	return TGeometryBufferPolicy::DisableFormat( );
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::BindVertexBuffer

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::BindVertexBuffer(st_uint32 uiStream) const
{
	st_bool bSuccess = false;

	#ifndef NDEBUG
		if (m_sVertexDecl.m_uiVertexSize > 0 && m_tGeometryBufferPolicy.VertexBufferIsValid( ))
		{
	#endif
			bSuccess = m_tGeometryBufferPolicy.BindVertexBuffer(uiStream, m_sVertexDecl.m_uiVertexSize);

	#ifndef NDEBUG
		}
		else
			CCore::SetError("CGeometryBufferRI::BindVertexBuffer, the vertex buffer has not been set up yet");
	#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::UnBindVertexBuffer

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::UnBindVertexBuffer(st_uint32 uiStream)
{
	return TGeometryBufferPolicy::UnBindVertexBuffer(uiStream);
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::SetIndexFormat

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::SetIndexFormat(EIndexFormat eFormat)
{
	if (eFormat == INDEX_FORMAT_UNSIGNED_16BIT)
		m_uiIndexSize = 2;
	else 
		m_uiIndexSize = 4;

	return m_tGeometryBufferPolicy.SetIndexFormat(eFormat);
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::AppendIndices

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::AppendIndices(const void* pIndexData, st_uint32 uiNumIndices)
{
	st_bool bSuccess = false;

	#ifndef NDEBUG
		if (pIndexData && uiNumIndices > 0)
		{
	#endif
			if (m_uiIndexSize > 0)
			{
				// grow the internal representation by the amount requested to append
				size_t siStartIndex = m_aIndexData.size( );
				m_aIndexData.resize(siStartIndex + uiNumIndices * m_uiIndexSize);

				// copy into the array
				memcpy(&m_aIndexData[siStartIndex], pIndexData, uiNumIndices * m_uiIndexSize);

				// m_uiNumIndices is used instead of m_aIndexData.size() because m_aIndexData will
				// be cleared after EndIndices() is called
				m_uiNumIndices += uiNumIndices;

				bSuccess = true;
			}
			else
				CCore::SetError("CGeometryBufferRI::AppendIndices, index buffer format must be set first");
	#ifndef NDEBUG
		}
	#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::OverwriteIndices

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::OverwriteIndices(const void* pIndexData, st_uint32 uiNumIndices, st_uint32 uiOffset)
{
	st_bool bSuccess = false;

	#ifndef NDEBUG
		if (pIndexData && uiNumIndices > 0)
		{
	#endif
			// if index buffer is in place, replace the buffer contents
			if (m_tGeometryBufferPolicy.IndexBufferIsValid( ))
			{
				bSuccess = m_tGeometryBufferPolicy.OverwriteIndices(pIndexData, uiNumIndices, uiOffset);
			}
	#ifndef NDEBUG
		}
	#endif

	if (bSuccess && uiNumIndices > m_uiNumIndices)
		m_uiNumIndices = uiNumIndices;

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::EndIndices

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::EndIndices(void)
{
	st_bool bSuccess = false;

	#ifndef NDEBUG
		if (!m_aIndexData.empty( ))
		{
	#endif
			m_uiNumIndices = st_uint32(m_aIndexData.size( ) / m_uiIndexSize);
			bSuccess = m_tGeometryBufferPolicy.CreateIndexBuffer(m_bDynamicIB, &m_aIndexData[0], m_uiNumIndices);
			
			if (bSuccess)
                CCore::ResourceAllocated(GFX_RESOURCE_INDEX_BUFFER, CFixedString::Format("IB_%s_%p", (m_pResourceId ? m_pResourceId : ""), &m_uiNumIndices), m_aIndexData.size( ));

			// don't need the client-side copy since the buffer's been created
			m_aIndexData.clear( );

			if (m_nIndexHeapHandle > -1)
			{
				CCore::TmpHeapBlockUnlock(m_nIndexHeapHandle);
				m_nIndexHeapHandle = -1;
			}

	#ifndef NDEBUG
		}
		else
			CCore::SetError("CGeometryBufferRI::EndIndices, AppendIndices() must be called before EndVertices");
	#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::CreateUninitializedIndexBuffer

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::CreateUninitializedIndexBuffer(st_uint32 uiNumIndices)
{
	st_bool bSuccess = false;

	#ifndef NDEBUG
		if (uiNumIndices > 0)
		{
	#endif
			if (m_uiIndexSize > 0)
			{
                // always pass true for dynamic when creating an unitialized buffer
				bSuccess = m_tGeometryBufferPolicy.CreateIndexBuffer(true, NULL, uiNumIndices);

				if (bSuccess)
				{
					m_uiNumIndices = uiNumIndices;
					CCore::ResourceAllocated(GFX_RESOURCE_INDEX_BUFFER, CFixedString::Format("IB_%s_%p", (m_pResourceId ? m_pResourceId : ""), &m_uiNumIndices), m_uiIndexSize * m_uiNumIndices);
				}
			}
			else
				CCore::SetError("CGeometryBufferRI::CreateUninitializedIndexBuffer, index buffer format must be set first");
	#ifndef NDEBUG
		}
	#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::NumIndices

CGeometryBufferRI_TemplateList
inline st_uint32 CGeometryBufferRI_t::NumIndices(void) const
{
	return m_uiNumIndices;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::IndexSize

CGeometryBufferRI_TemplateList
inline st_uint32 CGeometryBufferRI_t::IndexSize(void) const
{
	return m_tGeometryBufferPolicy.IndexSize( );
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::ClearIndices

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::ClearIndices(void)
{
	m_aIndexData.clear( ); // should already be cleared
	m_uiNumIndices = 0;

	return m_tGeometryBufferPolicy.ClearIndices( );
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::BindIndexBuffer

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::BindIndexBuffer(void) const
{
	st_bool bSuccess = false;

	#ifndef NDEBUG
		if (m_tGeometryBufferPolicy.IndexBufferIsValid( ))
		{
	#endif

			bSuccess = m_tGeometryBufferPolicy.BindIndexBuffer( );

	#ifndef NDEBUG
		}
	#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::UnBindIndexBuffer

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::UnBindIndexBuffer(void)
{
	return TGeometryBufferPolicy::UnBindIndexBuffer( );
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::RenderIndexed

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::RenderIndexed(EPrimitiveType ePrimType, 
												  st_uint32 uiStartIndex, 
												  st_uint32 uiNumIndices, 
												  st_uint32 uiMinIndex,
												  st_uint32 uiNumVertices) const
{
	return m_tGeometryBufferPolicy.RenderIndexed(ePrimType, uiStartIndex, uiNumIndices, uiMinIndex, uiNumVertices);
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::RenderIndexedInstanced

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::RenderIndexedInstanced(EPrimitiveType ePrimType, 
														   st_uint32 uiStartIndex, 
														   st_uint32 uiNumIndices, 
														   st_uint32 uiNumInstances,
														   st_uint32 uiStartInstanceLocation) const
{
	return m_tGeometryBufferPolicy.RenderIndexedInstanced(ePrimType, uiStartIndex, uiNumIndices, uiNumInstances, uiStartInstanceLocation);
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::RenderArrays

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::RenderArrays(EPrimitiveType ePrimType, st_uint32 uiStartVertex, st_uint32 uiNumVertices) const
{
	return m_tGeometryBufferPolicy.RenderArrays(ePrimType, uiStartVertex, uiNumVertices);
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::IsPrimitiveTypeSupported

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::IsPrimitiveTypeSupported(EPrimitiveType ePrimType)
{
	return TGeometryBufferPolicy::IsPrimitiveTypeSupported(ePrimType);
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::ReleaseGfxResources

CGeometryBufferRI_TemplateList
inline void CGeometryBufferRI_t::ReleaseGfxResources(void)
{
	if (m_uiNumVertices > 0)
        CCore::ResourceReleased(CFixedString::Format("VB_%s_%p", (m_pResourceId ? m_pResourceId : ""), &m_uiNumVertices));
	if (m_uiNumIndices > 0)
		CCore::ResourceReleased(CFixedString::Format("IB_%s_%p", (m_pResourceId ? m_pResourceId : ""), &m_uiNumIndices));

	m_tGeometryBufferPolicy.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::Reserve

CGeometryBufferRI_TemplateList
inline st_bool CGeometryBufferRI_t::Reserve(st_uint32 uiNumVertices, st_uint32 uiNumIndices)
{
	st_bool bSuccess = true;

	// vertices
	if (m_sVertexDecl.m_uiVertexSize > 0)
	{
		if (uiNumVertices > 0)
		{
			// block size
			const size_t c_siBlockSize = uiNumVertices * m_sVertexDecl.m_uiVertexSize;

			// get block
			st_byte* pBlock = CCore::TmpHeapBlockLock(c_siBlockSize, "CGeometryBufferRI_Vertices", m_nVertexHeapHandle);
			assert(pBlock && m_nVertexHeapHandle > -1);

			// assign block
			m_aVertexData.SetExternalMemory(pBlock, c_siBlockSize);
		}
	}
	else if (uiNumVertices > 0)
	{
		CCore::SetError("CGeometryBufferRI::Reserve failed, vertex size not yet known");
		bSuccess = false;
	}

	// indices
	if (m_uiIndexSize > 0)
	{
		if (uiNumIndices > 0)
		{
			// block size
			const size_t c_siBlockSize = uiNumIndices * m_uiIndexSize;

			// get block
			st_byte* pBlock = CCore::TmpHeapBlockLock(c_siBlockSize, "CGeometryBufferRI_Index", m_nIndexHeapHandle);
			assert(pBlock && m_nIndexHeapHandle > -1);

			// assign block
			m_aIndexData.SetExternalMemory(pBlock, c_siBlockSize);
		}

		bSuccess = true;
	}
	else if (uiNumIndices > 0)
	{
		CCore::SetError("CGeometryBufferRI::Reserve failed, index size not yet known");
		bSuccess = false;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferRI::GetVertexDecl

CGeometryBufferRI_TemplateList
inline const SVertexDecl& CGeometryBufferRI_t::GetVertexDecl(void) const
{
	return m_sVertexDecl;
}


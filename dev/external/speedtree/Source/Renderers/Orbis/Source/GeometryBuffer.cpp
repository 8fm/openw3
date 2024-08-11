///////////////////////////////////////////////////////////////////////
//  GeometryBuffer.cpp
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
//  Preprocessor

#include "Renderers/Orbis/OrbisRenderer.h"
#include "Utilities/Utility.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferOrbis::SetVertexFormat

st_bool CGeometryBufferOrbis::SetVertexDecl(const SVertexDecl& sVertexDecl, const CShaderTechnique* pTechnique, const SVertexDecl& sInstanceVertexDecl)
{
	st_bool bSuccess = true;

	st_int32 nOffset = 0;
	for (st_int32 i = 0; i < VERTEX_ATTRIB_COUNT; ++i)
	{
		const SVertexDecl::SAttribute& sAttrib = sVertexDecl.m_asAttributes[i];
		if (sAttrib.IsUsed( ))
		{
			SAttribParams& sParams = m_asAttribParams[i];

			// check if this attribute has already been set
			if (sParams.m_nOffset == -1)
			{
				// offset and size are all in bytes
				sParams.m_nOffset = nOffset;


				if (sAttrib.m_eFormat == VERTEX_FORMAT_FULL_FLOAT)
				{
					switch (sAttrib.NumUsedComponents( ))
					{
					case 1:
						sParams.m_eDataType = sce::Gnm::kDataFormatR32Float;
						break;
					case 2:
						sParams.m_eDataType = sce::Gnm::kDataFormatR32G32Float;
						break;
					case 3:
						sParams.m_eDataType = sce::Gnm::kDataFormatR32G32B32Float;
						break;
					case 4:
					default:
						sParams.m_eDataType = sce::Gnm::kDataFormatR32G32B32A32Float;
						break;
					}
				}
				else if (sAttrib.m_eFormat == VERTEX_FORMAT_HALF_FLOAT)
				{
					switch (sAttrib.NumUsedComponents( ))
					{
					case 1:
						sParams.m_eDataType = sce::Gnm::kDataFormatR16Float;
						break;
					case 2:
						sParams.m_eDataType = sce::Gnm::kDataFormatR16G16Float;
						break;
					case 3:
						st_assert(false, "R16G16B16 attribute format not supported");
						break;
					case 4:
					default:
						sParams.m_eDataType = sce::Gnm::kDataFormatR16G16B16A16Float;
						break;
					}
				}
				else // VERTEX_FORMAT_BYTE
				{
					switch (sAttrib.NumUsedComponents( ))
					{
					case 1:
						sParams.m_eDataType = sce::Gnm::kDataFormatR8Unorm;
						break;
					case 2:
						sParams.m_eDataType = sce::Gnm::kDataFormatR8G8Unorm;
						break;
					case 3:
						st_assert(false, "R8G8B8 attribute format not supported");
						break;
					case 4:
					default:
						sParams.m_eDataType = sce::Gnm::kDataFormatR8G8B8A8Unorm;
						break;
					}
				}

				nOffset += sAttrib.Size( );
			}
		}
	}

	BuildOrbisVertexBuffer( );

	return bSuccess;
}

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

#include "Renderers/DirectX11/DirectX11Renderer.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  Function: GetMatchingDx11Type

DXGI_FORMAT GetMatchingDx11Type(EVertexFormat eVertexFormat, st_int32 nNumComponents)
{
	DXGI_FORMAT eDx11Type = DXGI_FORMAT_UNKNOWN; // not found

	if (nNumComponents > 0 && nNumComponents < 5)
	{
		if (eVertexFormat == VERTEX_FORMAT_BYTE)
		{
			assert(nNumComponents == 4);
			eDx11Type = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (eVertexFormat == VERTEX_FORMAT_HALF_FLOAT)
		{
			if (nNumComponents == 2)
				eDx11Type = DXGI_FORMAT_R16G16_FLOAT;
			else if (nNumComponents == 4)
				eDx11Type = DXGI_FORMAT_R16G16B16A16_FLOAT;
			else
				assert(false);
		}
		else if (eVertexFormat == VERTEX_FORMAT_FULL_FLOAT)
		{
			if (nNumComponents == 1)
				eDx11Type = DXGI_FORMAT_R32_FLOAT;
			else if (nNumComponents == 2)
				eDx11Type = DXGI_FORMAT_R32G32_FLOAT;
			else if (nNumComponents == 3)
				eDx11Type = DXGI_FORMAT_R32G32B32_FLOAT;
			else
				eDx11Type = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
		else
			assert(false);
	}

	return eDx11Type;
}


///////////////////////////////////////////////////////////////////////  
//  Function: GetMatchingDx11Semantic

bool GetMatchingDx11Semantic(EVertexAttribute eAttrib, D3D11_INPUT_ELEMENT_DESC& sElement)
{
	sElement.SemanticIndex = 0;
	switch (eAttrib)
	{
	case VERTEX_ATTRIB_0:
		sElement.SemanticName = "POSITION";
		break;
	case VERTEX_ATTRIB_1:
	case VERTEX_ATTRIB_2:
	case VERTEX_ATTRIB_3:
	case VERTEX_ATTRIB_4:
	case VERTEX_ATTRIB_5:
	case VERTEX_ATTRIB_6:
	case VERTEX_ATTRIB_7:
	case VERTEX_ATTRIB_8:
	case VERTEX_ATTRIB_9:
	case VERTEX_ATTRIB_10:
	case VERTEX_ATTRIB_11:
	case VERTEX_ATTRIB_12:
	case VERTEX_ATTRIB_13:
	case VERTEX_ATTRIB_14:
	case VERTEX_ATTRIB_15:
		sElement.SemanticName = "TEXCOORD";
		sElement.SemanticIndex = eAttrib - VERTEX_ATTRIB_1;
		break;
	default:
		break;
	}

	return (sElement.SemanticName != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX11::SetVertexFormat

st_bool CGeometryBufferDirectX11::SetVertexDecl(const SVertexDecl& sVertexDecl, const CShaderTechnique* pTechnique, const SVertexDecl& sInstanceVertexDecl)
{
	st_bool bSuccess = false;

	// is the instance vertex decl empty?
	const st_bool c_bInstanceVertexDeclEmpty = (sInstanceVertexDecl.m_uiVertexSize == 0);

	// which vertex decl matches the shader vertex input?
	const SVertexDecl& sShaderVertexDecl = (c_bInstanceVertexDeclEmpty ? sVertexDecl : sInstanceVertexDecl);

	// count # of used attributes
	size_t siNumElements = 0;
	for (st_int32 i = 0; i < VERTEX_ATTRIB_COUNT; ++i)
		if (sShaderVertexDecl.m_asAttributes[i].IsUsed( ))
			++siNumElements;

	if (siNumElements > 0)
	{
		CStaticArray<D3D11_INPUT_ELEMENT_DESC> aElements(siNumElements + 1, "CGeometryBufferDirectX11::SetVertexDecl", false);

		st_uint32 aOffsets[2] = { 0, 0 };
		for (st_int32 i = 0; i < VERTEX_ATTRIB_COUNT; ++i)
		{
			const SVertexDecl::SAttribute& sAttrib = sShaderVertexDecl.m_asAttributes[i];
			if (sAttrib.IsUsed( ))
			{
				// setup element to match this attribute
				D3D11_INPUT_ELEMENT_DESC sElement;
				sElement.Format = GetMatchingDx11Type(sAttrib.m_eFormat, sAttrib.NumUsedComponents( ));
				if (sElement.Format != DXGI_FORMAT_UNKNOWN && GetMatchingDx11Semantic(EVertexAttribute(i), sElement))
				{			
					// semantic/method
					if (sElement.SemanticName != NULL)
					{
						sElement.InputSlot = (sAttrib.m_uiStream == c_nInstanceVertexStream? 1 : 0);
						sElement.AlignedByteOffset = aOffsets[sAttrib.m_uiStream];
						sElement.InputSlotClass = ((sAttrib.m_uiStream == c_nInstanceVertexStream) ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA);
						sElement.InstanceDataStepRate = (sAttrib.m_uiStream == c_nInstanceVertexStream? 1 : 0);

						aElements.push_back(sElement);
						bSuccess = true;
					}
					else
					{
						CCore::SetError("CGeometryBufferDirectX11::SetVertexDecl, cannot find matching DX11 semantic");
						bSuccess = false;
						break;
					}
				}
				else
				{
					CCore::SetError("CGeometryBufferDirectX11::SetVertexDecl, vertex attribute not supported by DX11: (%s, # elements: %d, data type: %s)\n",
						SVertexDecl::AttributeName(EVertexAttribute(i)), sAttrib.NumUsedComponents( ), SVertexDecl::FormatName(sAttrib.m_eFormat));
					bSuccess = false;
					break;
				}

				aOffsets[sAttrib.m_uiStream] += WORD(sAttrib.Size( ));
			}
		}

		if (bSuccess)
		{
			assert(DX11::Device( ));

			if (pTechnique) // instance geometry buffers don't have techniques
			{
				assert(pTechnique->IsValid( ));

				const CShaderTechniqueDirectX11::CVertexShader& cVS = pTechnique->GetVertexShader( );
				assert(cVS.m_pCompiledShaderCode);
				assert(cVS.m_uiCompiledShaderCodeSize > 0);

				if (FAILED(DX11::Device( )->CreateInputLayout(&aElements[0], 
															  UINT(aElements.size( )),
															  cVS.m_pCompiledShaderCode,
															  cVS.m_uiCompiledShaderCodeSize,
															  &m_pVertexLayout)))
				{
					CCore::SetError("CGeometryBufferDirectX11::SetVertexFormat, DX11::CreateInputLayout failed; possibly shader vertex input didn't match CPU side?");
				}
				else
					ST_NAME_DX11_OBJECT(m_pVertexLayout, "speedtree vertex layout");
			}
		}
	}

	return bSuccess;
}
